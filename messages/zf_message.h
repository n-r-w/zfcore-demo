#pragma once

#include "zf_error.h"
#include "zf_global.h"
#include "zf_uid.h"
#include "zf_defs.h"
#include "zf_property_table.h"
#include <QSharedDataPointer>

/*! Если задан Z_LOCAL_MESAGE_ID, то идентификаторы сообщений будут на базе quint64
 * Это быстрее, но можно применять только если сообщения не уходят за пределы процесса */
#define Z_LOCAL_MESAGE_ID true

namespace zf
{
class Message;
class MessageID;
class MessageCode;

class DataContainer;
typedef QList<DataContainer> DataContainerList;
} // namespace zf

ZCORESHARED_EXPORT QDataStream& operator<<(QDataStream& out, const zf::Message& obj);
ZCORESHARED_EXPORT QDataStream& operator>>(QDataStream& in, zf::Message& obj);

ZCORESHARED_EXPORT QDataStream& operator<<(QDataStream& out, const zf::MessageCode& obj);
ZCORESHARED_EXPORT QDataStream& operator>>(QDataStream& in, zf::MessageCode& obj);

ZCORESHARED_EXPORT QDebug operator<<(QDebug debug, const zf::Message& c);
ZCORESHARED_EXPORT QDebug operator<<(QDebug debug, const zf::MessageCode& c);

#if !Z_LOCAL_MESAGE_ID
ZCORESHARED_EXPORT QDataStream& operator<<(QDataStream& out, const zf::MessageID& obj);
ZCORESHARED_EXPORT QDataStream& operator>>(QDataStream& in, zf::MessageID& obj);
ZCORESHARED_EXPORT QDebug operator<<(QDebug debug, const zf::MessageID& c);
#endif

namespace zf
{
class Message_SharedData;

class Model;
typedef std::shared_ptr<Model> ModelPtr;

#if Z_LOCAL_MESAGE_ID
//! Идентификатор сообщения (на базе quint64)
class ZCORESHARED_EXPORT MessageID : public Handle<quint64>
{
public:
    MessageID();
    MessageID(quint64 id);
    MessageID(const Handle<quint64>& h);
};

#else
//! Идентификатор сообщения (на базе QString)
class ZCORESHARED_EXPORT MessageID : protected QString
{
public:
    MessageID();
    MessageID(const MessageID& m);
    MessageID& operator=(const MessageID& m);
    bool operator==(const MessageID& m) const;
    bool operator!=(const MessageID& m) const;
    bool operator<(const MessageID& m) const;

    bool isValid() const;
    //! Представить в виде строки
    QString toString() const;
    //! Сделать невалидным
    void clear();

    //! Для функции qHash
    uint hashValue() const;

    //! Создать новый идентификатор
    static MessageID generate();

private:
    QString& cast();
    const QString& cast() const;

    MessageID(const QString& s);

    friend QDataStream& ::operator<<(QDataStream& out, const zf::MessageID& obj);
    friend QDataStream& ::operator>>(QDataStream& in, zf::MessageID& obj);
    friend QDebug(::operator<<(QDebug debug, const zf::MessageID& c));
};
#endif

//! Код сообщения для создания нестандартных сообщений
class ZCORESHARED_EXPORT MessageCode
{
public:
    MessageCode();
    MessageCode(const MessageCode& c);
    MessageCode(int message_code);
    MessageCode(const EntityCode& entity_code, int message_code);

    bool operator==(const MessageCode& c) const;
    MessageCode& operator=(const MessageCode& c);

    bool isValid() const;
    bool isEntityBased() const;

    EntityCode entity() const;
    int code() const;

    //! Текстовое представление
    QString toPrintable() const;

private:
    // структура для формирования составного идентификатора
    union UniqueKeyUnion
    {
        quint64 key_64 = 0;
        struct UniqueKeyStruct
        {
            int entity_code : 32;
            int message_code : 32;
        };
        UniqueKeyStruct key_split;
    };
    static_assert(sizeof(UniqueKeyUnion::key_64) == sizeof(UniqueKeyUnion::UniqueKeyStruct));

    UniqueKeyUnion _key;

    friend QDataStream& ::operator<<(QDataStream& out, const zf::MessageCode& obj);
    friend QDataStream& ::operator>>(QDataStream& in, zf::MessageCode& obj);
    friend QDebug(::operator<<(QDebug debug, const zf::MessageCode& c));
};

//! Служебный интерфейс для реализации аогоритма поиска идентификаторов
class I_MessageContainsUid
{
public:
    virtual ~I_MessageContainsUid() {}

    //! Количество идентификаторов
    virtual int count_I_MessageContainsUid() const = 0;
    //! Идентификатор
    virtual Uid value_I_MessageContainsUid(int index) const = 0;

    //! Количество кодов идентификаторов
    virtual int count_I_MessageContainsUidCodes() const = 0;
    //! Код идентификатора
    virtual EntityCode value_I_MessageContainsUidCodes(int index) const = 0;
};

//! Служебный класс для быстрого поиска по идентификаторам
class MessageUidContainsHelper
{
public:
    MessageUidContainsHelper(I_MessageContainsUid* mi);

    //! Сообщение содержит информацию об идентификаторах указанного типа
    bool contains(const EntityCode& entity_code) const;
    //! Сообщение содержит информацию об указанных идентификаторах
    bool contains(const Uid& entity_uid) const;

private:
    I_MessageContainsUid* _mi = nullptr;
    // вместо bool целое, т.к. были странные баги
    mutable QHash<EntityCode, int> _contains_code;
    mutable QHash<Uid, int> _contains_uid;
};

//! Класс для универсальных сообщений
class ZCORESHARED_EXPORT Message : public I_MessageContainsUid
{
public:
    //! MessageType:Invalid
    Message();    
    //! Универсальный конструктор
    explicit Message(
        //! Вид сообщения. Для нестандартных сообщений - MessageType::General
        MessageType message_type,
        //! Произвольный код сообщения (например при взаимодействии с сервером БД, это может быть id команды)
        const MessageCode& message_code = MessageCode(),
        //! Идентификатор сообщения, на которое отвечает данное сообщение
        const MessageID& feedback_message_id = MessageID(),
        //! Данные
            const DataContainerList& data = {});
    //! Собственное сообщение с каким-то кодом
    explicit Message(
        //! Произвольный код сообщения (например при взаимодействии с сервером БД, это может быть id команды)
        const MessageCode& message_code,
        //! Идентификатор сообщения, на которое отвечает данное сообщение
        const MessageID& feedback_message_id = MessageID(),
        //! Данные
            const DataContainerList& data = {});

    //! Упрощенный конструктор, для обмена сообщениями между сущностями
    explicit Message(
        //! Код сущности, в рамках которой message_code является уникальным
        const EntityCode& entity_code,
        //! Код сообщения
        int message_code,
        //! Идентификатор сообщения, на которое отвечает данное сообщение
        const MessageID& feedback_message_id = MessageID(),
        //! Данные
            const DataContainerList& data = {});

    Message(const Message& m);

    virtual ~Message() override;

    //! Создать копию сообщения
    virtual Message* clone() const;

    Message& operator=(const Message& m);
    bool operator==(const Message& m) const;
    bool operator!=(const Message& m) const;
    bool operator<(const Message& m) const;
    void clear();

    //! Инициализировано
    bool isValid() const;

    //! Не создержит данных
    bool isEmpty() const;

    //! Уникальный идентификатор сообщения. Генерируется автоматически
    MessageID messageId() const;
    //! Тип сообщения zf::MessageType
    MessageType messageType() const;
    //! Название типа сообщения
    QString messageTypeName() const;

    //! Идентификатор сообщения, на которое отвечает данное сообщение
    MessageID feedbackMessageId() const;

    //! Произвольный код сообщения (например при взаимодействии с сервером БД, это может быть id команды)
    MessageCode messageCode() const;

    //! Данные без обработки
    const DataContainerList& rawData() const;

    //! Преобразовать в QVariant
    QVariant variant() const;
    //! Восстановить из QVariant
    static Message fromVariant(const QVariant& v);

    //! Сообщение содержит информацию об идентификаторах указанного типа
    virtual bool contains(const EntityCode& entity_code) const;
    //! Сообщение содержит информацию об указанных идентификаторах
    virtual bool contains(const Uid& entity_uid) const;
    //! Сообщение содержит информацию об идентификаторах указанного типа
    bool contains(const EntityCodeList& entity_codes) const;
    //! Сообщение содержит информацию об указанных идентификаторах
    bool contains(const UidList& entity_uids) const;

    //! Информация о сообщении для отладки
    QString toPrintable(bool show_type = true, bool show_id = true) const;
    //! Вывести содержимое для отладки
    void debPrint() const;

    //! Преобразовать в QByteArray
    QByteArray toByteArray() const;
    //! Восстановить из QByteArray
    static Message fromByteArray(const QByteArray& data, Error& error);
    //! Проверка сериализованного сообщения на поддерживаемую версию
    static bool isSupportedStreamVersion(const QByteArray& data, bool compressed);

    //! Разбить сообщение на группу сообщений по разным БД (id и тип всех сообщений будет одинаков)
    //! Если нечего разбивать (сообщение не разделяется по БД, то пустой результат)
    static QMap<DatabaseID, Message> splitByDatabase(const Message& message);
    //! Соединить однотипные сообщения, полученные от разных БД в одно (feedback_id и тип всех сообщений должен совпадать)
    static Message concatByDatabase(const QList<Message>& messages);

protected:
    //! Количество идентификаторов (I_MessageContainsUid)
    int count_I_MessageContainsUid() const override;
    //! Идентификатор (I_MessageContainsUid)
    Uid value_I_MessageContainsUid(int index) const override;
    //! Количество кодов идентификаторов
    int count_I_MessageContainsUidCodes() const override;
    //! Код идентификатора
    EntityCode value_I_MessageContainsUidCodes(int index) const override;

    static Message create(
        //! Вид сообшения
        MessageType message_type,
        //! Произвольный код сообщения (например при взаимодействии с сервером БД, это может быть id команды)
        const MessageCode& message_code,
        //! Идентификатор сообщения, на которое отвечает данное сообщение
        const MessageID& feedback_message_id,
        //! Данные
        const DataContainerList& data);
    static Message create(
        //! Вид сообшения
        MessageType message_type,
        //! Произвольный код сообщения (например при взаимодействии с сервером БД, это может быть id команды)
        const MessageCode& message_code,
        //! Идентификатор сообщения, на которое отвечает данное сообщение
        const MessageID& feedback_message_id,
        //! Данные
        const DataContainer& data);

    //! Для сообщений, содержащих только QVariantList
    static Message create(MessageType messageType,
                          //! Произвольный код сообщения (например при взаимодействии с сервером БД, это может быть id команды)
                          const MessageCode& message_code,
                          //! Идентификатор сообщения, на которое отвечает данное сообщение
                          const MessageID& feedback_message_id,
                          //! Произвольные данные
                          const QVariantList& data);
    //! Для сообщений, содержащих только QVariantList
    QVariantList variantList() const;

    //! Принудительно задать ID
    Message& setMessageId(const MessageID& id);

private:
    explicit Message(MessageType messageType, const DataContainerList& data);
    //! Универсальный конструктор
    explicit Message(
        //! Вид сообшения. Для нестандартных сообщений - MessageType::General
        MessageType message_type,
        //! Произвольный код сообщения (например при взаимодействии с сервером БД, это может быть id команды)
        const MessageCode& message_code,
        //! Идентификатор сообщения, на которое отвечает данное сообщение
        const MessageID& feedback_message_id,
        //! Данные
        const DataContainerList& data,
        //! ID сообщения
        const MessageID& message_id);

    //! Разделяемые данные
    QSharedDataPointer<Message_SharedData> _d;

    //! Хэш для поиска contains
    MessageUidContainsHelper* containsHelper() const;
    std::unique_ptr<MessageUidContainsHelper> _contains;

    friend class Framework;

    friend QDataStream& ::operator<<(QDataStream& out, const zf::Message& obj);
    friend QDataStream& ::operator>>(QDataStream& in, zf::Message& obj);
};

//! Список QVariant (MessageType:VariantList)
class ZCORESHARED_EXPORT VariantListMessage : public Message
{
public:
    VariantListMessage();

    VariantListMessage(const Message& m);    
    explicit VariantListMessage(const QString& data,
                                //! Произвольный код сообщения (например при взаимодействии с сервером БД, это может быть id команды)
                                const MessageCode& message_code = MessageCode(),
                                //! Идентификатор сообщения, на которое отвечает данное сообщение
                                const MessageID& feedback_message_id = MessageID());
    VariantListMessage(const char* data,
                       //! Произвольный код сообщения (например при взаимодействии с сервером БД, это может быть id команды)
                       const MessageCode& message_code = MessageCode(),
                       //! Идентификатор сообщения, на которое отвечает данное сообщение
                       const MessageID& feedback_message_id = MessageID());
    explicit VariantListMessage(const QVariantList& data,
                                //! Произвольный код сообщения (например при взаимодействии с сервером БД, это может быть id команды)
                                const MessageCode& message_code = MessageCode(),
                                //! Идентификатор сообщения, на которое отвечает данное сообщение
                                const MessageID& feedback_message_id = MessageID());
    explicit VariantListMessage(const QVariant& data,
                                //! Произвольный код сообщения (например при взаимодействии с сервером БД, это может быть id команды)
                                const MessageCode& message_code = MessageCode(),
                                //! Идентификатор сообщения, на которое отвечает данное сообщение
                                const MessageID& feedback_message_id = MessageID());

    //! Данные
    QVariantList data() const;
    //! Перевести данные в текст
    QString dataToText() const;

    //! Создать копию сообщения
    Message* clone() const override;
};

//! Список int (MessageType:IntList)
class ZCORESHARED_EXPORT IntListMessage : public Message
{
public:
    IntListMessage();
    IntListMessage(const Message& m);    
    explicit IntListMessage(const QList<int>& data,
                            //! Произвольный код сообщения (например при взаимодействии с сервером БД, это может быть id команды)
                            const MessageCode& message_code = MessageCode(),
                            //! Идентификатор сообщения, на которое отвечает данное сообщение
                            const MessageID& feedback_message_id = MessageID());

    //! Данные
    QList<int> data() const;
    //! Создать копию сообщения
    Message* clone() const override;

    //! Перевести данные в текст
    QString dataToText() const;
};

//! Список QString (MessageType:StringList)
class ZCORESHARED_EXPORT StringListMessage : public Message
{
public:
    StringListMessage();
    StringListMessage(const Message& m);    
    explicit StringListMessage(const QStringList& data,
                               //! Произвольный код сообщения (например при взаимодействии с сервером БД, это может быть id команды)
                               const MessageCode& message_code = MessageCode(),
                               //! Идентификатор сообщения, на которое отвечает данное сообщение
                               const MessageID& feedback_message_id = MessageID());

    //! Данные
    QStringList data() const;
    //! Создать копию сообщения
    Message* clone() const override;

    //! Перевести данные в текст
    QString dataToText() const;
};

//! Сообщение с QMap<QString, QVariant> (MessageType:StringList)
class ZCORESHARED_EXPORT VariantMapMessage : public Message
{
public:
    VariantMapMessage();
    VariantMapMessage(const Message& m);
    explicit VariantMapMessage(const QMap<QString, QVariant>& data,
                               //! Произвольный код сообщения (например при взаимодействии с сервером БД, это может быть id команды)
                               const MessageCode& message_code = MessageCode(),
                               //! Идентификатор сообщения, на которое отвечает данное сообщение
                               const MessageID& feedback_message_id = MessageID());

    //! Данные
    QMap<QString, QVariant> data() const;
    //! Создать копию сообщения
    Message* clone() const override;

    //! Перевести данные в текст
    QString dataToText() const;
};

//! Список QByteArray (MessageType:ByteArrayList)
class ZCORESHARED_EXPORT ByteArrayListMessage : public Message
{
public:
    ByteArrayListMessage();
    ByteArrayListMessage(const Message& m);
    explicit ByteArrayListMessage(const QList<QByteArray>& data,
                                  //! Произвольный код сообщения (например при взаимодействии с сервером БД, это может быть id команды)
                                  const MessageCode& message_code = MessageCode(),
                                  //! Идентификатор сообщения, на которое отвечает данное сообщение
                                  const MessageID& feedback_message_id = MessageID());

    //! Данные
    QList<QByteArray> data() const;
    //! Создать копию сообщения
    Message* clone() const override;

    //! Перевести данные в текст
    QString dataToText() const;
};

//! Сообщение логического типа (MessageType:Bool)
class ZCORESHARED_EXPORT BoolMessage : public Message
{
public:
    BoolMessage();
    BoolMessage(const Message& m);
    explicit BoolMessage(bool value,
                         //! Произвольный код сообщения (например при взаимодействии с сервером БД, это может быть id команды)
                         const MessageCode& message_code = MessageCode(),
                         //! Идентификатор сообщения, на которое отвечает данное сообщение
                         const MessageID& feedback_message_id = MessageID());

    //! Данные
    bool value() const;
    //! Создать копию сообщения
    Message* clone() const override;

    //! Перевести данные в текст
    QString dataToText() const;
};

//! PropertyTable (MessageType:PropertyTable)
class ZCORESHARED_EXPORT PropertyTableMessage : public Message
{
public:
    PropertyTableMessage();
    PropertyTableMessage(const Message& m);    
    explicit PropertyTableMessage(const PropertyTable& data,
                                  //! Произвольный код сообщения (например при взаимодействии с сервером БД, это может быть id команды)
                                  const MessageCode& message_code = MessageCode(),
                                  //! Идентификатор сообщения, на которое отвечает данное сообщение
                                  const MessageID& feedback_message_id = MessageID());

    //! Данные
    PropertyTable data() const;
    //! Создать копию сообщения
    Message* clone() const override;
};

//! IntTable (MessageType:IntTable)
class ZCORESHARED_EXPORT IntTableMessage : public Message
{
public:
    IntTableMessage();
    IntTableMessage(const Message& m);    
    explicit IntTableMessage(const IntTable& data,
                             //! Произвольный код сообщения (например при взаимодействии с сервером БД, это может быть id команды)
                             const MessageCode& message_code = MessageCode(),
                             //! Идентификатор сообщения, на которое отвечает данное сообщение
                             const MessageID& feedback_message_id = MessageID());

    //! Данные
    IntTable data() const;
    //! Создать копию сообщения
    Message* clone() const override { return new IntTableMessage(*this); }
};

//! Базовый класс для сообщений, содержащих список моделей
class ZCORESHARED_EXPORT ModelListMessage : public Message
{
public:
    //! Перевести данные в текст
    QString dataToText() const;

protected:
    ModelListMessage();
    ModelListMessage(const Message& m);    
    ModelListMessage(
        //! Вид сообщения
        MessageType type,
        //! id сообщения, на который дан ответ
        const MessageID& feedback_id,
        //! Модели
        const QList<ModelPtr>& models);

    QList<ModelPtr> models() const;

    //! Количество идентификаторов
    int count_I_MessageContainsUid() const override;
    //! Идентификатор
    Uid value_I_MessageContainsUid(int index) const override;

    //! Создать копию сообщения
    Message* clone() const override;
};

//! Сообщение с ошибкой (MessageType::Error)
class ZCORESHARED_EXPORT ErrorMessage : public Message
{
public:
    ErrorMessage();
    ErrorMessage(const Message& m);    
    explicit ErrorMessage(
        //! id сообщения, на который дан ответ
        const MessageID& feedback_message_id,
        //! Ошибка
        const Error& error);
    explicit ErrorMessage(
        //! Задать идентификатор сообщения
        const MessageID& message_id,
        //! id сообщения, на который дан ответ
        const MessageID& feedback_message_id,
        //! Ошибка
        const Error& error);

    //! Ошибка
    Error error() const;

    //! Создать копию сообщения
    Message* clone() const override { return new ErrorMessage(*this); }

    //! Перевести данные в текст
    QString dataToText() const;
};

//! Базовый класс для сообщений, содержащих список идентификаторов сущностей
class ZCORESHARED_EXPORT EntityUidListMessage : public Message
{
public:
    //! Идентификаторы
    UidList entityUidList() const;
    //! Произвольные параметры
    QList<QMap<QString, QVariant>> parameters() const;
    //! Дополнительные данные
    QVariant additionalInfo() const;

    //! Перевести данные в текст
    QString dataToText() const;

protected:
    //! Количество идентификаторов
    int count_I_MessageContainsUid() const override;
    //! Идентификатор
    Uid value_I_MessageContainsUid(int index) const override;

    EntityUidListMessage();
    EntityUidListMessage(const Message& m);
    explicit EntityUidListMessage(
        //! Вид сообщения
        MessageType messageType,
        //! Идентификаторы сущностей
        const UidList& entity_uids,
        //! id сообщения, на который дан ответ
        const MessageID& feedback_message_id,
        //! Произвольные параметры
        const QList<QMap<QString, QVariant>>& parameters,
        //! Дополнительные данные
        const QVariant& additional_info);

    Message* clone() const override;
};

//! Информация о прогрессе выполнения операций
class ZCORESHARED_EXPORT ProgressMessage : public Message
{
public:
    ProgressMessage();
    ProgressMessage(const Message& m);
    explicit ProgressMessage(
        //! Процент
        int percent,
        //! Информационное сообщение
        const QString& info = {},
        //! Идентификатор сообщения, на которое отвечает данное сообщение
        const MessageID& reply_message_id = {});

    //! Процент
    int percent() const;
    //! Информационное сообщение (может быть не задано)
    QString info() const;

    //! Идентификатор сообщения, на которое отвечает данное сообщение
    MessageID replyMessageID() const;

    //! Создать копию сообщения
    Message* clone() const override;
    //! Перевести данные в текст
    QString dataToText() const;
};

//! Широковещательное сообщение
class ZCORESHARED_EXPORT BroadcastMessage : public Message
{
public:
    BroadcastMessage(const EntityCode& entityCode, int id,
        //! Произвольные данные
        const QMap<QString, QVariant>& data = {});
    BroadcastMessage(const MessageCode& messageCode,
        //! Произвольные данные
        const QMap<QString, QVariant>& data = {});
    BroadcastMessage(const zf::Message& m);
    BroadcastMessage(const BroadcastMessage& m);

    EntityCode entityCode() const;
    int id() const;

    //! Произвольные данные
    QMap<QString, QVariant> data() const;
};

inline uint qHash(const zf::MessageID& id)
{
    return id.hashValue();
}

} // namespace zf

Q_DECLARE_METATYPE(zf::Message)
Q_DECLARE_METATYPE(zf::MessagePtr)
Q_DECLARE_METATYPE(zf::MessageID)
Q_DECLARE_METATYPE(zf::MessageCode)

Z_HANDLE_OPERATORS(zf::MessageID)
