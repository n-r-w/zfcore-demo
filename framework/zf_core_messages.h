#pragma once

#include "zf_highlight_model.h"
#include "zf_message.h"

namespace zf
{
class HighlightProcessor;

//! Обновить статус бар
class MessageUpdateStatusBar : public Message
{
public:
    //! Код сообщения обновления статус-бара
    static const MessageCode MSG_CODE_UPDATE_STATUS_BAR;

    MessageUpdateStatusBar(const QString& text);
    MessageUpdateStatusBar(const Message& message);

    //! Строка для отображения
    QString text() const;

    //! Создать копию сообщения
    Message* clone() const override;

    //! Формирует сообщение для обновления статус бара на основании текущей активной ошибки
    static void updateStatusBarErrorInfo(DataWidgets* widgets, HighlightProcessor* highlight);
};

//! Критическая ошибка
class MessageCriticalError : public Message
{
public:
    //! Код сообщения критической ошибки
    static const MessageCode MSG_CODE_CRITICAL_ERROR;

    MessageCriticalError(const Error& error);
    MessageCriticalError(const Message& message);

    //! Критическая ошибка
    Error error() const;

    //! Создать копию сообщения
    Message* clone() const override;
};

} // namespace zf
