#include "zf_message_box.h"
#include "ui_zf_message_box.h"
#include "zf_core.h"
#include "zf_error.h"
#include "zf_html_tools.h"
#include "zf_translation.h"

#include <QDebug>
#include <QPushButton>

#if QT_VERSION < QT_VERSION_CHECK(5, 10, 0)
#include <QDesktopWidget>
#else
#include <QScreen>
#endif

namespace zf
{
MessageBox::MessageBox(
    MessageType message_type, const QString& text, const QString& detail, const Uid& entity_uid)
    : Dialog(generateButtons(generateStandardButtons(message_type), entity_uid),
        generateButtonsText(generateStandardButtons(message_type), {}, entity_uid),
        generateButtonsRole(generateStandardButtons(message_type), {}, entity_uid))
    , _ui(new Ui::MessageBoxUI)
    , _message_type(message_type)
    , _text(text)
    , _detail(detail)
    , _entity_uid(entity_uid)
{
    init();

    _ui->detailMessageBox->setReadOnly(true);

    setWindowTitle(qApp->applicationName());

    adjustSize();
}

MessageBox::MessageBox(MessageType message_type, const QList<int>& buttons_list, const QStringList& buttons_text,
    const QList<int>& buttons_role, const QString& text, const QString& detail, const Uid& entity_uid)
    : Dialog(generateButtons(buttons_list, entity_uid), generateButtonsText(buttons_list, buttons_text, entity_uid),
        generateButtonsRole(buttons_list, buttons_role, entity_uid))
    , _ui(new Ui::MessageBoxUI)
    , _message_type(message_type)
    , _text(text)
    , _detail(detail)
    , _entity_uid(entity_uid)
{
    init();
}

MessageBox::~MessageBox()
{
    delete _ui;
}

QVBoxLayout* MessageBox::layout() const
{
    return static_cast<QVBoxLayout*>(Dialog::layout());
}

QLabel* MessageBox::iconLabel() const
{
    return _ui->iconMessageBox;
}

QWidget* MessageBox::iconWidget() const
{
    return _ui->icon_widget;
}

QFrame* MessageBox::mainFrame() const
{
    return _ui->mb_main_frame;
}

void MessageBox::insertLayout(QLayout* la)
{
    Z_CHECK_NULL(la);
    layout()->insertLayout(1, la);
}

void MessageBox::adjustDialogSize()
{
    int dialogWidth;
    int dialogHeigth;

    QSize screen_size = Utils::screenSize(QCursor::pos());

    QSize maxSize = screen_size;
    maxSize.setWidth(maxSize.width() / 2);
    maxSize.setHeight(maxSize.height() / 2);

    QSize minSize(400, 120);

    QString detailPlain = _ui->detailMessageBox->toPlainText();
    QWidget* buttonPannel = findChild<QWidget*>("buttonBoxDialog");

    if (_ui->textMessageBox->text().contains("\n") || HtmlTools::isHtml(_ui->textMessageBox->text())) {
        // Определяем размер основного текста
        QSize mainSize = Utils::stringSize(_ui->textMessageBox->text(), true);

        int mainWidth = mainSize.width() + _ui->iconMessageBox->minimumSize().width() + _ui->horizontalSpacer->minimumSize().width()
                        + _ui->mainLayoutMessageBox->spacing() * 2 + workspace()->layout()->margin() * 2 + Utils::scaleUI(10);

        // Определяем размер дополнительного текста
        QSize detailSize = !_ui->detailMessageBox->isHidden() ? Utils::stringSize(detailPlain) : QSize(0, 0);
        if (!_ui->detailMessageBox->isHidden())
            detailSize.setWidth(detailSize.width() + Utils::scaleUI(60));

        int detailWidth = detailSize.width();

        // Определяем ширину
        dialogWidth = qMax(mainWidth, detailWidth);

        // Определяем высоту
        dialogHeigth = mainSize.height() + workspace()->layout()->margin() * 2 + Utils::scaleUI(10);
        dialogHeigth += buttonPannel->height();

        if (!_ui->detailMessageBox->isHidden()) {
            dialogHeigth
                    += _ui->globalLayout->spacing() + qMax(detailSize.height(), _ui->detailMessageBox->minimumHeight());
        }

        if (dialogWidth < minSize.width())
            dialogWidth = minSize.width();
        if (dialogWidth > maxSize.width())
            dialogWidth = maxSize.width();

        if (dialogHeigth < minSize.height())
            dialogHeigth = minSize.height();
        if (dialogHeigth > maxSize.height())
            dialogHeigth = maxSize.height();

        if (_message_type == MessageType::About) {
            dialogHeigth = _ui->textMessageBox->height() + Utils::scaleUI(16) + buttonPannel->height()
                           + _ui->mb_main_frame->contentsMargins().top() + _ui->mb_main_frame->contentsMargins().bottom();
            dialogWidth = _ui->textMessageBox->width() + Utils::scaleUI(16) + _ui->iconMessageBox->minimumWidth()
                          + _ui->horizontalSpacer->minimumSize().width() + _ui->mb_main_frame->contentsMargins().left()
                          + _ui->mb_main_frame->contentsMargins().right();
        }

        setMinimumWidth(dialogWidth);
        setMinimumHeight(dialogHeigth);
        resize(dialogWidth, dialogHeigth);
        adjustSize();

    } else {
        if (_ui->textMessageBox->text().length() >= 50)
            setMinimumWidth(minSize.width());
        else
            setMinimumWidth(Utils::stringWidth(_ui->textMessageBox->text()) + _ui->iconMessageBox->minimumWidth()
                            + _ui->horizontalSpacer->minimumSize().width() + _ui->mb_main_frame->contentsMargins().left()
                            + _ui->mb_main_frame->contentsMargins().right() + _ui->globalLayout->spacing() + Utils::scaleUI(16));

        adjustSize();
        dialogWidth = size().width();
        dialogHeigth = size().height();
        // Пропорции
        float rate = 4;
        dialogHeigth = ((float)dialogHeigth + (float)dialogWidth) / (1 + rate);
        dialogWidth = (float)dialogHeigth * rate;

        if (_message_type == MessageType::About) {
            dialogHeigth = _ui->textMessageBox->height() + Utils::scaleUI(16) + buttonPannel->height()
                           + _ui->mb_main_frame->contentsMargins().top() + _ui->mb_main_frame->contentsMargins().bottom();
            dialogWidth = _ui->textMessageBox->width() + Utils::scaleUI(16) + _ui->iconMessageBox->minimumWidth()
                          + _ui->horizontalSpacer->minimumSize().width() + _ui->mb_main_frame->contentsMargins().left()
                          + _ui->mb_main_frame->contentsMargins().right() + _ui->globalLayout->spacing();
            setMinimumHeight(dialogHeigth);
        }

        setMinimumWidth(qMax(dialogWidth, minimumWidth()));
        adjustSize();
    }
}

bool MessageBox::onButtonClick(QDialogButtonBox::StandardButton button)
{
    bool res = Dialog::onButtonClick(button);
    if (res)
        return true;

    if (_entity_uid.isValid() && button == QDialogButtonBox::Open) {
        Error error = Core::windowManager()->openWindow(_entity_uid);
        if (error.isError())
            Core::error(error);
        else
            close();
        return true;
    } else
        return false;
}

void MessageBox::showEvent(QShowEvent* e)
{
    Dialog::showEvent(e);

    if (_message_type == MessageType::About) {
        _ui->iconMessageBox->setPixmap(qApp->windowIcon().pixmap(_ui->textMessageBox->height()));
        _ui->iconMessageBox->setMinimumSize(_ui->textMessageBox->height(), _ui->textMessageBox->height());
    }
}

QList<int> MessageBox::generateStandardButtons(MessageType messageType)
{
    QList<int> res;

    switch (messageType) {
        case MessageType::Info:
        case MessageType::Warning:
        case MessageType::About:
            res << static_cast<int>(QDialogButtonBox::Ok); //-V1037
            break;
        case MessageType::Error:
            res << static_cast<int>(QDialogButtonBox::Ok);
            break;
        case MessageType::Ask:
        case MessageType::AskError:
        case MessageType::AskWarning:
            res << static_cast<int>(QDialogButtonBox::Yes) << static_cast<int>(QDialogButtonBox::No);
            break;
        case MessageType::Choice:
            res << static_cast<int>(QDialogButtonBox::Yes) << static_cast<int>(QDialogButtonBox::No)
                << static_cast<int>(QDialogButtonBox::Cancel);
            break;
        default:
            Z_HALT("Неизвестный тип диалога");
            break;
    }

    return res;
}

QList<int> MessageBox::generateButtons(const QList<int>& buttonsList, const Uid& instanceUid)
{
    QList<int> res;
    if (instanceUid.isValid()) {
        res = QList<int>({static_cast<int>(QDialogButtonBox::Open)}) + buttonsList;
    } else
        res = buttonsList;

    return res;
}

QStringList MessageBox::generateButtonsText(
    const QList<int>& buttonsList, const QStringList& buttonsText, const Uid& instanceUid)
{
    QStringList res;
    if (!instanceUid.isValid()) {
        res = buttonsText;

    } else {
        if (buttonsText.isEmpty()) {
            QList<int> buttonsRoleTmp;
            getDefaultButtonOptions(buttonsList, res, buttonsRoleTmp);
        } else
            res = buttonsText;

        res = QStringList({Core::isBootstraped() ? ZF_TR(ZFT_OPEN_DOCUMENT) : "Open document"}) + res;
    }

    return res;
}

QList<int> MessageBox::generateButtonsRole(
    const QList<int>& buttonsList, const QList<int>& buttonsRole, const Uid& instanceUid)
{
    QList<int> res;
    if (!instanceUid.isValid()) {
        res = buttonsRole;

    } else {
        if (buttonsRole.isEmpty()) {
            QStringList buttonsTextTmp;
            getDefaultButtonOptions(buttonsList, buttonsTextTmp, res);
        } else
            res = buttonsRole;

        res = QList<int>({QDialogButtonBox::ResetRole}) + res;
    }

    return res;
}

void MessageBox::setText(const QString& text, const QString& detail)
{
    if (HtmlTools::isHtml(text.trimmed())) {
        _ui->textMessageBox->setTextFormat(Qt::AutoText);
        _ui->textMessageBox->setText(HtmlTools::correct(text.trimmed()));
    } else {
        _ui->textMessageBox->setTextFormat(Qt::PlainText);
        _ui->textMessageBox->setText(HtmlTools::plain(text.trimmed()));
    }

    bool detailVisible = !detail.trimmed().isEmpty();
    if (detailVisible) {
        if (HtmlTools::isHtml(detail.trimmed()))
            _ui->detailMessageBox->setHtml(HtmlTools::correct(detail.trimmed()));
        else {
            QString s = HtmlTools::plain(detail.trimmed());
            if (comp(text.trimmed(), s.trimmed()))
                detailVisible = false;
            else
                _ui->detailMessageBox->setPlainText(s);
        }
    }

    _ui->detailMessageBox->setVisible(detailVisible);
    setBottomLineVisible(!detailVisible);
}

void MessageBox::setType(MessageBox::MessageType messageType)
{
    QIcon icon;
    int size = Z_PM(PM_LargeIconSize);

    switch (messageType) {
        case MessageType::Info:
            icon = Utils::resizeSvg(":/share_icons/blue/info.svg", size);
            break;
        case MessageType::Warning:
        case MessageType::AskWarning:
            icon = Utils::resizeSvg(":/share_icons/warning.svg", size);
            break;
        case MessageType::Error:
        case MessageType::AskError:
            icon = Utils::resizeSvg(":/share_icons/red/important.svg", size);
            break;
        case MessageType::Ask:
        case MessageType::Choice:
            icon = Utils::resizeSvg(":/share_icons/help.svg", size);
            break;
        case MessageType::About:
            size = Utils::stringSize(_ui->textMessageBox->text()).height();
            icon = qApp->windowIcon();
            break;
        default:
            Z_HALT("Неизвестный тип диалога");
            break;
    }

    _ui->iconMessageBox->setPixmap(icon.pixmap(size));
    _ui->iconMessageBox->setMinimumSize(size, size);
}

void MessageBox::init()
{
    Z_CHECK_X(!_text.trimmed().isEmpty(), "Попытка отобразить MessageBox без текста");
    _ui->setupUi(workspace());

    if (Core::isBootstraped())
        Core::registerNonParentWidget(this);

    if (_message_type == MessageType::About)
        _ui->textMessageBox->setWordWrap(false);

    QString s = _detail;
    s.replace('\t', "");
    setText(_text, s);

    setType(_message_type);
    setModal(true);

    setAutoSaveConfiguration(false);

    if (_message_type == MessageType::About || _detail.isEmpty())
        setBottomLineVisible(false);
}
} // namespace zf
