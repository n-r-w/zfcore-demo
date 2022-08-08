#pragma once

#include "zf_data_widgets.h"

class QGridLayout;
class QGroupBox;

namespace zf
{
//! Схема расположения виджетов из DataWidgets
class ZCORESHARED_EXPORT WidgetScheme
{
public:
    WidgetScheme(DataWidgets* widgets);

    //! Добавить секцию
    void addSection(int section_id, const QString& translation_id = {}, const QIcon& icon = {});
    //! Добавить группу
    void addGroup(int section_id, int group_id, const QString& translation_id = {}, bool is_default = false);
    //! Указать в какой секции и группе будет свойство. Не добавленные свойства будут помещаться в конец секции/группы "по умолчанию"
    void addProperty(int section_id, int group_id, const PropertyID& property_id);

    //! Добавить неиспользуемые свойства в группу по умолчанию (если не добавлено ни одного свойства, то параметр
    //! игнорируется и добавляются все свойства)
    void setAddUnused(bool add_unused);
    //! Добавить неиспользуемые свойства в группу по умолчанию (если не добавлено ни одного свойства, то параметр
    //! игнорируется и добавляются все свойства). По умолчанию false
    bool isAddUnused() const;

    //! Создать составной виджет. nullptr при ошибке
    QWidget* generate(
        //! Уникальный ID в рамках окна для установки имени виджетов
        const QString& id,
        //! Имеет ли полученный виджет видимые рамки
        bool& has_borders,
        //! Показывать ошибки переменных перевода
        bool show_no_translate = true,
        //! Только для чтения
        bool read_only = false,
        //! Показывать скрытые поля и идентификаторы полей
        bool debug_mode = false) const;

private:
    struct Section;
    struct Group;
    typedef std::shared_ptr<Section> SectionPtr;
    typedef std::shared_ptr<Group> GroupPtr;

    //! Создать секцию (набор групп)
    QWidget* generateSection(
        //! Уникальный ID в рамках окна для установки имени виджетов
        const QString& id, const SectionPtr& section, bool show_no_translate, bool read_only, bool debug_mode) const;
    //! Создать группу
    QGridLayout* generateGroup(
        //! Уникальный ID в рамках окна для установки имени виджетов
        const QString& id, const PropertyIDList& properties, bool show_no_translate, bool& has_expanded_widget, bool read_only,
        bool debug_mode) const;
    //! Получить редактор
    QWidget* prepareEditor(const PropertyID& property_id, bool single, bool read_only, bool debug_mode) const;
    //! Создать виджет в случе если в секции он всего один. nullptr, если для данного свойства надо всегда создавать
    //! QGridLayout
    QWidget* generateSingleWidget(
        //! Уникальный ID в рамках окна для установки имени виджетов
        const QString& id, const PropertyID& property_id, bool read_only, bool debug_mode) const;

    //! Максимальная ширина колонки с названиями
    static int maxNameColumnWidth(QGridLayout* la);

    SectionPtr getSection(int section_id) const;
    GroupPtr getGroup(int section_id, int group_id) const;

    struct Section
    {
        int id = -1;
        QString translation_id;
        QIcon icon;
        QList<GroupPtr> groups;
        QHash<int, GroupPtr> groups_by_id;
    };

    struct Group
    {
        int id = -1;
        QString translation_id;
        std::weak_ptr<Section> section;
        PropertyIDList properties;
    };

    //! Все секции
    QList<SectionPtr> _sections;
    //! Секции по коду
    QHash<int, SectionPtr> _sections_by_id;
    //! Секция по умолчанию
    int _default_section_id = -1;
    //! Группа по умолчанию
    int _default_group_id = -1;
    //! Все свойства
    QSet<PropertyID> _all_properties;
    //! Сгенерированные виджеты
    DataWidgets* _widgets;
    //! Добавить неиспользуемые свойства в группу по умолчанию (если не добавлено ни одного свойства, то параметр
    //! игнорируется и добавляются все свойства)
    bool _add_unused = false;
};

} // namespace zf

