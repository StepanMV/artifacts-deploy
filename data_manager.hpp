#ifndef DATAMANAGER_HPP
#define DATAMANAGER_HPP

#include <QString>
#include <QJsonObject>
#include "ssh_connection.hpp"

class DataManager
{
public:
    DataManager() = delete;

    static const QJsonObject& getData();
    static void setApiUrl(const QString& apiUrl);
    static void setUserToken(const QString& token);
    static void addCoolerList(const QString& key);

    static void appendCoolerList(const QString& key, const QJsonObject& data);
    static void editCoolerList(const QString& key, size_t index, const QJsonObject& data);
    static void removeFromCoolerList(const QString& key, size_t start, size_t end);

    static bool isPresent(const QString& arrayKey, const QString& key, const QString& value);
    static bool isPresent(const QString& arrayKey, const QString& key, size_t value);
    static QJsonObject getObject(const QString& arrayKey, const QString& key, const QString& value);
    static QJsonObject getObject(const QString& arrayKey, const QString& key, size_t value);
    static QList<QString> getList(const QString& arrayKey, const QString& key);
    static SSHConnection getConnection(const QString& name);

private:
    static QJsonObject data;
};

#endif // DATAMANAGER_HPP
