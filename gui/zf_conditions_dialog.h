#pragma once

#include <QWidget>
#include "zf_dialog.h"
#include "zf_condition.h"
#include "zf_item_delegate.h"

namespace Ui {
class ConditionsDialogUI;
}

namespace zf
{
class TreeView;

class ConditionsDialog : public Dialog, public I_ItemDelegateCellProperty
{
    Q_OBJECT

public:
    explicit ConditionsDialog(
        //! Структура данных
        const DataStructure* data_structure,
        //! Набор данных
        const DataProperty& dataset_property,
        //! Условия
        ComplexCondition* conditions);
    ~ConditionsDialog();

    //! Условие для индекса
    ConditionPtr condition(const QModelIndex& index) const;
    //! Представление
    const DataStructure* structure() const;
    //! Набор данных
    DataProperty datasetProperty() const;

    //! Вернуть текущую конфигурацию
    QMap<QString, QVariant> getConfiguration() const override;
    //! Применить конфигурацию
    Error applyConfiguration(const QMap<QString, QVariant>& config) override;

    //! Интерфейс для динамического определения типа данных ячейки при работе с делегатом
    DataProperty delegateGetCellDataProperty(QAbstractItemView* item_view, const QModelIndex& index) const override;

protected:
    //! Применить новое состояние диалога
    Error onApply() override;
    //! Активирована кнопка reset
    void onReset() override;

private slots:
    //! Изменилось условие
    void sl_dataChanged(const QString& id);
    //! Добавлено условие
    void sl_dataInserted(const QString& id);
    //! Удалено условие
    void sl_dataRemoved(const QString& id);

    //! Нажата кнопка добавить
    void sl_add_clicked();
    //! Нажата кнопка удалить
    void sl_remove_clicked();

private:
    void updateNode(const QString& id);
    void removeNode(const QString& id);
    QModelIndex addNode(const ConditionPtr& c);
    QModelIndex findNode(const QString& id) const;
    void updateNodeHelper(const QModelIndex& node, const ConditionPtr& c);
    Q_SLOT void testEnabled();

    Ui::ConditionsDialogUI* ui;

    //! Вид сравнения
    enum class CompareType
    {
        Undefined,
        Or,
        And,
        Required,
        CompareValue,
        CompareColumns
    };
    //! Соответствие вида сравнения с ConditionType
    const QMap<CompareType, zf::ConditionType> _compare_mapping = {
        {CompareType::Undefined, ConditionType::Undefined},
        {CompareType::Or, ConditionType::Or},
        {CompareType::And, ConditionType::And},
        {CompareType::Required, ConditionType::Required},
        {CompareType::CompareValue, ConditionType::Compare},
        {CompareType::CompareColumns, ConditionType::Compare},
    };
    QString compareTypeToString(CompareType t) const;
    CompareType compareTypeFromCondition(const ConditionPtr& c) const;

    //! Структура данных
    const DataStructure* _data_structure = nullptr;
    //! Набор данных
    DataProperty _dataset_property;
    //! Исходные условия
    ComplexCondition* _source_conditions = nullptr;
    //! Условия в процессе редактирования
    std::unique_ptr<ComplexCondition> _conditions;

    std::unique_ptr<ItemModel> _model;
    TreeView* _view = nullptr;

    int _block_update_counter = 0;

    friend class ConditionItemDelegate;
};

} // namespace zf
