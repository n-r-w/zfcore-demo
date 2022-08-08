#pragma once

#include <build_options.h>
#include "zf_global.h"
#include "zf_dbg_leaks.h"
#include "zf_dbg_break.h"
#include "zf_malloc.h"

#include <QObject>
#include <QPointer>
#include <QWidget>
#include <memory>
#include <QLocale>
#include <QModelIndexList>

//! Аналог DECLARE_HANDLE из winnt.h
#define Z_DECLARE_HANDLE(name)                                                                                                                                 \
    struct name##__                                                                                                                                            \
    {                                                                                                                                                          \
        int unused;                                                                                                                                            \
    };                                                                                                                                                         \
    typedef struct name##__* name

//! Проверка хэндла на валидность
#define Z_VALID_HANDLE(handle) (handle != nullptr)

namespace zf
{
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
Q_NAMESPACE_EXPORT(ZCORESHARED_EXPORT)
#else
ZCORESHARED_EXPORT Q_NAMESPACE
#endif

//! Идентификатор временной сущности
typedef QString TempEntityID;
//! Значение для разных языков
typedef QMap<QLocale::Language, QVariant> LanguageMap;

typedef std::shared_ptr<QByteArray> QByteArrayPtr;
typedef std::shared_ptr<QVariant> QVariantPtr;
typedef std::shared_ptr<QObject> QObjectPtr;
typedef std::shared_ptr<QString> QStringPtr;

//! Режимы работы ядра
enum class CoreMode
{
    Undefinded,
    //! Приложение
    Application,
    //! Библиотека
    Library,
    //! Тест кейсы
    Test,
    //! Плагин для QtDesigner
    QtDesigner,
};
Q_ENUM_NS(CoreMode)
Q_DECLARE_FLAGS(CoreModes, CoreMode)

/*! Права доступа
 * ВАЖНО: при изменении проверить enum ObjectActionType, методы: Utils::accessForAction, AccessRights::onSet,
 * AccessRights::onRemove */
enum class AccessFlag : quint64
{
    //! Нет информации
    Undefined = 0,
    //! Нет вообще никаких прав
    Blocked = 1,
    //! Просмотр
    View = 2,
    //! Изменение
    Modify = 3,
    //! Создание
    Create = 4,
    //! Удаление
    Remove = 5,
    //! Печать
    Print = 6,
    //! Экспорт
    Export = 7,
    //! Владелец (расширенные права - интерпретируется программистом)
    Owner = 8,
    //! Настройка параметров (интерпретируется программистом)
    Configurate = 9,
    //! Администратор (полный доступ на уровне модулей, но не всей системы)
    Administrate = 10,
    //! Системный администратор (настройка приложения и прав доступа). Не должен задаваться на уровне модуля, только как
    //! глобальные права. Теоретически не имеет права доступа к данным
    SystemAdministrate = 11,
    //! Отладка приложения (может включать расширенные журналы отладки и т.п.)
    Debug = 12,
    //! Разработчик
    Developer = 13,
    //! Обычный пользователь. Доступны обычные модули (не админские)
    RegularUser = 14,

    //! С этого номера начинаются нестандартные права доступа
    Custom = 0x0100000,
};
Q_ENUM_NS(AccessFlag)

//! Стандартные действия с объектом (ВАЖНО: при изменении проверить enum AccessRight и метод Utils::accessForAction)
enum class ObjectActionType
{
    Undefined = 0,
    //! Просмотреть
    View,
    //! Создать
    Create,
    //! Добавить подчиненный объект (для иерархий)
    CreateLevelDown,
    //! Удалить
    Remove,
    //! Редактировать
    Modify,
    //! Экспортировать
    Export,
    //! Печать
    Print,
    //! Настройка
    Configurate,
    //! Андминистрирование
    Administrate,
};
Q_ENUM_NS(ObjectActionType)

//! Тип модуля
enum class ModuleType
{
    Invalid = 0x0,
    //! Может существовать только в одном экземпляре и не может иметь несколько сущностей. Идентифицируется кодом модуля
    Unique = 0x1,
    //! Может иметь несколько экземпляров (сущностей), каждая из которых имеет уникальный код сущности. Идентифицируется
    //! парой: код модуля и код сущности
    Multiple = 0x2
};
Q_ENUM_NS(ModuleType)

//! Состояния представления
enum class ViewState
{
    //! Нет состояния - это обычно означает что представление было создано программным образом
    NoStates = 0x0000,
    //! Главное представление MDI окна
    MdiWindow = 0x0001,
    //! Главное представление модального окна редактирования
    EditWindow = 0x0002,
    //! Главное представление модального окна просмотра (read only)
    ViewWindow = 0x0004,
    //! Главное представление модального окна просмотра (read only но для окна редактирования)
    ViewEditWindow = 0x0008,
    //! Отображение в модальном окне SelectFromModelDialog
    SelectItemModelDialog = 0x0010,
};
Q_ENUM_NS(ViewState)
Q_DECLARE_FLAGS(ViewStates, ViewState)

//! Параметры открытия окна просмотра/редактирования сущности
enum class ModuleWindowOption
{
    NoOptions = 0x0000,
    //! Для модального диалога: не сохранять изменения при нажатии на кнопку "Сохранить", а просто закрыть окно
    EditWindowNoSave = 0x0001,
    //! Для модального диалога: не подсвечивать кнопку "сохранить" красным при наличии ошибок
    EditWindowNoHighlightSave = 0x0002,

    /* Если ни один из флагов EditWindowBottomLineVisible, EditWindowBottomLineHidden не задан,
     * то линия скрывается для модальных окон просмотра и показывается для окон редактирования */
    //! Для модального диалога: показывать нижнюю горизонтальную линию над кнопками
    EditWindowBottomLineVisible = 0x0004,
    //! Для модального диалога: скрывать нижнюю горизонтальную линию над кнопками
    EditWindowBottomLineHidden = 0x0008,

    //! Добавить левый отступ
    LeftMargin = 0x0010,
    //! Добавить правый отступ
    RightMargin = 0x0020,
    //! Добавить верхний отступ
    TopMargin = 0x0040,
    //! Добавить нижний отступ
    BottomMargin = 0x0080,
    //! Добавить все отступы
    Margins = LeftMargin | RightMargin | TopMargin | BottomMargin,

    //! Не загружать отсутствующие данные модели при открытии View
    EditWindowDisableAutoLoadModel = 0x0100,
    //! Принудительно запрашивать окно с параметром ViewState::EditWindow при вызове WindowManager::viewWindow
    ForceEditView = 0x0200,
};
Q_ENUM_NS(ModuleWindowOption)
Q_DECLARE_FLAGS(ModuleWindowOptions, ModuleWindowOption)

//! Свойства объектов, наследников ModuleDataObject
enum class ModuleDataOption
{
    NoOptions = 0x0000,
    //! Требуется проверка на ошибки
    HighlightEnabled = 0x0001,
    /*! Упрощенная работа с highlight. Активно по умолчанию. Может тормозить для больших наборов данных
     * В дополнение к методу ModuleDataObject::getHighlight, вызываются:
     *      для полей ModuleDataObject::getFieldHighlight
     *      для ячеек таблиц ModuleDataObject::getCellHighlight */
    SimpleHighlight = 0x0002,
};
Q_ENUM_NS(ModuleDataOption)
Q_DECLARE_FLAGS(ModuleDataOptions, ModuleDataOption)

//! Свойства DatabaseObject
enum class DatabaseObjectOption
{
    //! Нестандартная загрузка из БД
    CustomLoad = 0x0001,
    //! Нестандартное сохранение в БД
    CustomSave = 0x0002,
    //! Нестандартное удаление из БД
    CustomRemove = 0x0004,
    //! Рассылать локальные сообщения об изменении данных. Опцию надо включать, если сервер не рассылает уведомления об
    //! изменениях данного объекта
    DataChangeBroadcasting = 0x0008,
    /*! Асинхронное расширение стандартной загрузки объекта из БД
     * Если задан флаг, то сначала будет выполнена стандартная загрузка объекта, а затем запущен сustomLoad (ядро вызовет onStartCustomLoad)
     * Окончание сustomLoad аналогично флагу CustomLoad (вызов программистом finishCustomLoad) */
    StandardLoadExtension = 0x0010,
    /*! Асинхронное расширение стандартного сохранения объекта в БД
     * Если задан флаг, то сначала будет запущен сustomSave (ядро вызовет onStartCustomSave)
     * После вызова программистом  finishCustomSave будет выполнено обычное сохранение */
    StandardBeforeSaveExtension = 0x0020,
    /*! Асинхронное расширение стандартного сохранения объекта в БД
     * Если задан флаг, то сначала будет запущено обычное сохранение, а затем CustomSave (ядро вызовет onStartCustomSave) */
    StandardAfterSaveExtension = 0x0040,
    //! Разрешить сохранять модели, которые не были "отсоеденены" (Model::isDetached() == false)
    AllowSaveNotDatached = 0x0080,
    //! Не содержит данных (не надо грузить из БД). Несовместимо с CustomLoad
    Dummy = 0x0100,
};
Q_ENUM_NS(DatabaseObjectOption)
Q_DECLARE_FLAGS(DatabaseObjectOptions, DatabaseObjectOption)

//! Свойства объектов, наследников View
enum class ViewOption
{
    NoOptions = 0x0000,
    //! Показывать блокировку пользовательского интерфейса для встроенных представлений
    ShowBlockEmbedUi = 0x0001,
    //! Запрещена сортировка всех наборов данных по умолчанию
    SortDefaultDisable = 0x0002,
};
Q_ENUM_NS(ViewOption)
Q_DECLARE_FLAGS(ViewOptions, ViewOption)

//! Свойства HighlightDialog
enum class HighlightDialogOption
{
    Undefined = 0x0000,
    //! Не показывать панель при запуске диалога. Если false, то надо самому вызвать requestHighlightShow когда надо
    NoAutoShowHighlight = 0x0001,
    /*! Упрощенная работа с highlight. Активно по умолчанию. Может тормозить для больших наборов данных     
     * В дополнение к методу HighlightDialog::getHighlight, вызываются:
     *      для полей HighlightDialog::getFieldHighlight
     *      для ячеек таблиц HighlightDialog::getCellHighlight */
    SimpleHighlight = 0x0002,
};
Q_ENUM_NS(HighlightDialogOption)
Q_DECLARE_FLAGS(HighlightDialogOptions, HighlightDialogOption)

//! Свойства загрузки из БД
enum class LoadOption
{
};
Q_ENUM_NS(LoadOption)
Q_DECLARE_FLAGS(LoadOptions, LoadOption)

//! Параметры для ImageListStorage
enum class ImageListOption
{
    NoOption,
    //! Можно ли перетаскивать
    CanMove,
    //! Можно сдвигать (перетаскивать или добавлять изображения перед этим, т.е. менять позицию)
    CanShift,
};
Q_ENUM_NS(ImageListOption)
Q_DECLARE_FLAGS(ImageListOptions, ImageListOption)

//! Параметры конвертации значений для отображения пользователю
enum class VisibleValueOption
{
    NoOptions = 0x0000,
    //! Отображение в приложении
    Application = 0x0001,
    //! Выгрузка во вне приложения
    Export = 0x0002,
};
Q_ENUM_NS(VisibleValueOption)
Q_DECLARE_FLAGS(VisibleValueOptions, VisibleValueOption)

//! Вид идентификатора
enum class UidType
{
    Invalid = 0,
    //! Идентификатор общего вида. Может содержать произвольное количество ключей
    General = 1,
    //! Сущность, которая имеет только один экземпляр (не имеет ключей)
    UniqueEntity = 2,
    //! Сущность, которая имеет несколько экземпляров (имеет один ключ)
    Entity = 3,
};
Q_ENUM_NS(UidType)

//! Параметры округления
enum class RoundOption
{
    Undefined = 0x0,
    //! Округлить вверх
    Up = 0x1,
    //! Округлить вниз
    Down = 0x2,
    //! Округлить дл ближайшего целого
    Nearest = 0x3,
    //! Округление процентов. Аналогично Nearest. Крайние значения (0,100) принимает только при точном совпадении
    Percent = 0x4
};
Q_ENUM_NS(RoundOption)

//! Тип данных
enum class DataType
{
    Undefined = 0x000000,
    //! Строчное текстовое
    String = 0x000001,
    //! Многострочное текстовое
    Memo = 0x000002,
    //! Целое знаковое
    Integer = 0x000004,
    //! Целое беззнаковое
    UInteger = 0x000008,
    //! Дробное
    Float = 0x000010,
    //! Дробное беззнаковое
    UFloat = 0x000020,
    //! Дата
    Date = 0x000040,
    //! Дата со временем
    DateTime = 0x000080,
    //! Логическое
    Bool = 0x000100,
    //! Идентификатор
    Uid = 0x000200,
    //! QVariant::Icon или QVariant::ByteArray
    Image = 0x000400,
    //! Набор данных - таблица
    Table = 0x000800,
    //! Набор данных - дерево
    Tree = 0x001000,
    //! Произвольные данные
    Variant = 0x002000,
    //! Деньги и т.п. знаковое
    Numeric = 0x004000,
    //! Деньги и т.п. беззнаковое
    UNumeric = 0x008000,
    //! Произвольный массив байтов
    Bytes = 0x010000,
    //! Время без даты
    Time = 0x020000,
    //! Многострочное текстовое с HTML
    RichMemo = 0x040000,
};
Q_ENUM_NS(DataType)
Q_DECLARE_FLAGS(DataTypes, DataType)

//! Формат хранения/отображения данных
enum class ValueFormat
{
    Undefined = 0,
    //! Формат, в котором данные хранятся в контейнерах
    Internal,
    //! Формат для редактирования на экране или отображения (как правило QString)
    Edit,
    //! Формат для вычисления (например int, double и т.п.)
    Calculate,
};
Q_ENUM_NS(ValueFormat)

//! Тип выборки из списка
enum class LookupType
{
    Undefined = 0,
    //! Выборка из набора данных
    Dataset = 1,
    //! Выборка из списка значений
    List = 2,
    //! Запросы к внешнему сервису
    Request = 3,
};
Q_ENUM_NS(LookupType)

//! Ответ да, нет
enum class Answer
{
    Undefined = 0,
    Yes = 1,
    No = 2,
};
Q_ENUM_NS(Answer)

//! Вид преобразования данных
enum class ConversionType
{
    Undefined,
    //! Преобразования нет
    Direct,
    //! В верхний регистр
    UpperCase,
    //! В нижний регистр
    LowerCase,
    //! Длина вместо самих данных
    Length,
    //! Расшифровка кода (для данных, которые могут содержать как код. так и его расшифровку)
    Name,
};

//! Вид условия
enum class ConditionType
{
    Undefined = 0x0001,
    //! Соединение нескольких условий по И
    And = 0x0002,
    //! Соединение нескольких условий по ИЛИ
    Or = 0x0004,
    //! Требуется наличие любых данных
    Required = 0x0008,
    //! Сравнение (с использованием CompareOperator)
    Compare = 0x0010,
    //! Максимальная длина текстового поля
    MaxTextLength = 0x0020,
    //! Проверка на соответствие регулярному выражению
    RegExp = 0x0040,
    //! Проверка на соответствие наследнику QValidator
    Validator = 0x0040,
};
Q_ENUM_NS(ConditionType)
Q_DECLARE_FLAGS(ConditionTypes, ConditionType)

//! Тип оператора для сравнения
enum class CompareOperator
{
    Undefined = 0x0000,
    Equal = 0x0001,
    NotEqual = 0x0002,
    More = 0x0003,
    MoreOrEqual = 0x0004,
    Less = 0x0005,
    LessOrEqual = 0x0006,
    Contains = 0x0007,
    StartsWith = 0x0008,
    EndsWith = 0x0009,
};
Q_ENUM_NS(CompareOperator)

//! Параметры сравнения
enum class CompareOption
{
    NoOption = 0x0000,
    CaseInsensitive = 0x0001,
};
Q_ENUM_NS(CompareOption)
Q_DECLARE_FLAGS(CompareOptions, CompareOption)

//! Режимы отображения названия сущности
enum class EntityNameMode
{
    //! Полное название
    Full,
    //! Сокращенное название
    Short,
    //! Код
    Code
};
Q_ENUM_NS(EntityNameMode)

enum class CatalogOption
{
    Undefined = 0x00001,
    //! Можно редактировать
    Editable = 0x00002,
    //! Можно добавлять строки
    CanAddRows = 0x00004,
    //! Можно удалять строки
    CanRemoveRows = 0x00008,
};
Q_ENUM_NS(CatalogOption)
Q_DECLARE_FLAGS(CatalogOptions, CatalogOption)

//! Математические операции
enum class MathOperaton
{
    Undefined = 0x0000,
    Plus = 0x0001,
    Minus = 0x0002,
    Multiply = 0x0003,
    Divide = 0x0004,
    OpeningBracket = 0x0005,
    ClosingBracket = 0x0006,
};
Q_ENUM_NS(MathOperaton)

/*! Режим выбора значений из справочника
 * Реализован как структура, т.к. enum class не работает для свойств в Qt Designer */
struct SelectionMode
{
    SelectionMode() = delete; // запрет на создание объекта

    enum Enum
    {
        Single = 0x0001,
        Multi = 0x0002
    };
    // регистрация в системе метаданных Qt
    Q_ENUM(Enum)
    Q_GADGET
};

/*! Типы информации (сообщений, записей в журналах и т.п.)
 *  ВАЖНО: номера по возрастанию приоритета! */
enum class InformationType
{
    Invalid = 0x0001,
    Success = 0x0002,
    Information = 0x0004,
    Warning = 0x0008,
    Error = 0x0010,
    Critical = 0x0020,
    Required = 0x0040,
    Fatal = 0x0080,
};
Q_ENUM_NS(InformationType)
Q_DECLARE_FLAGS(InformationTypes, InformationType)

//! Параметры ограничения
enum class ConstraintOption
{
    Undefined = 0x00000,
    //! Требуется ввод ID (например GUID для RequestSelector)
    RequiredID = 0x00001,
    //! Требуется ввод текста (например свободный ввод для RequestSelector)
    RequiredText = 0x00002,

    //! Значения по умолчанию
    Default = RequiredID,
};
Q_ENUM_NS(ConstraintOption)
Q_DECLARE_FLAGS(ConstraintOptions, ConstraintOption)

//! Вид свойства сущности
enum class PropertyType
{
    Undefined = 0,
    //! Поле с данными
    Field = 1,
    //! Ячейка набора данных
    Cell = 2,
    //! Строка набора данных полностью
    RowFull = 3,
    //! Любая колонка для строки набора данных
    RowPartial = 4,
    //! Колонка набора данных целиком
    ColumnFull = 5,
    //! Любая строка для колонки набора данных
    ColumnPartial = 6,
    //! Группа повторяющихся колонок. Для таблиц с динамическим количеством колонок
    //! ВНИМАНИЕ: В НАСТОЯЩИЙ МОМЕНТ НЕ РЕАЛИЗОВАНО
    ColumnGroup = 9,
    //! Набор данных целиком
    Dataset = 7,
    //! Сущность целиком
    Entity = 8,
    //! Независимая сущность целиком
    IndependentEntity = 10,
};
Q_ENUM_NS(PropertyType)

//! Параметры свойства. ВАЖНО: при изменении исправить метод Utils::allPropertyOptions
enum class PropertyOption : uint
{
    NoOptions = 0x0000,
    //! Имя
    Name = 0x0002,
    //! Полное имя
    FullName = 0x0004,
    //! Идентификатор (отключает Filtered и включает Hidden)
    Id = 0x0008,
    //! Идентификтор родителя (для иерархий) (отключает Filtered и включает Hidden)
    ParentId = 0x0010,
    //! Только для чтения (выглядит как поле ввода). Не влияет на логику сохранения данных на сервер
    ReadOnly = 0x0020,
    //! Только для чтения, выглядит как строка (не поле ввода). Автоматически устанавливает ReadOnly
    ForceReadOnly = 0x0040,
    //! Многоязычное свойство. Если этот параметр не задан, что ядро будет игнорировать попытки записи и чтения на
    //! разных языках
    MultiLanguage = 0x0080,
    /*! Для колонок набора данных: По этому свойству можно фильтровать содержимое таблицы
     * Для самих наборов данных, означает, что у них есть хотя бы одна колонка Filtered (выставляется автоматически) */
    Filtered = 0x0100,
    //! Скрытое
    Hidden = 0x0200,
    //! Ключевое поле для контроля уникальности (только для колонок наборов данных)
    KeyColumn = 0x0400,
    //! Ключевое поле которые надо заполнить, чтобы контроль уникальности начал работать (только для колонок наборов
    //! данных)
    KeyColumnBase = 0x0800,
    //! Колонка, отвечающая за отображение ошибок проверки на уникальность (только для колонок наборов данных)
    KeyColumnError = 0x1000,
    //! Перевод не используется. Вместо этого берется сам translationID
    NoTranslate = 0x2000,
    //! Сортировка по умолчанию для данной колонки. Задавать только для одной колонки в наборе данных
    //! Действует только для представлений (вызывается метод View::sortDataset)
    SortColumn = 0x4000,
    /*! Простой набор данных, состоящий из списка id, которые надо выбирать из lookup
     * Применимо только к PropertyType::Dataset, DataType::Table
     * Для таких наборов данных вместо TableView генерируется ItemSelector в режиме множественного выбора
     * В принципе такой набор данных может иметь любое количество колонок, но учитываться будет только колонка id */
    SimpleDataset = 0x8000,
    //! Колонка, в которой должны лежать значения SimpleDataset
    SimpleDatasetId = 0x10000,
    //! Поле или колонка набора данных является обязательной для заполнения
    //! Фактически, после создания свойства, выполняется метод DataProperty::setConstraintRequired
    Required = 0x20000,
    //! Игнорировать состояние ReadOnly. Если задано, виджет не будет блокироваться при блокировке формы.
    //! Нельзя использовать совместно с ReadOnly/ForceReadOnly
    IgnoreReadOnly = 0x40000,

    // Свойства, влияющие на взаимодействие с сервером
    //! Свойство игнорируется при записи в БД
    DBWriteIgnored = 0x80000,
    //! Свойство игнорируется при чтении из БД
    DBReadIgnored = 0x100000,
    //! Свойство не хранится на сервере и вычисляется программно на основании других свойств
    //! Полностью игнорируется драйвером БД
    ClientOnly = DBWriteIgnored | DBReadIgnored,

    //! Если задано, то свойства c lookup не будут инициализировать значением из lookup, когда оно единственное и свойство имеет ограничение Required
    NoInitLookup = 0x200000,
    //! Если задано, то свойства c lookup будут инициализировать значением из lookup не взирая на отсутствие Required
    ForceInitLookup = 0x400000,

    //! Для колонки таблицы заблокировано редактирование в ячейке
    NoEditTriggers = 0x800000,

    //! Для набора данных включить видимость и доступность операции просмотра ObjectActionType::View
    EnableViewOperation = 0x1000000,
    //! При записи в БД не убирать пробелы по краям (для текстовых полей)
    NoTrimOnDBWrite = 0x2000000,
    //! Если в поле находится InvalidValue, то выводить ошибку уровня InformationType::Error, а не Warning как по учмолчанию
    ErrorIfInvalid = 0x4000000,
    //! Если в поле находится InvalidValue, то не выводить автоматическую ошибку
    IgnoreInvalidHighlight = 0x8000000,

    //! Ноль для числовых полей (Integer, Float, Numeric) будет рассматриваться как значение "Задано". Т.е. условие ConstraintRequired будет выполнено
    ValidZero = 0x10000000,
    //! На набор данных не будет действовать метод setDatasetSortEnabledAll/setDatasetSortEnabled
    SortDisabled = 0x20000000,
    //! Отключить автоматическое скрытие при отображении в SelectFromModelDialog
    VisibleItemModelDialog = 0x40000000,

    //! Свойство явно отмечается как изменяемое пользователем
    //! Работает только вместе в RequestSelector. Им отмечаются свойств, используемые в PropertyLookup::attributes, для которых не надо перезатерать существующее значение
    //! Например имеется поле "почтовый индекс", которое загружается с сервиса ФИАС, но мы не хотим перетирать введенное ранее пользователем значение
    EditedByUser = 0x80000000,
};
Q_ENUM_NS(PropertyOption)
Q_DECLARE_FLAGS(PropertyOptions, PropertyOption)

//! Вид связи между свойствами
enum class PropertyLinkType
{
    //! Не задано
    Undefined = 0x0000,
    //! Указывает на поле, в которое надо помещать результат свободного ввода текста
    //! Используется для работы с ItemSelector в режиме editable или при вводе текста в RequestSelector
    LookupFreeText = 0x0001,
    //! Правило автоматического обновления значения свойства на основании другого свойства
    DataSource = 0x0002,
    //! Правило автоматического обновления одного поля на основании первого заданного значения в списке других свойств
    DataSourcePriority = 0x0003,
};
Q_ENUM_NS(PropertyLinkType)

//! Параметры для добавления в HighlightModel
enum class HighlightOption
{
    Undefined = 0x000,
    //! Не блокирует процесс изменения данных
    NotBlocked = 0x001,
};
Q_ENUM_NS(HighlightOption)
Q_DECLARE_FLAGS(HighlightOptions, HighlightOption)

//! Уровень отладочной информации
enum class LogLevel
{
    //! Общая
    Default = 0x0,
    //! Детальная
    Detail = 0x1,
    //! Подробная
    Verbose = 0x2,
};
Q_ENUM_NS(LogLevel)

//! Виды ошибок
enum class ErrorType
{
    //! Нет ошибки
    No = 0,
    //! Произвольная ошибка (не обрабатывается ядром)
    Custom = 1,
    //! Скрытая ошибка. Не должна отображать на экране, т.к. отображение будет проводиться иначе (например по
    //! сигналу)
    Hidden = 2,
    //! Общая системная ошибка. Сгенерирована ядром без привязки к конкретному действию
    Core = 3,
    //! Системная ошибка. Не приводит к остановке программы, но требует вмешательства разработчика
    System = 4,
    //! Это не ошибка, а просто указание на отмену операции. Никакой информации пользователю не выводится
    Cancel = 5,
    //! Синтаксическая ошибка
    Syntax = 6,
    //! Требуется подтверждение от пользователя
    NeedConfirmation = 7,
    //! Приложение в состоянии критической ошибки и будет закрыто
    ApplicationHalted = 8,

    //! Ошибка загрузки модуля в целом
    Module = 9,
    //! Не найдено представление
    ViewNotFound = 10,
    //! Плагин не найден
    ModuleNotFound = 11,

    //! Ошибка драйвера БД
    DatabaseDriver = 12,

    //! Ошибка с указанием (открытием) документа
    Document = 13,

    /*! Ошибка взаимодействия с БД (ошибка драйвера) */
    Database = 14,

    //! В модели есть несохраненные изменения и она отказывает в проведении операции (например операции обновления
    //! без опции force)
    HasUnsavedData = 15,
    //! Сущность не найдена в БД
    EntityNotFound = 16,

    //! Ошибка со журналом ошибок
    LogItems = 18,

    //! Файл не найден
    FileNotFound = 19,
    //! Некоректный файл
    BadFile = 20,
    //! Ошибка чтения/записи файла
    FileIO = 21,
    //! Ошибка копирования файла
    CopyFile = 22,
    //! Ошибка удаления файла
    RemoveFile = 23,
    //! Ошибка переименования файла
    RenameFile = 24,
    //! Ошибка создания директории
    CreateDir = 25,
    //! Ошибка удаления директории
    RemoveDir = 26,
    //! Превышен период ожидания
    Timeout = 24,
    //! Операция не поддерживается
    Unsupported = 25,
    //! Ошибка JSON документа
    Json = 26,
    //! Некорректные данные
    CorruptedData = 27,
    //! Запрещено
    Forbidden = 28,
    //! Не найдено. Например не найдена информация при HTTP запросе и т.п. Т.е. любые общие ошибки такого вида
    DataNotFound = 29,
    //! Преобразование данных
    Conversion = 30,
    //! Потеряно соединение
    ConnectionClosed = 31,
    //! Объект был изменен другим пользователем
    ChangedByOtherUser = 32,
};
Q_ENUM_NS(ErrorType)

//! Вид события для разделяемых объектов (модели и т.п.) (для отладки)
enum class SharedObjectEventType
{
    ToStorage = 0,
    ToCache = 1,
    RemoveFromStorage = 2,
    RemoveFromCache = 3,
    TakeFromCache = 4,
    TakeFromStorage = 5,
    New = 6,
    NewDetached = 7,
    Remove = 8,
    LoadedFromDatabase = 9,
    DetachedCloned = 10,
    Destroy = 11,
    DestroyDetached = 12,
};
Q_ENUM_NS(SharedObjectEventType)

/*! Виды сообщений
 * ВАЖНО: при любом изменении, внести исправление в: 
 * 1. DatabaseManager::_command_feedback_mapping - добавить соответствие команды и ответа (если сообщение относится к DatabaseManager)
 * 2. DatabaseManager::sl_feedback - добавить обработчик ответа на команду (если он не нужен, то просто пустую ветку if
 * else) (если сообщение относится к DatabaseManager)
 * 3. Дополнить методы Message::splitByDatabase, Message::concatByDatabase (если сообщение относится к DatabaseManager)
 */
enum class MessageType {
    Invalid = 0,

    /*! Ответ с подтверждением выполнения. Может содержать параметр */
    Confirm = 1,
    //! Ответ с ошибкой
    Error = 2,
    //! Нестандартное сообщение (структура не определена)
    General = 3,
    //! Список QVariant
    VariantList = 4,
    //! Список int
    IntList = 5,
    //! Список QString
    StringList = 6,
    //! PropertyTable
    PropertyTable = 7,
    //! IntTable
    IntTable = 8,
    //! Информация о прогрессе
    Progress = 9,
    //! Список QByteArray
    ByteArrayList = 10,
    //! Логическое значение
    Bool = 11,
    //! QMap<QString, QVariant>
    VariantMap = 12,

    //******* Команды менеджеру БД
    //! Произвольный запрос (ответ: DBEventQueryFeedback)
    DBCommandQuery = 100,
    //! Существуют ли такие сущности в БД (ответ: DBEventEntityExists)
    DBCommandIsEntityExists = 102,
    //! Получить сущности из БД (ответ: DBEventEntityLoaded)
    DBCommandGetEntity = 103,
    //! Записать сущности в БД (ответ: Confirm. Содержит в data() zf::UidList с идентификаторами записанных сущностей)
    DBCommandWriteEntity = 104,
    //! Удалить сущности из БД (ответ: Confirm)
    DBCommandRemoveEntity = 105,
    //! Получить идентификаторы сущностей указанного вида из БД (ответ: DBEventEntityList)
    DBCommandGetEntityList = 106,
    //! Получить таблицу свойств сущностей указанного вида из БД (ответ: DBEventPropertyTable)
    DBCommandGetPropertyTable = 107,
    //! Получить возможности (глобальные настройки программы для указанной БД) (Ответ: DBEventConnectionInformation)
    DBCommandGetConnectionInformation = 108,
    //! Получить права доступа к сущностям
    DBCommandGetAccessRights = 109,
    //! Запрос генерации отчета (ответ: DBEventReport)
    DBCommandGenerateReport = 110,
    //! Получить информацию о структуре каталога
    DBCommandGetCatalogInfo = 111,
    //! Запрос на подключение к БД
    DBCommandLogin = 112,
    //! Запрос выборки из таблицы БД (ответ: DBEventDataTable)
    DBCommandGetDataTable = 113,
    //! Запрос информации о таблице БД (ответ: DBEventDataTableInfo)
    DBCommandGetDataTableInfo = 114,
    //! Массовое обновление данных в БД (ответ: DBEventUpdateMessage)
    DBCommandUpdateEntities = 115,
    //! Переподключение к БД
    DBCommandReconnect = 116,

    //******* Ответа от менеджера БД на команды
    //! Ответ на DBCommandQuery (ответ на DBCommandGetEntity)
    DBEventQueryFeedback = 200,
    //! Загружены сущности (ответ на DBCommandGetEntity)
    DBEventEntityLoaded = 201,
    //! Информация о том, существуют ли такие сущности (ответ на DBCommandIsEntityExists)
    DBEventEntityExists = 203,
    //! Идентификаторы сущностей указанного вида из БД (ответ на DBCommandGetEntityList)
    DBEventEntityList = 204,
    //! Таблица свойств сущностей указанного вида из БД (ответ на DBCommandGetPropertyTable)
    DBEventPropertyTable = 205,
    //! Отклик сервера на запрос о возможностях ПО (глобальные настройки программы для указанной БД) (ответ на
    //! DBCommandGetConnectionInformationMessage)
    DBEventConnectionInformation = 206,
    //! Отклик на запрос прав доступа к сущностям (ответ на DBCommandGetAccessRightsMessage)
    DBEventAccessRights = 207,
    //! Отчет в ответ на DBCommandGenerateReportMessage
    DBEventReport = 208,
    //! Информация о структуре каталога в ответ на DBCommandGetCatalogInfo
    DBEventCatalogInfo = 209,
    //! Ответ на запрос подключения к БД (DBCommandLogin)
    DBEventLogin = 210,
    //! Результат запроса на выборку из таблицы сервера (в ответ на DBCommandGetDataTable)
    DBEventDataTable = 211,
    //! Результат запроса на информацию о таблице сервера (в ответ на DBCommandGetDataTableInfo)
    DBEventDataTableInfo = 212,

    //******** Сообщения, генерируемые сервером и распространяемые по соответствующим каналам (CoreChannels)
    //! Изменилась сущность CoreChannels::ENTITY_CHANGED
    DBEventEntityChanged = 300,
    //! Сущность удалена CoreChannels::ENTITY_REMOVED
    DBEventEntityRemoved = 301,
    //! Сущность создана CoreChannels::ENTITY_CREATED
    DBEventEntityCreated = 302,
    //! Некое информационное текстовое сообщение от сервера CoreChannels::SERVER_INFORMATION
    DBEventInformation = 303,
    //! Драйвер БД информирует что соединение с сервером выполнено
    //! CoreChannels::SERVER_INFORMATION
    DBEventConnectionDone = 304,
    //! Драйвер БД информирует что закончена начальная инициализация (подгрузка справочников и т.п.)
    //! CoreChannels::SERVER_INFORMATION
    DBEventInitLoadDone = 305,

    //****** Команды менеджеру моделей
    //! Вернуть список загруженных моделей (ответ MMEventModelList или Error)
    MMCommandGetModels = 400,
    //! Удалить модели (ответ Confirm или Error)
    MMCommandRemoveModels = 401,
    //! Запрос на получение имен сущностей  (ответ MMEventEntityNames)
    MMCommandGetEntityNames = 402,
    //! Запрос на получение значения каталога (ответ MMEventCatalogValue или Error)
    MMCommandGetCatalogValue = 403,

    //****** Ответы менеджера моделей
    //! Ответ на MMCommandGetModels: список std::shared_ptr<Model>
    MMEventModelList = 500,
    //! Ответ на MMCommandGetEntityNames: список имен сущностей
    MMEventEntityNames = 501,
    //!Ответ на MMCommandGetCatalogValueMessage: значение из каталога
    MMEventCatalogValue = 502,

    //****** Сообщения, рассылаемые моделями в соответствующие каналы
    //! Данные модели стали невалидными CoreChannels::MODEL_INVALIDATE
    ModelInvalide = 600,
};
Q_ENUM_NS(MessageType)

//! Вид локали
enum class LocaleType
{
    Undefined,
    //! Для хранения данных и т.п (C locale)
    Universal,
    //! Пользовательский интерфейс
    UserInterface,
    //! Документооборот
    Workflow
};
Q_ENUM_NS(LocaleType)

//! Область действия операции
enum class OperationScope
{
    Undefined,
    //! Весь модуль (не зависит от сущности)
    Module,
    //! Конкретная сущность
    Entity
};
Q_ENUM_NS(OperationScope)

//! Направление операции (пока не используется)
enum class OperationDirection
{
    Undefined,
    //! Произвольная операция не требующая устройство
    General,
    //! Экспорт (файл, каталог, принтер)
    Export,
    //! Импорт (файл, каталог)
    Import
};
Q_ENUM_NS(OperationDirection)

//! Тип операции (пока не используется)
enum OperationType
{
    Undefined,
    //! Произвольная операция не требующая устройство
    General,
    //! Файловая
    File,
    //! С каталогом
    Folder,
    //! С принтером
    Printer,
};
Q_ENUM_NS(OperationType)

//! Различные параметры операции
enum class OperationOption
{
    //! Запрещена
    Disabled = 0x0001,
    /*! Требуется ли запрашивать у пользователя подтверждение выполнения операции
     * Используется для операций OperationDirection::General, т.к. для остальных операций
     * фактом подтверждения является реакция пользователя на диалог выбора */
    Confirmation = 0x0002,
    //! Отображать состояние прогресса в процессе выполнения операции
    ShowProgress = 0x004,
    //! Отображать текст на кнопке тулбара
    ShowButtonText = 0x008,
    //! Не добавлять в тулбар
    NoToolbar = 0x010,
};
Q_ENUM_NS(OperationOption)
Q_DECLARE_FLAGS(OperationOptions, OperationOption)

//! Вид тулбара
enum class ToolbarType
{
    //! Главный тулбар приложения
    Main,
    //! Тулбар окна
    Window,
    //! Тулбар внутри окна (например у надор данных)
    Internal,
};
Q_ENUM_NS(ToolbarType)

//! Тип файла
enum class FileType
{
    Undefined = 0x000000,
    Docx = 0x000001,
    Doc = 0x000002,
    Xlsx = 0x000004,
    Xls = 0x000008,
    Html = 0x000010,
    Plain = 0x000020,
    Pdf = 0x000040,
    Json = 0x000080,
    Zip = 0x000100,
    Rtf = 0x000200,
    Png = 0x000400,
    Jpg = 0x000800,
    Bmp = 0x001000,
    Ico = 0x002000,
    Tif = 0x004000,
    Gif = 0x008000,
    Xml = 0x010000,
    Svg = 0x020000,
};
Q_ENUM_NS(FileType)
Q_DECLARE_FLAGS(FileTypes, FileType)

//! Свойства UniViewWidget
enum class UniViewWidgetOption
{
    //! Открыть в режиме редактирования. Доступно для Text и MSWord/MSExcel (под windows)
    Editable = 0x01,
    //! При попытке открыть файл Word или Excel в режиме просмотра, он будет предварительно скопирован во временную
    //! папку, чтобы избежать проблем с отображением в двух окнах (под windows). Не нужно, если файл гарантированно
    //! открывается в единственном окне
    MsOfficeClone = 0x02,
    //! Конвертация Word в PDF
    WordToPdf = 0x04,
    //! Конвертация Excel в PDF
    ExcelToPdf = 0x08,
};
Q_ENUM_NS(UniViewWidgetOption)
Q_DECLARE_FLAGS(UniViewWidgetOptions, UniViewWidgetOption)

//! Сторона при сетевом взаимодействии
enum class CommunicationSide
{
    Undefined = 0,
    Client = 1,
    Server = 2,
    Other = 3,
};
Q_ENUM_NS(CommunicationSide)

//! Режим Масштабирования
enum class FittingType
{
    Undefined = 0,
    Page = 1,
    Width = 2,
    Height = 3,
};
Q_ENUM_NS(FittingType)

//! Вид поворота
enum class RotationType
{
    Undefined = 0,
    Left = 1,
    Right = 2,
};
Q_ENUM_NS(RotationType)

//! Параметры универсального окна просмотра документов
enum class UniviewParameter
{
    Undefined = 0x0000,
    //! Изменение масштаба шагом
    ZoomStep = 0x0001,
    //! Установка процента
    ZoomPercent = 0x0002,
    //! Поворот на 90 градусов
    RotateStep = 0x0004,
    //! Установка масштаба по ширине, высоте страницы
    AutoZoom = 0x0008,
    //! Чтение номера текущей страницы
    ReadPageNo = 0x0010,
    //! Установка номера текущей страницы
    SetPageNo = 0x0012,
};
Q_ENUM_NS(UniviewParameter)
Q_DECLARE_FLAGS(UniviewParameters, UniviewParameter)

//! Режимы для метода DataContainer::setDataset/copyFrom
enum class CloneContainerDatasetMode
{
    //! Скопировать данные
    Clone,
    //! Копировать указатель
    CopyPointer,
    //! Переместить содержимое набора данных
    MoveContent
};
Q_ENUM_NS(CloneContainerDatasetMode)

//! Падежи
enum class Padeg
{
    Undefined,
    //! Именительный КТО-ЧТО
    Nominative,
    //! Родительный  КОГО-ЧЕГО
    Genitive,
    //! Дательный    КОМУ-ЧЕМУ
    Dative,
    //! Винительный  КОГО-ЧТО
    Accusative,
    //!Творительный КЕМ-ЧЕМ
    Instrumentale,
    //! Предложный  О КОМ-О ЧЕМ
    Preposizionale,
};
Q_ENUM_NS(Padeg)

//! Параметры метода zf::View::updateEmbeddedView
enum class UpdateEmbeddedViewOption
{
    //! Добавить в начало лайаута
    LayoutInsertToBegin,
};
Q_ENUM_NS(UpdateEmbeddedViewOption)
Q_DECLARE_FLAGS(UpdateEmbeddedViewOptions, UpdateEmbeddedViewOption)

//! Состояние шага
enum class WizardStepState
{
    NoState = 0,
    //! Шаг доступен (может при этом быть невидим)
    Enabled = 1,
    //! Шаг видим
    Visible = 2,
};
Q_ENUM_NS(WizardStepState)
Q_DECLARE_FLAGS(WizardStepStates, WizardStepState)

//! Результаты подключения к серверу
enum class ConnectionProperty
{
    NoProperty = 0,
    //! Версии сервера и клиента не совпадают
    VersionMismatch = 1,
};
Q_ENUM_NS(ConnectionProperty)
Q_DECLARE_FLAGS(ConnectionProperties, ConnectionProperty)

//! Результаты подключения к серверу
enum class DatasetConfigOption
{
    NoOptions = 0x000,
    //! Видимость и порядок колонок
    Visible = 0x001,
    //! Фильтрация
    Filter = 0x002,
    //! Экспорт
    Export = 0x004,
};
Q_ENUM_NS(DatasetConfigOption)
Q_DECLARE_FLAGS(DatasetConfigOptions, DatasetConfigOption)

//! Флаги для FlatItemModel
enum class FlatItemModelOption
{
    NoOptions = 0x000,
    //! Генерировать ли сигнал sg_dataChangedExtended
    GenerateDataChangedExtendedSignal = 0x001,
};
Q_ENUM_NS(FlatItemModelOption)
Q_DECLARE_FLAGS(FlatItemModelOptions, FlatItemModelOption)

//! Виды узлов XML
enum class XML_NodeType
{
    Undefined = 0,
    XML_ELEMENT_NODE = 1,
    XML_ATTRIBUTE_NODE = 2,
    XML_TEXT_NODE = 3,
    XML_CDATA_SECTION_NODE = 4,
    XML_ENTITY_REF_NODE = 5,
    XML_ENTITY_NODE = 6,
    XML_PI_NODE = 7,
    XML_COMMENT_NODE = 8,
    XML_DOCUMENT_NODE = 9,
    XML_DOCUMENT_TYPE_NODE = 10,
    XML_DOCUMENT_FRAG_NODE = 11,
    XML_NOTATION_NODE = 12,
    XML_HTML_DOCUMENT_NODE = 13,
    XML_DTD_NODE = 14,
    XML_ELEMENT_DECL = 15,
    XML_ATTRIBUTE_DECL = 16,
    XML_ENTITY_DECL = 17,
    XML_NAMESPACE_DECL = 18,
    XML_XINCLUDE_START = 19,
    XML_XINCLUDE_END = 20,
    XML_DOCB_DOCUMENT_NODE = 21
};

//! Виды элементов XML
enum class XML_ElementType
{
    XML_ELEMENT_NODE = 1,
    XML_ATTRIBUTE_NODE = 2,
    XML_TEXT_NODE = 3,
    XML_CDATA_SECTION_NODE = 4,
    XML_ENTITY_REF_NODE = 5,
    XML_ENTITY_NODE = 6,
    XML_PI_NODE = 7,
    XML_COMMENT_NODE = 8,
    XML_DOCUMENT_NODE = 9,
    XML_DOCUMENT_TYPE_NODE = 10,
    XML_DOCUMENT_FRAG_NODE = 11,
    XML_NOTATION_NODE = 12,
    XML_HTML_DOCUMENT_NODE = 13,
    XML_DTD_NODE = 14,
    XML_ELEMENT_DECL = 15,
    XML_ATTRIBUTE_DECL = 16,
    XML_ENTITY_DECL = 17,
    XML_NAMESPACE_DECL = 18,
    XML_XINCLUDE_START = 19,
    XML_XINCLUDE_END = 20,
    XML_DOCB_DOCUMENT_NODE = 21
};

//! Виды атрибутов XML
enum class XML_AttributeType
{
    XML_ATTRIBUTE_CDATA = 1,
    XML_ATTRIBUTE_ID,
    XML_ATTRIBUTE_IDREF,
    XML_ATTRIBUTE_IDREFS,
    XML_ATTRIBUTE_ENTITY,
    XML_ATTRIBUTE_ENTITIES,
    XML_ATTRIBUTE_NMTOKEN,
    XML_ATTRIBUTE_NMTOKENS,
    XML_ATTRIBUTE_ENUMERATION,
    XML_ATTRIBUTE_NOTATION
};

//! Свойства XML документа
enum class XML_DocProperty
{
    XML_DOC_WELLFORMED = 1 << 0, /* document is XML well formed */
    XML_DOC_NSVALID = 1 << 1, /* document is Namespace valid */
    XML_DOC_OLD10 = 1 << 2, /* parsed with old XML-1.0 parser */
    XML_DOC_DTDVALID = 1 << 3, /* DTD validation was successful */
    XML_DOC_XINCLUDE = 1 << 4, /* XInclude substitution was done */
    XML_DOC_USERBUILT = 1 << 5, /* Document was built using the API
                                       and not by parsing an instance */
    XML_DOC_INTERNAL = 1 << 6, /* built for internal processing */
    XML_DOC_HTML = 1 << 7 /* parsed or built HTML document */
};
Q_DECLARE_FLAGS(XML_DocProperties, XML_DocProperty)

//! Параметры парсера XML
enum class XML_ParserOption
{
    XML_PARSE_RECOVER = 1 << 0, /* recover on errors */
    XML_PARSE_NOENT = 1 << 1, /* substitute entities */
    XML_PARSE_DTDLOAD = 1 << 2, /* load the external subset */
    XML_PARSE_DTDATTR = 1 << 3, /* default DTD attributes */
    XML_PARSE_DTDVALID = 1 << 4, /* validate with the DTD */
    XML_PARSE_NOERROR = 1 << 5, /* suppress error reports */
    XML_PARSE_NOWARNING = 1 << 6, /* suppress warning reports */
    XML_PARSE_PEDANTIC = 1 << 7, /* pedantic error reporting */
    XML_PARSE_NOBLANKS = 1 << 8, /* remove blank nodes */
    XML_PARSE_SAX1 = 1 << 9, /* use the SAX1 interface internally */
    XML_PARSE_XINCLUDE = 1 << 10, /* Implement XInclude substitution  */
    XML_PARSE_NONET = 1 << 11, /* Forbid network access */
    XML_PARSE_NODICT = 1 << 12, /* Do not reuse the context dictionary */
    XML_PARSE_NSCLEAN = 1 << 13, /* remove redundant namespaces declarations */
    XML_PARSE_NOCDATA = 1 << 14, /* merge CDATA as text nodes */
    XML_PARSE_NOXINCNODE = 1 << 15, /* do not generate XINCLUDE START/END nodes */
    XML_PARSE_COMPACT = 1 << 16, /* compact small text nodes; no modification of
                                    the tree allowed afterwards (will possibly
                    crash if you try to modify the tree) */
    XML_PARSE_OLD10 = 1 << 17, /* parse using XML-1.0 before update 5 */
    XML_PARSE_NOBASEFIX = 1 << 18, /* do not fixup XINCLUDE xml:base uris */
    XML_PARSE_HUGE = 1 << 19, /* relax any hardcoded limit from the parser */
    XML_PARSE_OLDSAX = 1 << 20, /* parse using SAX2 interface before 2.7.0 */
    XML_PARSE_IGNORE_ENC = 1 << 21, /* ignore internal document encoding hint */
    XML_PARSE_BIG_LINES = 1 << 22, /* Store big lines numbers in text PSVI field */

    HTML_PARSE_RECOVER = 1 << 0, /* Relaxed parsing */
    HTML_PARSE_NODEFDTD = 1 << 2, /* do not default a doctype if not found */
    HTML_PARSE_NOERROR = 1 << 5, /* suppress error reports */
    HTML_PARSE_NOWARNING = 1 << 6, /* suppress warning reports */
    HTML_PARSE_PEDANTIC = 1 << 7, /* pedantic error reporting */
    HTML_PARSE_NOBLANKS = 1 << 8, /* remove blank nodes */
    HTML_PARSE_NONET = 1 << 11, /* Forbid network access */
    HTML_PARSE_NOIMPLIED = 1 << 13, /* Do not add implied html/body... elements */
    HTML_PARSE_COMPACT = 1 << 16, /* compact small text nodes */
    HTML_PARSE_IGNORE_ENC = 1 << 21, /* ignore internal document encoding hint */

    HTML_PARSE_DEFAULT = HTML_PARSE_RECOVER | HTML_PARSE_NOERROR | HTML_PARSE_NOWARNING | HTML_PARSE_NONET,
};
Q_DECLARE_FLAGS(XML_ParserOptions, XML_ParserOption)

//! Параметры сохранения XML в файл
enum class XML_SaveOption
{
    XML_SAVE_FORMAT = 1 << 0, /* format save output */
    XML_SAVE_NO_DECL = 1 << 1, /* drop the xml declaration */
    XML_SAVE_NO_EMPTY = 1 << 2, /* no empty tags */
    XML_SAVE_NO_XHTML = 1 << 3, /* disable XHTML1 specific rules */
    XML_SAVE_XHTML = 1 << 4, /* force XHTML1 specific rules */
    XML_SAVE_AS_XML = 1 << 5, /* force XML serialization on HTML doc */
    XML_SAVE_AS_HTML = 1 << 6, /* force HTML serialization on XML doc */
    XML_SAVE_WSNONSIG = 1 << 7 /* format with non-significant whitespace */
};
Q_DECLARE_FLAGS(XML_SaveOptions, XML_SaveOption)

inline uint qHash(const InformationType& type)
{
    return ::qHash(static_cast<int>(type));
}
inline uint qHash(const PropertyType& type)
{
    return ::qHash(static_cast<int>(type));
}
inline uint qHash(const PropertyLinkType& type)
{
    return ::qHash(static_cast<int>(type));
}

Q_DECLARE_OPERATORS_FOR_FLAGS(zf::InformationTypes);
Q_DECLARE_OPERATORS_FOR_FLAGS(zf::DataTypes);
Q_DECLARE_OPERATORS_FOR_FLAGS(zf::PropertyOptions);
Q_DECLARE_OPERATORS_FOR_FLAGS(zf::DatabaseObjectOptions);
Q_DECLARE_OPERATORS_FOR_FLAGS(zf::ModuleDataOptions);
Q_DECLARE_OPERATORS_FOR_FLAGS(zf::XML_ParserOptions);
Q_DECLARE_OPERATORS_FOR_FLAGS(zf::XML_DocProperties);
Q_DECLARE_OPERATORS_FOR_FLAGS(zf::XML_SaveOptions);
Q_DECLARE_OPERATORS_FOR_FLAGS(zf::ViewStates);
Q_DECLARE_OPERATORS_FOR_FLAGS(zf::ModuleWindowOptions);
Q_DECLARE_OPERATORS_FOR_FLAGS(zf::CompareOptions);
Q_DECLARE_OPERATORS_FOR_FLAGS(zf::CatalogOptions);
Q_DECLARE_OPERATORS_FOR_FLAGS(zf::CoreModes);
Q_DECLARE_OPERATORS_FOR_FLAGS(zf::UniviewParameters);
Q_DECLARE_OPERATORS_FOR_FLAGS(zf::ImageListOptions);
Q_DECLARE_OPERATORS_FOR_FLAGS(zf::FileTypes);
Q_DECLARE_OPERATORS_FOR_FLAGS(zf::UniViewWidgetOptions);
Q_DECLARE_OPERATORS_FOR_FLAGS(zf::HighlightDialogOptions);
Q_DECLARE_OPERATORS_FOR_FLAGS(zf::ConditionTypes);
Q_DECLARE_OPERATORS_FOR_FLAGS(zf::ConstraintOptions);
Q_DECLARE_OPERATORS_FOR_FLAGS(zf::UpdateEmbeddedViewOptions);
Q_DECLARE_OPERATORS_FOR_FLAGS(zf::HighlightOptions);
Q_DECLARE_OPERATORS_FOR_FLAGS(zf::WizardStepStates);
Q_DECLARE_OPERATORS_FOR_FLAGS(zf::ConnectionProperties);
Q_DECLARE_OPERATORS_FOR_FLAGS(zf::DatasetConfigOptions);
Q_DECLARE_OPERATORS_FOR_FLAGS(zf::VisibleValueOptions);

//! Для совместимости версий Qt
#if (QT_VERSION < QT_VERSION_CHECK(5, 14, 0))

#define Z_RECURSIVE_MUTEX z_RecursiveMutex

class z_RecursiveMutex : public QMutex
{
public:
    inline z_RecursiveMutex()
        : QMutex(QMutex::Recursive)
    {
    }
};
#else
#include <QRecursiveMutex>
#define Z_RECURSIVE_MUTEX QRecursiveMutex
#endif

} // namespace zf

inline uint qHash(const QPointer<QWidget>& w)
{
    return w.isNull() ? qHash(0) : qHash(reinterpret_cast<qintptr>(w.data()));
}

inline bool operator<(const QPointer<QWidget>& w1, const QPointer<QWidget>& w2)
{
    return (w1.isNull() ? 0 : reinterpret_cast<qintptr>(w1.data())) < (w2.isNull() ? 0 : reinterpret_cast<qintptr>(w2.data()));
}

Q_DECLARE_METATYPE(zf::QByteArrayPtr)
Q_DECLARE_METATYPE(zf::QVariantPtr)

Q_DECLARE_METATYPE(zf::OperationOptions)
Q_DECLARE_METATYPE(zf::DatabaseObjectOptions)
Q_DECLARE_METATYPE(zf::ModuleDataOptions)
Q_DECLARE_METATYPE(zf::LoadOptions)
Q_DECLARE_METATYPE(zf::PropertyOptions)
Q_DECLARE_METATYPE(zf::ViewStates)
Q_DECLARE_METATYPE(zf::ModuleWindowOptions)
Q_DECLARE_METATYPE(zf::CompareOptions)
Q_DECLARE_METATYPE(zf::FileTypes)
Q_DECLARE_METATYPE(zf::CatalogOptions)
Q_DECLARE_METATYPE(zf::CoreModes)
Q_DECLARE_METATYPE(zf::UniviewParameters)
Q_DECLARE_METATYPE(zf::ImageListOptions)
Q_DECLARE_METATYPE(zf::UniViewWidgetOptions)
Q_DECLARE_METATYPE(zf::HighlightDialogOptions)
Q_DECLARE_METATYPE(zf::ConditionTypes)
Q_DECLARE_METATYPE(zf::ConstraintOptions)
Q_DECLARE_METATYPE(zf::UpdateEmbeddedViewOptions)
Q_DECLARE_METATYPE(zf::HighlightOptions)
Q_DECLARE_METATYPE(zf::ViewOptions)
Q_DECLARE_METATYPE(zf::WizardStepStates)
Q_DECLARE_METATYPE(zf::ConnectionProperties)
Q_DECLARE_METATYPE(zf::DatasetConfigOptions)
Q_DECLARE_METATYPE(zf::VisibleValueOptions)
Q_DECLARE_METATYPE(zf::FlatItemModelOptions)
