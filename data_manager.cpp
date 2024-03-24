#include "data_manager.hpp"
#include <QJsonArray>

QJsonObject DataManager::data;

const QJsonObject& DataManager::getData()
{
    return data;
}

void DataManager::setApiUrl(const QString& apiUrl)
{
    data["apiUrl"] = apiUrl;
}

void DataManager::setUserToken(const QString& token)
{
    data["userToken"] = token;
}

void DataManager::addCoolerList(const QString& key)
{
    data[key] = QJsonArray();
}

void DataManager::appendCoolerList(const QString& key, const QJsonObject& data)
{
    DataManager::data[key] = DataManager::data.value(key).toArray() + data;
}

void DataManager::editCoolerList(const QString& key, size_t index, const QJsonObject& data)
{
    auto temp = DataManager::data.value(key).toArray();
    temp.replace(index, data);
    DataManager::data[key] = temp;
}

void DataManager::removeFromCoolerList(const QString& key, size_t start, size_t end)
{
    auto temp = data.value(key).toArray();
    for (size_t i = 0; i < end - start; ++i) {
        temp.removeAt(start);
    }
    data[key] = temp;
}

bool DataManager::isPresent(const QString& arrayKey, const QString& key, const QString& value)
{
    auto temp = data.value(arrayKey).toArray();
    for(auto it = temp.begin(); it != temp.end(); ++it)
    {
        if (it->toObject().value(key).toString() == value) return true;
    }
    return false;
}

bool DataManager::isPresent(const QString& arrayKey, const QString& key, size_t value)
{
    auto temp = data.value(arrayKey).toArray();
    for(auto it = temp.begin(); it != temp.end(); ++it)
    {
        if (it->toObject().value(key).toInt() == value) return true;
    }
    return false;
}

QList<QString> DataManager::getList(const QString& arrayKey, const QString& key)
{
    QList<QString> array;
    auto temp = data.value(arrayKey).toArray();
    for(auto it = temp.begin(); it != temp.end(); ++it)
    {
        array.append(it->toObject().value(key).toString());
    }
    return array;
}

QJsonObject DataManager::getObject(const QString& arrayKey, const QString& key, const QString& value)
{
    auto temp = data.value(arrayKey).toArray();
    for(auto it = temp.begin(); it != temp.end(); ++it)
    {
        if (it->toObject().value(key).toString() == value) return it->toObject();
    }
    return QJsonObject();
}

QJsonObject DataManager::getObject(const QString& arrayKey, const QString& key, size_t value)
{
    auto temp = data.value(arrayKey).toArray();
    for(auto it = temp.begin(); it != temp.end(); ++it)
    {
        if (it->toObject().value(key).toInt() == value) return it->toObject();
    }
    return QJsonObject();
}

SSHConnection DataManager::getConnection(const QString& name)
{
    QJsonObject sshData = DataManager::getObject("ssh", "name", name);
    std::string ip = sshData.value("ip").toString().toStdString();
    size_t port = sshData.value("port").toInt();
    std::string user = sshData.value("user").toString().toStdString();
    std::string keyPath = sshData.value("keyPath").toString().toStdString();
    std::string keyPass = sshData.value("keyPass").toString().toStdString();
    std::string password = sshData.value("password").toString().toStdString();
    if(password.empty()) return SSHConnection(ip, port, user, keyPath, keyPass);
    else return SSHConnection(ip, port, user, password);
}
