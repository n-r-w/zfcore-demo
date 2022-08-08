#include "zf_table_view_navigator.h"
#include "zf_translation.h"

namespace zf
{
TableViewNavigator::TableViewNavigator(View* view, const PropertyID& dataset, QWidget* parent)
    : QWidget(parent)
    , _view(view)
    , _dataset(_view->property(dataset))
{
    Z_CHECK_NULL(view);

    if (_dataset.dataType() == DataType::Table) {
        TableView* tableView = view->object<TableView>(_dataset);
        connect(tableView, &TableView::sg_checkedRowsChanged, this, &TableViewNavigator::sl_updateCheckedRowsCount);
    } else if (_dataset.dataType() == DataType::Tree) {
        TreeView* treeView = view->object<TreeView>(_dataset);
        connect(treeView, &TreeView::sg_checkedRowsChanged, this, &TableViewNavigator::sl_updateCheckedRowsCount);
    } else {
        Z_HALT_INT;
    }

    _item_model = _view->proxyDataset(_dataset);
    connect(_view, &View::sg_beforeModelChange, this, &TableViewNavigator::sl_disconnect);
    connect(_view, &View::sg_afterModelChange, this, &TableViewNavigator::sl_connect);
    connect(_view, &View::sg_afterModelChange, this, &TableViewNavigator::sl_updateCheckedRowsCount);
    connect(_view, &View::sg_afterModelChange, this, &TableViewNavigator::sl_updateRowsCount);

    _totalRowsCountLabel = new QLabel("Всего строк: 0");
    _checkedRowsCountLabel = new QLabel("Выбрано: 0");

    _previousButton = new QToolButton;
    _previousButton->setFixedSize(27, 30);
    _previousButton->setToolButtonStyle(Qt::ToolButtonIconOnly);
    _previousButton->setAutoRaise(true);
    _previousButton->setIcon(QIcon(":/share_icons/previous.svg"));
    _previousButton->setIconSize(QSize(20, 20));
    _previousButton->setToolTip(ZF_TR(ZFT_PREV));
    connect(_previousButton, &QToolButton::clicked, this, &TableViewNavigator::sl_searchPrevious);

    _searchEdit = new QLineEdit;
    _searchEdit->setMinimumWidth(200);
    _searchEdit->setPlaceholderText(ZF_TR(TR::ZFT_SEARCH));
    connect(_searchEdit, &QLineEdit::returnPressed, this, &TableViewNavigator::sl_search);

    _nextButton = new QToolButton;
    _nextButton->setFixedSize(27, 30);
    _nextButton->setToolButtonStyle(Qt::ToolButtonIconOnly);
    _nextButton->setAutoRaise(true);
    _nextButton->setIcon(QIcon(":/share_icons/next.svg"));
    _nextButton->setIconSize(QSize(20, 20));
    _nextButton->setToolTip(ZF_TR(ZFT_NEXT));
    connect(_nextButton, &QToolButton::clicked, this, &TableViewNavigator::sl_searchNext);

    QHBoxLayout* mainLayout = new QHBoxLayout;
    mainLayout->addWidget(_totalRowsCountLabel);
    mainLayout->addWidget(_checkedRowsCountLabel);
    mainLayout->addWidget(_previousButton);
    mainLayout->addWidget(_searchEdit);
    mainLayout->addWidget(_nextButton);
    mainLayout->addStretch(1);
    setLayout(mainLayout);

    sl_connect();
}

void TableViewNavigator::sl_updateCheckedRowsCount()
{
    int rowCount = _view->checkedDatasetRowsCount(_dataset);
    QString text = QString("Выбрано: %1").arg(QString::number(rowCount));
    _checkedRowsCountLabel->setText(text);
}

void TableViewNavigator::sl_updateRowsCount()
{
    int rowCount = _item_model->rowCount();
    QString text = QString("Всего строк: %1").arg(QString::number(rowCount));
    _totalRowsCountLabel->setText(text);
}

void TableViewNavigator::sl_search()
{
    _view->searchText(_dataset, _searchEdit->text());
}

void TableViewNavigator::sl_searchPrevious()
{
    _view->searchTextPrevious(_dataset, _searchEdit->text());
}

void TableViewNavigator::sl_searchNext()
{
    _view->searchTextNext(_dataset, _searchEdit->text());
}

void TableViewNavigator::sl_disconnect()
{
    disconnect(_item_model, &ProxyItemModel::rowsInserted, this, &TableViewNavigator::sl_updateRowsCount);
    disconnect(_item_model, &ProxyItemModel::rowsRemoved, this, &TableViewNavigator::sl_updateRowsCount);
    disconnect(_item_model, &ProxyItemModel::modelReset, this, &TableViewNavigator::sl_updateRowsCount);
}

void TableViewNavigator::sl_connect()
{
    _item_model = _view->proxyDataset(_dataset);
    connect(_item_model, &ProxyItemModel::rowsInserted, this, &TableViewNavigator::sl_updateRowsCount);
    connect(_item_model, &ProxyItemModel::rowsRemoved, this, &TableViewNavigator::sl_updateRowsCount);
    connect(_item_model, &ProxyItemModel::modelReset, this, &TableViewNavigator::sl_updateRowsCount);
}
} // namespace zf
