#pragma once

#include "zf_core.h"
#include <QObject>

class QLocalServer;

namespace zf
{
//! Проверка на наличие второй копии приложения
class ZCORESHARED_EXPORT SingleAppInstance : public QObject
{
    Q_OBJECT
public:
    SingleAppInstance();
    ~SingleAppInstance();

    enum Command
    {
        //! Завершить копию приложения
        Terminate,
        //! Запросить наличие копии и если запущена, то не запускать локальный сервер
        //! Если запущен, то запросить у него активацию приложения
        Activate,
        //! Запросить наличие копии и если запущена, то не запускать локальный сервер
        Request
    };

    //! Запустить локальный сервер, обеспечивающий одну копию приложения
    //! Если false, то другой объект SingleAppInstance с таким именем уже запущен
    //! Terminate - результат всегда true
    bool start(const QString& app_code,
            //! Команда
            Command command,
            //! Период ожидания в мс
            int wait_period = 500);
    //! Запущен локальный сервер
    bool isStarted() const;
    //! Код запущенного локального сервера
    QString code() const;

signals:
    //! Пришел запрос от другого экземпляра
    void sg_request(zf::SingleAppInstance::Command command);

private slots:
    //! Пришел запрос от другого экземпляра
    void sl_newLocalConnection();

private:
    void clear();

    static QString commandToString(Command c);
    static Command stringToCommand(const QString& s);

    QLocalServer* _localServer = nullptr;
    QString _app_code;
    int _wait_period = -1;
};

} // namespace zf
