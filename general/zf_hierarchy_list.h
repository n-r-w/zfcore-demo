#pragma once

#include "zf_global.h"

namespace zf
{
//! Список пар ключ/значение. Для упрощения передачи JSON подобных данных
class ZCORESHARED_EXPORT HierarchyList
{
public:
    HierarchyList(const QString& id = {}, const QVariant& value = {}, const QMap<QString, QVariant>& data = {}, const QList<HierarchyList>& children = {});

    //! Не содержит данных
    bool isEmpty() const;

    //! Идентификатор
    QString id() const;
    //! Значение
    QVariant value() const;

    //! Данные (пары ключ-значение)
    const QMap<QString, QVariant>& data() const;
    //! Дочерние элементы
    const QList<HierarchyList>& children() const;

    //! Добавить данные
    void addData(const QString& key, const QVariant& value);
    void addData(const QMap<QString, QVariant>& c);
    //! Значение из Data
    QVariant value(const QString& key) const;

    //! Добавить дочерний элемент
    void addChild(const HierarchyList& c);    
    //! Добавить дочерний элемент, который не имеет своих дочерних, а только идентификатор и значение
    void addChild(const QString& id, const QVariant& value);

    void clear();

private:
    QString _id;
    QVariant _value;
    QMap<QString, QVariant> _data;
    QList<HierarchyList> _children;

    friend ZCORESHARED_EXPORT QDataStream& operator<<(QDataStream& s, const zf::HierarchyList& v);
    friend ZCORESHARED_EXPORT QDataStream& operator>>(QDataStream& s, zf::HierarchyList& v);
};

ZCORESHARED_EXPORT QDataStream& operator<<(QDataStream& s, const zf::HierarchyList& v);
ZCORESHARED_EXPORT QDataStream& operator>>(QDataStream& s, zf::HierarchyList& v);

} // namespace zf

Q_DECLARE_METATYPE(zf::HierarchyList)
