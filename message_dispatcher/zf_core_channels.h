#ifndef ZF_CORE_CHANNELS_H
#define ZF_CORE_CHANNELS_H

#include "zf_message_channel.h"

namespace zf
{
//! Зарезервированные каналы
class ZCORESHARED_EXPORT CoreChannels
{
public:
    // Минимальный номер каналов MessageDispatcher, начиная с которго можно создавать свои каналы
    static const int MIN_MESSAGE_DISPATCHER_CHANNEL_NUM; // 500

    //! Общий канал для обмена сообщениями между модулями
    static MessageChannel GENERAL;

    //**** Каналы MessageDispatcher, относящиеся к DatabaseManager
    //! Канал на котором DatabaseManager рассылает сообщения об изменении сущностей
    static MessageChannel ENTITY_CHANGED;
    //! Канал на котором DatabaseManager рассылает сообщения об удалении сущностей
    static MessageChannel ENTITY_REMOVED;
    //! Канал на котором DatabaseManager рассылает сообщения об удалении сущностей
    static MessageChannel ENTITY_CREATED;
    //! Канал на котором DatabaseManager рассылает текстовые сообщения от сервера
    static MessageChannel SERVER_INFORMATION;
    //! Канал на котором DatabaseManager рассылает сообщения о состоянии подключения к серверу
    static MessageChannel CONNECTION_INFORMATION;

    //! Канал на котором модели сообщают что их данные стали невалидными
    static MessageChannel MODEL_INVALIDATE;
    //! Внутренний канал ядра
    static MessageChannel INTERNAL;
    //! Внутренний канал ядра для отладки сообщений
    static MessageChannel MESSAGE_DEBUG;
};

} // namespace zf

#endif // ZF_CORE_CHANNELS_H
