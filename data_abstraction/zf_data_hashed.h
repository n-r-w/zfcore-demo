#pragma once

#include "zf_data_structure.h"
#include "zf_rows.h"
#include "zf_resource_manager.h"
#include "zf_hashed_dataset.h"

namespace zf
{
//! Автоматический поиск с созданием хэш таблиц по наборам данных
class ZCORESHARED_EXPORT DataHashed : public QObject
{
    Q_OBJECT
public:
    DataHashed(
        //! Пары - код свойства набора данных, данные
        const QMap<PropertyID, const QAbstractItemModel*>& item_models = {},
        //! Управления освобождением ресурсов
        ResourceManager* resource_manager = nullptr);
    DataHashed(
        //! Пары - свойства набора данных, данные
        const QMap<DataProperty, const QAbstractItemModel*>& item_models,
        //! Управления освобождением ресурсов
        ResourceManager* resource_manager = nullptr);

    //! Содержит свойтство
    bool contains(const PropertyID& id) const;
    //! Содержит свойтство
    bool contains(const DataProperty& property) const;
    //! Удалить свойтство
    void remove(const PropertyID& id);
    //! Удалить свойтство
    void remove(const DataProperty& property);
    //! Добавить свойтство
    void add(const PropertyID& id, const QAbstractItemModel* item_model);
    //! Добавить свойтство
    void add(const DataProperty& property, const QAbstractItemModel* item_model);

    //! Сбросить хэш для свойства
    void clearHash(const PropertyID& id);
    //! Сбросить хэш для всех
    void clearHash();

    //! Управления освобождением ресурсов
    void setResourceManager(ResourceManager* resource_manager);

    //! Найти строки набора данных по комбинации номеров колонок. Количество элементов в первом и втором параметре
    //! должно совпадать
    Rows findRows(const DataPropertyList& columns, const QVariantList& values,
                  //! Колонки, по которым не учитыватся регистр. По остальным - учитывается
                  const DataPropertyList& case_insensitive_columns = DataPropertyList(),
                  //! Роли. Пусто или количество равно columns
                  const QList<int>& roles = QList<int>()) const;
    Rows findRows(const DataPropertyList& columns, const QStringList& values,
                  //! Колонки, по которым не учитыватся регистр. По остальным - учитывается
                  const DataPropertyList& case_insensitive_columns = DataPropertyList(),
                  //! Роли. Пусто или количество равно columns
                  const QList<int>& roles = QList<int>()) const;
    Rows findRows(const DataPropertyList& columns, const QList<int>& values,
                  //! Колонки, по которым не учитыватся регистр. По остальным - учитывается
                  const DataPropertyList& case_insensitive_columns = DataPropertyList(),
                  //! Роли. Пусто или количество равно columns
                  const QList<int>& roles = QList<int>()) const;

    //! Найти строки набора данных по комбинации полей. Количество элементов в первом параметре и
    //! количество элементов во вложенных массивах второго параметра должно совпадать
    //! Результаты поиска обобщаются
    Rows findRows(const DataPropertyList& columns, const QList<QVariantList>& values,
                  //! Колонки, по которым не учитыватся регистр. По остальным - учитывается
                  const DataPropertyList& case_insensitive_columns = DataPropertyList(),
                  //! Роли. Пусто или количество равно columns
                  const QList<int>& roles = QList<int>()) const;
    Rows findRows(const DataPropertyList& columns, const QList<QStringList>& values,
                  //! Колонки, по которым не учитыватся регистр. По остальным - учитывается
                  const DataPropertyList& case_insensitive_columns = DataPropertyList(),
                  //! Роли. Пусто или количество равно columns
                  const QList<int>& roles = QList<int>()) const;
    Rows findRows(const DataPropertyList& columns, const QList<QList<int>>& values,
                  //! Колонки, по которым не учитыватся регистр. По остальным - учитывается
                  const DataPropertyList& case_insensitive_columns = DataPropertyList(),
                  //! Роли. Пусто или количество равно columns
                  const QList<int>& roles = QList<int>()) const;

    //! Найти строки набора данных по одному полю
    Rows findRows(const DataProperty& column, const QVariant& value, bool case_insensitive = false, int role = Qt::DisplayRole) const;
    //! Найти строки набора данных по одному полю. Результаты поиска обобщаются
    Rows findRows(const DataProperty& column, const QVariantList& values, bool case_insensitive = false, int role = Qt::DisplayRole) const;
    Rows findRows(const DataProperty& column, const QStringList& values, bool case_insensitive = false, int role = Qt::DisplayRole) const;
    Rows findRows(const DataProperty& column, const QList<int>& values, bool case_insensitive = false, int role = Qt::DisplayRole) const;

    //! Есть ли строки со значениями value в колонке column
    bool contains(const DataProperty& column, const QVariant& value, bool case_insensitive = false, int role = Qt::DisplayRole) const;

private slots:
    void sl_freeResource();

private:
    //! Поиск по колонкам
    Rows findRowsHelper(const DataPropertyList& columns, const QVariantList& values, const DataPropertyList& case_insensitive_columns,
                        const QList<int>& roles) const;

    //! Быстрый поиск по комбинации полей набора данных
    QHash<PropertyID, std::shared_ptr<AutoHashedDataset>> _find_by_columns_hash;
    //! Управления освобождением ресурсов
    QPointer<ResourceManager> _resource_manager;
};
} // namespace zf
