#ifndef ZF_CALLBACK_H
#define ZF_CALLBACK_H

#include "zf_feedback_timer.h"
#include "zf_global.h"
#include "zf_object_extension.h"

#include <QQueue>
#include <QTimer>
#include <QPointer>
#include <QSet>
#include <QList>

namespace zf
{
/*! Реализует механизм последовательных обратных вызовов через встроенный таймер
 * Необходим для синхронизации отложенной обработки, чтобы гарантировать соблюдение порядка
 * отложенных вызовов и не полагаться на последовательность обработки тамеров Qt
 * Кроме того реализована возможность приостановить обработку при необходимости вызвать
 * критичный к обратным вызовам метод (например загрузка модели)
 * Функция обработчик должна иметь вид: void sl_callbackManager(int key, const QVariant& data);
 */
class ZCORESHARED_EXPORT CallbackManager : public QObject, public I_ObjectExtension
{
    Q_OBJECT
public:
    CallbackManager(QObject* parent = nullptr);
    ~CallbackManager();

public: // реализация I_ObjectExtension
    //! Удален ли объект
    bool objectExtensionDestroyed() const final;

    //! Безопасно удалить объект
    void objectExtensionDestroy() override;

    //! Получить данные
    QVariant objectExtensionGetData(
        //! Ключ данных
        const QString& data_key) const final;
    //! Записать данные
    //! Возвращает признак - были ли записаны данные
    bool objectExtensionSetData(
        //! Ключ данных
        const QString& data_key, const QVariant& value,
        //! Замещение. Если false, то при наличии такого ключа, данные не замещаются и возвращается false
        bool replace) const final;

    //! Другой объект сообщает этому, что будет хранить указатель на него
    void objectExtensionRegisterUseExternal(I_ObjectExtension* i) final;
    //! Другой объект сообщает этому, что перестает хранить указатель на него
    void objectExtensionUnregisterUseExternal(I_ObjectExtension* i) final;

    //! Другой объект сообщает этому, что будет удален и надо перестать хранить указатель на него
    //! Реализация этого метода, кроме вызова ObjectExtension::objectExtensionDeleteInfoExternal должна
    //! очистить все ссылки на указанный объект
    void objectExtensionDeleteInfoExternal(I_ObjectExtension* i) final;

    //! Этот объект начинает использовать другой объект
    void objectExtensionRegisterUseInternal(I_ObjectExtension* i) final;
    //! Этот объект прекращает использовать другой объект
    void objectExtensionUnregisterUseInternal(I_ObjectExtension* i) final;

public:
    //! Приостановить обработку
    void stop();
    //! Продолжить обработку
    void start();
    //! Активен или находится ли в состоянии паузы
    bool isActive() const;

    //! Зарегистрировать объект
    void registerObject(
        //! Объект
        const I_ObjectExtension* object,
        //! Слот вида: void sl_callback(int key, const QVariant& data);
        const QString& callback_slot);

    //! Зарегистрирован ли объект
    bool isObjectRegistered(const I_ObjectExtension* object) const;

    //! Поставить объект в очередь на обработку
    //! ВАЖНО: не использовать ключи (key) меньше нуля. Они зарезервированы для ядра
    void addRequest(
        //! Объект
        const I_ObjectExtension* obj,
        //! Уникальный код в рамках объекта >= 0. Он будет возвращен в сигнале sg_callback
        int key,
        //! Произвольные данные
        const QVariant& data = QVariant(),
        //! Если объект не зарегистрировать, то критическая ошибка
        bool halt_if_not_registered = true);

    //! Объект зарегистрирован в очереди на обработку
    bool isRequested(const I_ObjectExtension* obj, int key) const;

    //! Удалить запрос объекта из очереди на обработку
    void removeRequest(const I_ObjectExtension* obj, int key);
    //! Удалить все запросы объекта из очереди на обработку
    void removeObjectRequests(const I_ObjectExtension* obj);

    //! Данные, привязанные к объекту
    QVariant data(const I_ObjectExtension* obj, int key) const;

    static bool isAllStopped();
    //! Приостановить обработку для всех Callback. Допустимы вложенные вызовы
    static void stopAll();
    //! Продолжить обработку для всех Callback. Допустимы вложенные вызовы
    static void startAll();

    //! Принудительно игнорировать не активированный фреймворк (используется при инициализации лаунчера до создания
    //! главного окна)
    static void forceIgnoreInactiveFramework(bool b);

private slots:
    void sl_requestsTimeout();
    void sl_callbacksTimeout();

private:
    //! Зарегистрирован ли объект
    bool isObjectRegisteredHelper(const I_ObjectExtension* object) const;

    //! Информация о запросе
    struct Data
    {
        const I_ObjectExtension* object_ptr;
        int key;
        QVariant data;
        QString callback_slot;
    };
    typedef std::shared_ptr<Data> DataPtr;

    // Поиск позиции в очереди
    int indexOf(const I_ObjectExtension* obj, int key) const;
    // Поиск всех позиций в очереди. Результат отсортирован по убыванию
    QList<int> indexesOf(const I_ObjectExtension* obj, int key, bool all_requests) const;

    DataPtr findData(const I_ObjectExtension* object, int key) const;
    QList<DataPtr> findAllData(const I_ObjectExtension* object, int key, bool all_requests) const;

    //! Удалить объект из очереди на обработку
    bool removeRequestHelper(const I_ObjectExtension* obj, int key, bool all_requests);

    //! Название свойства объекта, где будет храниться его ключ
    static const char* _OBJECT_KEY_PROPERTY;

    struct ObjectInfo
    {
        //! Объект
        const I_ObjectExtension* object = nullptr;
        //! Слот вида: void sl_callback(int key, const QVariant& data);
        QString callback_slot;
    };
    //! Информация об объектах
    QHash<I_ObjectExtension*, std::shared_ptr<ObjectInfo>> _object_info;

    //! Очередь запросов
    QQueue<DataPtr> _requests;
    QMultiHash<const I_ObjectExtension*, DataPtr> _requests_by_object;

    //! Таймер для обработки запросов
    mutable FeedbackTimer* _requests_timer = nullptr;
    //! Таймер для обработки обратных вызовов
    mutable FeedbackTimer* _callback_timer = nullptr;

    //! Очередь на обработку. Сюда попадают записи при обработке очередей
    struct CallbackQueue
    {
        const I_ObjectExtension* object_ptr;
        int key;
        QVariant data;
        QString callback_slot;
    };
    QQueue<std::shared_ptr<CallbackQueue>> _callback_queue;    
    //! Найти элемент в очереди на обработку
    QPair<CallbackQueue*, int> getCallbackQueueItem(const I_ObjectExtension* obj, int key) const;

    //! Счетчик запросов на остановку
    QAtomicInt _pause_request_count = 0;

    //! Контроль обращения к списку зарегистрированных объектов
    mutable QMutex _objects_mutex;
    //! Контроль обращения к очереди колбеков
    mutable QMutex _callback_mutex;
    //! Контроль обращения к запросам
    mutable QMutex _requests_mutex;

    //! Потокобезопасное расширение возможностей QObject
    ObjectExtension* _object_extension = nullptr;

    //! Счетчик остановки всех callback
    static QAtomicInt _stop_all_counter;
    //! Игнорировать не активированный фреймворк
    static QAtomicInt _ignore_inactive_framework;

    friend bool operator<(const CallbackManager::DataPtr& p1, const CallbackManager::DataPtr& p2);
};

inline bool operator<(const CallbackManager::DataPtr& p1, const CallbackManager::DataPtr& p2)
{
    return p1.get() < p2.get();
}

} // namespace zf

#endif // ZF_CALLBACK_H
