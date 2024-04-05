#include "ssh_connection.hpp"
#include <libssh/libssh.h>
#include <libssh/sftp.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <fstream>

AllocationError::AllocationError() : std::runtime_error("") {}
ConnectionError::ConnectionError() : std::runtime_error("") {}
AuthenticationError::AuthenticationError() : std::runtime_error("") {}
VerificationError::VerificationError() : std::runtime_error("") {}

SSHConnection::SSHConnection(const std::string &ip, size_t port, const std::string &user)
{
    this->ip = ip;
    this->port = port;
    this->user = user;
}

SSHConnection::SSHConnection(const std::string &ip, size_t port, const std::string &user, const std::string &keyPath, const std::string &keyPass) : SSHConnection(ip, port, user)
{
    this->keyPath = keyPath;
    this->keyPass = keyPass;
    this->session = connect();
}

SSHConnection::SSHConnection(const std::string &ip, size_t port, const std::string &user, const std::string &password) : SSHConnection(ip, port, user)
{
    this->password = password;
    this->session = connect();
}

ssh_session SSHConnection::connect()
{
    ssh_session sshSession = ssh_new();
    if (sshSession == NULL)
        throw AllocationError();

    ssh_options_set(sshSession, SSH_OPTIONS_HOST, this->ip.c_str());
    ssh_options_set(sshSession, SSH_OPTIONS_PORT, &this->port);
    ssh_options_set(sshSession, SSH_OPTIONS_USER, this->user.c_str());
    int code = ssh_connect(sshSession);
    if (code != SSH_OK)
    {
        ssh_disconnect(sshSession);
        ssh_free(sshSession);
        throw ConnectionError();
    }

    if (verifyHost(sshSession) < 0)
    {
        ssh_disconnect(session);
        ssh_free(session);
        throw VerificationError();
    }

    if (!this->password.empty())
        code = ssh_userauth_password(sshSession, NULL, password.c_str());
    else
    {
        ssh_key key = ssh_key_new();
        ssh_pki_import_privkey_file(keyPath.c_str(), keyPass.c_str(), NULL, NULL, &key);
        code = ssh_userauth_publickey(sshSession, NULL, key);
        ssh_key_free(key);
    }
    if (code != SSH_AUTH_SUCCESS)
    {
        ssh_disconnect(sshSession);
        ssh_free(sshSession);
        throw AuthenticationError();
    }
    return sshSession;
}

QString SSHConnection::sendCommand(const std::string &command)
{
    ssh_channel channel = ssh_channel_new(session);
    if (channel == NULL)
        throw std::runtime_error("Error creating ssh channel");

    int code = ssh_channel_open_session(channel);
    if (code != SSH_OK)
    {
        ssh_channel_free(channel);
        throw std::runtime_error("Error openning ssh channel session");
    }

    code = ssh_channel_request_exec(channel, command.c_str());
    if (code != SSH_OK)
    {
        ssh_channel_close(channel);
        ssh_channel_free(channel);
        throw std::runtime_error("Error sending ssh command");
    }

    QString response;
    char buffer[256];
    int nbytes = ssh_channel_read(channel, buffer, sizeof(buffer), 0);
    while (nbytes > 0)
    {
        response.append(buffer);
        nbytes = ssh_channel_read(channel, buffer, sizeof(buffer), 0);
    }

    if (nbytes < 0)
    {
        ssh_channel_close(channel);
        ssh_channel_free(channel);
        throw std::runtime_error("Error reading ssh command response");
    }

    ssh_channel_send_eof(channel);
    ssh_channel_close(channel);
    ssh_channel_free(channel);

    return response;
}

void SSHConnection::sendFile(const std::string &source, const std::string &dest)
{
    this->sendCommand("mkdir -p " + dest.substr(0, dest.rfind("/")));
    sftp_session sftp;
    int rc;

    sftp = sftp_new(session);
    if (sftp == NULL)
    {
        throw std::runtime_error("Error allocating SFTP session");
    }

    rc = sftp_init(sftp);
    if (rc != SSH_OK)
    {
        sftp_free(sftp);
        throw std::runtime_error("Error initializing SFTP session");
    }

    int access_type = O_WRONLY | O_CREAT | O_TRUNC;
    sftp_file file;

    file = sftp_open(sftp, dest.c_str(),
                     access_type, S_IRWXU);
    if (file == NULL)
    {
        throw std::runtime_error("Can't open file for writing");
    }

    std::ifstream fin(source, std::ios::binary);
    while (fin)
    {
        constexpr size_t max_xfer_buf_size = 10240;
        char buffer[max_xfer_buf_size];
        fin.read(buffer, sizeof(buffer));
        if (fin.gcount() > 0)
        {
            ssize_t nwritten = sftp_write(file, buffer, fin.gcount());
            if (nwritten != fin.gcount())
            {
                sftp_close(file);
                throw std::runtime_error("Can't write data to file");
            }
        }
    }

    rc = sftp_close(file);
    if (rc != SSH_OK)
    {
        throw std::runtime_error("Can't close the written file");
    }
    sftp_free(sftp);
}

int SSHConnection::verifyHost(ssh_session session)
{
    enum ssh_known_hosts_e state;
    unsigned char *hash = NULL;
    ssh_key srv_pubkey = NULL;
    size_t hlen;
    char buf[10];
    char *hexa;
    char *p;
    int cmp;
    int rc;

    rc = ssh_get_server_publickey(session, &srv_pubkey);
    if (rc < 0)
    {
        return -1;
    }

    rc = ssh_get_publickey_hash(srv_pubkey,
                                SSH_PUBLICKEY_HASH_SHA1,
                                &hash,
                                &hlen);
    ssh_key_free(srv_pubkey);
    if (rc < 0)
    {
        return -1;
    }

    state = ssh_session_is_known_server(session);
    switch (state)
    {
    case SSH_KNOWN_HOSTS_OK:
        /* OK */

        break;
    case SSH_KNOWN_HOSTS_CHANGED:
        fprintf(stderr, "Host key for server changed: it is now:\n");
        ssh_print_hexa("Public key hash", hash, hlen);
        fprintf(stderr, "For security reasons, connection will be stopped\n");
        ssh_clean_pubkey_hash(&hash);

        return -1;
    case SSH_KNOWN_HOSTS_OTHER:
        fprintf(stderr, "The host key for this server was not found but an other"
                        "type of key exists.\n");
        fprintf(stderr, "An attacker might change the default server key to"
                        "confuse your client into thinking the key does not exist\n");
        ssh_clean_pubkey_hash(&hash);

        return -1;
    case SSH_KNOWN_HOSTS_NOT_FOUND:
        fprintf(stderr, "Could not find known host file.\n");
        fprintf(stderr, "If you accept the host key here, the file will be"
                        "automatically created.\n");

        /* FALL THROUGH to SSH_SERVER_NOT_KNOWN behavior */

    case SSH_KNOWN_HOSTS_UNKNOWN:
        hexa = ssh_get_hexa(hash, hlen);
        fprintf(stderr, "The server is unknown. Do you trust the host key?\n");
        fprintf(stderr, "Public key hash: %s\n", hexa);
        ssh_string_free_char(hexa);
        ssh_clean_pubkey_hash(&hash);
        p = fgets(buf, sizeof(buf), stdin);
        if (p == NULL)
        {
            return -1;
        }

        cmp = strncasecmp(buf, "yes", 3);
        if (cmp != 0)
        {
            return -1;
        }

        rc = ssh_session_update_known_hosts(session);
        if (rc < 0)
        {
            fprintf(stderr, "Error %s\n", strerror(errno));
            return -1;
        }

        break;
    case SSH_KNOWN_HOSTS_ERROR:
        fprintf(stderr, "Error %s", ssh_get_error(session));
        ssh_clean_pubkey_hash(&hash);
        return -1;
    }

    ssh_clean_pubkey_hash(&hash);
    return 0;
}