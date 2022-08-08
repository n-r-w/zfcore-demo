#include "zf_change_info.h"
#include "zf_core.h"

namespace zf
{
//! Разделяемые данные для ChangeInfo
class ChangeInfo_data : public QSharedData
{
public:
    ChangeInfo_data();
    ChangeInfo_data(const ChangeInfo_data& d);
    ChangeInfo_data(const Message& _message);
    ~ChangeInfo_data();

    bool valid = false;
    Message message;
};

ChangeInfo_data::ChangeInfo_data()
{
}

ChangeInfo_data::ChangeInfo_data(const ChangeInfo_data& d)
    : QSharedData(d)
{
}

ChangeInfo_data::ChangeInfo_data(const Message& _message)
{
    valid = true;
    message = _message;
}

ChangeInfo_data::~ChangeInfo_data()
{
}

ChangeInfo::ChangeInfo()
    : _d(new ChangeInfo_data)
{
}

ChangeInfo::ChangeInfo(const ChangeInfo& i)
    : _d(i._d)
{
}

ChangeInfo::ChangeInfo(const Message& m)
    : _d(new ChangeInfo_data(m))
{
}

ChangeInfo::~ChangeInfo()
{
}

ChangeInfo& ChangeInfo::operator=(const ChangeInfo& i)
{
    if (this != &i)
        _d = i._d;
    return *this;
}

bool ChangeInfo::isValid() const
{
    return _d->valid;
}

void ChangeInfo::clear()
{
    *this = ChangeInfo();
}

const Message& ChangeInfo::message() const
{
    return _d->message;
}

ChangeInfo ChangeInfo::compress(const ChangeInfo& old_info, const ChangeInfo& new_info)
{
    Z_CHECK(old_info.isValid());
    Z_CHECK(new_info.isValid());

    if (!old_info.message().isValid() || !new_info.message().isValid()
        || old_info.message().messageType() != new_info.message().messageType())
        return ChangeInfo();

    if (old_info.message().messageType() == MessageType::DBEventEntityChanged) {
        DBEventEntityChangedMessage old_msg(old_info.message());
        DBEventEntityChangedMessage new_msg(new_info.message());
        if (old_msg.entityUids().toSet() == new_msg.entityUids().toSet())
            return old_info;

    } else if (old_info.message().messageType() == MessageType::DBEventEntityCreated) {
        DBEventEntityCreatedMessage old_msg(old_info.message());
        DBEventEntityCreatedMessage new_msg(new_info.message());
        if (old_msg.entityUids().toSet() == new_msg.entityUids().toSet())
            return old_info;

    } else if (old_info.message().messageType() == MessageType::DBEventEntityRemoved) {
        DBEventEntityRemovedMessage old_msg(old_info.message());
        DBEventEntityRemovedMessage new_msg(new_info.message());
        if (old_msg.entityUids().toSet() == new_msg.entityUids().toSet())
            return old_info;
    }

    return ChangeInfo();
}

} // namespace zf
