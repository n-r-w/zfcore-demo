#pragma once

#include "zf_global.h"

#include <QEvent>
#include <QObject>
#include <QMutex>

namespace zf
{
/*! Аналог QTimer с setInterval(0) и setSingleShot(true)
 * В отличие от QTimer, timeout срабатывает при открытом модальном диалоге
 */
class ZCORESHARED_EXPORT FeedbackTimer : public QObject
{
    Q_OBJECT
public:
    FeedbackTimer(QObject* parent = nullptr);

    void start();
    void stop();
    bool isActive() const;

protected:
    void customEvent(QEvent* event) override;

signals:
    void timeout();

private:
    mutable QMutex _mutex;

    bool _active = false;
    //! Чтобы избежать спама сообщений TIMEOUT_EVENT
    bool _event_posted = false;
};

} // namespace zf
