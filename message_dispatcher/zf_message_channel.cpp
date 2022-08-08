#include "zf_message_channel.h"

namespace zf
{
MessageChannel::MessageChannel(int id)
    : _id(id)
{
}

MessageChannel::MessageChannel(const MessageChannel& channel)
    : _id(channel._id)
{
}

MessageChannel& MessageChannel::operator=(const MessageChannel& channel)
{
    _id = channel._id;
    return *this;
}

} // namespace zf
