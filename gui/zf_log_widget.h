#pragma once

#include "zf_log.h"
#include "zf_table_view.h"
#include <QWidget>

class QPlainTextEdit;
class QStandardItemModel;
class QSortFilterProxyModel;
class QModelIndex;

namespace zf
{
//! Виджет для отображения журнала
class ZCORESHARED_EXPORT LogWidget : public QWidget
{
    Q_OBJECT
public:
    explicit LogWidget(const Log* log, QWidget* parent = nullptr);
    ~LogWidget() override;

    LogRecord currentRecord() const;
    LogRecord setCurrentRecord(const LogRecord& record);

    //! Фильтровать по типу записей
    void setFilter(InformationType type = InformationType::Invalid);
    //! Текущее состояние фильтрации
    InformationType filter() const;

    //! Скрыть колонку с типом записи
    void hideRecordTypeTypeColumn();

    TableView* table() const { return _table; }

    void setVisible(bool b) override;

signals:
    //! Перед началом перефильтрации
    void sg_beforeRefilter();
    //! После перефильтрации
    void sg_afterRefilter();

private slots:
    //! Запись была добавлена
    void sl_recordAdded(const zf::LogRecord& record);
    //! Лог был очищен
    void sl_cleared();
    //! Смена строки в логе пользователем
    void currentRowChanged(const QModelIndex& current, const QModelIndex& previous);

private:
    void addRecord(const LogRecord& record);
    InformationType convertInfoType(InformationType it) const;

    //! Лог
    const Log* _log;
    QSortFilterProxyModel* _proxyModel;
    QStandardItemModel* _logModel;
    QPlainTextEdit* _detail;
    TableView* _table;
    InformationType _filter;
};

} // namespace zf
