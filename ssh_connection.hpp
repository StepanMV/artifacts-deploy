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

class AllocationError : public std::runtime_error
{
public:
    AllocationError();
};

class ConnectionError : public std::runtime_error
{
public:
    ConnectionError();
};

class AuthenticationError : public std::runtime_error
{
public:
    AuthenticationError();
};

class SSHConnection
{
public:
    SSHConnection() = default;
    SSHConnection(const std::string& ip, size_t port, const std::string& user, const std::string& keyPath, const std::string& keyPass);
    SSHConnection(const std::string& ip, size_t port, const std::string& user, const std::string& password);
    ~SSHConnection() = default;
    QString sendCommand(const std::string& command);
    void sendFile(const std::string& source, const std::string& dest);
    ssh_session connect();

private:
    SSHConnection(const std::string& ip, size_t port, const std::string& user);

    std::string ip;
    size_t port;
    std::string user;
    std::string keyPath;
    std::string keyPass;
    std::string password;
};

#endif // SSHCONNECTION_HPP
