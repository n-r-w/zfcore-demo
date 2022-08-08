#include "zf_core_channels.h"

namespace zf
{
// Минимальный номер каналов MessageDispatcher, начиная с которго можно создавать свои каналы
const int CoreChannels::MIN_MESSAGE_DISPATCHER_CHANNEL_NUM = 500;

//! Общий канал для обмена сообщениями между модулями
MessageChannel CoreChannels::GENERAL = MessageChannel(1);

//! Канал на котором DatabaseManager рассылает сообщения об изменении сущностей
MessageChannel CoreChannels::ENTITY_CHANGED = MessageChannel(20);
//! Канал на котором DatabaseManager рассылает сообщения об удалении сущностей
MessageChannel CoreChannels::ENTITY_REMOVED = MessageChannel(21);
//! Канал на котором DatabaseManager рассылает сообщения об удалении сущностей
MessageChannel CoreChannels::ENTITY_CREATED = MessageChannel(22);
//! Канал на котором DatabaseManager рассылает текстовые сообщения от сервера
MessageChannel CoreChannels::SERVER_INFORMATION = MessageChannel(23);
//! Канал на котором DatabaseManager рассылает сообщения о состоянии подключения к серверу
MessageChannel CoreChannels::CONNECTION_INFORMATION = MessageChannel(24);

//! Канал на котором модели сообщают что их данные стали невалидными
MessageChannel CoreChannels::MODEL_INVALIDATE = MessageChannel(30);
//! Внутренний канал ядра
MessageChannel CoreChannels::INTERNAL = MessageChannel(40);
//! Внутренний канал ядра для отладки сообщений
MessageChannel CoreChannels::MESSAGE_DEBUG = MessageChannel(50);
} // namespace zf
