#pragma once

#include "zf_defs.h"
#include "zf_message_box.h"
#include "zf_feedback_timer.h"

namespace zf
{
class ZCORESHARED_EXPORT SyncWaitWindow : public QDialog, public I_ObjectExtension
{
    Q_OBJECT
public:
    SyncWaitWindow(const QString& text,
                   //! Ждать сообщения ProgressMessage для данного идентификатора и показывать состояние
                   const zf::Uid& progress_uid = {});
    ~SyncWaitWindow();

    //! Идентификатор, на который можно отправлять сообщения для показа прогресса
    Uid progressUid() const;

    //! Открывает окно ожидания. Окно не модальное (метод завершается сразу), но обладает модальным поведением (блокирует все действия пользователя)
    void run();
    //! Задать отображаемый текст
    void setText(const QString& text);

    //! Создает окно ожидания. Окно не модальное (метод завершается сразу), но обладает модальным поведением (блокирует все действия пользователя)
    //! За удаление отвечает вызывающий
    static SyncWaitWindow* run(const QString& text,
                               //! Ждать сообщения ProgressMessage для данного идентификатора и показывать состояние
                               const zf::Uid& progress_uid = {});

    //! Создать идентификатор, на который можно отправлять сообщения для показа прогресса
    static Uid generateProgressUid();

    //! Минимальное время показа диалога
    static const int MINIMUM_TIME_MS;

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

protected:
    void showEvent(QShowEvent* event) override;
    void hideEvent(QHideEvent* event) override;
    bool event(QEvent* event) override;

private slots:
    //! Входящие сообщения от диспетчера
    void sl_message_dispatcher_inbound(const zf::Uid& sender, const zf::Message& message, zf::SubscribeHandle subscribe_handle);

private:
    void updateText();

    QLabel* _text_lalel = nullptr;
    //! Ждать сообщения ProgressMessage для данного идентификатора и показывать состояние
    zf::Uid _progress_uid;

    QString _base_text;
    int _progress_percent = 0;
    QString _progress_text;

    FeedbackTimer _timer;

    //! Потокобезопасное расширение возможностей QObject
    ObjectExtension* _object_extension = nullptr;
};

//! Диалог вопроса с выполнением синхронной функции
class ZCORESHARED_EXPORT SyncAskDialog : public MessageBox
{
    Q_OBJECT
public:
    //! Запуск диалога
    static bool run(
        //! Текст вопроса. Обязательно
        const QString& question,
        //! Текст ожидания. Обязательно
        const QString& wait_info,
        //! Синхронная функция
        SyncAskDialogFunction sync_function,
        //! Ошибка по итогам вызова функции. Помимо того, что она возвращается тут - сообщение об ошибке показывается пользователю при show_error true
        Error& error,
        //! Текст с положительным результатом. Если задано, то будет показан по итогам вызова функции
        const QString& result_info = {},
        //! Показывать ошибку по итогам выполнения функции
        bool show_error = false,
        //! Запускать функцию в отдельном потоке. Имеет смысл использовать, если функция не использует QEventLoop и может подвесить UI
        bool run_in_thread = false,
        //! Ждать сообщения ProgressMessage для данного идентификатора и показывать состояние
        const zf::Uid& progress_uid = {});

protected:
    //! Была нажата кнопка. Если событие обработано и не требуется стандартная реакция - вернуть true
    bool onButtonClick(QDialogButtonBox::StandardButton button) override;

private:
    SyncAskDialog(const QString& question, const QString& wait_info, SyncAskDialogFunction sync_function, const QString& result_info,
                  bool show_error, bool run_in_thread, const zf::Uid& progress_uid);

    QString _wait_info;
    QString _result_info;
    SyncAskDialogFunction _sync_function;
    Error _error;
    bool _show_error;
    bool _run_in_thread;
    //! Ждать сообщения ProgressMessage для данного идентификатора и показывать состояние
    zf::Uid _progress_uid;
};

} // namespace zf
