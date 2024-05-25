#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <QString>

enum LogType
{
    INFO,
    WARNING,
    ERROR
};


class Logger
{
public:
    static void setLogPath(const QString &path);
    static void log(const QString &message, LogType type = INFO);

private:
    static QString logPath;
};


#endif // LOGGER_HPP
