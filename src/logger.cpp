#include "logger.hpp"

#include <QFile>
#include <QTextStream>
#include <QDateTime>

QString Logger::logPath;

void Logger::setLogPath(const QString &path)
{
    logPath = path;
}

void Logger::log(const QString &message, LogType type)
{
    QFile file(logPath);
    if (file.open(QIODevice::WriteOnly | QIODevice::Append))
    {
        QTextStream stream(&file);
        stream << QDateTime::currentDateTime().toString("[hh:mm:ss]") << " ";
        switch (type)
        {
        case INFO:
            stream << "[INFO]: ";
            break;
        case WARNING:
            stream << "[WARNING]: ";
            break;
        case ERROR:
            stream << "[ERROR]: ";
            break;
        }
        stream << message << "\n";
    }
    file.close();
}