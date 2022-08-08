#ifndef TABLEVIEWNAVIGATOR_H
#define TABLEVIEWNAVIGATOR_H

#include "zf_view.h"
#include "zf_table_view.h"
#include "zf_tree_view.h"
#include <QWidget>

namespace zf
{
class ZCORESHARED_EXPORT TableViewNavigator : public QWidget
{
    Q_OBJECT
public:
    explicit TableViewNavigator(View* view, const PropertyID& dataset, QWidget* parent = nullptr);

signals:

private slots:
    void sl_updateCheckedRowsCount();
    void sl_updateRowsCount();
    void sl_search();
    void sl_searchPrevious();
    void sl_searchNext();
    void sl_disconnect();
    void sl_connect();

private:
    View* _view;
    DataProperty _dataset;
    ProxyItemModel* _item_model;
    //    zf::ItemModel* _item_model;
    QLabel* _totalRowsCountLabel;
    QLabel* _checkedRowsCountLabel;
    QToolButton* _previousButton;
    QLineEdit* _searchEdit;
    QToolButton* _nextButton;
};
} // namespace zf

#endif // TABLEVIEWNAVIGATOR_H
