#include "zf_app_message_receiver.h"

#include "zf_core.h"
#include "zf_utils.h"

namespace zf
{
AppMessageReceiver::AppMessageReceiver(SingleAppInstance* app)    
{
    Z_CHECK_NULL(app);
    connect(app, &SingleAppInstance::sg_request, this, &AppMessageReceiver::sl_messageReceived);
}

AppMessageReceiver::~AppMessageReceiver()
{
}

void AppMessageReceiver::sl_messageReceived(SingleAppInstance::Command command)
{
    if (command == SingleAppInstance::Activate) {
        QWidget* w = Utils::getTopWindow();
        if (w)
            w->activateWindow();

    } else if (command == SingleAppInstance::Terminate) {
        exit(1);
    }
}

} // namespace zf
