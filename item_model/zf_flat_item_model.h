#pragma once

#include <QAbstractItemModel>
#include <QBrush>
#include <QIcon>
#include <QFont>
#include <QLocale>
#include <QMap>

#include "zf.h"

namespace zf
{
class FlatRow;
class FlatItemModel;
class _FlatRows;
class _FlatRowData;
class _FlatIndexData;

//! Функция для нестандартной сортировки
typedef std::function<bool(const FlatItemModel* source_model, const QModelIndex& source_left, const QModelIndex& source_right)> FlatItemModelSortFunction;

/*! Строка таблицы. Используется в качестве аналога QStandardItem.
 * В отличие от QStandardItem не имеет связи с FlatItemModel. Т.е. после получения строк из FlatItemModel любые
 * изменения в них никак не отражаются на FlatItemModel.
 * Если важна скорость записи данных в FlatItemModel, то лучше
 * напрямую использовать методы FlatItemModel, а не готовить отдельно FlatRow и вставлять их в FlatItemModel.
 */
class ZCORESHARED_EXPORT FlatRow
{
public:
    //! Создание строки вне модели
    FlatRow(int column_count = 0);
    FlatRow(const FlatRow& r);
    ~FlatRow();

    FlatRow& operator=(const FlatRow& r);

    //! Родительская строка
    FlatRow* parent() const;
    //! Положение в списке строк родителя. Затратно с точки зрения времени выполнения
    int pos() const;

    //! Получить данные для ячейки в указанной колонке
    QVariant data(int column, int role = Qt::DisplayRole, QLocale::Language language = QLocale::AnyLanguage) const;
    //! Записать данные для ячейки в указанной колонке
    bool setData(int column, const QVariant& value, int role = Qt::EditRole, QLocale::Language language = QLocale::AnyLanguage);

    QString text(int column, QLocale::Language language = QLocale::AnyLanguage) const;
    void setText(int column, const QString& text, QLocale::Language language = QLocale::AnyLanguage);

    QIcon icon(int column, QLocale::Language language = QLocale::AnyLanguage) const;
    void setIcon(int column, const QIcon& icon, QLocale::Language language = QLocale::AnyLanguage);

    QString toolTip(int column, QLocale::Language language = QLocale::AnyLanguage) const;
    void setToolTip(int column, const QString& toolTip, QLocale::Language language = QLocale::AnyLanguage);

    QString statusTip(int column, QLocale::Language language = QLocale::AnyLanguage) const;
    void setStatusTip(int column, const QString& statusTip, QLocale::Language language = QLocale::AnyLanguage);

    QString whatsThis(int column, QLocale::Language language = QLocale::AnyLanguage) const;
    void setWhatsThis(int column, const QString& whatsThis, QLocale::Language language = QLocale::AnyLanguage);

    QSize sizeHint(int column) const;
    void setSizeHint(int column, const QSize& sizeHint);

    QFont font(int column) const;
    void setFont(int column, const QFont& font);

    Qt::Alignment textAlignment(int column) const;
    void setTextAlignment(int column, Qt::Alignment textAlignment);

    QBrush background(int column) const;
    void setBackground(int column, const QBrush& brush);

    QBrush foreground(int column) const;
    void setForeground(int column, const QBrush& brush);

    Qt::CheckState checkState(int column) const;
    void setCheckState(int column, Qt::CheckState checkState);

    QString accessibleText(int column, QLocale::Language language = QLocale::AnyLanguage) const;
    void setAccessibleText(int column, const QString& accessibleText, QLocale::Language language = QLocale::AnyLanguage);

    QString accessibleDescription(int column, QLocale::Language language = QLocale::AnyLanguage) const;
    void setAccessibleDescription(int column, const QString& accessibleDescription, QLocale::Language language = QLocale::AnyLanguage);

    bool isEnabled(int column) const;
    void setEnabled(int column, bool enabled);

    bool isEditable(int column) const;
    void setEditable(int column, bool editable);

    bool isSelectable(int column) const;
    void setSelectable(int column, bool selectable);

    bool isCheckable(int column) const;
    void setCheckable(int column, bool checkable);

    bool isAutoTristate(int column) const;
    void setAutoTristate(int column, bool tristate);

    bool isUserTristate(int column) const;
    void setUserTristate(int column, bool tristate);

    bool isDragEnabled(int column) const;
    void setDragEnabled(int column, bool dragEnabled);

    bool isDropEnabled(int column) const;
    void setDropEnabled(int column, bool dropEnabled);

    //! Очистить все роли для ячейки в указанной колонке
    bool clearData(int column);

    //! Количество дочерних строк
    int rowCount() const;
    //! Количество колонок
    int columnCount() const;

    //! Имеются дочерние строки
    bool hasChildren() const;

    //! Все дочерние строки
    const QList<FlatRow*>& children() const;
    //! Дочерняя строка
    FlatRow* child(int row) const;

    //! Поиск положения дочерней строки
    //! TODO - кэширование поиска (если оно вообще надо)
    int indexOf(FlatRow* row) const;

    //! Задать количество дочерних строк
    void setRowCount(int n);
    //! Добавить дочерние строки
    bool insertRows(int row, int count);
    //! Добавить дочерние строки
    bool appendRows(int count);
    //! Добавить дочерние строки
    bool appendRow();
    //! Удалить дочерние строки
    bool removeRows(int row, int count);
    //! Удалить дочерние строки
    bool removeRow(int row);
    //! Переместить дочерние строки
    bool moveRows(int source, int count, int destination);

    //! Задать количество колонок
    void setColumnCount(int n);
    //! Добавить колонки
    bool insertColumns(int column, int count);
    //! Добавить колонки
    bool appendColumns(int count);
    //! Добавить колонки
    bool appendColumn();
    //! Удалить колонки
    bool removeColumns(int column, int count);
    //! Удалить колонки
    bool removeColumn(int column);
    //! Переместить колонки
    bool moveColumns(int source, int count, int destination);

    //! Скопировать строки. За удаление отвечает вызывающий
    bool copyRows(int row, int count, QList<FlatRow*>& rows);
    //! Изъять строки. За удаление отвечает вызывающий
    bool takeRows(int row, int count, QList<FlatRow*>& rows);
    //! Добавить строки. Количество колонок должно совпадать во всех строках. Владение строками передается в этот объект
    bool insertRows(int row, const QList<FlatRow*>& rows);
    bool appendRows(const QList<FlatRow*>& rows);

    QMap<int, QVariant> itemData(int column, QLocale::Language language) const;
    void setItemData(int column, const QMap<int, QVariant>& roles, QLocale::Language language);

    Qt::ItemFlags flags(int column) const;
    void setFlags(int column, Qt::ItemFlags flags);

public: // не использовать (нельзя поместить в приват при работе кастомного аллокатора)
    FlatRow(_FlatRowData* data, FlatRow* parent);
    FlatRow(int column_count, FlatRow* parent);

private:
    void copyFrom(_FlatRowData* data, FlatRow* parent);
    void copyFromEx(const FlatRow& r);

    void changeFlags(int column, bool enable, Qt::ItemFlags f);

    //! Данные
    _FlatRowData* _data = nullptr;
    //! Дочерние строки
    QList<FlatRow*> _children;
    //! Родительская строка
    FlatRow* _parent = nullptr;

    friend class FlatItemModel;
};

/*
 * Реализация QStandardItemModel, предназначенная для хранения больших объемов даных.
 * Отличия от QStandardItemModel:
 * 1. Все узлы содержат одинаковое количество колонок
 * 2. Нет прямого аналога QStandardItem. Вместо него используется FlatRow - строка таблицы
 * 3. Qt::EditRole и Qt::DisplayRole - не отличаются (запись в один, меняет другой)
 * 4. Потребляет примерно на 40% меньше оперативной памяти за счет хранения информации в QVector
 * 5. Память выделяется только под ячейки, которые содержат данные ("ленивое" выделение памяти)
 * 6. Заполнение данными быстрее примерно в 2-3 раза (при условии что количество строк/колонок установлено заранее)
 * 7. Операции удаления/вставки строк/колонок медленнее чем в QStandardItemModel, т.к. в ней используeтся QList
 */
class ZCORESHARED_EXPORT FlatItemModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    explicit FlatItemModel(QObject* parent = nullptr);
    explicit FlatItemModel(int rows, int columns, QObject* parent = nullptr);
    ~FlatItemModel() override;

    //! Параметры
    FlatItemModelOptions options() const;
    void setOptions(const FlatItemModelOptions& options);

    QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex& child) const override;

    //! Язык на котором записываются и читаются данные (по умолчанию QLocale::AnyLanguage)
    QLocale::Language language() const;
    //! Устанавливает новый язык и возвращает старый
    QLocale::Language setLanguage(QLocale::Language language);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;

    bool hasChildren(const QModelIndex& parent = QModelIndex()) const override;

    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;

    QVariant data(int row, int column, int role = Qt::DisplayRole, const QModelIndex& parent = QModelIndex()) const;
    QVariant data(int row, int column, const QModelIndex& parent) const;

    bool setData(int row, int column, const QVariant& value, int role = Qt::EditRole, const QModelIndex& parent = QModelIndex());
    bool setData(int row, int column, const QVariant& value, const QModelIndex& parent);

    QString text(const QModelIndex& index) const;
    void setText(const QModelIndex& index, const QString& text);

    QIcon icon(const QModelIndex& index) const;
    void setIcon(const QModelIndex& index, const QIcon& icon);

    QString toolTip(const QModelIndex& index) const;
    void setToolTip(const QModelIndex& index, const QString& toolTip);

    QString statusTip(const QModelIndex& index) const;
    void setStatusTip(const QModelIndex& index, const QString& statusTip);

    QString whatsThis(const QModelIndex& index) const;
    void setWhatsThis(const QModelIndex& index, const QString& whatsThis);

    QSize sizeHint(const QModelIndex& index) const;
    void setSizeHint(const QModelIndex& index, const QSize& sizeHint);

    QFont font(const QModelIndex& index) const;
    void setFont(const QModelIndex& index, const QFont& font);

    Qt::Alignment textAlignment(const QModelIndex& index) const;
    void setTextAlignment(const QModelIndex& index, Qt::Alignment textAlignment);

    QBrush background(const QModelIndex& index) const;
    void setBackground(const QModelIndex& index, const QBrush& brush);

    QBrush foreground(const QModelIndex& index) const;
    void setForeground(const QModelIndex& index, const QBrush& brush);

    Qt::CheckState checkState(const QModelIndex& index) const;
    void setCheckState(const QModelIndex& index, Qt::CheckState checkState);

    QString accessibleText(const QModelIndex& index) const;
    void setAccessibleText(const QModelIndex& index, const QString& accessibleText);

    QString accessibleDescription(const QModelIndex& index) const;
    void setAccessibleDescription(const QModelIndex& index, const QString& accessibleDescription);

    bool isEnabled(const QModelIndex& index) const;
    void setEnabled(const QModelIndex& index, bool enabled);

    bool isEditable(const QModelIndex& index) const;
    void setEditable(const QModelIndex& index, bool editable);

    bool isSelectable(const QModelIndex& index) const;
    void setSelectable(const QModelIndex& index, bool selectable);

    bool isCheckable(const QModelIndex& index) const;
    void setCheckable(const QModelIndex& index, bool checkable);

    bool isAutoTristate(const QModelIndex& index) const;
    void setAutoTristate(const QModelIndex& index, bool tristate);

    bool isUserTristate(const QModelIndex& index) const;
    void setUserTristate(const QModelIndex& index, bool tristate);

    bool isDragEnabled(const QModelIndex& index) const;
    void setDragEnabled(const QModelIndex& index, bool dragEnabled);

    bool isDropEnabled(const QModelIndex& index) const;
    void setDropEnabled(const QModelIndex& index, bool dropEnabled);

    Qt::DropActions supportedDropActions() const override;

    QStringList mimeTypes() const override;
    QMimeData* mimeData(const QModelIndexList& indexes) const override;
    bool dropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent) override;

    //! Очистить все данные для индекса
    bool clearData(const QModelIndex& index);

    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    bool setHeaderData(int section, Qt::Orientation orientation, const QVariant& value, int role = Qt::EditRole) override;

    bool insertRows(int row, int count, const QModelIndex& parent = QModelIndex()) override;
    bool insertColumns(int column, int count, const QModelIndex& parent = QModelIndex()) override;

    bool removeRows(int row, int count, const QModelIndex& parent = QModelIndex()) override;
    bool removeColumns(int column, int count, const QModelIndex& parent = QModelIndex()) override;

    /*! Стандартный подход Qt к moveRows подразумевает что если мы перемещаем строки в рамках одного родителя, 
     *  то destinationChild указывает на позицию куда надо вставить строки до перемещения. 
     *  Т.е. если мы имеем две строки и хотим переставить строку 0 на место строки 1, то
     * destinationChild должен быть равен 2 */
    bool moveRows(const QModelIndex& sourceParent, int sourceRow, int count, const QModelIndex& destinationParent, int destinationChild) override;
    bool moveRows(int sourceRow, int count, int destinationRow);

    /*! Стандартный подход Qt к moveColumns подразумевает что если мы перемещаем колонки в рамках одного родителя, 
     *  то destinationChild указывает на позицию куда надо вставить строки до перемещения. 
     *  Т.е. если мы имеем две колонки и хотим переставить колонкe 0 на место колонки 1, то
     * destinationChild должен быть равен 2 */
    bool moveColumns(const QModelIndex& sourceParent, int sourceColumn, int count, const QModelIndex& destinationParent, int destinationChild) override;
    bool moveColumns(int sourceColumn, int count, int destinationColumn);

    QMap<int, QVariant> itemData(const QModelIndex& index) const override;
    bool setItemData(const QModelIndex& index, const QMap<int, QVariant>& roles) override;

    Qt::ItemFlags flags(const QModelIndex& index) const override;
    void setFlags(const QModelIndex& index, Qt::ItemFlags flags);
    void setDefaultFlags(Qt::ItemFlags flags);
    Qt::ItemFlags defaultFlags() const;

    void setRowCount(int n, const QModelIndex& parent = QModelIndex());
    void setColumnCount(int n,
        //! Родитель оставлен для совместимости. Параметр не нужен, т.к. модель поддерживает одинаковое количество колонок для всех узлов
        //! дерева
        const QModelIndex& parent = QModelIndex());

    void appendRows(int count, const QModelIndex& parent = QModelIndex());
    void appendColumns(int count,
        //! Родитель оставлен для совместимости. Параметр не нужен, т.к. модель поддерживает одинаковое количество колонок для всех узлов
        //! дерева
        const QModelIndex& parent = QModelIndex());

    int appendRow(const QModelIndex& parent = QModelIndex());
    int appendColumn(
        //! Родитель оставлен для совместимости. Параметр не нужен, т.к. модель поддерживает одинаковое количество колонок для всех узлов
        //! дерева
        const QModelIndex& parent = QModelIndex());

    //! Удалить строку
    bool removeRow(const QModelIndex& index);
    bool removeRow(int row,
        //! Родитель оставлен для совместимости. Параметр не нужен, т.к. модель поддерживает одинаковое количество колонок для
        //! всех узлов дерева
        const QModelIndex& parent = QModelIndex());
    bool removeColumn(int column,
        //! Родитель оставлен для совместимости. Параметр не нужен, т.к. модель поддерживает одинаковое количество колонок для
        //! всех узлов дерева
        const QModelIndex& parent = QModelIndex());

    //! Скопировать строки. За удаление отвечает вызывающий. Полученные строки никак не связаны с моделью
    bool getRows(int row, int count, QList<FlatRow*>& rows, const QModelIndex& parent = QModelIndex());
    //! Изъять строки. За удаление отвечает вызывающий. Полученные строки никак не связаны с моделью
    bool takeRows(int row, int count, QList<FlatRow*>& rows, const QModelIndex& parent = QModelIndex());
    //! Добавить строки. Количество колонок должно совпадать во всех строках с моделью
    //! Объекты FlatRow остаются без изменений, не передаются во владение модели и остаются никак с ней не связанными
    bool insertRows(int row, const QList<FlatRow*>& rows, const QModelIndex& parent = QModelIndex());
    bool appendRows(const QList<FlatRow*>& rows, const QModelIndex& parent = QModelIndex());

    //! Были ли изменения
    bool isChanged(const QModelIndex& index, int role = Qt::DisplayRole) const;
    //! Сбросить флаг изменений
    void resetChanged(const QModelIndex& index, int role = Qt::DisplayRole);
    //! Сбросить флаг изменений для всех ячеек
    void resetChanged();

    //! Выделить память под все требуемые строки и колонки. Может слегка увеличить быстродействие при
    //! массированном заполнении модели
    void alloc();

    //! Переместить данные из указанной модели. Модель source при этом УДАЛЯЕТСЯ
    //! Генерирует сигнал modelReset
    void moveData(
        //! После выполнения метода указатель source становится недействительным!
        FlatItemModel* source);

    //! Поиск. В отличие от стандартного поиска добавлено кэширование в случае:
    //! start - корневой индекс, Qt::MatchFixedString и Qt::MatchCaseSensitive
    QModelIndexList match(const QModelIndex& start, int role, const QVariant& value, int hits = 1,
        Qt::MatchFlags flags = Qt::MatchFlags(Qt::MatchStartsWith | Qt::MatchWrap)) const override;

    using QAbstractItemModel::sort;
    //! Сортировка. Записи сортируются путем удаления и добавления в новом порядке.
    void sort(
        //! Колонки
        const QList<int>& columns,
        //! Порядок сортировки. Если не задано, то все по порядку
        const QList<Qt::SortOrder>& order = {},
        //! Роли. Если не задано, то Qt::DisplayRole
        const QList<int>& role = {});
    //! Сортировка. Записи сортируются путем удаления и добавления в новом порядке.
    void sort(FlatItemModelSortFunction sort_function);

    QVariant dataHelper(const QModelIndex& index, int role, QLocale::Language language) const;
    LanguageMap dataHelperLanguageMap(const QModelIndex& index, int role) const;
    QMap<int, LanguageMap> dataHelperLanguageRoleMap(const QModelIndex& index) const;

    bool setDataHelper(const QModelIndex& index, const QVariant& value, int role, QLocale::Language language);
    bool setDataHelperLanguageMap(const QModelIndex& index, const LanguageMap& value, int role);
    bool setDataHelperLanguageRoleMap(const QModelIndex& index, const QMap<int, LanguageMap>& value);

    //! Преобразовать в QVariant. После этого владение данным объектом переходит к QVariant
    //! и указатель на него использовать нельзя. При вызове метода у объекта устанавливается parent в nullptr и отключаются все коннекты
    QVariant toVariant() const;
    //! Получить указатель на FlatItemModel, содержащуюся в QVariant, созданный через FlatItemModel::toVariant
    static std::shared_ptr<FlatItemModel> fromVariant(const QVariant& v);
    //! Содержит ли QVariant FlatItemModel
    static bool isVariant(const QVariant& v);
    //! Пользовательский тип для QVariant
    static int metaType();

    //! Установка произвольного имени для отладки. Сохраняется при выполнении moveData
    void setInternalName(const QString& name);
    QString internalName() const;

    //! Аналог QAbstractItemModel::beginResetModel только возможностью вложенных вызовов
    void beginResetModel();
    //! Аналог QAbstractItemModel::endResetModel только возможностью вложенных вызовов
    void endResetModel();

signals:
    //! Изменились данные. В связи с особенностями реализации, данные могу меняться только для одного индекса за раз
    //! Генерируется только при ручной установке флага FlatItemModelOption::GenerateDataChangedExtendedSignal
    void sg_dataChangedExtended(const QModelIndex& index, const zf::LanguageMap& new_value, const zf::LanguageMap& old_value, int role);

private:
    void changeFlags(const QModelIndex& index, bool enable, Qt::ItemFlags f);

    QModelIndex indexFromData(int row, int column, _FlatIndexData* data) const;

    //! Строки для родительского индекса
    _FlatRows* getRows(const QModelIndex& parent) const;

    //! Преобразование индекса в структуру с данными
    static _FlatIndexData* indexData(const QModelIndex& index);

    //! Перемещение заголовков
    void moveHeaders(Qt::Orientation orientation, int source, int count, int destination);
    //! Можно ли перемещать. При вызове beginMoveRows/beginMoveColumns есть проверка на допустимость перемещения, но она
    //! не полная В этом методе добавлена только дополнительная проверка
    bool allowMove(const QModelIndex& srcParent, int start, int end, const QModelIndex& destinationParent, int destinationStart, Qt::Orientation orientation);

    //! Рекурсивное заполнение кэша поиска
    void fillMatchCache(const std::shared_ptr<QMultiHash<QString, QModelIndex>>& cache, int column, int role, const QModelIndex& parent) const;
    //! Очистить кэш поиска
    void clearMatchCache();

    //! Количество колонок
    int _column_count = 0;
    //! Строки верхнего уровня
    _FlatRows* _rows = nullptr;

    //! Горизонтальные заголовки
    //! QMap<роль, значение>
    QVector<QMap<int, QVariant>*> _h_headers;
    //! Вертикальные заголовки
    //! QMap<роль, значение>
    QVector<QMap<int, QVariant>*> _v_headers;

    //! Флаги по умолчанию
    Qt::ItemFlags _flags;

    //! Язык на котором записываются и читаются данные
    QLocale::Language _language = QLocale::AnyLanguage;
    //! Принудительно установить язык на время обращения к данным
    mutable QLocale::Language* _language_force = nullptr;

    //! Удалить содержимое в деструкторе
    bool _clear_on_destructor = true;

    //! Кэш поиска. Ключ: column_role_flags
    mutable QMap<QString, std::shared_ptr<QMultiHash<QString, QModelIndex>>> _match_cache;

    //! Параметры
    FlatItemModelOptions _options;

    int _reset_counter = 0;

    friend class _FlatRows;
};

typedef QSharedPointer<FlatItemModel> FlatItemModelPointer;
typedef QSharedPointer<FlatRow> FlatRowPointer;

} // namespace zf
