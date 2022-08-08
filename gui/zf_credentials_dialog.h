#pragma once

#include "zf_dialog.h"
#include "zf_core.h"
#include "zf_database_credentials.h"

namespace Ui
{
class CredentialsDialogUI;
}

namespace zf
{
class NcFramelessHelper;

class ZCORESHARED_EXPORT CredentialsDialog : public QWidget
{
    Q_OBJECT

public:
    explicit CredentialsDialog(const QString& caption, const QIcon& logo);
    ~CredentialsDialog();

    Credentials run(
        //! Значения для инициализации
        const Credentials& credentials);

    //! Что ввел пользователь
    Credentials credentials() const;

    //! Установить текст
    void setMessage(const QString& s);

protected:
    bool event(QEvent* event) override;
    void showEvent(QShowEvent* event) override;

private slots:
    void testEnabled();

private:
    Ui::CredentialsDialogUI* ui = nullptr;
    NcFramelessHelper* _no_frame = nullptr;
    QEventLoop _main_loop;
    QDialogButtonBox::StandardButton _result = QDialogButtonBox::StandardButton::NoButton;
    QIcon _logo;
    int _logo_size = -1;
};

} // namespace zf
