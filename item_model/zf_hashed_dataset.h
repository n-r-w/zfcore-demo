#pragma once

#include "zf_data_structure.h"
#include "zf_rows.h"
#include <QSharedDataPointer>

namespace zf
{

//! Интерфейс для кастомизации HashedDataset
class ZCORESHARED_EXPORT I_HashedDatasetCutomize
{
public:
    //! Преобразовать набор значений ключевых полей в уникальную строку
    virtual QString hashedDatasetkeyValuesToUniqueString(
        //! Ключ для идентификации при обратном вызове I_HashedDatasetCutomize
        const QString& key,
        //! Строка набора данных
        int row,
        //! Ролитель строки
        const QModelIndex& parent,
        //! Ключевые значения
        const QVariantList& keyValues) const = 0;
};

//! Хэширование строк набора данных по ключевым полям
class ZCORESHARED_EXPORT HashedDataset : public QObject
{
    Q_OBJECT
public:
    HashedDataset(const QAbstractItemModel* itemModel, const QList<int>& keyColumns,
                  //! Роли. Пусто или количество равно keyColumns
                  const QList<int>& roles = QList<int>());
    HashedDataset(const QAbstractItemModel* itemModel, int keyColumn1, int keyColumn2 = -1, int keyColumn3 = -1,
                  int keyColumn4 = -1, int keyColumn5 = -1, int keyColumn6 = -1, int keyColumn7 = -1,
                  int keyColumn8 = -1, int keyColumn9 = -1);
    ~HashedDataset() override;

    //! Связать с DataContainer. Используется для блокировки реакции на сигналы
    //! Если установлено, то класс не будет реагировать на сигналы модели при заблокированном наборе данных
    void setDataContainer(const DataContainer* data, const DataProperty& dataset_property);

    const DataContainer* dataContainer() const;
    DataProperty datasetProperty() const;

    //! Учитывать ли регистр по колонке
    bool isCaseInsensitive(int keyColumn) const;
    //! Задать учитывать ли регистр по колонке
    void setCaseInsensitive(int keyColumn, bool isCaseInsensitive);
    //! Полностью заменяет список не чувствительных к регистру колонок
    void setCaseInsensitive(const QList<int>& case_insensitive_columns);

    //! Надо ли автоматически перестраивать хэш при изменении данных
    bool isAutoUpdate() const;
    void setAutoUpdate(bool b);

    //! Список ключевых колонок
    QList<int> keyColumns() const;
    //! Роли для каждой колонки
    QList<int> roles() const;

    //! Уникальная строка на основании комбинации колонок. Характеризует уникальность именно набора колонок, а не данных
    QString uniqueKey() const;
    //! Сформировать уникальную строку на основании комбинации колонок. Характеризует уникальность именно набора
    //! колонок, а не данных
    static QString uniqueKeyGenerate(const QList<int>& columns,
                                     //! Колонки, по которым не учитывается регистр
                                     const QList<int>& case_insensitive_columns = QList<int>(),
                                     //! Роли. Пусто или количество равно columns
                                     const QList<int>& roles = QList<int>());
    //! Сгенерировать хэш ключ по списку значений
    QString generateKey(const QVariantList& values) const;

    //! Найти строки по набору ключевых полей
    Rows findRows(const QVariantList& values) const;
    Rows findRows(const QVariant& value) const;
    Rows findRows(const QVariant& value1, const QVariant& value2) const;
    Rows findRows(const QVariant& value1, const QVariant& value2, const QVariant& value3) const;
    Rows findRows(const QVariant& value1, const QVariant& value2, const QVariant& value3, const QVariant& value4) const;
    Rows findRows(const QVariant& value1, const QVariant& value2, const QVariant& value3, const QVariant& value4,
                  const QVariant& value5) const;
    Rows findRows(const QVariant& value1, const QVariant& value2, const QVariant& value3, const QVariant& value4,
                  const QVariant& value5, const QVariant& value6) const;
    Rows findRows(const QVariant& value1, const QVariant& value2, const QVariant& value3, const QVariant& value4,
                  const QVariant& value5, const QVariant& value6, const QVariant& value7) const;
    Rows findRows(const QVariant& value1, const QVariant& value2, const QVariant& value3, const QVariant& value4,
                  const QVariant& value5, const QVariant& value6, const QVariant& value7, const QVariant& value8) const;
    Rows findRows(const QVariant& value1, const QVariant& value2, const QVariant& value3, const QVariant& value4,
                  const QVariant& value5, const QVariant& value6, const QVariant& value7, const QVariant& value8,
                  const QVariant& value9) const;

    //! Поиск по конкретному хэш-ключу
    Rows findRowsByHash(const QString& hash_key) const;

    //! Количество уникальных строк
    int uniqueRowCount() const;
    //! Набор значений ключевых полей для уникальной строки
    QVariantList uniqueRowValues(int i) const;

    //! Набор данных
    const QAbstractItemModel* itemModel() const;

    //! Количество строк в наборе даннных
    int rowCount(const QModelIndex& parent = QModelIndex()) const;
    //! Количество колонок в наборе даннных
    int columnCount(const QModelIndex& parent = QModelIndex()) const;

    //! Данные из dataset - QVariant
    QVariant variant(int row, int column, int role = Qt::DisplayRole, const QModelIndex& parent = QModelIndex()) const;
    //! Данные из dataset - QString
    QString string(int row, int column, int role = Qt::DisplayRole, const QModelIndex& parent = QModelIndex()) const;
    //! Данные из dataset - int
    int integer(int row, int column, int role = Qt::DisplayRole, const QModelIndex& parent = QModelIndex()) const;

    //! Сбросить хэш. Будет сгенерирован при первом запросе findRows
    void clear();

    //! Очистить хэш при необходимости
    void clearIfNeed(const QModelIndex& topLeft, const QModelIndex& bottomRight);

    //! Установить интерфейс кастомизации
    void setCustomization(const I_HashedDatasetCutomize* ci,
                          //! Ключ для идентификации при обратном вызове I_HashedDatasetCutomize
                          const QString& key);

private slots:
    // Реакция на сигналы QAbstractItemModel
    //! Поменялись данные
    void sl_itemDataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight);

    // Реакция на сигналы DataContainer
    /* эти сигналы пока не нужны
    void sl_dataset_dataChanged(const DataProperty& p, const QModelIndex& topLeft, const QModelIndex& bottomRight);
    void sl_dataset_rowsRemoved(const DataProperty& p, const QModelIndex& parent, int first, int last);
    void sl_dataset_rowsInserted(const DataProperty& p, const QModelIndex& parent, int first, int last);
    void sl_dataset_rowsMoved(const DataProperty& p, const QModelIndex& parent, int start, int end,
                              const QModelIndex& destination, int row);
    void sl_dataset_columnsInserted(const DataProperty& p, const QModelIndex& parent, int first, int last);
    void sl_dataset_columnsRemoved(const DataProperty& p, const QModelIndex& parent, int first, int last);
    void sl_dataset_columnsMoved(const DataProperty& p, const QModelIndex& parent, int start, int end,
                                 const QModelIndex& destination, int column); */
    void sl_allPropertiesUnBlocked();
    void sl_propertyUnBlocked(const zf::DataProperty& p);

private:
    void connectToDataContainer();

    //! Принудительно сгенерировать хэш для всех строк
    void updateHash();
    void initKeyColumns(int keyColumn1, int keyColumn2, int keyColumn3, int keyColumn4, int keyColumn5, int keyColumn6,
            int keyColumn7, int keyColumn8, int keyColumn9);
    void init();

    // Очистка хэша
    void clearHelper(
            //! Принудительная
            bool force);
    //! Очистить хэш при необходимости
    void clearIfNeedHelper(const QModelIndex& topLeft, const QModelIndex& bottomRight,
            //! Принудительная
            bool force);

    //! Сгенерировать хэш для всех строк указанного родителя (рекурсия)
    void updateHashHelper(const QModelIndex& parent);
    //! Обновить хэш для указанной строки
    void updateRowHash(int row, const QModelIndex& parent);

    //! Сгенерировать хэш ключ для строки набора данных
    QString generateRowHashKey(int row, const QModelIndex& parent) const;

    void prepareCaseInsensitive();

    const QAbstractItemModel* _item_model = nullptr;
    QList<int> _key_columns;

    //! Роли для каждой колонки
    QList<int> _roles;

    struct HashData
    {
        HashData() {}
        HashData(const QModelIndex& idx)
            : index(idx)
        {
        }

        QModelIndex index;
    };

    //! Соответствие ключа и данных о строке набора данных
    QMultiHash<QString, HashData*> _hash;
    //! Набор уникальных ключевых значений
    QList<QVariantList> _unique_values;

    //! Имеется сгенерированный хэш
    bool _hash_generated = false;

    //! Надо ли автоматически перестраивать хэш при изменении данных
    bool _is_auto_update = true;

    const DataContainer* _data_container = nullptr;
    DataProperty _dataset_property;

    //! Уникальная строка на основании комбинации колонок
    QString _unique_key;

    //! Учитывать ли регистр по колонке
    QHash<int, bool> _case_insensitive;
    QList<bool> _case_insensitive_prepared;

    //! Интерфейс кастомизации
    const I_HashedDatasetCutomize* _customize = nullptr;
    //! Ключ для идентификации при обратном вызове I_HashedDatasetCutomize
    QString _customize_key;
};

//! Автоматически создает хеш по набору данных для каждой из запрашиваемых комбинаций колонок
class ZCORESHARED_EXPORT AutoHashedDataset
{
public:
    AutoHashedDataset(const QAbstractItemModel* itemModel,
                      //! Удалять ли набор данных при удалении этого объекта
                      bool take_ownership = false);
    ~AutoHashedDataset();

    //! Исходные данные
    const QAbstractItemModel* itemModel() const { return _item_model; }

    //! Количество строк в наборе даннных
    int rowCount(const QModelIndex& parent = QModelIndex()) const;
    //! Количество колонок в наборе даннных
    int columnCount(const QModelIndex& parent = QModelIndex()) const;

    //! Задать имена колонок. Количество должно совпадать с количеством колонок в dataset
    void setColumnNames(const QStringList& names);
    //! Номер колонки по имени
    int col(const QString& column_name) const;

    //! Данные из dataset - QVariant
    QVariant variant(int row, int column, int role = Qt::DisplayRole, const QModelIndex& parent = QModelIndex()) const;
    //! Данные из dataset - QVariant. По имени колонки
    QVariant variant(int row, const QString& column_name, int role = Qt::DisplayRole,
            const QModelIndex& parent = QModelIndex()) const;
    //! Данные из dataset - QString
    QString string(int row, int column, int role = Qt::DisplayRole, const QModelIndex& parent = QModelIndex()) const;
    //! Данные из dataset - QString. По имени колонки
    QString string(int row, const QString& column_name, int role = Qt::DisplayRole,
            const QModelIndex& parent = QModelIndex()) const;
    //! Данные из dataset - int
    int integer(int row, int column, int role = Qt::DisplayRole, const QModelIndex& parent = QModelIndex()) const;
    //! Данные из dataset - int. По имени колонки
    int integer(int row, const QString& column_name, int role = Qt::DisplayRole,
                const QModelIndex& parent = QModelIndex()) const;

    //! Связать с DataContainer. Используется для блокировки реакции на сигналы
    //! Если установлено, то класс не будет реагировать на сигналы модели при заблокированном наборе данных
    void setDataContainer(const DataContainer* data, const DataProperty& dataset_property);

    //*** Методы поиска. Для каждой комбинации полей автоматически создается хэш

    //! Найти строки набора данных по комбинации номеров колонок. Количество элементов в первом и втором параметре
    //! должно совпадать
    Rows findRows(const QList<int>& columns, const QVariantList& values,
                  //! Колонки, по которым не учитыватся регистр. По остальным - учитывается
                  const QList<int>& case_insensitive_columns = QList<int>(),
                  //! Роли. Пусто или количество равно columns
                  const QList<int>& roles = QList<int>()) const;
    Rows findRows(const QList<int>& columns, const QStringList& values,
                  //! Колонки, по которым не учитыватся регистр. По остальным - учитывается
                  const QList<int>& case_insensitive_columns = QList<int>(),
                  //! Роли. Пусто или количество равно columns
                  const QList<int>& roles = QList<int>()) const;
    Rows findRows(const QList<int>& columns, const QList<int>& values,
                  //! Колонки, по которым не учитыватся регистр. По остальным - учитывается
                  const QList<int>& case_insensitive_columns = QList<int>(),
                  //! Роли. Пусто или количество равно columns
                  const QList<int>& roles = QList<int>()) const;

    //! По именам колонок (если они заданы через setColumnNames)
    Rows findRows(const QStringList& columns, const QVariantList& values,
                  //! Колонки, по которым не учитыватся регистр. По остальным - учитывается
                  const QList<int>& case_insensitive_columns = QList<int>(),
                  //! Роли. Пусто или количество равно columns
                  const QList<int>& roles = QList<int>()) const;

    //! Найти строки набора данных по комбинации полей. Количество элементов в первом параметре и
    //! количество элементов во вложенных массивах второго параметра должно совпадать
    //! Результаты поиска обобщаются
    Rows findRows(const QList<int>& columns, const QList<QVariantList>& values,
                  //! Колонки, по которым не учитыватся регистр. По остальным - учитывается
                  const QList<int>& case_insensitive_columns = QList<int>(),
                  //! Роли. Пусто или количество равно columns
                  const QList<int>& roles = QList<int>()) const;
    Rows findRows(const QList<int>& columns, const QList<QStringList>& values,
                  //! Колонки, по которым не учитыватся регистр. По остальным - учитывается
                  const QList<int>& case_insensitive_columns = QList<int>(),
                  //! Роли. Пусто или количество равно columns
                  const QList<int>& roles = QList<int>()) const;
    Rows findRows(const QList<int>& columns, const QList<QList<int>>& values,
                  //! Колонки, по которым не учитыватся регистр. По остальным - учитывается
                  const QList<int>& case_insensitive_columns = QList<int>(),
                  //! Роли. Пусто или количество равно columns
                  const QList<int>& roles = QList<int>()) const;

    //! Найти строки набора данных по одному полю
    Rows findRows(int column, const QVariant& value, bool caseInsensitive = false, int role = Qt::DisplayRole) const;
    //! Найти строки набора данных по одному полю. Результаты поиска обобщаются
    Rows findRows(int column, const QVariantList& value, bool caseInsensitive = false,
                  int role = Qt::DisplayRole) const;
    Rows findRows(int column, const QStringList& values, bool caseInsensitive = false,
                  int role = Qt::DisplayRole) const;
    Rows findRows(int column, const QList<int>& values, bool caseInsensitive = false, int role = Qt::DisplayRole) const;

    //! Найти строки набора данных по одному полю (по имени поля, если имена заданы через setColumnNames)
    Rows findRows(const QString& column_name, const QVariant& value, bool caseInsensitive = false,
                  int role = Qt::DisplayRole) const;

    //! Количество уникальных строк
    int uniqueRowCount(const QList<int>& columns, const QList<int>& case_insensitive_columns = QList<int>(),
                       const QList<int>& roles = QList<int>()) const;
    //! Набор значений ключевых полей для уникальной строки
    QVariantList uniqueRowValues(int i, const QList<int>& columns,
                                 const QList<int>& case_insensitive_columns = QList<int>(),
                                 const QList<int>& roles = QList<int>()) const;

    //! Очистить хэш
    void clear();

    //! Очистить хэш при необходимости
    void clearIfNeed(const QModelIndex& topLeft, const QModelIndex& bottomRight);

    //! Вернуть все строки набора данных, кроме указанных
    Rows invertRows(const Rows& rows) const;

private:
    //! Получить HashedDataset для данной комбинации колонок и т.д.
    HashedDataset* getHashedDataset(const QList<int>& columns, const QList<int>& case_insensitive_columns,
                                    const QList<int>& roles) const;

    //! Поиск по колонкам
    Rows findRowsHelper(const QList<int>& columns, const QVariantList& values,
                        const QList<int>& case_insensitive_columns, const QList<int>& roles) const;

    const QAbstractItemModel* _item_model = nullptr;
    const DataContainer* _data_container = nullptr;
    DataProperty _dataset_property;

    bool _take_ownership = false;
    //! Ключ - код колонки в нижнем регистре, значение - номер колонки
    QHash<QString, int> _column_names;

    //! поиск по комбинации полей Ключ: HashedDataset::uniqueKey
    mutable QMap<QString, HashedDataset*> _find_by_columns_hash;
};

} // namespace zf
