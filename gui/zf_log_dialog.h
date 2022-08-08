#pragma once

#include "zf_dialog.h"
#include "zf_log.h"

namespace Ui
{
class LogDialogUI;
}

namespace zf
{
class LogWidget;

//! Диалог для отображения журнала
class LogDialog : public Dialog
{
    Q_OBJECT
public:
    LogDialog(const Log* log, const QIcon& messageIcon = QIcon(":/share_icons/blue/info.svg"), const QString& message = QString());
    ~LogDialog() override;

    void setVisible(bool visible) override;

protected:
    //! Изменение размера при запуске
    void adjustDialogSize() override;
    //! Была нажата кнопка
    bool onButtonClick(QDialogButtonBox::StandardButton button) override;
    //! Загрузить состояние диалога
    void doLoad() override;

private slots:
    void filterIndexChanged(int index);
    //! Перед началом перефильтрации
    void sl_beforeRefilter();
    //! После перефильтрации
    void sl_afterRefilter();

private:
    QList<int> getButtonsList() const;
    QStringList getButtonText() const;
    QList<int> getButtonRoles() const;

    Ui::LogDialogUI* _ui;
    const Log* _log;
    LogWidget* _logWidget;

    //! Текущая запись (сохранение после фильтрации)
    LogRecord _savedRecord;
};
} // namespace zf
