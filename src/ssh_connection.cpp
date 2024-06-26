#include "ssh_connection.hpp"
#include <libssh/libssh.h>
#include <libssh/sftp.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <fstream>
#include <sstream>

SshError::SshError() : std::runtime_error("") {}

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
    this->sftpSession = connectSftp();
}

SSHConnection::SSHConnection(const std::string &ip, size_t port, const std::string &user, const std::string &password) : SSHConnection(ip, port, user)
{
    this->password = password;
    this->session = connect();
    this->sftpSession = connectSftp();
}

SSHConnection::~SSHConnection()
{
    if (sftpSession)
    {
        sftp_free(sftpSession);
    }
    if (session && ssh_is_connected(session))
    {
        ssh_disconnect(session);
        ssh_free(session);
    }
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

sftp_session SSHConnection::connectSftp()
{
    sftp_session sftpSession = sftp_new(session);
    if (sftpSession == NULL)
    {
        throw SshError();
    }

    int code = sftp_init(sftpSession);
    if (code != SSH_OK)
    {
        throw SshError();
    }

    return sftpSession;
}

QString SSHConnection::sendCommand(const std::string &command)
{
    ssh_channel channel = ssh_channel_new(session);
    if (channel == NULL)
        throw SshError();

    int code = ssh_channel_open_session(channel);
    if (code != SSH_OK)
    {
        ssh_channel_free(channel);
        throw SshError();
    }

    code = ssh_channel_request_exec(channel, command.c_str());
    if (code != SSH_OK)
    {
        ssh_channel_close(channel);
        ssh_channel_free(channel);
        throw SshError();
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
        throw SshError();
    }

    ssh_channel_send_eof(channel);
    ssh_channel_close(channel);
    ssh_channel_free(channel);

    return response;
}

void SSHConnection::sendFile(const std::string &source, const std::string &dest)
{
    std::istringstream ss(dest.substr(0, dest.rfind("/")));
    std::string token;
    std::string concatToken;
    int rc;

    while (std::getline(ss, token, '/'))
    {
        concatToken += token;
        rc = sftp_mkdir(this->sftpSession, concatToken.c_str(), S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);
        if (rc != SSH_OK)
        {
            if (sftp_get_error(this->sftpSession) != SSH_FX_FILE_ALREADY_EXISTS)
            {
                throw SshError();
            }
        }
        concatToken += "/";
    }


    struct stat fileStat;
    if (stat(source.c_str(), &fileStat) < 0)
    {
        throw SshError();
    }

    int access_type = O_WRONLY | O_CREAT | O_TRUNC;
    sftp_file file;

    file = sftp_open(this->sftpSession, dest.c_str(),
                     access_type, fileStat.st_mode);
    if (file == NULL)
    {
        throw SshError();
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
                throw SshError();
            }
        }
    }
    rc = sftp_close(file);
    if (rc != SSH_OK)
    {
        throw SshError();
    }
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
        ssh_clean_pubkey_hash(&hash);

        return -1;
    case SSH_KNOWN_HOSTS_OTHER:
        ssh_clean_pubkey_hash(&hash);

        return -1;
    case SSH_KNOWN_HOSTS_NOT_FOUND:

        /* FALL THROUGH to SSH_SERVER_NOT_KNOWN behavior */

    case SSH_KNOWN_HOSTS_UNKNOWN:
        hexa = ssh_get_hexa(hash, hlen);
        ssh_string_free_char(hexa);
        ssh_clean_pubkey_hash(&hash);

        rc = ssh_session_update_known_hosts(session);
        if (rc < 0)
        {
            return -1;
        }
        break;

    case SSH_KNOWN_HOSTS_ERROR:
        ssh_clean_pubkey_hash(&hash);
        return -1;
    }

    ssh_clean_pubkey_hash(&hash);
    return 0;
}