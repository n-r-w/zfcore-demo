#include "zf_widget_replacer.h"
#include "zf_core.h"
#include "zf_utils.h"

#include <QApplication>
#include <QBoxLayout>
#include <QDebug>
#include <QDynamicPropertyChangeEvent>
#include <QFormLayout>
#include <QHeaderView>
#include <QMetaProperty>
#include <QStackedLayout>
#include <QTableView>
#include <QTreeView>
#include <QWidget>
#include <QSplitter>
#include <QLineEdit>

namespace zf
{
WidgetReplacer::WidgetReplacer()
{
}

WidgetReplacer::~WidgetReplacer()
{
}

QList<QWidget*> WidgetReplacer::replace(QWidget* widget)
{
    Z_CHECK_NULL(widget);

    QList<QWidget*> result;

    QList<QWidget*> to_remove;
    QList<QWidget*> widgets = widget->findChildren<QWidget*>();

    for (auto base_widget : widgets) {
        QString full_name = base_widget->objectName();
        if (full_name.isEmpty())
            continue;

        QString tag;
        QString new_name;
        if (!isNeedReplace(base_widget, tag, new_name))
            continue;

        DataProperty data_property;
        QHash<QString, QSet<QString>>* properties = nullptr;
        QWidget* new_widget = nullptr;
        QWidget* new_widget_container = nullptr;
        bool is_hide = false;
        createReplacement(tag, new_name, base_widget, is_hide, new_widget, new_widget_container, properties, data_property);
        QScopedPointer<QHash<QString, QSet<QString>>> properties_ptr;
        if (properties != nullptr)
            properties_ptr.reset(properties);

        if (new_widget == nullptr) {
            if (is_hide)
                base_widget->setHidden(true);
            else
                Core::logError(QString("invalid widget name started from reserved prefix %1: %2").arg(tag, full_name));

            continue;
        }

        result << new_widget;

        if (new_widget_container == nullptr)
            new_widget_container = new_widget;

        bool to_process = false;
        QLayout* parent_la = nullptr;
        QSplitter* parent_splitter = nullptr;

        parent_splitter = qobject_cast<QSplitter*>(base_widget->parent());
        if (parent_splitter) {
            // сплиттер содержит подчиненные виджеты без QLayout
            to_process = true;

        } else {
            // Ищем родительский layout
            parent_la = Utils::findParentLayout(base_widget);
            if (parent_la != nullptr)
                to_process = true;
        }

        if (!to_process)
            continue;

        if (new_widget->objectName().isEmpty())
            new_widget->setObjectName(new_name);

        // копируем свойства
        copyProperties(base_widget, new_widget, properties, data_property);

        // инициализируем стили
        initStyle(new_widget, data_property);

        // заменяем
        bool processed = false;

        if (parent_splitter != nullptr) {
            int index = parent_splitter->indexOf(base_widget);
            parent_splitter->insertWidget(index, new_widget_container);
            processed = true;
        }

        if (!processed) {
            QBoxLayout* box_la = qobject_cast<QBoxLayout*>(parent_la);
            if (box_la != nullptr) {
                int index = parent_la->indexOf(base_widget);
                int stretch = box_la->stretch(index);

                QLayoutItem* la_item = parent_la->takeAt(index);
                delete la_item;

                box_la->insertWidget(index, new_widget_container, stretch);
                processed = true;
            }
        }

        if (!processed) {
            QFormLayout* form_la = qobject_cast<QFormLayout*>(parent_la);
            if (form_la != nullptr) {
                int row;
                QFormLayout::ItemRole role;
                form_la->getWidgetPosition(base_widget, &row, &role);
                Z_CHECK(row >= 0);
                QFormLayout::TakeRowResult take = form_la->takeRow(row);
                form_la->insertRow(
                    row, (take.labelItem != nullptr) ? take.labelItem->widget() : nullptr, new_widget_container);
                if (take.labelItem != nullptr)
                    delete take.labelItem;
                delete take.fieldItem;

                processed = true;
            }
        }

        if (!processed) {
            QGridLayout* grid_la = qobject_cast<QGridLayout*>(parent_la);
            if (grid_la != nullptr) {
                int row = -1;
                int col = -1;
                int col_span = 1;
                int row_span = 1;
                for (row = 0; row < grid_la->rowCount(); ++row) {
                    for (col = 0; col < grid_la->columnCount(); ++col) {
                        auto grid_field_widget = grid_la->itemAtPosition(row, col);
                        if (grid_field_widget && grid_field_widget->widget() == base_widget) {
                            int index = grid_la->indexOf(grid_field_widget);
                            grid_la->getItemPosition(index, &row, &col, &row_span, &col_span);
                            processed = true;
                            break;
                        }
                    }
                    if (processed)
                        break;
                }
                Z_CHECK(row >= 0);

                grid_la->removeWidget(base_widget);
                grid_la->addWidget(new_widget_container, row, col, row_span, col_span);
            }
        }

        if (!processed) {
            QStackedLayout* stack_la = qobject_cast<QStackedLayout*>(parent_la);
            if (stack_la != nullptr) {
                int index = -1;
                for (index = 0; index < stack_la->count(); index++) {
                    if (stack_la->widget(index) == base_widget)
                        break;
                }

                stack_la->removeWidget(base_widget);
                stack_la->insertWidget(index, new_widget_container);

                processed = true;
            }
        }

        if (processed)
            to_remove << base_widget;
    }

    for (auto i : qAsConst(to_remove)) {
        auto o_ex = dynamic_cast<I_ObjectExtension*>(i);
        if (o_ex)
            Utils::deleteLater(i);
        else
            delete i;
    }

    return result;
}

void WidgetReplacer::initStyle(QWidget* widget, DataProperty& data_property)
{
    Q_UNUSED(widget)
    Q_UNUSED(data_property)
}

void WidgetReplacer::copyProperties(QWidget* from, QWidget* to, const QHash<QString, QSet<QString>>* properties,
                                    const DataProperty& data_property)
{
    QSet<QString> properties_set;

    if (properties != nullptr) {
        for (auto i = properties->constBegin(); i != properties->constEnd(); ++i) {
            if (i.key().isEmpty() || from->inherits(i.key().toLatin1().constData()))
                properties_set += i.value();
        }

    } else {
        // добавляем все
        for (int i = 0; i < from->metaObject()->propertyCount(); i++) {
            properties_set << from->metaObject()->property(i).name();
        }
    }

    for (int i = 0; i < from->metaObject()->propertyCount(); i++) {
        QMetaProperty property = from->metaObject()->property(i);
        const char* name = property.name();
        if (!property.isWritable() || !property.isReadable() || !property.isStored(from)
            || !properties_set.contains(name))
            continue;

        QString name_str = QString(name);
        if (name_str == QStringLiteral("visible") && from->isHidden()) {
            to->setHidden(from->isHidden());
            continue;

        } else {
            int p_index = to->metaObject()->indexOfProperty(name);
            if (p_index >= 0) {
                QMetaProperty to_property = to->metaObject()->property(p_index);
                if (!to_property.isWritable())
                    continue;

                if (data_property.isValid()) {
                    // если данные в режиме только для чтения, то свойства readonly и enabled игнорируем и выставляем
                    // вручную

                    if ((name_str == QStringLiteral("readOnly") || name_str == QStringLiteral("enabled"))
                        && (data_property.options() & PropertyOption::ReadOnly) > 0)
                        continue;
                }

                if (to_property.isReadable() && property.read(from) != to_property.read(to)) {
                    if (to->setProperty(name, from->property(name))) {
                        // setProperty работает очень странно. он шлет QDynamicPropertyChangeEvent, только если такого
                        // свойства у объекта нет вообще (метод возвращает false), поэтому шлем его сами
                        QDynamicPropertyChangeEvent event(name);
                        QApplication::sendEvent(to, &event);
                    }
                }
            }
        }
    }

    if (data_property.isValid() && (data_property.options() & PropertyOption::ReadOnly) > 0) {
        // свойства readonly и enabled выставляем вручную
        Utils::setWidgetReadOnly(to, true);
    }

    auto table_view_from = qobject_cast<QTableView*>(from);
    if (table_view_from != nullptr) {
        auto table_view_to = qobject_cast<QTableView*>(to);
        if (table_view_to != nullptr) {
            copyProperties(
                table_view_from->horizontalHeader(), table_view_to->horizontalHeader(), properties, DataProperty());
            copyProperties(
                table_view_from->verticalHeader(), table_view_to->verticalHeader(), properties, DataProperty());

        } else {
            auto tree_view_to = qobject_cast<QTreeView*>(to);
            if (tree_view_to != nullptr) {
                copyProperties(table_view_from->verticalHeader(), tree_view_to->header(), properties, DataProperty());
            }
        }

    } else {
        auto tree_view_from = qobject_cast<QTreeView*>(from);
        if (tree_view_from != nullptr) {
            auto table_view_to = qobject_cast<QTableView*>(to);
            if (table_view_to != nullptr) {
                copyProperties(tree_view_from->header(), table_view_to->horizontalHeader(), properties, DataProperty());

            } else {
                auto tree_view_to = qobject_cast<QTreeView*>(to);
                if (tree_view_to != nullptr) {
                    copyProperties(tree_view_from->header(), tree_view_to->header(), properties, DataProperty());
                }
            }
        }
    }
}

} // namespace zf
