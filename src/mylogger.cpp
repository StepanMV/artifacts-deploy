#include "mylogger.hpp"

#include <QFile>
#include <QTextStream>
#include <QDateTime>

QString MyLogger::logPath;

void MyLogger::setLogPath(const QString &path)
{
    logPath = path;
}

void MyLogger::log(const QString &message, MyLogType type)
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