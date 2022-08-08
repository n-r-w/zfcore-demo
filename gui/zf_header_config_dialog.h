#pragma once

#include "zf_dialog.h"
#include "zf_global.h"
#include "zf_tree_view.h"

#include <QSortFilterProxyModel>
#include <QStandardItemModel>
#include <QWidget>

namespace Ui {
class ItemViewHeaderConfig;
}

namespace zf
{
class HeaderItem;
class HeaderView;

class ZCORESHARED_EXPORT ItemViewHeaderConfigDialog : public Dialog
{
    Q_OBJECT

public:
    explicit ItemViewHeaderConfigDialog();
    ~ItemViewHeaderConfigDialog() override;

    bool run(
        //! Количество зафиксированных групп (узлов верхнего уровня). Если -1, то фукционал фиксации не поддерживается
        int& frozen_group, HeaderView* header_view, bool first_column_movable);

protected:
    //! Изменение размера при запуске
    void adjustDialogSize() override;
    //! Загрузить состояние диалога
    void doLoad() override;
    //! Очистить данные при закрытии диалога
    void doClear() override;
    //! Применить новое состояние диалога
    Error onApply() override;
    //! Была нажата кнопка. Если событие обработано и не требуется стандартная реакция - вернуть true
    bool onButtonClick(QDialogButtonBox::StandardButton button) override;

    bool eventFilter(QObject* obj, QEvent* e) override;

private slots:
    void sl_moveLeft();
    void sl_moveRight();
    void sl_moveFirst();
    void sl_moveLast();
    void sl_dataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight, const QVector<int>& roles);

private:
    QPair<QStandardItem*, QStandardItem*> getMoveLeftData() const;
    QPair<QStandardItem*, QStandardItem*> getMoveRightData() const;

    HeaderItem* headerItem(const QModelIndex& index) const;
    QModelIndex headerIndex(HeaderItem* item) const;

    int columnOrder(const QModelIndex& index) const;

    bool isColumnFrozen(const QModelIndex& index) const;
    void setColumnFrozen(const QModelIndex& index, bool b);

    bool isColumnVisible(const QModelIndex& index) const;
    void setColumnVisible(const QModelIndex& index, bool b);
    bool canHideColumn(const QModelIndex& index) const;

    void updateEnabled();

    void updateFrozen(const QModelIndex& index, bool moved);

    void clearFrozen(const QModelIndex& index);
    void allVisible(const QModelIndex& index);

    void applyOrder(const QModelIndex& index);
    void applyVisible(const QModelIndex& index);

    Ui::ItemViewHeaderConfig* _ui;

    QStandardItemModel* createModel();
    void createModelHelper(QStandardItemModel* model, HeaderView* view, HeaderItem* item, QStandardItem* model_item);
    void reloadOrder(const QModelIndex& index);

    TreeView* _view = nullptr;
    HeaderView* _header_view = nullptr;
    HeaderItem* _root = nullptr;
    int _frozen_group = 0;
    bool _first_column_movable = true;
    QStandardItemModel* _model = nullptr;
    QSortFilterProxyModel* _proxy_model = nullptr;
    bool _block_data_change = false;
    QMap<HeaderItem*, QStandardItem*> _model_items;
};

} // namespace zf
