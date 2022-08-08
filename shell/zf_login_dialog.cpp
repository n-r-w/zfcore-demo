#include "zf_login_dialog.h"
#include "ui_zf_login_dialog.h"
#include "zf_translation.h"
#include "NcFramelessHelper.h"
#include "zf_colors_def.h"

#include <QPushButton>
#include <QApplication>
#include <QKeyEvent>
#include <QMovie>
#include <QApplication>

namespace zf
{
LoginDialog::LoginDialog(const QString& app_name, const QIcon& logo)
    : QWidget()
    , ui(new Ui::LoginDialogUI)
    , _logo(logo)
{
    ui->setupUi(this);

    setMessage("");
    ui->loader->setHidden(true);
    _wait_movie = new QMovie(":/share_icons/loader-horizontal-16.gif");
    ui->loader->setMovie(_wait_movie);

    setWindowTitle(app_name);
    setWindowIcon(logo);

    ui->user_label->setText(ZF_TR(ZFT_LOGIN));
    ui->password_label->setText(ZF_TR(ZFT_PASSWORD));

    ui->buttonBox->button(QDialogButtonBox::Ok)->setText(ZF_TR(ZFT_ENTER));
    ui->buttonBox->button(QDialogButtonBox::Ok)->setIcon(Utils::buttonIcon(":/share_icons/green/check.svg"));

    ui->buttonBox->button(QDialogButtonBox::Cancel)->setText(ZF_TR(ZFT_CANCEL));
    ui->buttonBox->button(QDialogButtonBox::Cancel)->setIcon(Utils::buttonIcon(":/share_icons/red/close.svg"));

    connect(ui->buttonBox, &QDialogButtonBox::clicked, this, &LoginDialog::sl_buttonClicked);

    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, [&]() {
        close();
        emit sg_cancel();
    });

    connect(ui->le_login, &QLineEdit::textEdited, this, &LoginDialog::testEnabled);
    connect(ui->le_password, &QLineEdit::textEdited, this, &LoginDialog::testEnabled);

#ifdef Q_OS_WIN
    setAttribute(Qt::WA_TranslucentBackground, true);
#endif

    // Избавляемся от рамки
    _no_frame = new NcFramelessHelper();
    _no_frame->setWidgetMovable(true);
    _no_frame->setWidgetResizable(false);
    _no_frame->activateOn(this);

    ui->main_frame->setStyleSheet(QString("#main_frame {"
                                          "font-size:%1pt; font-family: %2; "
                                          "border: 1px %3;"
                                          "border-top-style: solid;"
                                          "border-right-style: solid; "
                                          "border-bottom-style: solid; "
                                          "border-left-style: solid;"
                                          "background-color: %4;"
#ifdef Q_OS_WIN
                                          "border-radius: 5px;"
#endif
                                          "}")
                                      .arg(qApp->font().weight())
                                      .arg(qApp->font().family())
                                      .arg(Colors::uiLineColor(true).name())
                                      .arg(Colors::uiButtonColor().name()));

    adjustSize();
}

LoginDialog::~LoginDialog()
{
    delete _no_frame;
    delete ui;
}

void LoginDialog::run(const Credentials& credentials)
{
    ui->le_login->setText(credentials.login());
    ui->le_password->setText(credentials.password());

    testEnabled();
    show();

    ui->buttonBox->button(QDialogButtonBox::Ok)->setDefault(false);
    ui->buttonBox->button(QDialogButtonBox::Ok)->setAutoDefault(false);

    QApplication::processEvents();
    Utils::centerWindow(this);

    testEnabled();

    if (!ui->le_login->text().isEmpty())
        ui->le_password->setFocus();

    if (!credentials.login().isEmpty() && !credentials.password().isEmpty())
        QMetaObject::invokeMethod(ui->buttonBox->button(QDialogButtonBox::Ok), "clicked", Qt::QueuedConnection);
}

Credentials LoginDialog::credentials() const
{
    return Credentials(ui->le_login->text(), ui->le_password->text());
}

void LoginDialog::setMessage(const QString& s)
{
    ui->message->setText(s);
    ui->message->setHidden(s.isEmpty());
}

bool LoginDialog::event(QEvent* event)
{
    if (event->type() == QEvent::KeyPress) {
        QKeyEvent* ke = static_cast<QKeyEvent*>(event);
        if (ke->key() == Qt::Key_Enter || ke->key() == Qt::Key_Return) {
            if (ui->buttonBox->button(QDialogButtonBox::Cancel)->hasFocus()) {
                ui->buttonBox->button(QDialogButtonBox::Cancel)->click();

            } else if (ui->buttonBox->button(QDialogButtonBox::Ok)->isEnabled())
                ui->buttonBox->button(QDialogButtonBox::Ok)->click();

        } else if (ke->key() == Qt::Key_Escape) {
            ui->buttonBox->button(QDialogButtonBox::Cancel)->click();
        }
    }

    return QWidget::event(event);
}

void LoginDialog::showEvent(QShowEvent* event)
{
    QWidget::showEvent(event);

    _logo_size = qMax(_logo_size, height());
    ui->lb_logo->setPixmap(_logo.pixmap(_logo_size));
}

void LoginDialog::testEnabled()
{
    ui->buttonBox->button(QDialogButtonBox::Ok)
        ->setEnabled(!ui->le_login->text().simplified().isEmpty() && !ui->le_password->text().simplified().isEmpty());
}

void LoginDialog::sl_buttonClicked(QAbstractButton* button)
{
    if (ui->buttonBox->buttonRole(button) == QDialogButtonBox::AcceptRole) {
        if (ui->buttonBox->isHidden())
            return;

        ui->loader->setMinimumHeight(ui->buttons->height());
        ui->buttons->setHidden(true);
        ui->loader->setVisible(true);
        _wait_movie->start();

        ui->le_password->setEnabled(false);
        ui->le_login->setEnabled(false);

        ui->buttonBox->setHidden(true);

        emit sg_login(Credentials(ui->le_login->text(), ui->le_password->text()));
    }
}

} // namespace zf
