#include "zf_conditions_dialog.h"
#include "ui_zf_conditions_dialog.h"
#include "zf_tree_view.h"
#include "zf_header_view.h"
#include "zf_core.h"
#include "zf_translation.h"
#include "zf_item_delegate.h"
#include "zf_view.h"
#include "zf_colors_def.h"

#define COL_CONDITION 0
#define COL_FIELD 1
#define COL_OPERATOR 2
#define COL_VALUE 3

#define ID_ROLE (Qt::UserRole + 1)

namespace zf
{
class ConditionItemDelegate : public ItemDelegate
{
public:
    ConditionItemDelegate(ConditionsDialog* dialog, QAbstractItemView* item_view)
        : ItemDelegate(item_view, nullptr, dialog, nullptr, nullptr, DataProperty(), item_view)
        , _dialog(dialog)
    {
    }

    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override
    {
        ItemDelegate::paint(painter, option, index);

        if (currentEditor() != nullptr && index == itemView()->currentIndex())
            return; // чтобы не было отрисовки ошибки вокруг редактора

        auto c = _dialog->condition(index);
        Z_CHECK_NULL(c);

        auto errors = c->errors();
        if (errors.isEmpty())
            return;

        for (auto e : errors) {
            bool paint_error = false;

            if (c->isLogical())
                paint_error = true;
            else if (index.column() == COL_CONDITION && e == Condition::ErrorType::Type)
                paint_error = true;
            else if (index.column() == COL_FIELD && (e == Condition::ErrorType::Source || e == Condition::ErrorType::SourceConversion))
                paint_error = true;
            else if (index.column() == COL_OPERATOR && e == Condition::ErrorType::Operator)
                paint_error = true;
            else if (index.column() == COL_VALUE && (e == Condition::ErrorType::Target || e == Condition::ErrorType::TargetConversion))
                paint_error = true;

            if (paint_error) {
                QStyleOptionViewItem opt = option;
                opt.rect.adjust(1, 1, -1, -1);
                paintErrorBox(painter, opt, index, Qt::red, 1);
                break;
            }
        }
    }

    QWidget* createEditorInternal(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index,
                                  DataProperty& property) const override
    {
        Q_UNUSED(option);
        property.clear();

        auto c = _dialog->condition(index);
        Z_CHECK_NULL(c);

        QWidget* editor = nullptr;
        if (index.column() == COL_CONDITION) {
            auto w = new ComboBox(parent);
            editor = w;

            w->addItem(
                _dialog->compareTypeToString(ConditionsDialog::CompareType::And), static_cast<int>(ConditionsDialog::CompareType::And));
            w->addItem(
                _dialog->compareTypeToString(ConditionsDialog::CompareType::Or), static_cast<int>(ConditionsDialog::CompareType::Or));
            if (!c->isRoot()) {
                w->addItem(_dialog->compareTypeToString(ConditionsDialog::CompareType::Required),
                    static_cast<int>(ConditionsDialog::CompareType::Required));
                w->addItem(_dialog->compareTypeToString(ConditionsDialog::CompareType::CompareValue),
                    static_cast<int>(ConditionsDialog::CompareType::CompareValue));
                w->addItem(_dialog->compareTypeToString(ConditionsDialog::CompareType::CompareColumns),
                    static_cast<int>(ConditionsDialog::CompareType::CompareColumns));
            }

        } else if (index.column() == COL_FIELD) {
            if (c->isLogical())
                return nullptr;

            auto w = new ComboBox(parent);
            editor = w;

            for (auto& p : _dialog->datasetProperty().columns()) {
                if (p.options() & PropertyOption::Filtered)
                    w->addItem(p.name(), p.id().value());
            }

        } else if (index.column() == COL_OPERATOR) {
            if (c->isLogical() || c->type() == ConditionType::Required)
                return nullptr;

            auto w = new ComboBox(parent);
            editor = w;

            w->addItem(Utils::compareOperatorToText(CompareOperator::Equal), static_cast<int>(CompareOperator::Equal));
            w->addItem(Utils::compareOperatorToText(CompareOperator::NotEqual), static_cast<int>(CompareOperator::NotEqual));
            w->addItem(Utils::compareOperatorToText(CompareOperator::More), static_cast<int>(CompareOperator::More));
            w->addItem(Utils::compareOperatorToText(CompareOperator::MoreOrEqual), static_cast<int>(CompareOperator::MoreOrEqual));
            w->addItem(Utils::compareOperatorToText(CompareOperator::Less), static_cast<int>(CompareOperator::Less));
            w->addItem(Utils::compareOperatorToText(CompareOperator::LessOrEqual), static_cast<int>(CompareOperator::LessOrEqual));
            w->addItem(Utils::compareOperatorToText(CompareOperator::Contains), static_cast<int>(CompareOperator::Contains));
            w->addItem(Utils::compareOperatorToText(CompareOperator::StartsWith), static_cast<int>(CompareOperator::StartsWith));
            w->addItem(Utils::compareOperatorToText(CompareOperator::EndsWith), static_cast<int>(CompareOperator::EndsWith));

        } else if (index.column() == COL_VALUE) {
            if (c->isLogical() || c->type() == ConditionType::Required)
                return nullptr;

            if (c->isPropertyCompare()) {
                auto w = new ComboBox(parent);
                editor = w;

                for (auto& p : _dialog->datasetProperty().columns()) {
                    if (p.options() & PropertyOption::Filtered)
                        w->addItem(p.name(), p.id().value());
                }

            } else {
                if (c->source().isValid() && c->compareOperator() != CompareOperator::Undefined) {
                    if (!c->isRawCompare() || c->source().lookup() == nullptr) {
                        if (c->source().dataType() == DataType::Bool) {
                            // обновляем условие, а не данные. Данные сами обновяться на основании условия
                            auto c = _dialog->condition(index);
                            Z_CHECK_NULL(c);
                            _dialog->_conditions->updateCompare(c, c->source(), c->compareOperator(), !index.data(Qt::DisplayRole).toBool(),
                                                                c->sourceConversion());
                            return nullptr;
                        }

                        auto wi
                            = DataWidgets::generateWidget(_dialog->structure(), c->source(), false, false, true, nullptr, nullptr, nullptr);
                        editor = wi->field;

                    } else {
                        editor = new LineEdit(parent);
                    }
                }
            }

        } else
            Z_HALT_INT;

        if (editor != nullptr)
            editor->setObjectName(QStringLiteral("zf_editor"));
        return editor;
    }

    void setEditorDataInternal(QWidget* editor, const QModelIndex& index) const override
    {        
        auto c = _dialog->condition(index);
        Z_CHECK_NULL(c);

        if (index.column() == COL_CONDITION) {
            ComboBox* w = qobject_cast<ComboBox*>(editor);
            Z_CHECK_NULL(w);

            w->setCurrentIndex(w->findData(static_cast<int>(_dialog->compareTypeFromCondition(c))));

        } else if (index.column() == COL_FIELD) {
            Z_CHECK(!c->isLogical());
            ComboBox* w = qobject_cast<ComboBox*>(editor);
            Z_CHECK_NULL(w);

            w->setCurrentIndex(w->findData(c->source().id().value()));

        } else if (index.column() == COL_OPERATOR) {
            Z_CHECK(!c->isLogical());
            ComboBox* w = qobject_cast<ComboBox*>(editor);
            Z_CHECK_NULL(w);

            w->setCurrentIndex(w->findData(static_cast<int>(c->compareOperator())));

        } else if (index.column() == COL_VALUE) {
            Z_CHECK(!c->isLogical());

            if (c->isPropertyCompare()) {
                ComboBox* w = qobject_cast<ComboBox*>(editor);
                Z_CHECK_NULL(w);

                w->setCurrentIndex(w->findData(c->targetProperty().id().value()));

            } else {
                if (!c->isRawCompare() || c->source().lookup() == nullptr) {
                    DataWidgets::updateWidget(editor, c->source(), QMap<PropertyID, QVariant> {{c->source().id(), c->targetValue()}},
                                              DataWidgets::UpdateReason::NoReason);

                } else {
                    LineEdit* le = qobject_cast<LineEdit*>(editor);
                    Z_CHECK_NULL(le);
                    le->setText(Utils::variantToString(c->targetValue()));
                }
            }

        } else
            Z_HALT_INT;
    }

    void setModelDataInternal(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const override
    {
        Q_UNUSED(model)
        // обновляем условие, а не данные. Данные сами обновяться на основании условия

        auto c = _dialog->condition(index);
        Z_CHECK_NULL(c);

        ConditionType type = c->type();
        ConversionType source_conversion = c->sourceConversion();
        DataProperty source = c->source();
        CompareOperator compare_operator = c->compareOperator();
        ConversionType target_conversion = c->targetConversion();
        DataProperty target_property = c->targetProperty();
        QVariant target_value = c->targetValue();
        bool is_property_compare = c->isPropertyCompare();

        if (index.column() == COL_CONDITION) {
            ComboBox* w = qobject_cast<ComboBox*>(editor);
            Z_CHECK_NULL(w);
            Z_CHECK(w->currentIndex() >= 0);

            auto c_type = static_cast<ConditionsDialog::CompareType>(w->currentData().toInt());
            type = _dialog->_compare_mapping.value(c_type);
            if (c_type == ConditionsDialog::CompareType::CompareValue) {
                target_property.clear();
                target_conversion = ConversionType::Undefined;
                is_property_compare = false;

            } else if (c_type == ConditionsDialog::CompareType::CompareColumns) {
                target_value.clear();
                is_property_compare = true;

            } else if (c_type == ConditionsDialog::CompareType::Required) {
                compare_operator = CompareOperator::Undefined;
                target_value.clear();
                target_property.clear();
                target_conversion = ConversionType::Undefined;
                is_property_compare = false;

            } else if (c_type == ConditionsDialog::CompareType::And || c_type == ConditionsDialog::CompareType::Or) {
                source.clear();
                source_conversion = ConversionType::Undefined;
                compare_operator = CompareOperator::Undefined;
                target_value.clear();
                target_property.clear();
                target_conversion = ConversionType::Undefined;
                is_property_compare = false;

            } else
                Z_HALT_INT;

        } else if (index.column() == COL_FIELD) {
            Z_CHECK(!c->isLogical());
            ComboBox* w = qobject_cast<ComboBox*>(editor);
            Z_CHECK_NULL(w);

            DataProperty new_source;
            if (w->currentIndex() >= 0)
                new_source = _dialog->structure()->property(PropertyID(w->currentData().toInt()));
            else
                new_source.clear();

            if (new_source != source)
                target_value.clear();

            source = new_source;

        } else if (index.column() == COL_OPERATOR) {
            Z_CHECK(!c->isLogical());
            ComboBox* w = qobject_cast<ComboBox*>(editor);
            Z_CHECK_NULL(w);

            CompareOperator new_compare_operator;
            if (w->currentIndex() >= 0)
                new_compare_operator = static_cast<CompareOperator>(w->currentData().toInt());
            else
                new_compare_operator = CompareOperator::Undefined;

            if (type == ConditionType::Compare && !is_property_compare && c->source().lookup() != nullptr
                && (compare_operator == CompareOperator::Equal || compare_operator == CompareOperator::NotEqual)
                    != (new_compare_operator == CompareOperator::Equal || new_compare_operator == CompareOperator::NotEqual))
                target_value.clear();

            compare_operator = new_compare_operator;

        } else if (index.column() == COL_VALUE) {
            Z_CHECK(!c->isLogical());

            if (c->isPropertyCompare()) {
                ComboBox* w = qobject_cast<ComboBox*>(editor);
                Z_CHECK_NULL(w);
                if (w->currentIndex() >= 0)
                    target_property = _dialog->structure()->property(PropertyID(w->currentData().toInt()));
                else
                    target_property.clear();

            } else {
                if (!c->isRawCompare() || c->source().lookup() == nullptr) {
                    auto values = DataWidgets::extractWidgetValues(source, editor, view());
                    Z_CHECK(values.contains(source.id()));
                    target_value = values.value(source.id());

                } else {
                    LineEdit* le = qobject_cast<LineEdit*>(editor);
                    Z_CHECK_NULL(le);
                    target_value = le->text();
                }
            }

        } else
            Z_HALT_INT;

        if (type == ConditionType::Or || type == ConditionType::And) {
            _dialog->_conditions->updateLogical(c, type);

        } else if (type == ConditionType::Required) {
            if (source_conversion == ConversionType::Undefined)
                source_conversion = ConversionType::Direct;

            _dialog->_conditions->updateRequired(c, source, source_conversion);

        } else if (type == ConditionType::Compare) {
            if (source_conversion == ConversionType::Undefined)
                source_conversion = ConversionType::Direct;

            if (is_property_compare)
                _dialog->_conditions->updateCompare(c, source, compare_operator, target_property, source_conversion,
                    target_conversion == ConversionType::Undefined ? ConversionType::Direct : target_conversion);
            else
                _dialog->_conditions->updateCompare(c, source, compare_operator, target_value, source_conversion);
        }
    }

    ConditionsDialog* _dialog;
};

ConditionsDialog::ConditionsDialog(const DataStructure* data_structure, const DataProperty& dataset_property, ComplexCondition* conditions)
    : Dialog({QDialogButtonBox::Save, QDialogButtonBox::Cancel, QDialogButtonBox::Reset})
    , ui(new Ui::ConditionsDialogUI)
    , _data_structure(data_structure)
    , _dataset_property(dataset_property)
    , _source_conditions(conditions)
    , _conditions(std::make_unique<ComplexCondition>())
    , _model(std::make_unique<ItemModel>(0, COL_VALUE + 1))
{
    Z_CHECK_NULL(conditions);
    Z_CHECK_NULL(data_structure);
    Z_CHECK_NULL(conditions);
    Z_CHECK(data_structure->contains(dataset_property) && dataset_property.propertyType() == PropertyType::Dataset);

    Core::registerNonParentWidget(this);

    QString dataset_name;
    if (dataset_property.hasName())
        dataset_name = dataset_property.name();
    else if (data_structure->isSingleDataset() && dataset_property.entity().hasName())
        dataset_name = dataset_property.entity().name();
    if (!dataset_name.isEmpty())
        setWindowTitle(ZF_TR(ZFT_FILTER) + ": " + dataset_name);
    else
        setWindowTitle(ZF_TR(ZFT_FILTER));

    setWindowIcon(QIcon(":/share_icons/filter.svg"));

    disableDefaultAction();

    ui->setupUi(workspace());

    connect(_conditions.get(), &ComplexCondition::sg_changed, this, &ConditionsDialog::sl_dataChanged);
    connect(_conditions.get(), &ComplexCondition::sg_inserted, this, &ConditionsDialog::sl_dataInserted);
    connect(_conditions.get(), &ComplexCondition::sg_removed, this, &ConditionsDialog::sl_dataRemoved);

    _conditions->setValidation(false);

    _view = new TreeView;
    _view->setUniformRowHeights(true);
    _view->setObjectName(QStringLiteral("zf_view"));
    _view->setItemDelegate(new ConditionItemDelegate(this, _view));
    _view->setModel(_model.get());
    _view->setConfigMenuOptions(DatasetConfigOptions());
    _view->horizontalHeader()->setStretchLastSection(true);
    _view->rootHeaderItem()->append(COL_CONDITION, ZF_TR(ZFT_COND_DIAL_TYPE));
    _view->rootHeaderItem()->append(COL_FIELD, ZF_TR(ZFT_COND_DIAL_FIELD));
    _view->rootHeaderItem()->append(COL_OPERATOR, ZF_TR(ZFT_COND_DIAL_OPERATOR));
    _view->rootHeaderItem()->append(COL_VALUE, ZF_TR(ZFT_COND_DIAL_VALUE));
    _view->horizontalHeader()->setSectionsMovable(false);
    _view->setFrameShape(QFrame::NoFrame);
    _view->setStyleSheet(QStringLiteral("QTreeView {border: 1px %1;"
                                        "border-top-style: none; "
                                        "border-right-style: solid; "
                                        "border-bottom-style: none; "
                                        "border-left-style: solid;}")
                             .arg(Colors::uiLineColor(true).name()));

    ui->la_tree->addWidget(_view);

    connect(ui->add, &QToolButton::clicked, this, &ConditionsDialog::sl_add_clicked);
    connect(ui->remove, &QToolButton::clicked, this, &ConditionsDialog::sl_remove_clicked);
    connect(_view->selectionModel(), &QItemSelectionModel::currentRowChanged, this, &ConditionsDialog::testEnabled);
    connect(_model.get(), &QStandardItemModel::dataChanged, this, &ConditionsDialog::testEnabled);
    connect(_model.get(), &QStandardItemModel::rowsRemoved, this, &ConditionsDialog::testEnabled);
    connect(_model.get(), &QStandardItemModel::rowsInserted, this, &ConditionsDialog::testEnabled);

    // корневой добавляем только если у него есть дочерние
    if (!_conditions->root()->children().isEmpty())
        addNode(_conditions->root());

    _conditions->copyFrom(_source_conditions, true);
    _view->expandAll();
}

ConditionsDialog::~ConditionsDialog()
{
    delete ui;
}

ConditionPtr ConditionsDialog::condition(const QModelIndex& index) const
{
    Z_CHECK(index.isValid());
    return _conditions->condition(_model->index(index.row(), COL_CONDITION, index.parent()).data(ID_ROLE).toString());
}

const DataStructure* ConditionsDialog::structure() const
{
    return _data_structure;
}

DataProperty ConditionsDialog::datasetProperty() const
{
    return _dataset_property;
}

#define CONFIG_VERSION_KEY QStringLiteral("CONFIG_VERSION_KEY")
#define CONFIG_VERSION_VALUE 1
#define CONFIG_COLUMN_KEY QStringLiteral("CONFIG_COLUMN_KEY")

QMap<QString, QVariant> ConditionsDialog::getConfiguration() const
{
    QMap<QString, QVariant> config;

    config[CONFIG_VERSION_KEY] = CONFIG_VERSION_VALUE;
    for (int i = 0; i < _view->rootHeaderItem()->bottomCount(); i++) {
        config[CONFIG_COLUMN_KEY + QString::number(i)] = _view->rootHeaderItem()->bottomItem(i)->sectionSize();
    }

    return config;
}

Error ConditionsDialog::applyConfiguration(const QMap<QString, QVariant>& config)
{
    if (config.value(CONFIG_VERSION_KEY).toInt() != CONFIG_VERSION_VALUE)
        return Error();

    for (auto it = config.constBegin(); it != config.constEnd(); ++it) {
        bool ok;
        int column = it.key().rightRef(it.key().length() - CONFIG_COLUMN_KEY.length()).toInt(&ok);
        if (ok && column >= 0 && column < _view->rootHeaderItem()->bottomCount() - 1) {
            int size = it.value().toInt(&ok);
            if (ok) {
                _view->rootHeaderItem()->bottomItem(column)->setSectionSize(size);
            }
        }
    }

    // последнюю секцию уменьшаем, т.к. она сама растянется
    _view->rootHeaderItem()->bottomItem(_view->rootHeaderItem()->bottomCount() - 1)->setSectionSize(1);

    return Error();
}

DataProperty ConditionsDialog::delegateGetCellDataProperty(QAbstractItemView* item_view, const QModelIndex& index) const
{
    Q_UNUSED(item_view)

    if (index.column() != COL_VALUE)
        return DataProperty();

    auto c = condition(index);
    if (c == nullptr || c->isPropertyCompare() || (c->isRawCompare() && c->source().lookup() != nullptr))
        return DataProperty();

    return c->source();
}

Error ConditionsDialog::onApply()
{
    _source_conditions->copyFrom(_conditions.get(), false);
    return Dialog::onApply();
}

void ConditionsDialog::onReset()
{
    _conditions->clear();
    Dialog::onReset();
}

void ConditionsDialog::sl_dataChanged(const QString& id)
{
    updateNode(id);
}

void ConditionsDialog::sl_dataInserted(const QString& id)
{
    if (!_conditions->root()->children().isEmpty() && _model->rowCount() == 0)
        addNode(_conditions->root());

    addNode(_conditions->condition(id));
}

void ConditionsDialog::sl_dataRemoved(const QString& id)
{
    removeNode(id);
}

void ConditionsDialog::sl_add_clicked()
{
    if (_conditions->root()->children().isEmpty() && _model->rowCount() == 0) {
        addNode(_conditions->root());
        _view->setCurrentIndex(_model->index(0, 0));
    }

    ConditionPtr current = _view->currentIndex().isValid() ? condition(_view->currentIndex()) : _conditions->root();
    Z_CHECK_NULL(current);

    if (current->isLogical())
        current = _conditions->addEmpty(current);
    else
        current = _conditions->addEmpty(current->parent());

    _view->setCurrentIndex(findNode(current->id()));
}

void ConditionsDialog::sl_remove_clicked()
{
    if (!_view->currentIndex().isValid())
        return;

    auto c = condition(_view->currentIndex());
    Z_CHECK_NULL(c);
    if (c->isRoot()) {
        _conditions->clear();
        return;
    }

    _conditions->remove(c);
}

void ConditionsDialog::updateNode(const QString& id)
{
    if (_block_update_counter > 0)
        return;

    auto c = _conditions->condition(id);
    Z_CHECK_NULL(c);
    QModelIndex node = findNode(id);
    if (!node.isValid()) {
        if (c->isRoot() && c->children().isEmpty())
            return;

        addNode(c);
        return;
    }

    updateNodeHelper(node, c);
}

void ConditionsDialog::removeNode(const QString& id)
{
    if (_block_update_counter > 0)
        return;

    QModelIndex node = findNode(id);
    if (!node.isValid())
        return;

    _block_update_counter++;
    _model->removeRow(node.row(), node.parent());
    if (_conditions->root()->children().isEmpty()) {
        // если после удаления осталось только одно корневое условие, то удаляем все строки
        _model->setRowCount(0);
    }
    _block_update_counter--;
}

QModelIndex ConditionsDialog::addNode(const ConditionPtr& c)
{
    Z_CHECK_NULL(c);

    if (_block_update_counter > 0)
        return QModelIndex();

    QModelIndex node = findNode(c->id());
    if (node.isValid()) {
        updateNode(c->id());
        return node;
    }

    QModelIndex parent;
    if (c->parent() != nullptr) {
        parent = findNode(c->parent()->id());
        if (!parent.isValid()) {
            parent = addNode(c->parent());
            Z_CHECK(parent.isValid());
        }
    }

    _block_update_counter++;

    int row = _model->appendRow(parent);
    node = _model->index(row, 0, parent);
    updateNodeHelper(node, c);

    _block_update_counter--;

    return node;
}

QModelIndex ConditionsDialog::findNode(const QString& id) const
{
    if (_model->rowCount() == 0)
        return QModelIndex();

    auto res = _model->match(_model->index(0, COL_CONDITION), ID_ROLE, id, 1, Qt::MatchFixedString | Qt::MatchRecursive);
    return res.isEmpty() ? QModelIndex() : res.first();
}

void ConditionsDialog::updateNodeHelper(const QModelIndex& node, const ConditionPtr& c)
{
    Z_CHECK(node.isValid());
    Z_CHECK_NULL(c);

    _block_update_counter++;

    _model->setData(node.row(), COL_CONDITION, compareTypeToString(compareTypeFromCondition(c)), node.parent());
    _model->setData(node.row(), COL_CONDITION, c->id(), ID_ROLE, node.parent());
    _model->setData(node.row(), COL_CONDITION, c->isLogical() ? Utils::fontBold(font()) : font(), Qt::FontRole, node.parent());

    _model->setData(node.row(), COL_FIELD, c->isLogical() || !c->source().isValid() ? QVariant() : QVariant(c->source().name()),
                    node.parent());

    if (c->isLogical())
        _model->setData(node.row(), COL_OPERATOR, QVariant(), node.parent());
    else
        _model->setData(node.row(), COL_OPERATOR, c->compareOperatorText(false, true), node.parent());

    if (c->isLogical()) {
        _model->setData(node.row(), COL_VALUE, QVariant(), node.parent());

    } else {
        if (c->isPropertyCompare())
            _model->setData(node.row(), COL_VALUE, c->targetProperty().name(), node.parent());
        else
            _model->setData(node.row(), COL_VALUE, c->targetValue(), node.parent());
    }

    if (c->isLogical()) {
        if (!_view->isFirstColumnSpanned(node.row(), node.parent()))
            _view->setFirstColumnSpanned(node.row(), node.parent(), true);
    } else {
        if (_view->isFirstColumnSpanned(node.row(), node.parent()))
            _view->setFirstColumnSpanned(node.row(), node.parent(), false);
    }

    _block_update_counter--;

    testEnabled();
}

void ConditionsDialog::testEnabled()
{
    setOkButtonEnabled(_conditions->isValid());

    ConditionPtr c = _view->currentIndex().isValid() ? condition(_view->currentIndex()) : nullptr;

    //    ui->add->setEnabled(c != nullptr || _model->rowCount() == 0);
    ui->remove->setEnabled(c != nullptr);
}

QString ConditionsDialog::compareTypeToString(ConditionsDialog::CompareType t) const
{
    ConditionType ct = _compare_mapping.value(t, ConditionType::Undefined);
    if (ct == ConditionType::Undefined)
        return QString();

    switch (t) {
        case CompareType::Or:
        case CompareType::And:
        case CompareType::Required:
            return Utils::conditionTypeToText(ct);
        case CompareType::CompareValue:
            return ZF_TR(ZFT_COND_DIAL_COMPARE_VALUE);
        case CompareType::CompareColumns:
            return ZF_TR(ZFT_COND_DIAL_COMPARE_COLUMNS);
        case CompareType::Undefined:
            break;
    }

    Z_HALT_INT;
    return QString();
}

ConditionsDialog::CompareType ConditionsDialog::compareTypeFromCondition(const ConditionPtr& c) const
{
    Z_CHECK_NULL(c);
    auto type = _compare_mapping.keys(c->type());
    if (type.count() == 1)
        return type.first();

    if (c->isPropertyCompare())
        return CompareType::CompareColumns;
    else
        return CompareType::CompareValue;
}

} // namespace zf
