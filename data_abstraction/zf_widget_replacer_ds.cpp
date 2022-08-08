#include "zf_widget_replacer_ds.h"
#include "zf_core.h"
#include "zf_data_widgets.h"
#include <QCheckBox>
#include <QComboBox>
#include <QLabel>
#include <QLineEdit>
#include <QWidget>
#include <QApplication>
#include <QAbstractItemView>

namespace zf
{
//! Профикс виджетов DataStructure
const QString WidgetReplacerDataStructure::PREFIX_DATA_PROPERTY = QStringLiteral("_p_");
//! Профикс виджета поиска сущности
const QString WidgetReplacerDataStructure::PREFIX_ENTITY_SEARCH_WIDGET = QStringLiteral("_sw_");

//! Свойства виджетов для копирования
const QHash<QString, QSet<QString>> WidgetReplacerDataStructure::_properties = {
    {QString(),
     {
         //         "visible",
         "enabled",
         "readOnly",
         "geometry",
         "sizePolicy",
         "minimumSize",
         "maximumSize",
         "palette",
         "font",
         "toolTip",
         "statusTip",
         "autoFillBackground",
         "calendarPopup",
         "frameShape",
         "frameShadow",
     }},

    {"QLabel",
     {
         "margin",
         "indent",
         "openExternalLinks",
         "textInteractionFlags",
         "textInteractionFlags",
         "alignment",
         "scaledContents",
         "wordWrap",
     }},

    {"QCheckBox",
     {
         "text",
     }},

    {"QLineEdit",
     {
         "placeholderText",
     }},

    {"QPlainTextEdit",
     {
         "placeholderText",
         "_max_length",
         "maximumLines",
     }},

    {"QTextEdit",
     {
         "placeholderText",
         "_max_length",
     }},

    {"QComboBox",
     {
         "placeholderText",
         "maxVisibleItems",
         "insertPolicy",
         "sizeAdjustPolicy",
         "minimumContentsLength",
         "iconSize",
     }},

    {"QAbstractItemView",
     {
         "dragDropOverwriteMode",
         "defaultDropAction"
         "dragDropMode"
         "dragEnabled",
         // "selectionBehavior",
         // "selectionMode",
         "alternatingRowColors",
         // "editTriggers",
     }},

    {"QTableView",
     {
         "cornerButtonEnabled",
         // "sortingEnabled",
         //            "gridStyle",
         "wordWrap",
         //            "showGrid",
     }},

    {"QTreeView",
     {
         //            "allColumnsShowFocus",
         //            "animated",
         //            "autoExpandDelay",
         //            "expandsOnDoubleClick",
         "headerHidden",
         "indentation",
         //            "itemsExpandable",
         //            "rootIsDecorated",
         // "sortingEnabled",
         "uniformRowHeights",
         "wordWrap",
     }},

    {"QHeaderView",
     {
         "cascadingSectionResizes", "defaultAlignment",
         //            "defaultSectionSize",
         //            "firstSectionMovable",
         //            "highlightSections",
         //         "showSortIndicator",
         "stretchLastSection",
         //         "maximumSectionSize",
         //         "minimumSectionSize",
     }},
};

WidgetReplacerDataStructure::WidgetReplacerDataStructure(const DataWidgets* data_widgets, QWidget* entity_search_widget)
    : _data_widgets(data_widgets)
    , _entity_search_widget(entity_search_widget)
{
    Z_CHECK_NULL(_data_widgets);
}

void WidgetReplacerDataStructure::setEntitySearchWidget(QWidget* entity_search_widget)
{
    _entity_search_widget = entity_search_widget;
}

bool WidgetReplacerDataStructure::isNeedReplace(const QWidget* widget, QString& tag, QString& new_name)
{
    static const QStringList prefixes = {PREFIX_DATA_PROPERTY, PREFIX_ENTITY_SEARCH_WIDGET};

    QString full_name = widget->objectName();

    for (auto& p : qAsConst(prefixes)) {
        Z_CHECK(!p.isEmpty());
        if (!full_name.startsWith(p, Qt::CaseInsensitive))
            continue;
        tag = p.toLower();
        new_name = full_name.right(full_name.length() - tag.length());
        return true;
    }
    tag.clear();
    return false;
}

void WidgetReplacerDataStructure::createReplacement(const QString& tag, const QString& new_name, const QWidget* widget, bool& is_hide,
                                                    QWidget*& replacement, QWidget*& replacement_container,
                                                    QHash<QString, QSet<QString>>*& properties, DataProperty& data_property)
{
    Q_UNUSED(widget)

    replacement = nullptr;
    replacement_container = nullptr;
    data_property.clear();

    if (tag == PREFIX_DATA_PROPERTY) {
        // виджет, связанный с данными
        is_hide = false;
        bool ok;
        int id = static_cast<int>(new_name.toUInt(&ok));
        if (!ok || id < Consts::MINUMUM_PROPERTY_ID || !_data_widgets->data()->structure()->contains(PropertyID(id)))
            return;

        properties = new QHash<QString, QSet<QString>>(_properties);
        data_property = _data_widgets->data()->structure()->property(PropertyID(id));
        replacement = _data_widgets->field(data_property);
        replacement_container = _data_widgets->complexField(data_property);

    } else if (tag == PREFIX_ENTITY_SEARCH_WIDGET) {
        // виджет поиска сущностей
        if (_entity_search_widget == nullptr) {
            // виджет поиска есть на форме но не определена его замена, значит скрываем
            is_hide = true;
            return;
        }

        is_hide = false;
        properties = new QHash<QString, QSet<QString>>(_properties);
        replacement = _entity_search_widget;
        replacement_container = _entity_search_widget;

    } else
        Z_HALT_INT;
}

void WidgetReplacerDataStructure::initStyle(QWidget* widget, DataProperty& data_property)
{
    if (data_property.propertyType() == PropertyType::Dataset) {
        if (data_property.options().testFlag(PropertyOption::SimpleDataset))
            return;

        QAbstractItemView* view = qobject_cast<QAbstractItemView*>(widget);
        Z_CHECK_NULL(view);

        QTreeView* tree_view = qobject_cast<QTreeView*>(widget);
        QTableView* table_view = qobject_cast<QTableView*>(widget);
        bool has_left_border = false;
        if (tree_view != nullptr)
            has_left_border = tree_view->header()->stretchLastSection();
        if (table_view != nullptr)
            has_left_border = false;

        DataWidgets::setDatasetToolbarWidgetStyle(has_left_border, view->frameShape() != QFrame::NoFrame,
                                                  view->frameShape() != QFrame::NoFrame, view->frameShape() != QFrame::NoFrame,
                                                  _data_widgets->datasetToolbarWidget(data_property));
    }
}

} // namespace zf
