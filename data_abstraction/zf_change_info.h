#pragma once

#include "zf.h"
#include <QSharedDataPointer>

namespace zf
{
class Message;
class ChangeInfo_data;

//! Информация об источнике изменения
class ZCORESHARED_EXPORT ChangeInfo
{
public:
    ChangeInfo();
    ChangeInfo(const ChangeInfo& i);
    //! Изменение на основе сообщения
    ChangeInfo(const Message& m);
    ~ChangeInfo();
    ChangeInfo& operator=(const ChangeInfo& i);

    bool isValid() const;
    void clear();

    const Message& message() const;

    //! "Складывает" два изменения. Если они одинаковые, то возвращается одно. Иначе invalid
    static ChangeInfo compress(const ChangeInfo& old_info, const ChangeInfo& new_info);

private:
    //! Данные
    QSharedDataPointer<ChangeInfo_data> _d;
};
} // namespace zf

Q_DECLARE_METATYPE(zf::ChangeInfo);
