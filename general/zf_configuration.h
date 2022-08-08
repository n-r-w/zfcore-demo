#pragma once

#include "zf_basic_types.h"
#include <QObject>
#include <QSettings>
#include <QVariant>
#include <QDateTime>

class QSettings;

namespace zf
{
class Uid;

/*! Хранение настроек (информация хранится на компьютере пользователя)
 * Поддерживает управление версиями на уровне всей конфигурации и на уровне отдельных ключей
 * При смене типа данных необходимо повысить версию. Тогда старые значения автоматически сбросятся и
 * не произойдет падения приложения при чтении из потока */
class ZCORESHARED_EXPORT Configuration : public QObject
{
    Q_OBJECT
public:
    //! Надо задать config_сode или folder (можно оба)
    explicit Configuration(
        //! Код конфигурации. Используется для именования файла, в котором будут сохранены настройки
        const QString& config_сode,
        //! При несовпадении версии, используются разные файлы с постфиксом номера версии
        int version);
    explicit Configuration(
        //! Код сущности. Используется для именования файла, в котором будут сохранены настройки
        const EntityCode& entity_code,
        //! Код конфигурации. Используется для именования файла, в котором будут сохранены настройки
        const QString& config_сode,
        //! При несовпадении версии, используются разные файлы с постфиксом номера версии
        int version);
    ~Configuration();

    //! Версия
    int version() const;
    //! Код конфигурации
    QString configCode() const;

    //! Установить значение для ключа
    void setValue(const QString& key, const QVariant& value, int keyVersion = 0);
    void setValue(const char* key, const QVariant& value, int keyVersion = 0);
    void setUidValue(const QString& key, const Uid& value, int keyVersion = 0);
    void setUidValue(const char* key, const Uid& value, int keyVersion = 0);

    //! Получить значение для ключа
    QVariant value(const QString& key, const QVariant& defaultValue = QVariant(), int keyVersion = 0) const;
    Uid uidValue(const QString& key, int keyVersion = 0);
    //! Удалить ключ
    void remove(const QString& key);
    //! Содержит ли ключ
    bool contains(const QString& key, int keyVersion = 0) const;

    //! Записать изменения (иначе запишутся только при удалении объекта)
    void sync();
    //! Очистить
    void clear();

    //! Состояние
    QSettings::Status status() const;
    //! Файлы настроек
    QStringList fileNames() const;

signals:
    //! Ключ был изменен или удален
    void sg_keyChange(const QString& key);
    //! Удалены все настройки
    void sg_clear();

private:
    void init();

    //! Ключ для версии, связанный с ключом key
    QString versionKey(const QString& key) const;
    //! Проверка на ошибки
    void checkError();

    QSettings* updateSettingsFiles() const;
    QString activeConfigFile() const;

    //! Код конфигурации
    QString _config_code;
    //! Код сущности. Используется для именования файла, в котором будут сохранены настройки
    EntityCode _entity_code;
    //! Настройки. Ключ - имя файла
    mutable QMap<QString, QSettings*> _settings;
    //! Версия
    int _version;
    //! Была ли запротоколирована ошибка
    bool _error_logged = false;    
};

} // namespace zf
