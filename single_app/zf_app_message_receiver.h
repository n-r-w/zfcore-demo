#pragma once

#include "zf_single_app.h"
class QMainWindow;

namespace zf
{
//! Обработка сообщений от других экземпляров данной программы
class ZCORESHARED_EXPORT AppMessageReceiver : public QObject
{
    Q_OBJECT
public:
    AppMessageReceiver(SingleAppInstance* app);
    ~AppMessageReceiver();

private slots:
    void sl_messageReceived(zf::SingleAppInstance::Command command);
};

} // namespace zf
