#include "zf_core_messages.h"
#include "zf_core.h"
#include "zf_data_widgets.h"
#include "zf_message_dispatcher.h"
#include "zf_framework.h"

namespace zf
{
const MessageCode MessageUpdateStatusBar::MSG_CODE_UPDATE_STATUS_BAR = 1;
const MessageCode MessageCriticalError::MSG_CODE_CRITICAL_ERROR = 2;

MessageUpdateStatusBar::MessageUpdateStatusBar(const QString& text)
    : Message(MessageType::General, MSG_CODE_UPDATE_STATUS_BAR, MessageID(), {DataContainer::createVariantProperties({text})})
{
}

MessageUpdateStatusBar::MessageUpdateStatusBar(const Message& message)
    : Message(message)
{
    Z_CHECK(!isValid() || !message.isValid() || message.messageType() == MessageType::General);
}

QString MessageUpdateStatusBar::text() const
{
    return isValid() ? rawData().at(0).value(PropertyID::def()).toString() : QString();
}

Message* MessageUpdateStatusBar::clone() const
{
    return new MessageUpdateStatusBar(*this);
}

void MessageUpdateStatusBar::updateStatusBarErrorInfo(DataWidgets* widgets, HighlightProcessor* highlight)
{
    Z_CHECK_NULL(widgets);
    Z_CHECK_NULL(highlight);

    if (!Core::mode().testFlag(CoreMode::Application) || Core::mode().testFlag(CoreMode::Test))
        return;

    QStringList text;
    if (qApp->focusWidget() != nullptr) {
        DataProperty prop = widgets->widgetProperty(qApp->focusWidget());
        if (prop.isValid()) {
            QList<HighlightItem> items = highlight->highlight()->items(prop);
            if (items.isEmpty() && prop.propertyType() == PropertyType::Dataset
                && !prop.options().testFlag(PropertyOption::SimpleDataset)) {
                QModelIndex idx = widgets->currentIndex(prop);
                if (idx.isValid()) {
                    prop = widgets->data()->propertyCell(idx);
                    items = highlight->highlight()->items(prop);
                }
            }
            for (auto& i : qAsConst(items))
                text << i.text();
        }
    }

    Core::messageDispatcher()->postMessage(zf::Core::fr(), CoreUids::SHELL, MessageUpdateStatusBar(text.join("; ")));
}

MessageCriticalError::MessageCriticalError(const Error& error)
    : Message(MessageType::General, MSG_CODE_CRITICAL_ERROR, MessageID(), {DataContainer::createVariantProperties({error.variant()})})
{
}

MessageCriticalError::MessageCriticalError(const Message& message)
    : Message(message)
{
    Z_CHECK(!isValid() || !message.isValid() || message.messageType() == MessageType::General);
}

Error MessageCriticalError::error() const
{
    return isValid() ? Error::fromVariant(rawData().at(0).value(PropertyID::def())) : Error();
}

Message* MessageCriticalError::clone() const
{
    return new MessageCriticalError(*this);
}

} // namespace zf
