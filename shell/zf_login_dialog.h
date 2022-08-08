#pragma once

#include <QDialog>
#include "zf_core.h"
#include "zf_database_credentials.h"

namespace Ui {
class LoginDialogUI;
}

namespace zf
{
class NcFramelessHelper;

class ZCORESHARED_EXPORT LoginDialog : public QWidget
{
    Q_OBJECT

public:
    explicit LoginDialog(const QString& app_name, const QIcon& logo);
    ~LoginDialog();

    void run(
        //! Значения для инициализации
        const Credentials& credentials);

    //! Что ввел пользователь
    Credentials credentials() const;

    //! Установить текст
    void setMessage(const QString& s);

protected:
    bool event(QEvent* event) override;
    void showEvent(QShowEvent* event) override;

signals:
    void sg_login(const zf::Credentials& credentials);
    void sg_cancel();

private slots:
    void testEnabled();
    void sl_buttonClicked(QAbstractButton* button);

private:
    Ui::LoginDialogUI* ui = nullptr;
    NcFramelessHelper* _no_frame = nullptr;
    QMovie* _wait_movie = nullptr;
    QIcon _logo;
    int _logo_size = -1;
};

} // namespace zf
