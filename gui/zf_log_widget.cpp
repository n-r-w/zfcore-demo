#include "zf_log_widget.h"
#include "zf_core.h"
#include "zf_html_tools.h"
#include "zf_translation.h"

#include <QDebug>
#include <QLabel>
#include <QPlainTextEdit>
#include <QSortFilterProxyModel>
#include <QStandardItemModel>
#include <QVBoxLayout>

#define RECORD_ROLE (Qt::UserRole + 100)

namespace zf
{
LogWidget::LogWidget(const Log* log, QWidget* parent)
    : QWidget(parent)
    , _log(log)
    , _proxyModel(new QSortFilterProxyModel(this))
    , _logModel(new QStandardItemModel(this))
    , _detail(new QPlainTextEdit)
    , _table(new TableView)
    , _filter(InformationType::Invalid)
{
    Z_CHECK_NULL(_log);

    _detail->setMaximumHeight(100);
    _detail->setReadOnly(true);

    _logModel->setColumnCount(5);
    for (int row = 0; row < log->count(); row++) {        
        addRecord(log->record(row));
    }
    _proxyModel->setSourceModel(_logModel);
    _proxyModel->setFilterKeyColumn(3);
    _proxyModel->setDynamicSortFilter(true);

    _table->setModel(_proxyModel);

    setLayout(new QVBoxLayout);
    layout()->setMargin(0);
    layout()->addWidget(_table);
    layout()->setObjectName(QStringLiteral("zfla"));
    QLabel* detailLabel = new QLabel(ZF_TR(ZFT_DETAIL));
    detailLabel->setObjectName(QStringLiteral("zf_detail_label"));
    detailLabel->setFont(Utils::fontBold(detailLabel->font()));
    layout()->addWidget(detailLabel);
    layout()->addWidget(_detail);

    _table->verticalHeader()->setVisible(true);
    // Формируем заголовки колонок
    auto root = _table->horizontalRootHeaderItem();
    root->beginUpdate();
    root->append(0, ZF_TR(ZFT_STATUS))->setSectionSize(150);
    root->append(1, ZF_TR(ZFT_INFORMATION))->setSectionSize(150);
    root->endUpdate();

    connect(log, &Log::sg_recordAdded, this, &LogWidget::sl_recordAdded);
    connect(log, &Log::sg_cleared, this, &LogWidget::sl_cleared);
    connect(_table->selectionModel(), &QItemSelectionModel::currentRowChanged, this, &LogWidget::currentRowChanged);
}

LogWidget::~LogWidget()
{
}

LogRecord LogWidget::currentRecord() const
{
    if (!_table->currentIndex().isValid())
        return LogRecord();

    return _proxyModel->index(_table->currentIndex().row(), 0).data(RECORD_ROLE).value<LogRecord>();
}

LogRecord LogWidget::setCurrentRecord(const LogRecord& record)
{
    QModelIndexList il = _logModel->match(
            _logModel->index(0, 4), Qt::DisplayRole, QString::number(record.num()), 1, Qt::MatchFixedString);

    QModelIndex idx;
    if (!il.isEmpty()) {
        if (il.count() > 1)
            Z_HALT_DEBUG("LogWidget::setCurrentRecord");
        idx = il.at(0);
    }

    idx = _proxyModel->mapFromSource(idx);
    idx = _proxyModel->index(idx.row(), 0);

    _table->setCurrentIndex(idx);

    if (idx.isValid())
        _table->scrollTo(idx);

    return currentRecord();
}

void LogWidget::setFilter(InformationType type)
{
    if (type == InformationType::Error || type == InformationType::Critical || type == InformationType::Fatal
        || type == InformationType::Required)
        type = InformationType::Error;

    QString key = QString::number(static_cast<int>(type));

    QString fString;
    if (type != InformationType::Invalid)
        fString = key.leftJustified(3, '_');

    _filter = type;

    emit sg_beforeRefilter();
    _proxyModel->setFilterFixedString(fString);
    emit sg_afterRefilter();
}

InformationType LogWidget::filter() const
{
    return _filter;
}

void LogWidget::hideRecordTypeTypeColumn()
{
    _table->horizontalHeader()->hideSection(0);
    _table->horizontalHeader()->setVisible(false);
    _table->verticalHeader()->setVisible(false);
}

void LogWidget::setVisible(bool b)
{
    if (b && !_table->currentIndex().isValid() && _logModel->rowCount() > 0)
        _table->setCurrentIndex(_logModel->index(0, 0));

    QWidget::setVisible(b);
}

void LogWidget::sl_recordAdded(const LogRecord& record)
{
    addRecord(record);
}

void LogWidget::sl_cleared()
{
    _logModel->setRowCount(0);
}

void LogWidget::currentRowChanged(const QModelIndex& current, const QModelIndex& previous)
{
    Q_UNUSED(previous)

    if (current.isValid()) {
        QModelIndex idx = _proxyModel->mapToSource(current);

        QString main_text = _logModel->data(_logModel->index(idx.row(), 1)).toString();
        QString detail_text = _logModel->data(_logModel->index(idx.row(), 2)).toString();
        if (detail_text.isEmpty()) {
            detail_text = main_text;

        } else if (detail_text.left(main_text.length()) != main_text) {
            detail_text = main_text.right(1) == QStringLiteral(".") ? main_text + QStringLiteral(" ") + detail_text
                                                                    : main_text + QStringLiteral(". ") + detail_text;
        }
        HtmlTools::plainIfHtml(detail_text, false);
        _detail->setPlainText(detail_text);

    } else
        _detail->setPlainText(QString());
}

void LogWidget::addRecord(const LogRecord& record)
{
    InformationType type = convertInfoType(record.type());

    if (type == InformationType::Error || type == InformationType::Critical || type == InformationType::Fatal
        || type == InformationType::Required)
        type = InformationType::Error;

    int row = _logModel->rowCount();
    _logModel->setRowCount(_logModel->rowCount() + 1);
    _logModel->setData(_logModel->index(row, 0), record.typeText());
    _logModel->setData(_logModel->index(row, 0), QVariant::fromValue<LogRecord>(record), RECORD_ROLE);

    _logModel->setData(_logModel->index(row, 1), record.info());
    _logModel->setData(_logModel->index(row, 2), record.detail());
    _logModel->setData(_logModel->index(row, 3), QString::number(static_cast<int>(type)).leftJustified(3, '_'));
    _logModel->setData(_logModel->index(row, 4), QString::number(record.num()));

    QColor lineColor = QColor(Qt::white);
    QColor fontColor = QColor(Qt::black);
    if (type == InformationType::Error) {
        fontColor = QColor(Qt::darkRed);
        lineColor = QColor(Qt::white);
    } else if (type == InformationType::Warning) {
        fontColor = QColor(Qt::darkYellow);
        lineColor = QColor(Qt::white);
    } else if (type == InformationType::Information) {
        lineColor = QColor(Qt::white);
        fontColor = QColor("#0074C3");
    } else if (type == InformationType::Success) {
        fontColor = QColor(Qt::darkGreen);
        lineColor = QColor(Qt::white);
    }

    QStandardItem* item = _logModel->itemFromIndex(_logModel->index(row, 0));
    item->setBackground(lineColor);
    item->setForeground(fontColor);
    item->setFont(Utils::fontBold(item->font()));

    if (_table)
        _table->scrollToBottom();
}

InformationType LogWidget::convertInfoType(InformationType it) const
{
    return it;
}
} // namespace zf
