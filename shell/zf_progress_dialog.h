#pragma once

#include <QDateTime>
#include <QDialog>

#include "zf_relaxed_progress.h"

namespace Ui {
class ProgressDialog;
}

namespace zf
{
class MainWindow;

class ZCORESHARED_EXPORT ProgressDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ProgressDialog(MainWindow* window = nullptr);
    ~ProgressDialog() override;

protected:
    void keyPressEvent(QKeyEvent* e) override;

signals:
    void sg_show();
    void sg_hide();

private slots:
    void sl_cancel_click();

    //! Активирован прогресс
    void sl_startProgress();
    //! Изменился процент выполнения задачи
    void sl_progressChange();

    //! Активирован прогресс с задержкой
    void sl_startRelaxedProgress();
    //! Деактивирован прогресс
    void sl_finishProgress();
    //! Изменился процент выполнения задачи с задержкой
    void sl_progressRelaxedChange();
    //! Активирована поддержка отмены операции
    void sl_cancelSupportActivated();

private:
    void reset();

    void updateProgress();
    //! Позиционирование окна
    void updatePosition(bool process_events);
    void processEvents(bool force);

    bool transition(int percent);
    int progressValue() const { return _progressValue; }

    void setProgressValue(int i, bool force = false);
    void setOverralProgressValue(int i, bool force = false);

    void setProgressStyleLess50();
    void setProgressStyleMore50();

    void setOverralProgressStyleLess50();
    void setOverralProgressStyleMore50();

    Ui::ProgressDialog* _ui;
    MainWindow* _window = nullptr;
    RelaxedProgress* _relaxed_progress = nullptr;

    bool _canceled = false;
    //! Время последнего обновления информации
    QDateTime _last_update_time;
    //! Установлено ли время последнего обновления информации
    bool _last_update_time_valid = false;
    //! Время последнего вызова process events
    QDateTime _last_process_events_time;
    //! Время, когда было запрошено скрытие общего прогресса
    QDateTime _overral_progress_hide_request;

    QString _last_progress_key;

    int _progressValue = 0;
};

} // namespace zf
