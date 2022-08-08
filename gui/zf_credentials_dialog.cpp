#include "zf_credentials_dialog.h"
#include "ui_zf_credentials_dialog.h"
#include "zf_translation.h"
#include "zf_colors_def.h"
#include "NcFramelessHelper.h"

#include <QPushButton>
#include <QApplication>
#include <QKeyEvent>

namespace zf
{
CredentialsDialog::CredentialsDialog(const QString& caption, const QIcon& logo)
    : QWidget()
    , ui(new Ui::CredentialsDialogUI)
    , _logo(logo)
{
    ui->setupUi(this);

    setWindowModality(Qt::WindowModality::ApplicationModal);
    setWindowTitle(caption);
    setWindowIcon(logo);    
    setMessage("");

    ui->buttonBox->button(QDialogButtonBox::Ok)->setText(ZF_TR(ZFT_ENTER));
    ui->buttonBox->button(QDialogButtonBox::Ok)->setIcon(QIcon(":/share_icons/green/check.svg"));

    ui->buttonBox->button(QDialogButtonBox::Cancel)->setText(ZF_TR(ZFT_CANCEL));
    ui->buttonBox->button(QDialogButtonBox::Cancel)->setIcon(QIcon(":/share_icons/red/close.svg"));

    connect(ui->le_login, &QLineEdit::textEdited, this, &CredentialsDialog::testEnabled);
    connect(ui->le_password, &QLineEdit::textEdited, this, &CredentialsDialog::testEnabled);

    connect(ui->buttonBox, &QDialogButtonBox::clicked, this, [&](QAbstractButton* button) {
        if (ui->buttonBox->buttonRole(button) == QDialogButtonBox::AcceptRole)
            _result = QDialogButtonBox::StandardButton::Ok;
        else
            _result = QDialogButtonBox::StandardButton::Cancel;
        _main_loop.quit();
    });

    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, [&]() {
        _result = QDialogButtonBox::StandardButton::Cancel;
        _main_loop.quit();
    });

    // Избавляемся от рамки
    _no_frame = new NcFramelessHelper();
    _no_frame->setWidgetMovable(true);
    _no_frame->setWidgetResizable(false);
    _no_frame->activateOn(this);

#ifdef Q_OS_WIN
    setAttribute(Qt::WA_TranslucentBackground, true);
#endif

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

    testEnabled();
}

CredentialsDialog::~CredentialsDialog()
{
    delete _no_frame;
    delete ui;
}

Credentials CredentialsDialog::run(const Credentials& credentials)
{
    ui->le_login->setText(credentials.login());
    ui->le_password->setText(credentials.password());
    testEnabled();

    Utils::centerWindow(this);

    show();
    _main_loop.exec();

    if (_result == QDialogButtonBox::StandardButton::Ok)
        return this->credentials();

    return Credentials();
}

Credentials CredentialsDialog::credentials() const
{
    return Credentials(ui->le_login->text(), ui->le_password->text());
}

void CredentialsDialog::setMessage(const QString& s)
{
    ui->message->setText(s);
    ui->message->setHidden(s.isEmpty());
}

bool CredentialsDialog::event(QEvent* event)
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

void CredentialsDialog::showEvent(QShowEvent* event)
{
    QWidget::showEvent(event);

    adjustSize();
    Utils::centerWindow(this);

    if (!ui->le_login->text().isEmpty())
        ui->le_password->setFocus();

    _logo_size = qMax(_logo_size, height());
    ui->lb_logo->setPixmap(_logo.pixmap(_logo_size));
}

void CredentialsDialog::testEnabled()
{
    ui->buttonBox->button(QDialogButtonBox::Ok)
        ->setEnabled(!ui->le_login->text().simplified().isEmpty() && !ui->le_password->text().simplified().isEmpty());
}

} // namespace zf
