#pragma once

#include "zf_error.h"
#include <QSize>

namespace zf
{
class Dialog;

class ZCORESHARED_EXPORT DialogConfigurationData
{
    friend ZCORESHARED_EXPORT QDataStream& operator<<(QDataStream& out, const DialogConfigurationData* obj);
    friend ZCORESHARED_EXPORT QDataStream& operator>>(QDataStream& in, DialogConfigurationData* obj);

public:
    DialogConfigurationData();
    ~DialogConfigurationData();

    //! Размер и положение
    QByteArray actualGeometry() const;
    //! Произвольные данные
    QMap<QString, QVariant> data() const;

    void load(Dialog* dialog);
    void apply(Dialog* dialog) const;

private:
    QByteArray _actual_geometry;
    QMap<QString, QVariant> _data;
};

ZCORESHARED_EXPORT QDataStream& operator<<(QDataStream& out, const DialogConfigurationData* obj);
ZCORESHARED_EXPORT QDataStream& operator>>(QDataStream& in, DialogConfigurationData* obj);

//! Сохранение состояния диалогов
class ZCORESHARED_EXPORT DialogConfigurationRepository
{
    friend ZCORESHARED_EXPORT QDataStream& operator<<(QDataStream& out, const DialogConfigurationRepository* obj);
    friend ZCORESHARED_EXPORT QDataStream& operator>>(QDataStream& in, DialogConfigurationRepository* obj);

public:
    DialogConfigurationRepository();
    ~DialogConfigurationRepository();

    DialogConfigurationData* dialogConfig(const QString& dialogCode) const;

    void load(Dialog* dialog);
    void apply(Dialog* dialog);

    //! Сохранить в файл
    Error saveToDevice(QIODevice* d) const;
    //! Загрузить из файла
    Error loadFromDevice(QIODevice* d);

private:
    //! Ключ - код диалога
    QHash<QString, DialogConfigurationData*> _dialogs;
};

ZCORESHARED_EXPORT QDataStream& operator<<(QDataStream& out, const DialogConfigurationRepository* obj);
ZCORESHARED_EXPORT QDataStream& operator>>(QDataStream& in, DialogConfigurationRepository* obj);

} // namespace zf
