#pragma once

#include "zf_progress_object.h"

#include <QTimer>

namespace zf
{
/*! Реагирует на изменение состояния прогресса с задержкой. Используется для визуального отображения прогресса */
class ZCORESHARED_EXPORT RelaxedProgress : public QObject
{
    Q_OBJECT
public:
    RelaxedProgress(ProgressObject* progress, QObject* parent = nullptr);
    ~RelaxedProgress() override;

    ProgressObject* progress() const;

    //! Активирован или нет
    bool isRelaxedProgress() const;

    //! Задать время реакции в мс.
    void setRelaxedTimeout(int ms);
    //! Время реакции в мс.
    int relaxedTimeout() const;

    //! Принудительно активировать
    void force();

signals:
    //! Активирован
    void sg_startProgress(const QString& text = QString(), int start_persent = 0, bool cancel_support = false);
    //! Деактивирован
    void sg_finishProgress();
    //! Изменился процент выполнения задачи. Только для текущей
    void sg_progressChange(int percent = 0, const QString& text = QString());
    //! Активирована поддержка отмены операции
    void sg_cancelSupportActivated();

private slots:
    void sl_timeout();
    //! Объект перешел в состояние выполнения задачи. Вызывается только при первом входе в состояние прогресса
    void sl_progressBegin();
    //! Окончено выполнение задачи. Вызывается только при полном выходе из состояния прогресса
    void sl_progressEnd();
    //! Изменился процент выполнения задачи. Только для текущей
    void sl_progressChange(int percent, const QString& text = QString());
    //! Активирована поддержка отмены операции
    void sl_cancelSupportActivated();

    void sl_progressDestroyed();

private:
    ProgressObject* _progress = nullptr;
    int _relaxed_timeout = 100;
    bool _active = false;
    QTimer* _timer = nullptr;
};
} // namespace zf
