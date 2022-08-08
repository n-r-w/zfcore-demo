#pragma once

#include "zf.h"
#include <QQueue>

namespace zf
{
//! Класс для отложенного удаления std::shared_ptr
//! Должен быть создан в главном потоке.
class ZCORESHARED_EXPORT SharedPtrDeleter : public QObject
{
    Q_OBJECT
public:
    SharedPtrDeleter();

    //! Отложенное удаление (только если не других переменных с этим указателем). Потокобезопасно.
    void deleteLater(const std::shared_ptr<void>& ptr);

private slots:
    void sl_timeout();

private:
    QTimer* _timer;
    QQueue<std::shared_ptr<void>> _buffer;
    QMutex _mutex;
};

} // namespace zf
