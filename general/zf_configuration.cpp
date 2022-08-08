#include "zf_configuration.h"
#include "zf_core.h"
#include "zf_framework.h"

#include <QSettings>

#define Z_FORM_CONFIG_VERSION QStringLiteral("version")

namespace zf
{
Configuration::Configuration(const QString& config_сode, int version)
    : QObject()
    , _config_code(config_сode.trimmed())
    , _version(version)
{
    Z_CHECK(!_config_code.isEmpty());

    init();
}

Configuration::Configuration(const EntityCode& entity_code, const QString& config_сode, int version)
    : QObject()
    , _config_code(config_сode.trimmed())
    , _entity_code(entity_code)
    , _version(version)
{
    Z_CHECK(_entity_code.isValid());

    init();
}

Configuration::~Configuration()
{
    sync();
    qDeleteAll(_settings);
}

QString Configuration::configCode() const
{
    return _config_code;
}

void Configuration::clear()
{
    for (auto s : qAsConst(_settings)) {
        s->clear();
    }
    emit sg_clear();
}

QSettings::Status Configuration::status() const
{
    for (auto s : qAsConst(_settings)) {
        auto status = s->status();
        if (status == QSettings::Status::NoError)
            continue;

        return status;
    }

    return QSettings::Status::NoError;
}

QStringList Configuration::fileNames() const
{
    QStringList res;
    for (auto s : qAsConst(_settings)) {
        res << s->fileName();
    }

    return res;
}

void Configuration::init()
{
    if (contains(Z_FORM_CONFIG_VERSION)) {
        if (_version != value(Z_FORM_CONFIG_VERSION).toInt()) {
            // Версия не совпадает, очищаем настройки
            clear();
            // Устанавливаем новое значение версии
            setValue(Z_FORM_CONFIG_VERSION, _version);
        }
    } else {
        // Данных о версии нет
        clear();
        // Устанавливаем новое значение версии
        setValue(Z_FORM_CONFIG_VERSION, _version);
    }
}

QString Configuration::versionKey(const QString& key) const
{
    return key + QStringLiteral("_VERSION");
}

void Configuration::checkError()
{
    if (_error_logged)
        return;

    if (status() != QSettings::NoError) {
        _error_logged = true;
        Core::logError("Configuration error: " + fileNames().join("; "));
    }
}

QSettings* Configuration::updateSettingsFiles() const
{
    QString file_name = activeConfigFile();

    QSettings* s = _settings.value(file_name);
    if (s == nullptr) {
        QSettings* copy_from = _settings.isEmpty() || QFile::exists(file_name) ? nullptr : _settings.constBegin().value();
        s = new QSettings(file_name, QSettings::IniFormat);
        _settings[file_name] = s;

        if (copy_from != nullptr) {
            auto keys = copy_from->allKeys();
            for (auto& key : qAsConst(keys)) {
                s->setValue(key, copy_from->value(key));
            }
        }
    }
    return s;
}

QString Configuration::activeConfigFile() const
{
    QString folder;
    QString file;
    if (_entity_code.isValid()) {
        folder = Utils::moduleDataLocation(_entity_code);
        if (_config_code.isEmpty())
            file = QStringLiteral("entity%1_%2.ini").arg(_entity_code.string()).arg(_version);
        else
            file = QStringLiteral("%1_%2.ini").arg(_config_code).arg(_version);

    } else {
        folder = Utils::dataLocation();
        file = QStringLiteral("%1_%2.ini").arg(_config_code).arg(_version);
    }

    return QStringLiteral("%1/%2").arg(folder).arg(file);
}

void Configuration::setValue(const QString& key, const QVariant& value, int keyVersion)
{
    updateSettingsFiles();

    for (auto s : qAsConst(_settings)) {
        s->setValue(versionKey(key), keyVersion);
        s->setValue(key, value);
    }
    emit sg_keyChange(key);

    checkError();
}

void Configuration::setValue(const char* key, const QVariant& value, int keyVersion)
{
    setValue(utf(key), value, keyVersion);
}

void Configuration::setUidValue(const QString& key, const Uid& value, int keyVersion)
{
    setValue(key, QVariant::fromValue<Uid>(value), keyVersion);
}

void Configuration::setUidValue(const char* key, const Uid& value, int keyVersion)
{
    setValue(utf(key), QVariant::fromValue<Uid>(value), keyVersion);
}

QVariant Configuration::value(const QString& key, const QVariant& defaultValue, int keyVersion) const
{
    QSettings* active = updateSettingsFiles();

    if (!contains(key, keyVersion)) {
        for (auto s : qAsConst(_settings)) {
            s->remove(versionKey(key));
            s->remove(key);
        }
        return defaultValue;
    }

    return active->value(key, defaultValue);
}

Uid Configuration::uidValue(const QString& key, int keyVersion)
{
    return value(key, keyVersion).value<Uid>();
}

void Configuration::remove(const QString& key)
{
    updateSettingsFiles();

    for (auto s : qAsConst(_settings)) {
        s->remove(versionKey(key));
        s->remove(key);
    }
    emit sg_keyChange(key);

    checkError();
}

bool Configuration::contains(const QString& key, int keyVersion) const
{
    updateSettingsFiles();

    for (auto s : qAsConst(_settings)) {
        QString keyVer = versionKey(key);
        if (!s->contains(keyVer))
            continue;

        bool ok;
        int ver = s->value(keyVer, -1).toInt(&ok);
        if (!ok || ver != keyVersion)
            continue;

        if (s->contains(key))
            return true;
    }
    return false;
}

void Configuration::sync()
{
    for (auto s : qAsConst(_settings)) {
        s->sync();
    }
    checkError();
}
} // namespace zf
