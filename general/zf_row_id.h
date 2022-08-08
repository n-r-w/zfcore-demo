#pragma once

#include "zf_basic_types.h"
#include <QAbstractItemModel>
#include <QSharedDataPointer>

namespace zf
{
class RowID_data;
class RowID;
} // namespace zf

ZCORESHARED_EXPORT QDataStream& operator<<(QDataStream& s, const zf::RowID& c);
ZCORESHARED_EXPORT QDataStream& operator>>(QDataStream& s, zf::RowID& c);

namespace zf
{
/*! Идентификатор строки набора данных
 * Есть два основных вида идентификаторов: 
 * 1) основанный на реальном ключе набора данных
 * 2) основанный на случайно сгенерированном ключе (для новых строк)
 * Кроме того в дополнение к ключу можно запомнить положение строки в наборе данных. 
 * Это может быть полезно ,к примеру, при восстановлении положения курсора в таблице после перезагрузки из БД, когда 
 * на сервере строка была удалена и по ключу мы ее найти не сможем */
class ZCORESHARED_EXPORT RowID
{
public:
    RowID();
    RowID(const RowID& u);
    ~RowID();

    //! Создать идентификатор строки на основе реального ключа
    //! Использовать при переопределении методов ModuleDataObject::readDatasetRowID, ModuleDataObject::writeDatasetRowID
    static RowID createRealKey(int key,
                               //! Индекс для сохранения информации о физическом положении. Сам QModelIndex не сохраняется
                               const QModelIndex& index = QModelIndex());
    //! Создать идентификатор строки на основе реального ключа
    //! Использовать при переопределении методов ModuleDataObject::readDatasetRowID, ModuleDataObject::writeDatasetRowID
    static RowID createRealKey(const QString& key,
                               //! Индекс для сохранения информации о физическом положении. Сам QModelIndex не сохраняется
                               const QModelIndex& index = QModelIndex());
    //! Создать идентификатор строки на основе идентификатора сущности
    //! Использовать при переопределении методов ModuleDataObject::readDatasetRowID, ModuleDataObject::writeDatasetRowID
    static RowID createRealKey(const EntityID& key,
                               //! Индекс для сохранения информации о физическом положении. Сам QModelIndex не сохраняется
                               const QModelIndex& index = QModelIndex());
    //! Создать идентификатор строки на основе положения в наборе данных. Использовать осторожно.
    //! Использовать при переопределении методов ModuleDataObject::readDatasetRowID, ModuleDataObject::writeDatasetRowID
    static RowID createRealKey(const QModelIndex& index);
    //! Создать идентификатор строки на основе уникальной строки
    //! Использовать при переопределении методов ModuleDataObject::readDatasetRowID, ModuleDataObject::writeDatasetRowID
    static RowID createGenerated(
        //! Индекс для сохранения информации о физическом положении. Сам QModelIndex не сохраняется
        const QModelIndex& index = QModelIndex());

    bool operator==(const RowID& u) const;
    bool operator!=(const RowID& u) const;
    bool operator<(const RowID& u) const;
    RowID& operator=(const RowID& u);

    bool isValid() const;
    void clear();

    //! Был создан на основе ключа
    bool isRealKey() const;
    //! Был ли создан с использованием уникального идентификатора
    bool isGenerated() const;

    //! Идентификатор. Доступен только при использовании createRealKey
    QString key() const;

    //! Номера строк по уровням вложенности от нулевого и ниже.
    //! Доступно только если идентификатор был создан на основе QModelIndex (isRowBased() == true)
    const QList<int>& positionInfo() const;
    //! Был ли создан с использованием информации о положении в наборе данных
    bool hasPositionInfo() const;
    //! Добавить информацию о положении в наборе данных
    void setPositionInfo(const QModelIndex& index);

    //! Восстановить индекс на основании сохраненных номеров строк.
    //! Доступно только если идентификатор был создан на основе QModelIndex (isRowBased() == true)
    QModelIndex restoreModelIndex(const QAbstractItemModel* model, int column) const;

    //! Преобразовать в QVariant
    QVariant variant() const;
    //! Преобразовать из QVariant
    static RowID fromVariant(const QVariant& v);

    uint hashValue() const;
    //! Составной уникальный ключ
    QString uniqueKey() const;

    QString toPrintable() const;

private:
    RowID(const QString& key, const QString& unique_id, const QModelIndex& index);

    //! Данные
    QSharedDataPointer<RowID_data> _d;

    friend QDataStream& ::operator<<(QDataStream& s, const zf::RowID& c);
    friend QDataStream& ::operator>>(QDataStream& s, zf::RowID& c);
};

inline uint qHash(const RowID& u)
{
    return ::qHash(u.hashValue());
}

} // namespace zf

Q_DECLARE_METATYPE(zf::RowID)
ZCORESHARED_EXPORT QDebug operator<<(QDebug debug, const zf::RowID& c);
