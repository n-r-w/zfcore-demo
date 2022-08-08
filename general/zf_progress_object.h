#ifndef ZF_PROGRESS_OBJECT_H
#define ZF_PROGRESS_OBJECT_H

#include "zf.h"
#include "zf_object_extension.h"

#include <QDateTime>
#include <QObject>
#include <QPointer>
#include <QStack>
#include <QSet>

namespace zf
{
class ProgressObject;

//! Используется для гарантированного завершения операций прогресса при выходе из метода
class ZCORESHARED_EXPORT ProgressHelper
{
    Q_DISABLE_COPY(ProgressHelper)
public:
    ProgressHelper(const ProgressObject* object = nullptr);
    ~ProgressHelper();

    void reset(const ProgressObject* object);
    bool isInitialized() const { return !_object.isNull(); }

private:
    QPointer<const ProgressObject> _object;
};

/*! Базовый класс отслеживающий состояние прогресса
 * Прогресс имеет стек вызовов. Каждый вызов метода startProgress увеличивает уровень вложенности прогресса.
 * Все методы работы с процентом или текстом прогресса относятся к нижнему уровню стека */
class ZCORESHARED_EXPORT ProgressObject : public QObject, public I_ObjectExtension
{
    Q_OBJECT
public:
    ProgressObject(QObject* parent = nullptr);
    ~ProgressObject();

    //! Имя объекта для отладки
    QString debugName() const;

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
    void objectExtensionDeleteInfoExternal(I_ObjectExtension* i) override;

    //! Этот объект начинает использовать другой объект
    void objectExtensionRegisterUseInternal(I_ObjectExtension* i) final;
    //! Этот объект прекращает использовать другой объект
    void objectExtensionUnregisterUseInternal(I_ObjectExtension* i) final;

public:
    //! Запросить удаление объекта после окончания прогресса
    void requestDelete() const;
    //! Выполнен запрос на удаление
    bool isDeleteRequested() const;

public: // Методы управления прогрессом       
    //! Выполняется ли задача, требующая отображения прогресс-бара
    bool isProgress() const;
    //! Процент выполнения задачи: 0-100
    int progressPercent() const;
    //! Текст с описанием текущей задачи
    QString progressText() const;
    //! Глубина вложенности прогресса
    int progressDepth() const;
    /*! Принудительно остановить прогресс до его окончания (взводит флаг isProgressToCancel)
     * Необходимо в цикле прогресса отслеживать флаг isProgressToCancel и прекращать прогресс при его установке */
    Q_SLOT virtual void cancelProgress() const;
    //! Поддерживается ли управление остановкой прогресса для текущего прогресса
    bool isCancelProgressSupported() const;

    /*! Отмечает данную операцию как длительную. Имеет смысл для операций, для которых нет возможности установки
     * промежуточных процентов между 0 и 100 (например запрос к БД). Если операци отмечена как длительная,
     * то она будет принудительно отображаться в окне прогресса. Сбрасывается в false при окончании прогресса */
    void setLongOperation() const;
    bool isLongOperation() const;

    //! Начать выполнения задачи. Повторный вызов приводит к формированию стека операций прогресса
    void startProgress(const QString& text = QString(), int start_persent = 0) const;
    //! Начало выполнения задачи c поддержкой управления остановкой прогресса
    void startProgressWithCancel(const QString& text = QString(), int start_persent = 0) const;
    //! Начало выполнения задачи. Все методы startProgress работают через этот метод
    Q_SLOT virtual void startProgressOperation(const QString& text, int start_persent, bool cancel_support);

    //! Изменить процент выполнения задачи
    Q_SLOT void setProgressPercent(int percent, const QString& text = QString()) const;
    //! Принудительно обновить отображение информации о прогрессе
    Q_SLOT void updateProgressInfo() const;

    //! Окончить выполнения задачи. Если в стеке их несколько, то убирается последняя
    Q_SLOT void finishProgress() const;

    //! Произвольные данные
    QVariant progressData() const;
    Q_SLOT void setProgressData(const QVariant& v);

    //! Вызывается в длительном цикле, для которого невозможно определить текущий прогресс,
    //! но необходимо обеспечить отзывчивость пользовательского интерфейса (например на нажатие кнопки "остановить")
    //! Вызов этого метода вместо QApplication::processEvents необходим чтобы оболочка сама выбрала правильный параметр
    //! для QApplication::processEvents в завивисмости от необходимости реагировать на нажатие на кнопку "отмена"
    Q_SLOT void processUserEvents() const;

    /*! Установить текст операции, который будет отображаться по умолчанию если запуск произошел через startProgress
     * без указания текста */
    void setDefaultProgressText(const QString& text);
    QString defaultProgressText() const;

    //! Необходимо остановить прогресс
    bool isCanceled() const;

    //! Подключиться к сигналам прогресса другого объекта
    void followProgress(ProgressObject* obj,
                        //! Если истина, то подключаться ко всем сигналам, иначе только к началу и окончанию прогресса
                        bool full_follow = false,
                        //! Тип подключения
                        Qt::ConnectionType c_type = Qt::AutoConnection);
    void followProgress(const std::shared_ptr<ProgressObject>& obj,
                        //! Если истина, то подключаться ко всем сигналам, иначе только к началу и окончанию прогресса
                        bool full_follow = false,
                        //! Тип подключения
                        Qt::ConnectionType c_type = Qt::AutoConnection);
    //! Отключиться от сигналов прогресса другого объекта
    void unFollowProgress(ProgressObject* obj);
    void unFollowProgress(const std::shared_ptr<ProgressObject>& obj);

    //! Подключен ли к сигналам прогресса указанного объекта
    bool isFollowProgress(ProgressObject* object) const;
    bool isFollowProgress(const std::shared_ptr<ProgressObject>& obj) const;
    //! Список объектов, к сигналам прогресса которых подключены
    QList<ProgressObject*> allFollowProgressObjects() const;

    //! Данные из стека прогресса
    struct Progress
    {
        Progress();

        //! Является ли операция длительной
        bool isLongOperation() const;

        //! Указатель на владельца
        const ProgressObject* owner = nullptr;
        //! Процент выполнения фоновой задачи
        int percent = 0;
        //! Текстовое сообщение прогреса
        QString text;
        //! Уникальный код
        QString key;
        //! Поддержка отмены операции
        bool cancel_support = false;
        //! Время начала операции
        QDateTime start_time;
    };
    //! Элемент стека прогресса по его коду
    const Progress* progressByKey(const QString& key) const;
    //! Уникальный код текущего прогресса
    QString progressKey() const;
    //! Стек вызовов прогресса
    const QStack<std::shared_ptr<Progress>>& progressStack() const;

protected:
    //! Начато выполнение задачи. Вызывается только при первом входе в состояние прогресса
    virtual void onProgressBegin(const QString& text, int start_persent);
    //! Изменился процент выполнения задачи или текст
    virtual void onProgressChange(int percent, const QString& text);
    //! Окончено выполнение задачи. Вызывается только при полном выходе из состояния прогресса
    virtual void onProgressEnd();
    //! Изменились данные
    virtual void onProgressDataChange(const QVariant& v);

    //! Имя объекта для отладки
    virtual QString getDebugName() const;

signals:
    //! Объект перешел в состояние выполнения задачи. Вызывается только при первом входе в состояние прогресса
    void sg_progressBegin(const QString& text = QString(), int start_persent = 0, bool cancel_support = false);
    //! Окончено выполнение задачи. Вызывается только при полном выходе из состояния прогресса
    void sg_progressEnd();

    //! Начало выполнения задачи. Вызывается как первого входа в состояние прогресса, так и для вложенных
    void sg_startProgress(const QString& text = QString(), int start_persent = 0, bool cancel_support = false);
    //! Окончание выполнения задачи. Вызывается как для выхода из первого состояния прогресса, так и для вложенных
    void sg_finishProgress(const QString& key);

    //! Изменился процент выполнения задачи. Только для текущей
    void sg_progressChange(int percent, const QString& text = QString());
    //! Изменились данные
    void sg_progressDataChange(const QVariant& v);

    //! Активирована поддержка отмены операции
    void sg_cancelSupportActivated();

    //! Запрошено принудительное остановка выполнения задачи
    void sg_cancelProgress();

    //! Необходимо обеспечить отзывчивость пользовательского интерфейса (например на нажатие кнопки "остановить")
    void sg_processUserEvents();

    //! Необходимо принудительно обновить отображение информации о прогрессе
    void sg_updateProgressInfo();

private slots:
    // слоты для follow объектов
    void sl_followObjectDestroyed(QObject* obj);
    void sl_followObjectStartProgress(const QString& text = QString(), int start_persent = 0, bool cancel_support = false);
    void sl_followObjectFinishProgress(const QString& key);
    void sl_followObjectProgressChange(int percent, const QString& text = QString());
    void sl_followObjectProgressDataChange(const QVariant& v);
    void sl_followObjectCancelProgress();
    void sl_followObjectProcessUserEvents();
    void sl_followObjectUpdateProgressInfo();

private:
    //! Стек вызовов прогресса
    mutable QStack<std::shared_ptr<Progress>> _progress_stack;
    /*! Текст операции, который будет отображаться по умолчанию если запуск произошел через startProgress
     * без указания текста */
    QString _default_progress_text;

    //! Произвольные данные
    QVariant _progress_data;

    //! Необходимо остановить прогресс
    mutable bool _is_progress_to_cancel = false;
    //! Поддерживается ли отмена текущего процесса
    mutable bool _is_cancel_support = false;
    //! Отмечает данную операцию как длительную
    mutable bool _is_long_operation = false;

    //! Выполнен запрос на удаление
    mutable bool _is_delete_requested = false;

    //! Информация о подключении к прогрессу других объектов
    struct FollowInfo
    {
        QMetaObject::Connection sg_startProgress;
        QMetaObject::Connection sg_finishProgress;
        QMetaObject::Connection sg_progressChange;
        QMetaObject::Connection sg_progressDataChange;
        QMetaObject::Connection sg_cancelProgress;
        QMetaObject::Connection sg_processUserEvents;
        QMetaObject::Connection sg_updateProgressInfo;
        QMetaObject::Connection sg_destroyed;
        Qt::ConnectionType c_type = Qt::AutoConnection;
        int count = 1;

        bool isEqual(FollowInfo* i) const;
    };
    //! Объекты, к прогрессу которых подключены
    QHash<QObject*, std::shared_ptr<FollowInfo>> _follow_info;

    //! Потокобезопасное расширение возможностей QObject
    ObjectExtension* _object_extension = nullptr;
};

} // namespace zf

#endif // ZF_PROGRESS_OBJECT_H
