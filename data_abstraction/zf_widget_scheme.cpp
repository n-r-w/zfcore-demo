#include "zf_widget_scheme.h"
#include "zf_translation.h"
#include "zf_plain_text_edit.h"
#include "zf_table_view.h"
#include "zf_tree_view.h"
#include "zf_colors_def.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QTabWidget>
#include <QGroupBox>
#include <QScrollArea>

//! Длина текстового названия в QFormLayout
#define Z_FORM_LABEL_WIDTH 30

namespace zf
{
WidgetScheme::WidgetScheme(DataWidgets* widgets)
    : _widgets(widgets)
{
    Z_CHECK_NULL(widgets);
}

void WidgetScheme::addSection(int section_id, const QString& translation_id, const QIcon& icon)
{
    Z_CHECK(section_id >= 0);
    Z_CHECK(getSection(section_id) == nullptr);

    auto section = Z_MAKE_SHARED(Section);
    section->id = section_id;
    section->translation_id = translation_id;
    section->icon = icon;

    _sections << section;
    _sections_by_id[section_id] = section;
}

void WidgetScheme::addGroup(int section_id, int group_id, const QString& translation_id, bool is_default)
{
    Z_CHECK(section_id >= 0);
    Z_CHECK(group_id >= 0);
    auto section = getSection(section_id);
    Z_CHECK_NULL(section);
    Z_CHECK(getGroup(section_id, group_id) == nullptr);

    auto group = Z_MAKE_SHARED(Group);
    group->id = group_id;
    group->section = section;
    group->translation_id = translation_id;

    section->groups << group;
    section->groups_by_id[group_id] = group;

    if (is_default || _default_group_id < 0) {
        _default_section_id = section_id;
        _default_group_id = group_id;
    }
}

void WidgetScheme::addProperty(int section_id, int group_id, const PropertyID& property_id)
{
    Z_CHECK_X(_widgets->structure()->contains(property_id),
              QStringLiteral("Property not found %1:%2").arg(_widgets->structure()->entityCode().value()).arg(property_id.value()));
    auto group = getGroup(section_id, group_id);
    if (group == nullptr)
        Z_HALT(QStringLiteral("Group not found %1:%2:%3").arg(_widgets->structure()->entityCode().value()).arg(section_id).arg(group_id));

    Z_CHECK(!group->properties.contains(property_id));
    group->properties << property_id;

    Z_CHECK(!_all_properties.contains(property_id));
    _all_properties << property_id;
}

void WidgetScheme::setAddUnused(bool add_unused)
{
    _add_unused = add_unused;
}

bool WidgetScheme::isAddUnused() const
{
    return _add_unused;
}

QWidget* WidgetScheme::generate(const QString& id, bool& has_borders, bool show_no_translate, bool read_only, bool debug_mode) const
{
    has_borders = false;

    if (_sections.isEmpty()) {
        // создаем один QFormLayout со всеми доступными свойствами
        PropertyIDList properties;
        for (auto& p : _widgets->structure()->propertiesMainSorted()) {
            properties << p.id();
        }

        if (properties.count() == 1) {
            QWidget* w = generateSingleWidget(id, properties.first(), read_only, debug_mode);
            if (w != nullptr)
                return w;
        }

        bool has_expanded_widget = false;
        QGridLayout* group_la = generateGroup(id, properties, show_no_translate, has_expanded_widget, read_only, debug_mode);

        // определяем максимальную ширину колонки с названиями
        int max_label_width = maxNameColumnWidth(group_la);
        if (max_label_width > 0)
            group_la->setColumnMinimumWidth(0, max_label_width);

        QScrollArea* scroll = new QScrollArea;
        scroll->setObjectName(QStringLiteral("zfs%1").arg(id));
        QWidget* scroll_widget = new QWidget;
        scroll_widget->setObjectName(QStringLiteral("zfsw"));
        scroll_widget->setLayout(group_la);
        scroll_widget->layout()->setMargin(9);
        scroll->setWidget(scroll_widget);
        scroll->setWidgetResizable(true);
        scroll->setFrameShape(QFrame::NoFrame);
        scroll_widget->setLayout(group_la);

        if (!has_expanded_widget)
            scroll_widget->layout()->addItem(new QSpacerItem(0, 0, QSizePolicy::Preferred, QSizePolicy::Expanding));

        return scroll;
    }

    if (_sections.count() == 1) {
        // таб не нужен
        return generateSection(id, _sections.first(), show_no_translate, read_only, debug_mode);
    }

    has_borders = true;
    QTabWidget* tab_widget = new QTabWidget;
    tab_widget->setObjectName(QStringLiteral("zftw%1").arg(id));
    tab_widget->setAutoFillBackground(true);
    //    tab_widget->setDocumentMode(true);

    for (auto& section : _sections) {
        QWidget* section_widget = generateSection(id, section, show_no_translate, read_only, debug_mode);
        section_widget->setAutoFillBackground(true);
        tab_widget->addTab(section_widget, section->icon, Translator::translate(section->translation_id, show_no_translate));
    }

    return tab_widget;
}

QWidget* WidgetScheme::generateSection(const QString& id, const SectionPtr& section, bool show_no_translate, bool read_only,
                                       bool debug_mode) const
{
    Z_CHECK_NULL(section);

    QWidget* result;
    if (section->groups.isEmpty()) {
        result = new QWidget; // нечего делать

    } else {
        QList<QGridLayout*> group_layouts;
        QWidget* section_widget = new QWidget;
        section_widget->setObjectName(QStringLiteral("zfse"));
        QScrollArea* scroll = new QScrollArea;
        scroll->setWidget(section_widget);
        scroll->setWidgetResizable(true);
        scroll->setFrameShape(QFrame::NoFrame);

        int max_label_width = 0; // Максимальная ширина колонки с названиями
        bool has_expanded_widget = false;
        for (auto& group : section->groups) {
            PropertyIDList properties = group->properties;

            if (_add_unused && section->id == _default_section_id && group->id == _default_group_id) {
                // добавляем остальные свойства в группу по умолчанию
                auto all_props = _widgets->structure()->propertiesMainSorted();
                for (auto& p : qAsConst(all_props)) {
                    if (!_all_properties.contains(p.id()))
                        properties << p.id();
                }
            }

            if (properties.count() == 1) {
                QWidget* w = generateSingleWidget(id, properties.first(), read_only, debug_mode);
                if (w != nullptr)
                    return w;
            }

            bool has_expanded;
            QGridLayout* group_la = generateGroup(id, properties, show_no_translate, has_expanded, read_only, debug_mode);
            has_expanded_widget = has_expanded_widget || has_expanded;

            if (section->groups.count() == 1) {
                // если всего одна группа то создавать QGroupBox не надо
                section_widget->setLayout(group_la);
                break;
            }

            // определяем максимальную ширину колонки с названиями
            max_label_width = qMax(max_label_width, maxNameColumnWidth(group_la));

            if (section_widget->layout() == nullptr) {
                section_widget->setLayout(new QVBoxLayout);
                section_widget->layout()->setObjectName(QStringLiteral("zfla"));
            }
            group_layouts << group_la;

            if (/*!group->translation_id.isEmpty()*/ true) {
                QGroupBox* group_box = new QGroupBox(Translator::translate(group->translation_id, show_no_translate));
                group_box->setObjectName(QStringLiteral("zfgb"));
                group_box->setStyleSheet(QStringLiteral("QGroupBox {"
                                                        "border-radius: 0px;"
                                                        "margin-top: 0.5em; "
                                                        "margin-bottom: 0px; "
                                                        "padding-top: 0.1em;"
                                                        "border: 1px solid %1;}"
                                                        "QGroupBox::title {"
                                                        "top: -0.5em;"
                                                        "left: 10px; "
                                                        "subcontrol-origin: border"
                                                        "}")
                                             .arg(Colors::uiLineColor(false).name()));
                group_box->setLayout(group_la);
                section_widget->layout()->addWidget(group_box);

            } /*else {
                // если для группы не задана константа перевода, то не создаем групбокс
                QFrame* group_widget = new QFrame;
                group_widget->setObjectName(QStringLiteral("zfgw"));
                group_widget->setStyleSheet(QStringLiteral("QFrame#zfgw {border: 1px solid %1;}").arg(Colors::uiLineColor(false).name()));
                group_widget->setLayout(group_la);
                section_widget->layout()->addWidget(group_widget);
            }*/
        }

        if (max_label_width > 0) {
            // выравнивание всех QFormLayout в секции
            for (auto la : qAsConst(group_layouts)) {
                la->setColumnMinimumWidth(0, max_label_width);
            }
        }

        if (!has_expanded_widget)
            section_widget->layout()->addItem(new QSpacerItem(0, 0, QSizePolicy::Preferred, QSizePolicy::Expanding));

        section_widget->layout()->setMargin(9);
        result = scroll;
    }
    result->setObjectName(QStringLiteral("zfw%1").arg(id));
    return result;
}

QGridLayout* WidgetScheme::generateGroup(const QString& id, const PropertyIDList& properties, bool show_no_translate,
                                         bool& has_expanded_widget, bool read_only, bool debug_mode) const
{
    Q_UNUSED(id)

    QGridLayout* layout = new QGridLayout;
    layout->setObjectName(QStringLiteral("zfla"));

    int label_max_width = Z_FORM_LABEL_WIDTH * QApplication::fontMetrics().horizontalAdvance('O');
    int row = 0;
    has_expanded_widget = false;
    for (auto& property_id : qAsConst(properties)) {
        DataProperty p = _widgets->structure()->property(property_id);
        if (p.options() & PropertyOption::Hidden && !debug_mode)
            continue;

        QString label_text = Utils::stringToMultiline(Translator::translate(p.translationID(), show_no_translate), label_max_width);
        if (label_text.isEmpty())
            label_text = "ID_" + p.id().string();
        else if (debug_mode)
            label_text = "ID_" + p.id().string() + ": " + label_text;

        QLabel* label = new QLabel(label_text);

        QWidget* editor = prepareEditor(property_id, false, read_only, debug_mode);

        // Каждый эдитор помещаем в горизонтальный лайаут чтобы можно было добавлять в него что-то при необходимости
        QHBoxLayout* editor_la = new QHBoxLayout;
        editor_la->setObjectName(QStringLiteral("zfla"));
        editor_la->setMargin(0);
        editor_la->addWidget(editor);

        if (Utils::isExpanding(editor, Qt::Vertical))
            has_expanded_widget = true;

        Qt::Alignment align_label;
        if (p.propertyType() == PropertyType::Dataset || p.dataType() == DataType::Memo || p.dataType() == DataType::RichMemo)
            align_label = Qt::AlignLeft | Qt::AlignTop;
        else
            align_label = Qt::AlignLeft | Qt::AlignCenter;

        Qt::Alignment align_editor = Qt::AlignLeft | Qt::AlignCenter;

        layout->addWidget(label, row, 0, align_label);
        layout->addLayout(editor_la, row, 1, align_editor);

        row++;
    }

    return layout;
}

QWidget* WidgetScheme::prepareEditor(const PropertyID& property_id, bool single, bool read_only, bool debug_mode) const
{
    Q_UNUSED(debug_mode);

    DataProperty p = _widgets->structure()->property(property_id);
    QWidget* editor = _widgets->complexField(p);

    // Названия некоторых полей надо выравнивать вверх
    Qt::Alignment align = Qt::Alignment();
    if (p.dataType() == DataType::Memo || p.dataType() == DataType::RichMemo || p.propertyType() == PropertyType::Dataset)
        align = Qt::AlignLeft | Qt::AlignTop;

    if (p.dataType() == DataType::Memo || p.dataType() == DataType::RichMemo) {
        editor->setSizePolicy(editor->sizePolicy().horizontalPolicy(), QSizePolicy::Maximum);
    }

    if (p.propertyType() == PropertyType::Dataset && !p.options().testFlag(PropertyOption::SimpleDataset)) {
        if (read_only)
            _widgets->datasetToolbarWidget(property_id)->setHidden(true);

        QAbstractItemView* view = qobject_cast<QAbstractItemView*>(_widgets->field(property_id));
        Z_CHECK_NULL(view);
        if (single)
            view->setFrameShape(QFrame::NoFrame);
        else
            view->setFrameShape(QFrame::StyledPanel);

        view->setMinimumHeight(300);

        // если всего одна видимая колонка, то скрываем
        HeaderItem* root_header = nullptr;
        auto table = qobject_cast<TableView*>(view);
        if (table != nullptr)
            root_header = table->horizontalRootHeaderItem();

        auto tree = qobject_cast<TreeView*>(view);
        if (tree != nullptr)
            root_header = tree->rootHeaderItem();

        if (root_header != nullptr && root_header->bottomPermanentVisibleCount() == 1) {
            if (table != nullptr)
                table->horizontalHeader()->setVisible(false);
            if (tree != nullptr)
                tree->header()->setVisible(false);
        }

        if (debug_mode) {
            if (table != nullptr) {
                table->horizontalRootHeaderItem()->setPermanentHidden(false);
                table->horizontalRootHeaderItem()->setVisible(true);
            }
            if (tree != nullptr) {
                tree->rootHeaderItem()->setPermanentHidden(false);
                tree->rootHeaderItem()->setVisible(true);
            }
        }

    } else if (p.propertyType() == PropertyType::Field || p.options().testFlag(PropertyOption::SimpleDataset)) {
        if (read_only)
            Utils::setWidgetReadOnly(editor, true);
    }

    return editor;
}

QWidget* WidgetScheme::generateSingleWidget(const QString& id, const PropertyID& property_id, bool read_only, bool debug_mode) const
{
    // для полей всегда надо создавать QGridLayout
    if (_widgets->structure()->property(property_id).propertyType() != PropertyType::Dataset)
        return nullptr;

    // всего одно свойство. не надо создавать QGridLayout
    QWidget* editor = prepareEditor(property_id, true, read_only, debug_mode);

    if (Utils::isExpanding(editor, Qt::Vertical)) {
        // можно просто вернуть редактор
        return editor;
    }
    // помещаем редактор в виджет для выравнивания
    QWidget* container = new QWidget;
    container->setObjectName(QStringLiteral("zfc%1").arg(id));
    container->setLayout(new QVBoxLayout);
    container->layout()->setObjectName(QStringLiteral("zfla"));
    container->layout()->addWidget(editor);
    container->layout()->addItem(new QSpacerItem(0, 0, QSizePolicy::Preferred, QSizePolicy::Expanding));

    return container;
}

int WidgetScheme::maxNameColumnWidth(QGridLayout* la)
{
    int n = 0;
    for (int i = 0; i < la->rowCount(); i++) {
        n = qMax(n, Utils::stringWidth(qobject_cast<QLabel*>(la->itemAtPosition(i, 0)->widget())->text()));
    }
    return n;
}

WidgetScheme::SectionPtr WidgetScheme::getSection(int section_id) const
{
    return _sections_by_id.value(section_id);
}

WidgetScheme::GroupPtr WidgetScheme::getGroup(int section_id, int group_id) const
{
    auto section = getSection(section_id);
    if (section == nullptr)
        return nullptr;
    return section->groups_by_id.value(group_id);
}

} // namespace zf
