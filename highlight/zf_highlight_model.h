#pragma once

#include <QObject>

#include "zf.h"
#include "zf_data_structure.h"

namespace zf
{
class HighlightItem_data;

//! Элемент информации об ошибках
class ZCORESHARED_EXPORT HighlightItem
{
public:
    HighlightItem();
    HighlightItem(
        //! Вид ошибки
        InformationType type,
        //! Свойство, к которому относится ошибка
        const DataProperty& property,
        //! Код ошибки (должен быть уникален в рамках property)
        int id,
        //! Текст ошибки
        const QString& text,
        //! Код группы для логического объединения разных ошибок в группу
        int group_code = -1,
        //! Произвольные данные
        const QVariant& data = QVariant(),
        //! Параметры
        const HighlightOptions& options = {});
    HighlightItem(const HighlightItem& item);
    ~HighlightItem();
    HighlightItem& operator=(const HighlightItem& item);
    bool operator==(const HighlightItem& item) const;
    bool operator!=(const HighlightItem& item) const;
    bool operator<(const HighlightItem& item) const;

    bool isValid() const;

    //! Вид ошибки
    InformationType type() const;
    //! Свойство, к которому относится ошибка
    DataProperty property() const;
    //! Код ошибки (должен быть уникален в рамках property)
    int id() const;
    //! Текст ошибки
    QString text() const;
    //! Текст с убранными HTML тегами
    QString plainText() const;
    //! Код группы для логического объединения разных ошибок в группу
    int groupCode() const;
    //! Произвольные данные
    QVariant data() const;
    //! Параметры
    HighlightOptions options() const;

    //! Уникальный ключ
    QString uniqueKey() const;
    //! Ключ для qHash
    uint hashKey() const;

    QVariant variant() const;
    static HighlightItem fromVariant(const QVariant& v);

private:
    static QString createKey(const DataProperty& p, int id);

    //! Данные
    QSharedDataPointer<HighlightItem_data> _d;

    friend class HighlightModel;
};
typedef std::shared_ptr<HighlightItem> HighlightItemPtr;

//! Информация об ошибках
class ZCORESHARED_EXPORT HighlightModel : public QObject
{
    Q_OBJECT

public:
    explicit HighlightModel(QObject* parent = nullptr);

    //! Очистить данные
    void clear();

    //! Количество элементов
    int count(const HighlightOptions& options = {}) const;
    //! Количество элементов определенного типа
    int count(InformationTypes types, const HighlightOptions& options = {}) const;
    //! Количество элементов определенной группы
    int groupCount(int groupCode, const HighlightOptions& options = {}) const;

    //! Количество элементов, у которых нет указанных свойств
    int countWithoutOptions(const HighlightOptions& options) const;
    //! Количество элементов определенного типа, у которых нет указанных свойств
    int countWithoutOptions(InformationTypes types, const HighlightOptions& options) const;
    //! Количество элементов определенной группы, у которых нет указанных свойств
    int groupCountWithoutOptions(int groupCode, const HighlightOptions& options) const;

    //! Содержит ошибки
    bool hasErrors(const HighlightOptions& options = {}) const;
    //! Содержит ошибки или предупреждения
    bool hasWarnings(const HighlightOptions& options = {}) const;
    //! Пустая модель
    bool isEmpty() const;

    //! Содержит ошибки, у которых нет указанных свойств
    bool hasErrorsWithoutOptions(const HighlightOptions& options) const;
    //! Содержит ошибки или предупреждения, у которых нет указанных свойств
    bool hasWarningsWithoutOptions(const HighlightOptions& options) const;

    //! Содержит элемент
    bool contains(const HighlightItem& item) const;
    //! Содержит элемент
    bool contains(const DataProperty& property) const;
    //! Содержит элемент
    bool contains(const DataProperty& property, int id) const;
    //! Содержит ошибки указанного типа для данного свойства
    bool contains(const DataProperty& property, InformationType type, const HighlightOptions& options = {}) const;

    //! Добавить
    HighlightItem add(
        //! Вид ошибки
        InformationType type,
        //! Свойство, к которому относится ошибка
        const DataProperty& property,
        //! Код ошибки (должен быть уникален в рамках property)
        int id,
        //! Текст ошибки
        const QString& text,
        //! Код группы для логического объединения разных ошибок в группу
        int group_code = -1,
        //! Произвольные данные
        const QVariant& data = QVariant(),
        //! Параметры
        const HighlightOptions& options = {});
    void add(const HighlightItem& item);
    void add(const QList<HighlightItem>& items);
    void add(const QSet<HighlightItem>& items);

    //! Удалить
    void remove(const HighlightItem& item);
    //! Удалить по свойству
    void remove(const DataProperty& property);
    //! Удалить по свойству с конкретным номером HighlightItem
    void remove(const DataProperty& property,
        //! HighlightItem::id()
        int id,
        //! Не удалять данные свойства при анализе дочерних (removeChildProperties)
        const DataPropertySet& ignore_child);
    //! Удалить несколько элементов
    void remove(const QList<HighlightItem>& items);
    void remove(const QSet<HighlightItem>& items);

    //! Обновить элемент
    void change(const HighlightItem& item);

    //! Содержит элементы с указанными кодом группы
    bool hasGroup(int group_code, const HighlightOptions& options = {}) const;

    //! Элемент по порядковому номеру
    HighlightItem item(int pos) const;
    //! Найти элементы для свойства
    QList<HighlightItem> items(const DataProperty& property, bool sort = true, const HighlightOptions& options = {}) const;
    //! Найти все элементы, по типу ошибки
    QList<HighlightItem> items(const InformationTypes& types, bool sort = true, const HighlightOptions& options = {}) const;
    //! Найти все элементы, по типу ошибки
    QList<HighlightItem> items(InformationType type, bool sort = true, const HighlightOptions& options = {}) const;
    //! Найти все элементы, по типу свойства
    QList<HighlightItem> items(PropertyType type, bool sort = true, const HighlightOptions& options = {}) const;
    //! Найти все элементы, связанные с кодом группы
    QList<HighlightItem> groupItems(int group_code, bool sort = true, const HighlightOptions& options = {}) const;

    //! Наиболее важный уровень ошибки для элементов
    static InformationType topInformationType(const QList<HighlightItem>& items);

    //! Все свойства
    DataPropertyList properties(bool sort = true, const HighlightOptions& options = {}) const;
    //! Все свойства с ошибками указанных типов
    DataPropertyList properties(const InformationTypes& types, bool sort = true, const HighlightOptions& options = {}) const;
    //! Все свойства с ошибками указанного типа
    DataPropertyList properties(InformationType type, bool sort = true, const HighlightOptions& options = {}) const;
    //! Все свойства указанного типа с ошибками
    DataPropertyList properties(PropertyType type, bool sort = true, const HighlightOptions& options = {}) const;

    //! Преобразовать в список строк
    QStringList toStringList(InformationTypes types = InformationType::Invalid,
        //! Переводить html в текст
        bool plain = true,
        //! Показывать тип
        bool show_type = true) const;
    //! Преобразовать в список ошибок
    ErrorList toErrorList(InformationTypes types = InformationType::Invalid,
        //! Переводить html в текст
        bool plain = false,
        //! Показывать тип
        bool show_type = true) const;
    //! Преобразовать в список ошибок
    Error toError(InformationTypes types = InformationType::Invalid,
        //! Переводить html в текст
        bool plain = false,
        //! Показывать тип
        bool show_type = true) const;
    //! Вывести все ошибки в виде таблицы HTML
    QString toHtmlTable(InformationTypes types = InformationType::Invalid,
        //! Переводить html в текст
        bool plain = false,
        //! Показывать тип
        bool show_type = true) const;

    //! Начать групповое изменение. При этом не будут генерироваться сигналы sg_itemInserted, sg_itemRemoved,
    //! sg_itemChanged
    void beginUpdate();
    //! Закончить групповое изменение
    void endUpdate();
    bool isUpdating() const;

signals:
    //! Добавлен элемент
    void sg_itemInserted(const zf::HighlightItem& item);
    //! Удален элемент
    void sg_itemRemoved(const zf::HighlightItem& item);
    //! Изменен элемент
    void sg_itemChanged(const zf::HighlightItem& item);
    //! Начато групповое изменение
    void sg_beginUpdate();
    //! Закончено групповое изменение
    void sg_endUpdate();

private:
    //! Сортировка списка элементов
    static void sortHelper(QList<HighlightItem>& items);
    //! Сгруппировать по свойству
    static QMap<DataProperty, QList<HighlightItem>> groupByProperty(const QList<HighlightItem>& items);

    //! Расширить свойство его дочерними элементами
    void expandPropertyChild(const DataProperty& p, QList<HighlightItem>& expanded);
    //! Удалить дочерние элементы свойства
    void removeChildProperties(const DataProperty& property,
        //! Не удалять данные свойства при анализе дочерних
        const DataPropertySet& ignore_child);

    void removeHelper(const HighlightItem& item, bool emit_signals);
    void addHelper(const HighlightItem& item, bool emit_signals);

    //! Поиск списка элементов по набору свойств
    const QList<HighlightItem>& itemsByOptions(const HighlightOptions& options) const;

    //! Ключ - HighlightItem::uniqueKey
    QHash<QString, HighlightItem> _items;
    //! Ключ - свойство
    QMultiHash<DataProperty, HighlightItem> _item_properties;
    //! Ключ - HighlightItem::groupCode
    QMultiHash<int, HighlightItem> _item_groups;
    //! Ключ - InformationType
    QMultiHash<InformationType, HighlightItem> _item_types;
    //! Ключ - PropertyType
    QMultiHash<PropertyType, HighlightItem> _item_property_types;

    //! Отсортированные
    mutable QList<HighlightItem> _ordered;
    //! Ключ - HighlightOptions
    mutable QHash<HighlightOptions, QList<HighlightItem>> _items_options_hash;

    //! Счетчик блокировки генерации сигналов
    int _update_counter = 0;
    //! Защита от криворукости
    bool _child_removing = false;
};

//! Служебный класс для управления ошибками из zf::model
class ZCORESHARED_EXPORT HighlightInfo
{
    Q_DISABLE_COPY(HighlightInfo)
    HighlightInfo(const DataStructure* structure);

public:
    ~HighlightInfo();

    //! Была ли добавлена ошибка
    bool contains(const DataProperty& property) const;
    bool contains(const PropertyID& property_id) const;

    //! Имеется ли такой уровень ошибки или выше
    bool hasLevel(const DataProperty& property, InformationType type) const;
    //! Имеется ли такой уровень ошибки или выше
    bool hasLevel(const PropertyID& property_id, InformationType type) const;

    //! Была ли добавлена ошибка и ее максимальный уровень
    zf::InformationType getHighlight(const DataProperty& property) const;
    //! Была ли добавлена ошибка и ее максимальный уровень
    zf::InformationType getHighlight(const PropertyID& property_id) const;

    //! Добавить информацию об ошибке
    void insert(const DataProperty& property,
        //! Код ошибки (>= zf::Consts::MINIMUM_HIGHLIGHT_ID, должен быть уникален в рамках property)
        int id,
        //! Текст ошибки
        const QString& text,
        //! Вид ошибки
        InformationType type = InformationType::Error,
        //! Код группы для логического объединения разных ошибок в группу
        int group_code = -1,
        //! Произвольные данные
        const QVariant& data = QVariant(),
        //! Переводить текст ошибки
        bool is_translate = true,
        //! Параметры
        const HighlightOptions& options = {});

    //! Добавить информацию об ошибке
    void insert(const PropertyID& property_id,
        //! Код ошибки (>= 0, должен быть уникален в рамках property)
        int id,
        //! Текст ошибки
        const QString& text,
        //! Вид ошибки
        InformationType type = InformationType::Error,
        //! Код группы для логического объединения разных ошибок в группу
        int group_code = -1,
        //! Произвольные данные
        const QVariant& data = QVariant(),
        //! Переводить текст ошибки
        bool is_translate = true,
        //! Параметры
        const HighlightOptions& options = {});

    //! Добавить информацию об ошибке в ячейке набора данных
    void insert(
        //! Строка или ячейка набора данных
        const DataProperty& property,
        //! Колонка
        const PropertyID& column_property_id,
        //! Код ошибки (>= zf::Consts::MINIMUM_HIGHLIGHT_ID, должен быть уникален в рамках property)
        int id,
        //! Текст ошибки
        const QString& text,
        //! Вид ошибки
        InformationType type = InformationType::Error,
        //! Код группы для логического объединения разных ошибок в группу
        int group_code = -1,
        //! Произвольные данные
        const QVariant& data = QVariant(),
        //! Переводить текст ошибки
        bool is_translate = true,
        //! Параметры
        const HighlightOptions& options = {});

    /*! Отметить отсутствие ошибки. Если этого не сделать, то ядро не поймет, что статус ошибки надо снять, т.к. перед
     * началом опроса модуля на наличе ошибок, текущие ошибки не сбрасываются. */
    void empty(const DataProperty& property,
        //! Код ошибки (>= zf::Consts::MINIMUM_HIGHLIGHT_ID, должен быть уникален в рамках property)
        int id);
    /*! Отметить отсутствие ошибки. Если этого не сделать, то ядро не поймет, что статус ошибки надо снять, т.к. перед
     * началом опроса модуля на наличе ошибок, текущие ошибки не сбрасываются. */
    void empty(const PropertyID& property_id,
        //! Код ошибки (>= zf::Consts::MINIMUM_HIGHLIGHT_ID, должен быть уникален в рамках property)
        int id);
    /*! Отметить отсутствие ошибки. Если этого не сделать, то ядро не поймет, что статус ошибки надо снять, т.к. перед
     * началом опроса модуля на наличе ошибок, текущие ошибки не сбрасываются. */
    void empty(
        //! Строка или ячейка набора данных
        const DataProperty& property,
        //! Колонка
        const PropertyID& column_property_id,
        //! Код ошибки (>= zf::Consts::MINIMUM_HIGHLIGHT_ID, должен быть уникален в рамках property)
        int id);

    //! Метод, комбинирующий методы insert и empty. Если is_insert true, то выполняется insert, иначе empty
    void set(
        //! Свойство данных
        const DataProperty& property,
        //! Код ошибки (>= zf::Consts::MINIMUM_HIGHLIGHT_ID, должен быть уникален в рамках property)
        int id,
        //! Добавлять ли ошибку
        bool is_insert,
        //! Текст ошибки
        const QString& text,
        //! Вид ошибки
        InformationType type = InformationType::Error,
        //! Код группы для логического объединения разных ошибок в группу
        int group_code = -1,
        //! Произвольные данные
        const QVariant& data = QVariant(),
        //! Переводить текст ошибки
        bool is_translate = true,
        //! Параметры
        const HighlightOptions& options = {});

    //! Метод, комбинирующий методы insert и empty. Если is_insert true, то выполняется insert, иначе empty
    void set(
        //! Свойство данных
        const PropertyID& property_id,
        //! Код ошибки (>= zf::Consts::MINIMUM_HIGHLIGHT_ID, должен быть уникален в рамках property)
        int id,
        //! Добавлять ли ошибку
        bool is_insert,
        //! Текст ошибки
        const QString& text,
        //! Вид ошибки
        InformationType type = InformationType::Error,
        //! Код группы для логического объединения разных ошибок в группу
        int group_code = -1,
        //! Произвольные данные
        const QVariant& data = QVariant(),
        //! Переводить текст ошибки
        bool is_translate = true,
        //! Параметры
        const HighlightOptions& options = {});

    //! Метод, комбинирующий методы insert и empty. Если is_insert true, то выполняется insert, иначе empty
    void set(
        //! Строка или ячейка набора данных
        const DataProperty& property,
        //! Колонка
        const PropertyID& column_property_id,
        //! Код ошибки (>= zf::Consts::MINIMUM_HIGHLIGHT_ID, должен быть уникален в рамках property)
        int id,
        //! Добавлять ли ошибку
        bool is_insert,
        //! Текст ошибки
        const QString& text,
        //! Вид ошибки
        InformationType type = InformationType::Error,
        //! Код группы для логического объединения разных ошибок в группу
        int group_code = -1,
        //! Произвольные данные
        const QVariant& data = QVariant(),
        //! Переводить текст ошибки
        bool is_translate = true,
        //! Параметры
        const HighlightOptions& options = {});

    //! Выполнить проверку свойства на возможность добавления
    void checkProperty(const DataProperty& p);

private:
    //! Добавить информацию об ошибке
    void insertHelper(const DataProperty& p,
        //! Код ошибки (должен быть уникален в рамках property)
        int id,
        //! Текст ошибки
        const QString& text,
        //! Вид ошибки
        InformationType type,
        //! Код группы для логического объединения разных ошибок в группу
        int group_code,
        //! Произвольные данные
        const QVariant& data,
        //! Переводить текст ошибки
        bool is_translate,
        //! Параметры
        const HighlightOptions& options);
    //! Задать текущее свойство для проверки
    void setCurrentProperty(const DataProperty& p = DataProperty());

    //! Заблокировать проверку на корректный id
    void blockCheckId();
    //! Разблокировать проверку на корректный id
    void unBlockCheckId();

    //! Заблокировать проверку на корректность добавляемого свойства
    void blockCheckCurrent();
    //! Разблокировать проверку на корректный добавляемого свойства
    void unBlockCheckCurrent();

    typedef std::shared_ptr<QHash<int, HighlightItemPtr>> HighlightItemHashPtr;

    HighlightItemHashPtr dataHelper(const DataProperty& p) const;
    const QHash<DataProperty, HighlightItemHashPtr>& data() const { return _data; }

    //! Структура данных
    const DataStructure* _structure;
    //! Информация об ошибках
    mutable QHash<DataProperty, HighlightItemHashPtr> _data;
    //! Счетчик блокировки проверки на корректный id
    int _block_check_id = 0;
    //! Текущее свойство для проверки
    DataProperty _current_property;
    int _block_check_property = 0;

    friend class ModuleDataObject;
    friend class HighlightProcessor;
    friend class View;
};

inline uint qHash(const HighlightItem& item)
{
    return item.hashKey();
}

} // namespace zf

Q_DECLARE_METATYPE(zf::HighlightItem)

