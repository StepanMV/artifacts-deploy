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

    static void clearCoolerList(const QString& key);
    static void removeFromCoolerList(const QString& key, size_t index);
    static void removeFromCoolerList(const QString& key, size_t start, size_t end);

    static bool isPresent(const QString& arrayKey, const QString& key, const QString& value);
    // ssh is a list inside the data: "find element in a list whose key has a value == value"
    static QJsonObject getObject(const QString& arrayKey, const QString& key, const QString& value);
    // get all values of a key in a list
    static QList<QString> getList(const QString& arrayKey, const QString& key);
    static SSHConnection *getConnection(const QString& name);

    static void dumpData(const QString& path);
    static void loadData(const QString& path);

private:
    static QJsonObject data;
};

#endif // DATAMANAGER_HPP
