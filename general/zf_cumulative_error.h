#pragma once

#include "zf_error.h"

namespace zf
{
//! Потокобезопасно накапливает ошибки и отображает из разом при выходе в EventLoop
class ZCORESHARED_EXPORT CumulativeError : public QObject
{
    Q_OBJECT
public:
    CumulativeError();
    ~CumulativeError();

    //! Добавить ошибку
    void addError(const Error& error);

    //! Заблокировать обработку ошибок
    static void block();
    //! Разблокировать обработку ошибок
    static void unblock();

private slots:
    void sl_timeout();
    //! Менеджер обратных вызовов
    //    void sl_callbackManager(int key, const QVariant& data);

private:
    QMutex _mutex;    
    QList<Error> _errors;
    bool _show_on = false;
    QTimer* _timer;

    static QMutex _block_count_mutex;
    static int _block_count;
};

} // namespace zf
