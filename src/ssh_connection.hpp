#ifndef SSHCONNECTION_HPP
#define SSHCONNECTION_HPP

#include <libssh/libssh.h>
#include <QByteArray>
#include <QThread>
#include <QVector>
#include <QFile>
#include <sys/stat.h>
#include <sys/types.h>
#include <QDebug>

class SshError : public std::runtime_error
{
public:
    SshError();
};

class AllocationError : public SshError
{
public:
    AllocationError() = default;
};

class ConnectionError : public SshError
{
public:
    ConnectionError() = default;
};

class VerificationError : public SshError
{
public:
    VerificationError() = default;
};

class AuthenticationError : public SshError
{
public:
    AuthenticationError() = default;
};

class SSHConnection
{
public:
    SSHConnection(const std::string& ip, size_t port, const std::string& user, const std::string& keyPath, const std::string& keyPass);
    SSHConnection(const std::string& ip, size_t port, const std::string& user, const std::string& password);
    ~SSHConnection();
    QString sendCommand(const std::string& command);
    void sendFile(const std::string& source, const std::string& dest);

private:
    ssh_session                                                            connect();
    SSHConnection(const std::string& ip, size_t port, const std::string& user);
    int verifyHost(ssh_session session);

    ssh_session session = nullptr;

    std::string ip;
    size_t port;
    std::string user;
    std::string keyPath;
    std::string keyPass;
    std::string password;
};

#endif // SSHCONNECTION_HPP
