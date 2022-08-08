#pragma once

#include "zf_defs.h"
#include "zf_utils.h"
#include <QQueue>
#include <QMutexLocker>
#include <limits>

namespace zf
{
//! Генерация последовательностей для целых чисел. Потокобезопасно
template <class T> class SequenceGenerator
{
public:
    SequenceGenerator(
        //! Следующий номер, который будет выдан через take
        T max = std::numeric_limits<T>::min() + 1)
        : _max(max)
    {
    }

    //! Получить следующий свободный номер. После освобождения вызвать release
    T take()
    {
        QMutexLocker lock(&_mutex);

        if (!_free.isEmpty())
            return _free.dequeue();

#if !defined(RELEASE_MODE) && defined(QT_DEBUG)
        Z_CHECK(_max < std::numeric_limits<T>::max());
#endif
        return _max++;
    }
    //! Освободить номер
    void release(T num)
    {
        QMutexLocker lock(&_mutex);

#if !defined(RELEASE_MODE) && defined(QT_DEBUG)
        Z_CHECK(!_free.contains(num));
        Z_CHECK(num < _max);
#endif
        if (num == _max - 1)
            _max--;
        else
            _free.enqueue(num);
    }

private:
    QMutex _mutex;
    //! Следующий номер при отсутствии свободных во _free
    T _max;
    //! Свободные номера
    QQueue<T> _free;
};

} // namespace zf
