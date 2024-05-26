#ifndef MYLOGGER_HPP
#define MYLOGGER_HPP

#include <QString>

enum MyLogType
{
    INFO,
    WARNING,
    ERROR
};


class MyLogger
{
public:
    static void setLogPath(const QString &path);
    static void log(const QString &message, MyLogType type = INFO);

private:
    static QString logPath;
};


#endif // MYLOGGER_HPP
