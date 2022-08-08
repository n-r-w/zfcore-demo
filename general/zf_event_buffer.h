#ifndef ZF_EVENT_BUFFER_H
#define ZF_EVENT_BUFFER_H

#include <QMap>
#include <QObject>
#include <QPointer>
#include <QReadLocker>
#include <QWriteLocker>
#include <QEventLoop>
#include <QApplication>

namespace zf
{
/*! Буфер для межпоточного взаимодействия
 * В отличие от сигналов/слотов Qt позволяет вызывать метод подписчика по условию совпадения "домена" (переменная
 * domain) */
template <class T> class EventBuffer
{
public:
    EventBuffer() {}
    ~EventBuffer() { qDeleteAll(_callbacks); }

    //! Подписать объект на события
    void subscribe(
            //! Подписчик
            QObject* subscriber,
            //! Слот в subscriber. Слот не должен иметь параметры
            const QString& slot,
            //! Код "домена". Слот будет вызвать только при условии что отправитель вызвал putData с таким кодом
            //! "домена"
            int domain = 0)
    {
        QWriteLocker locker(&_mutex);
        CallbackData* c_data = new CallbackData;
        c_data->slot = slot;
        c_data->domain = domain;
        _callbacks[new QPointer<QObject>(subscriber)] = c_data;
    }

    //! Забрать данные из буфера. Вызывать из метода слота
    void getData(QObject* subscriber, QList<T>& data)
    {
        QWriteLocker locker(&_mutex);

        for (int k = _buffer.count() - 1; k >= 0; k--) {
            BufferData* b_data = _buffer.at(k);
            for (int n = 0; n < b_data->not_processing.count(); n++) {
                if (b_data->not_processing.at(n)->data() == subscriber) {
                    data << b_data->data;
                    b_data->not_processing.removeAt(n);
                    break;
                }
            }
            if (b_data->not_processing.isEmpty()) {
                _buffer.removeAt(k);
                delete b_data;
            }
        }
    }

    //! Положить данные в буфер и проинформировать получателей
    void putData(const T& data, int domain = 0)
    {
        QList<QPointer<QObject>*> to_remove;

        BufferData* b_data = new BufferData;
        b_data->data = data;
        _mutex.lockForWrite();
        _buffer << b_data;
        // отправляем извещение о наличии данных

        for (auto i = _callbacks.constBegin(); i != _callbacks.constEnd(); ++i) {
            QPointer<QObject>* subscriber = i.key();
            if (subscriber->isNull()) {
                to_remove << subscriber;
                continue;
            }
            if (i.value()->domain != domain)
                continue;

            b_data->not_processing << subscriber;
            QMetaObject::invokeMethod(subscriber->data(), i.value()->slot.toLatin1().data(), Qt::AutoConnection);
        }

        // очищаем удаленных получателей
        for (int i = 0; i < to_remove.count(); i++) {
            _callbacks.remove(to_remove.at(i));
            delete b_data->not_processing.at(i);
            b_data->not_processing.removeAt(i);
        }

        // Хотели отправить сообщение, но некому
        if (b_data->not_processing.isEmpty()) {
            _buffer.removeLast();
            delete b_data;
        }

        _mutex.unlock();
    }

    //! Текущий размер буфера
    int bufferSize() const
    {
        QReadLocker locker(&_mutex);
        return _buffer.count();
    }

private:
    mutable QReadWriteLock _mutex;

    struct CallbackData
    {
        QString slot;
        int domain = 0;
    };
    QMap<QPointer<QObject>*, CallbackData*> _callbacks;

    struct BufferData
    {
        T data;
        QList<QPointer<QObject>*> not_processing;
    };
    QList<BufferData*> _buffer;
};

} // namespace zf

#endif // ZF_EVENT_BUFFER_H
