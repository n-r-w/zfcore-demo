#pragma once

#include <QAbstractNativeEventFilter>

namespace zf
{
class NativeEventFilter : public QAbstractNativeEventFilter
{
public:
    bool nativeEventFilter(const QByteArray& eventType, void* message, long*) override;
};

} // namespace zf
