#ifndef ZF_MESSAGECHANNEL_H
#define ZF_MESSAGECHANNEL_H

#include "zf_global.h"

namespace zf
{
//! Канал диспетчера сообщений
class ZCORESHARED_EXPORT MessageChannel
{
public:
    MessageChannel() {}
    explicit MessageChannel(int id);
    MessageChannel(const MessageChannel& channel);
    MessageChannel& operator=(const MessageChannel& channel);
    bool operator==(const MessageChannel& channel) const { return _id == channel._id; }
    bool operator!=(const MessageChannel& channel) const { return !(operator==(channel)); }

    bool isValid() const { return _id >= 0; }
    //! Идентификатор канала
    int id() const { return _id; }

private:
    int _id = -1;
};

inline uint qHash(const MessageChannel& channel)
{
    return ::qHash(channel.id());
}

} // namespace zf

Q_DECLARE_METATYPE(zf::MessageChannel)

#endif // MESSAGECHANNEL_H
