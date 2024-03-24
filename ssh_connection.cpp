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

SSHConnection::SSHConnection(const std::string& ip, size_t port, const std::string& user)
{
    this->ip = ip;
    this->port = port;
    this->user = user;
}

SSHConnection::SSHConnection(const std::string& ip, size_t port, const std::string& user, const std::string& keyPath, const std::string& keyPass) : SSHConnection(ip, port, user)
{
    this->keyPath = keyPath;
    this->keyPass = keyPass;
}

SSHConnection::SSHConnection(const std::string& ip, size_t port, const std::string& user, const std::string& password) : SSHConnection(ip, port, user)
{
    this->password = password;
}

ssh_session SSHConnection::connect()
{
    ssh_session sshSession = ssh_new();
    if (sshSession == NULL) throw AllocationError();

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
    if (!this->password.empty()) code = ssh_userauth_password(sshSession, NULL, password.c_str());
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

QString SSHConnection::sendCommand(const std::string& command)
{
    ssh_session sshSession = connect();
    ssh_channel channel = ssh_channel_new(sshSession);
    if (channel == NULL) throw std::runtime_error("Error creating ssh channel");

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

void SSHConnection::sendFile(const std::string& source, const std::string& dest)
{
    this->sendCommand("mkdir -p " + dest.substr(0, dest.rfind("/")));
    qDebug() << "hello gere";
    ssh_session session = connect();
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
    qDebug() << "starting to send";
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
    qDebug() << "finish";
    sftp_free(sftp);
}
