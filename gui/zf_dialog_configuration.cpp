#include "zf_dialog_configuration.h"
#include "zf_core.h"
#include "zf_dialog.h"
#include <QDesktopWidget>

namespace zf
{
DialogConfigurationRepository::DialogConfigurationRepository()
{
}

DialogConfigurationRepository::~DialogConfigurationRepository()
{
    qDeleteAll(_dialogs);
}

DialogConfigurationData* DialogConfigurationRepository::dialogConfig(const QString& dialogCode) const
{
    return _dialogs.value(dialogCode.toLower());
}

void DialogConfigurationRepository::load(Dialog* dialog)
{
    Z_CHECK_NULL(dialog);

    QString dCode = dialog->dialogCode().trimmed();
    if (dCode.isEmpty())
        return;

    if (dCode == QStringLiteral("zf::Dialog")) {
        Core::logError("Использование zf::Dialog без наследования и установки dialogCode. Размеры не будут сохранены");
        return;
    }

    DialogConfigurationData* dc = dialogConfig(dCode);
    if (!dc) {
        dc = new DialogConfigurationData();
        _dialogs[dCode.toLower()] = dc;
    }

    dc->load(dialog);
}

void DialogConfigurationRepository::apply(Dialog* dialog)
{
    Z_CHECK_NULL(dialog);

    QString dCode = dialog->dialogCode().trimmed();
    if (dCode.isEmpty())
        return;

    if (dCode == QStringLiteral("zf::Dialog")) {
        Core::logError("Использование zf::Dialog без наследования и установки dialogCode. Размеры не будут сохранены");
        return;
    }

    DialogConfigurationData* dc = dialogConfig(dCode);
    if (!dc) {
        dc = new DialogConfigurationData();
        _dialogs[dCode.toLower()] = dc;
    }

    dc->apply(dialog);
}

Error DialogConfigurationRepository::saveToDevice(QIODevice* d) const
{
    Z_CHECK_NULL(d);
    Z_CHECK(d->isOpen() && d->isWritable());

    QDataStream ds;
    ds.setVersion(Consts::DATASTREAM_VERSION);
    ds.setDevice(d);
    ds << this;

    if (ds.status() == QDataStream::Ok)
        return Error();
    else
        return Error::fileIOError(d);
}

Error DialogConfigurationRepository::loadFromDevice(QIODevice* d)
{
    Z_CHECK_NULL(d);
    Z_CHECK(d->isOpen() && d->isReadable());

    QDataStream ds;
    ds.setVersion(Consts::DATASTREAM_VERSION);
    ds.setDevice(d);
    ds >> this;

    if (ds.status() == QDataStream::Ok)
        return Error();
    else
        return Error::fileIOError(d);
}

DialogConfigurationData::DialogConfigurationData()
{
}

DialogConfigurationData::~DialogConfigurationData()
{
}

QByteArray DialogConfigurationData::actualGeometry() const
{
    return _actual_geometry;
}

void DialogConfigurationData::load(Dialog* dialog)
{
    Z_CHECK_NULL(dialog);
    _actual_geometry = dialog->saveGeometry();
    _data = dialog->getConfigurationInternal();
}

void DialogConfigurationData::apply(Dialog* dialog) const
{
    Z_CHECK_NULL(dialog);

    if (!_actual_geometry.isEmpty()) {
        dialog->restoreGeometry(_actual_geometry);
        if (dialog->normalGeometry() == dialog->geometry() && dialog->windowState().testFlag(Qt::WindowState::WindowMaximized)) {
            // борьба с криворукими дятлами из Qt, которые некорректно сохраняют normalGeometry
            dialog->showNormal();
            Utils::resizeWindowToScreen(dialog, QSize(), true, 25);
            dialog->showMaximized();
        }
    }

    dialog->applyConfigurationInternal(_data);
}

//! Версия данных стрима DialogConfigurationData
static int _DialogConfigurationDataStreamVersion = 3;
QDataStream& operator<<(QDataStream& out, const DialogConfigurationData* obj)
{
    out << _DialogConfigurationDataStreamVersion;
    out << obj->_actual_geometry;
    out << obj->_data;

    return out;
}

QDataStream& operator>>(QDataStream& in, DialogConfigurationData* obj)
{
    int version;
    in >> version;
    if (version != _DialogConfigurationDataStreamVersion) {
        if (in.status() == QDataStream::Ok)
            in.setStatus(QDataStream::ReadCorruptData);
        return in;
    }

    in >> obj->_actual_geometry;
    in >> obj->_data;

    return in;
}

//! Версия данных стрима DialogConfigurationRepository
static int _DialogConfigurationRepository = 1;
QDataStream& operator<<(QDataStream& out, const DialogConfigurationRepository* obj)
{
    out << _DialogConfigurationRepository;
    out << obj->_dialogs.count();

    for (auto& code : obj->_dialogs.keys()) {
        DialogConfigurationData* c = obj->_dialogs.value(code);

        out << code;
        out << c;
    }
    return out;
}

QDataStream& operator>>(QDataStream& in, DialogConfigurationRepository* obj)
{
    qDeleteAll(obj->_dialogs);
    obj->_dialogs.clear();

    int version;
    in >> version;
    if (version != _DialogConfigurationRepository) {
        if (in.status() == QDataStream::Ok)
            in.setStatus(QDataStream::ReadCorruptData);
        return in;
    }

    int count;
    in >> count;

    for (int i = 0; i < count; i++) {
        QString code;
        in >> code;

        DialogConfigurationData* dc = new DialogConfigurationData();
        in >> dc;

        if (code.isEmpty()) {
            delete dc;
            continue;
        }

        obj->_dialogs[code] = dc;
    }

    if (in.status() != QDataStream::Ok) {
        qDeleteAll(obj->_dialogs);
        obj->_dialogs.clear();
        Core::logError("DialogConfigurationRepository: error loading from stream");
    }

    return in;
}
} // namespace zf
