#pragma once
#include "zf.h"

namespace zf
{
class FlatItemModel;
class _FlatRows;
class _FlatRowData;
class _FlatIndexData;

//! Данные для элемента
struct _FlatIndexDataValues
{
    _FlatIndexDataValues(const QVariant& value, int role, QLocale::Language language);
    _FlatIndexDataValues(const LanguageMap& values, int role);
    ~_FlatIndexDataValues();

    QVariant* getValue(QLocale::Language language) const;
    void setValue(QLocale::Language lang, const QVariant& value);
    void setValues(const LanguageMap& v);

    LanguageMap toLanguageMap() const;

    int role;
    bool changed = false; // были ли изменения
    LanguageMap* multi_values = nullptr;
    QVariant* single_value = nullptr;
};

//! Информация об элементе
class _FlatIndexData
{
public:
    _FlatIndexData(_FlatRowData* _row);
    ~_FlatIndexData();

    //! Строка к которой относится элемент
    _FlatRowData* row() const;
    //! Флаги элемента
    Qt::ItemFlags flags() const;
    //! Назначены ли флаги
    bool hasFlags() const;

    QList<int> roles() const;

    LanguageMap values(int role) const;
    void remove(int role);
    //! Возвращает указатель на вектор со списком очищенных ролей или nullptr. За удаление отвечает вызывающий
    QVector<int>* clear(bool take_data);
    void set(const QVariant& v, int role, QLocale::Language language);
    void set(const LanguageMap& v, int role);
    void resetChanged(int role);
    void resetChanged();

    bool isChanged(int role) const;

    void setFlags(const Qt::ItemFlags& f);
    // по ролям
    QMap<int, QVariant> toMap(QLocale::Language language) const;
    QMap<int, LanguageMap> toMap() const;

    void fromMap(const QMap<int, QVariant>& v, QLocale::Language language);
    void fromMap(const QMap<int, LanguageMap>& v);

    _FlatIndexData* clone(_FlatRowData* row) const;

private:
    Qt::ItemFlags* _flags = nullptr;
    QVarLengthArray<_FlatIndexDataValues*, 1> _data;
    _FlatRowData* _row = nullptr;
};

//! Набор строк
class _FlatRows
{
public:
    _FlatRows(FlatItemModel* model, _FlatRowData* parent);
    ~_FlatRows();

    //! Основная модель
    FlatItemModel* model() const { return _model; }
    //! Индекс узла-родителя
    QModelIndex parentIndex() const;

    //! Родительская строка
    _FlatRowData* parent() const { return _parent; }

    //! Количество строк
    int rowCount() const { return _row_count; }
    //! Количество колонок
    int columnCount() const { return _column_count; }

    //! Строка содержит данные
    bool hasRowData(int row) const;
    //! Получить строку
    _FlatRowData* getRow(int row) const;

    //! Задать количество строк
    void setRowCount(int n);
    //! Задать количество колонок
    void setColumnCount(int n);

    //! Найти порядковый номер строки для указателя на строку
    int findRow(_FlatRowData* row_data) const;

    //! Вставить строки
    bool insertRows(int row, int count);
    //! Удалить строки
    bool removeRows(int row, int count,
                    //! Если истина, то объект строк удаляться не будут
                    bool keep_objects);

    //! Вставить колонки
    bool insertColumns(int column, int count);
    //! Удалить колонки
    bool removeColumns(int column, int count);

    //! Переместить строки
    bool moveRows(int source, int count, int destination);

    //! Переместить колонки
    bool moveColumns(int source, int count, int destination);

    //! Изъять строки
    QList<_FlatRowData*> takeRows(int row, int count);
    //! Вставить изъятые строки
    bool putRows(const QList<_FlatRowData*>& rows, int row);

    //! Заменить строку на другую
    bool replaceRow(int row, _FlatRowData* row_data);

    //! Создать копию объекта. Новый объект не относится к какой либо модели
    _FlatRows* clone(_FlatRowData* parent, bool clone_children) const;

    //! Выделить память под все требуемые строки и колонки
    void alloc();

    //! Сбросить флаг изменений для всех ячеек
    void resetChanged();

    //! Установка произвольного имени для отладки
    void setInternalName(const QString& name) { _internal_name = name; }
    QString internalName() const { return _internal_name; }

private:
    void clearCache();

    //! Обновить указатель на модель
    void setItemModel(FlatItemModel* m);

    FlatItemModel* _model = nullptr;
    int _column_count = 0;
    int _row_count = 0;
    //! Данные по строкам
    mutable QVector<_FlatRowData*> _rows;
    //! Родительская строка
    _FlatRowData* _parent = nullptr;

    //! Кэшированное значение строки, найденной через findRow
    mutable _FlatRowData* _last_found_data = nullptr;
    //! Кэшированное значение номера строки, найденного через findRow
    mutable int _last_found_row = -1;

    //! Произвольное имени для отладки
    QString _internal_name;

    friend class FlatItemModel;
};

//! Данные строки
class _FlatRowData
{
public:
    _FlatRowData(int column_count, _FlatRows* owner);
    ~_FlatRowData();

    //! Указатель на контейнер
    _FlatRows* owner() const { return _owner; }
    void setOwner(_FlatRows* owner) { _owner = owner; }
    //! Родитель
    _FlatRowData* parent() const { return _owner == nullptr ? nullptr : _owner->parent(); }

    //! Количество колонок
    int columnCount() const { return _column_count; }

    //! Есть дочерние строки
    bool hasChildren() const { return _children && _children->rowCount() > 0; }
    //! Дочерние строки
    _FlatRows* children() const;
    //! Количество дочерних строк
    int childCount() const;

    //! Получить значение элемента строки
    _FlatIndexData* getData(int column) const;
    //! Задать количество колонок
    void setColumnCount(int n);

    //! Вставить колонки
    bool insertColumns(int column, int count,
                       //! Не менять количество колонок у детей
                       bool no_childred);
    //! Удалить колонки
    bool removeColumns(int column, int count,
                       //! Не менять количество колонок у детей
                       bool no_childred);

    //! Переместить колонки
    bool moveColumns(int source, int count, int destination);

    //! Найти порядковый номер дочерней строки для указателя на строку
    int findChildRow(_FlatRowData* row_data) const;

    //! Создать копию объекта
    _FlatRowData* clone(_FlatRows* owner, bool clone_children) const;

    //! Выделить память под все требуемые строки и колонки
    void alloc();

    //! Сбросить флаг изменений для всех ячеек
    void resetChanged();

private:
    //! Количество колонок
    int _column_count = 0;
    //! Элементы строки
    mutable QVector<_FlatIndexData*> _data;
    //! Указатель на контейнер
    _FlatRows* _owner = nullptr;
    //! Дочерние строки
    mutable _FlatRows* _children = nullptr;

    friend class _FlatRows;
};

//! Внутренние разделяемые данные _FlatItemModel_data
class _FlatItemModel_SharedData : public QSharedData
{
public:
    _FlatItemModel_SharedData();
    _FlatItemModel_SharedData(FlatItemModel* model);
    _FlatItemModel_SharedData(const _FlatItemModel_SharedData& d);
    ~_FlatItemModel_SharedData();

    void copyFrom(const _FlatItemModel_SharedData* d);

    static _FlatItemModel_SharedData* shared_null();

    std::shared_ptr<FlatItemModel> _model;
};

//! Данные для преобразования FlatItemModel в QVariant
class _FlatItemModel_data
{
public:
    _FlatItemModel_data();
    _FlatItemModel_data(FlatItemModel* model);
    _FlatItemModel_data(const _FlatItemModel_data& d);
    ~_FlatItemModel_data();

    _FlatItemModel_data& operator=(const _FlatItemModel_data& d);

    void detachHelper();

    QSharedDataPointer<_FlatItemModel_SharedData> _d;

    //! Номер регистрации через qRegisterMetaType
    static int _meta_type_number;
};

} // namespace zf

Q_DECLARE_METATYPE(zf::_FlatItemModel_data);
