#include "zf_dialog.h"

#include "ui_zf_dialog.h"
#include "zf_core.h"
#include "zf_translation.h"
#include "zf_framework.h"
#include "zf_dialog_configuration.h"
#include "zf_graphics_effects.h"
#include "zf_objects_finder.h"
#include "zf_colors_def.h"
#include "zf_ui_size.h"
#include "zf_utils.h"
#include "zf_core.h"

#include <QCheckBox>
#include <QCloseEvent>
#include <QDebug>
#include <QGuiApplication>
#include <QLineEdit>
#include <QMainWindow>
#include <QMetaObject>
#include <QPushButton>
#include <QScreen>
#include <QStyle>
#include <QTimer>
#include <QVBoxLayout>
#include <QWindow>
#include <QPainter>
#include <QToolBox>
#include <QSplitter>

namespace zf
{
int Dialog::_execCount = 0;

Dialog::Dialog(const QList<int>& buttons_list, const QStringList& buttons_text, const QList<int>& buttons_role)
    : QDialog(Utils::getTopWindow(), Qt::Dialog | Qt::WindowCloseButtonHint)
    , _buttons_list(buttons_list)
    , _buttons_text(buttons_text)
    , _buttons_role(buttons_role)
    , _ui(new Ui::ZFDialog)
    , _is_adjusted(false)
    ,
#ifndef Q_WS_WIN
    _is_resizable(true)
    ,
#endif
    _pure_modal(false)
    , _initialized(false)
    , _objects_finder(new ObjectsFinder())
    , _object_extension(new ObjectExtension(this))
{
    Framework::internalCallbackManager()->registerObject(this, "sl_callbackManager");

    setWindowFlag(Qt::WindowMinimizeButtonHint, false);
    setWindowIcon(qApp->windowIcon());

    // Если не задана ни одна кнопка, то считаем что это ОК
    if (_buttons_list.isEmpty()) {
        _buttons_list << QDialogButtonBox::Ok;

        if (_buttons_text.isEmpty())
            _buttons_text << "Ok";

        _buttons_role.clear();
        _buttons_role << QDialogButtonBox::AcceptRole;
    }

    QStringList buttonsTextTmp;
    QList<int> buttonsRoleTmp;
    getDefaultButtonOptions(_buttons_list, buttonsTextTmp, buttonsRoleTmp);

    // Если текст не задан, то заполняем его значениями по умолчанию
    if (_buttons_text.isEmpty())
        _buttons_text = buttonsTextTmp;

    // Если роли не заданы, то заполняем их значениями по умолчанию
    if (_buttons_role.isEmpty())
        _buttons_role = buttonsRoleTmp;

    for (int i = 0; i < _buttons_list.count(); i++)
        _button_roles = _button_roles | static_cast<QDialogButtonBox::StandardButton>(_buttons_list.at(i));

    Z_CHECK(_buttons_list.count() == _buttons_text.count() && _buttons_text.count() == _buttons_role.count());

    setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);

    // Формирование базовой структуры диалога
    _ui->setupUi(this);

    _workspace = _ui->zfws;

    _objects_finder->addSource(_ui->zfws);
    _ui->zfws->installEventFilter(this);

    _ui->lineDialog->setStyleSheet(QStringLiteral("QFrame {border: 1px %1;"
                                                  "border-top-style: none; "
                                                  "border-right-style: none; "
                                                  "border-bottom-style: solid; "
                                                  "border-left-style: none;}")
                                       .arg(Colors::uiLineColor(true).name()));
    _ui->buttonBoxDialog->clear();

    // Формирование блока кнопок
    for (int i = 0; i < _buttons_list.count(); i++) {
        createButton(static_cast<QDialogButtonBox::StandardButton>(_buttons_list.at(i)),
            // Добавляем пробел, т.к. иначе иконки слишком близко
            " " + _buttons_text.at(i).trimmed(), static_cast<QDialogButtonBox::ButtonRole>(_buttons_role.at(i)));
    }

    connect(_ui->buttonBoxDialog, &QDialogButtonBox::clicked, this, &Dialog::sl_clicked);

    setWindowTitle(QApplication::applicationDisplayName());
    setModal(true);

    Framework::internalCallbackManager()->addRequest(this, Framework::DIALOG_AFTER_CREATED_KEY);
    Framework::internalCallbackManager()->addRequest(this, Framework::DIALOG_CHANGE_BOTTOM_LINE_VISIBLE);

    _ui->buttonBoxDialog->installEventFilter(this);

    connect(_objects_finder, &ObjectsFinder::sg_objectAdded, this, &Dialog::sl_containerObjectAdded);
    connect(_objects_finder, &ObjectsFinder::sg_objectRemoved, this, &Dialog::sl_containerObjectRemoved);

    requestCheckButtonsEnabled();
}

Dialog::Dialog(Dialog::Type type)
    : Dialog(buttonsListForType(type), buttonsTextForType(type), buttonsRoleForType(type))
{
}

Dialog::~Dialog()
{
    _destroying = true;

    if (workspace() != nullptr)
        Utils::safeDelete(workspace());

    delete _objects_finder;
    delete _ui;

    if (_dialog_code)
        delete _dialog_code;

    delete _object_extension;
}

bool Dialog::objectExtensionDestroyed() const
{
    return _object_extension->objectExtensionDestroyed();
}

void Dialog::objectExtensionDestroy()
{
    _object_extension->objectExtensionDestroy();
}

QVariant Dialog::objectExtensionGetData(const QString& data_key) const
{
    return _object_extension->objectExtensionGetData(data_key);
}

bool Dialog::objectExtensionSetData(const QString& data_key, const QVariant& value, bool replace) const
{
    return _object_extension->objectExtensionSetData(data_key, value, replace);
}

void Dialog::objectExtensionRegisterUseExternal(I_ObjectExtension* i)
{
    _object_extension->objectExtensionRegisterUseExternal(i);
}

void Dialog::objectExtensionUnregisterUseExternal(I_ObjectExtension* i)
{
    _object_extension->objectExtensionUnregisterUseExternal(i);
}

void Dialog::objectExtensionDeleteInfoExternal(I_ObjectExtension* i)
{
    _object_extension->objectExtensionDeleteInfoExternal(i);
}

void Dialog::objectExtensionRegisterUseInternal(I_ObjectExtension* i)
{
    _object_extension->objectExtensionRegisterUseInternal(i);
}

void Dialog::objectExtensionUnregisterUseInternal(I_ObjectExtension* i)
{
    _object_extension->objectExtensionUnregisterUseInternal(i);
}

void Dialog::open()
{
    createUI_helper();

    _pure_modal = false;
    setModal(true);
    QDialog::open();
}

int Dialog::exec()
{
    createUI_helper();

    _pure_modal = true;
    _execCount++;

    int res = QDialog::exec();

    _execCount--;

    return res;
}

bool Dialog::activateButton(QDialogButtonBox::StandardButton buttonType)
{
    checkButtonsEnabled();

    auto b = button(buttonType);
    Z_CHECK_NULL(b);
    if (b->isEnabled()) {
        b->click();
        return true;
    }

    return false;
}

void Dialog::accept()
{
    Error error = onApply();
    if (error.isError()) {
        Core::error(error);
        _processing_button = QDialogButtonBox::NoButton;
        _processing_button_role = QDialogButtonBox::NoRole;
        return;
    }

    if (buttonRoles().testFlag(QDialogButtonBox::Ok))
        done(QDialogButtonBox::Ok);
    else if (buttonRoles().testFlag(QDialogButtonBox::Save))
        done(QDialogButtonBox::Save);
    else
        QDialog::accept();
}

void Dialog::reject()
{
    onCancel();
    if (buttonRoles().testFlag(QDialogButtonBox::Cancel))
        done(QDialogButtonBox::Cancel);
    else
        QDialog::reject();
}

void Dialog::done(int code)
{
    QDialog::done(code);
}

void Dialog::click(QDialogButtonBox::StandardButton button)
{
    auto b = this->button(button);
    Z_CHECK_NULL(b);
    b->click();
}

void Dialog::replaceWidgets(DataWidgets* data_widgets)
{
    Z_CHECK_NULL(data_widgets);
    zf::Core::replaceWidgets(data_widgets, workspace());
}

void Dialog::replaceWidgets(DataWidgetsPtr data_widgets)
{
    zf::Core::replaceWidgets(data_widgets.get(), workspace());
}

void Dialog::createUI()
{
}

bool Dialog::isPureModalExists()
{
    return _execCount > 0;
}

bool Dialog::isLoading() const
{
    return _is_loading;
}

void Dialog::resize(int w, int h)
{
    resize(QSize(w, h));
}

void Dialog::resize(const QSize& size)
{
    Utils::resizeWindowToScreen(this, size, true, 10);
}

bool Dialog::isAutoSaveConfiguration() const
{
    return _auto_save_config;
}

void Dialog::setAutoSaveConfiguration(bool b)
{
    _auto_save_config = b;
}

QMap<QString, QVariant> Dialog::getConfiguration() const
{
    return QMap<QString, QVariant>();
}

Error Dialog::applyConfiguration(const QMap<QString, QVariant>& config)
{
    Q_UNUSED(config)
    return Error();
}

void Dialog::afterLoadConfiguration()
{
}

QString Dialog::dialogCode() const
{
    if (!_dialog_code)
        return metaObject()->className();
    return *_dialog_code;
}

void Dialog::setDialogCode(const QString& code)
{
    if (_dialog_code)
        delete _dialog_code;
    _dialog_code = new QString(code);
}

void Dialog::setDefaultSize(const QSize& size)
{
    _default_size = size;
}

void Dialog::setDefaultSize(int x, int y)
{
    setDefaultSize(QSize(x, y));
}

QWidget* Dialog::workspace() const
{
    return _workspace;
}

bool Dialog::isBottomLineVisible() const
{
    return !_ui->lineDialog->isHidden();
}

void Dialog::setBottomLineVisible(bool b)
{
    if (objectExtensionDestroyed())
        return;

    if (!b == _ui->lineDialog->isHidden())
        return;

    _ui->lineDialog->setHidden(!b);
    _ui->verticalSpacer->changeSize(0, b ? 6 : 0, QSizePolicy::Minimum, QSizePolicy::Fixed);

    Framework::internalCallbackManager()->addRequest(this, Framework::DIALOG_CHANGE_BOTTOM_LINE_VISIBLE);
}

void Dialog::disableDefaultAction()
{
    _disable_default_action = true;

    QObjectList buttons;
    Utils::findObjectsByClass(buttons, workspace(), "QPushButton", false, false);

    for (auto b : qAsConst(buttons)) {
        auto button = qobject_cast<QPushButton*>(b);
        button->setDefault(false);
        button->setAutoDefault(false);
    }
}

bool Dialog::isDisableDefaultAction() const
{
    return _disable_default_action;
}

void Dialog::disableEscapeClose()
{
    _disable_escape_close = true;
}

bool Dialog::isDisableEscapeClose() const
{
    return _disable_escape_close;
}

void Dialog::disableCrossClose(bool b)
{
    _disable_cross_close = b;
}

bool Dialog::isDisableCrossClose() const
{
    return _disable_cross_close;
}

bool Dialog::isResizable() const
{
#ifdef Q_WS_WIN
    return (windowFlags() & Qt::MSWindowsFixedSizeDialogHint) == 0;
#else
    return _is_resizable;
#endif
}

void Dialog::setResizable(bool b)
{
    if (b == isResizable())
        return;

#ifdef Q_WS_WIN
    if (b)
        setWindowFlags(windowFlags() & (~Qt::MSWindowsFixedSizeDialogHint));
    else
        setWindowFlags(windowFlags() | Qt::MSWindowsFixedSizeDialogHint);
#else
    if (b) {
        setMaximumSize(QSize());
        setMinimumSize(QSize());
        adjustDialogSize();
    } else {
        adjustDialogSize();
        setMaximumSize(size());
        setMinimumSize(size());
    }

    if (_auto_save_config)
        Core::fr()->dialogConfiguration()->apply(this);
#endif
}

QDialogButtonBox* Dialog::buttonBox() const
{
    return _ui->buttonBoxDialog;
}

QDialogButtonBox::StandardButtons Dialog::buttonRoles() const
{
    return _button_roles;
}

QList<QPushButton*> Dialog::buttons() const
{
    return _buttons;
}

QList<QPushButton*> Dialog::allButtons() const
{
    QList<QPushButton*> res;
    QObjectList buttons;

    _objects_finder->findObjectsByClass(buttons, QStringLiteral("QPushButton"), false, false);

    for (auto b : qAsConst(buttons)) {
        auto button = qobject_cast<QPushButton*>(b);
        Z_CHECK_NULL(button);
        res << button;
    }

    return res;
}

void Dialog::sl_clicked(QAbstractButton* b)
{
    if (objectExtensionDestroyed())
        return;

    if (Utils::isProcessingEvent())
        return;

    if (Framework::internalCallbackManager()->isRequested(this, Framework::DIALOG_CHECK_BUTTONS_KEY)) {
        Framework::internalCallbackManager()->removeRequest(this, Framework::DIALOG_CHECK_BUTTONS_KEY);
        checkButtonsEnabled();
    }

    QPushButton* button = qobject_cast<QPushButton*>(b);
    Z_CHECK_NULL(button);

    QDialogButtonBox::StandardButton buttonCode = _button_codes.key(button);

    _processing_button = buttonCode;
    QDialogButtonBox::ButtonRole role
        = (customRole(buttonCode) == QDialogButtonBox::InvalidRole) ? _ui->buttonBoxDialog->buttonRole(button) : customRole(buttonCode);
    _processing_button_role = role;

    if (onButtonClick(buttonCode)) {
        _processing_button = QDialogButtonBox::NoButton;
        _processing_button_role = QDialogButtonBox::NoRole;
        return;
    }

    if (role == QDialogButtonBox::AcceptRole || role == QDialogButtonBox::YesRole) {        
        accept();
        setResult(static_cast<int>(buttonCode));

    } else if (role == QDialogButtonBox::RejectRole || role == QDialogButtonBox::NoRole) {        
        reject();
        setResult(static_cast<int>(buttonCode));

    } else if (role == QDialogButtonBox::ResetRole) {
        onReset();
    }

    _processing_button = QDialogButtonBox::NoButton;
    _processing_button_role = QDialogButtonBox::NoRole;
}

void Dialog::sl_callbackManager(int key, const QVariant& data)
{
    if (objectExtensionDestroyed())
        return;

    onCallbackManagerInternal(key, data);
}

void Dialog::sl_containerObjectAdded(QObject* obj)
{
    onObjectAdded(obj);
}

void Dialog::sl_containerObjectRemoved(QObject* obj)
{
    onObjectRemoved(obj);
}

void Dialog::createUI_helper() const
{
    if (_ui_created)
        return;
    _ui_created = true;

    WaitCursor wc(true);

    // создаем UI
    const_cast<Dialog*>(this)->createUI();
}

void Dialog::prepareWidget(QWidget* w)
{
    Z_CHECK_NULL(w);
    w->installEventFilter(this);
    Translator::translate(w);

    Utils::prepareWidgetScale(w);

    bool prepared = false;

    QTabWidget* tw = qobject_cast<QTabWidget*>(w);
    if (tw) {
        prepared = true;
        tw->setAutoFillBackground(true);
        // Делаем активной первую доступную вкладку
        for (int i = 0; i < tw->count(); i++) {
            tw->widget(i)->setAutoFillBackground(true);

            if (!tw->isTabEnabled(i))
                continue;

            tw->setCurrentIndex(i);
            break;
        }
    }

    if (!prepared) {
        QToolBox* toolBox = qobject_cast<QToolBox*>(w);
        if (toolBox) {
            prepared = true;
            // Делаем активной первую доступную вкладку
            for (int i = 0; i < toolBox->count(); i++) {
                if (!toolBox->isItemEnabled(i))
                    continue;

                toolBox->setCurrentIndex(i);
                break;
            }
        }
    }

    if (!prepared) {
        QAbstractItemView* item_view = qobject_cast<QAbstractItemView*>(w);
        if (item_view) {            
            QTableView* tv = qobject_cast<QTableView*>(item_view);
            if (tv != nullptr) {
                // подгоняем высоту строк под текущий шрифт
                if (auto tv_zf = qobject_cast<zf::TableView*>(item_view)) {
                    if (!tv_zf->isAutoResizeRowsHeight())
                        tv_zf->verticalHeader()->setDefaultSectionSize(UiSizes::defaultTableRowHeight());

                } else {
                    tv->verticalHeader()->setDefaultSectionSize(UiSizes::defaultTableRowHeight());
                }

            } else {
                QTreeView* tv = qobject_cast<QTreeView*>(item_view);
                if (tv != nullptr) {
                    tv->setUniformRowHeights(true);
                }
            }
        }
    }    
}

QList<int> Dialog::buttonsOkCancel() const
{
    QList<int> res;
    res << static_cast<int>(QDialogButtonBox::Ok) << static_cast<int>(QDialogButtonBox::Cancel);
    return res;
}

void Dialog::adjustDialogSize()
{
    if (_default_size.isValid())
        resize(_default_size);
    else
        adjustSize();
}

bool Dialog::isOkButtonEnabled() const
{
    const_cast<Dialog*>(this)->checkButtonsEnabled();
    QPushButton* button = okButton();
    Z_CHECK_NULL(button);
    return button->isEnabled();
}

void Dialog::setOkButtonEnabled(bool b)
{
    QPushButton* button = okButton();
    Z_CHECK_NULL(button);
    button->setEnabled(b);
}

void Dialog::setButtonHighlightFrame(QDialogButtonBox::StandardButton button, InformationType info_type)
{        
    Z_CHECK(button != QDialogButtonBox::NoButton);
    Z_CHECK(info_type != InformationType::Invalid);

    if (buttonHighlightFrame(button) == info_type)
        return;

    QPushButton* b = this->button(button);
    Z_CHECK_NULL(b);

    Utils::setHighlightWidgetFrame(b, info_type);
    _color_frames[button] = info_type;
}

InformationType Dialog::buttonHighlightFrame(QDialogButtonBox::StandardButton button) const
{
    Z_CHECK(button != QDialogButtonBox::NoButton);
    return _color_frames.value(button, InformationType::Invalid);
}

void Dialog::removeButtonHighlightFrame(QDialogButtonBox::StandardButton button)
{
    Z_CHECK(button != QDialogButtonBox::NoButton);

    QPushButton* b = this->button(button);
    Z_CHECK_NULL(b);

    Utils::removeHighlightWidgetFrame(b);
    _color_frames.remove(button);
}

QPushButton* Dialog::okButton() const
{
    QPushButton* button = _button_codes.value(QDialogButtonBox::Ok);
    if (button != nullptr)
        return button;
    button = _button_codes.value(QDialogButtonBox::Save);
    if (button != nullptr)
        return button;
    button = _button_codes.value(QDialogButtonBox::Apply);
    if (button != nullptr)
        return button;
    button = _button_codes.value(QDialogButtonBox::Yes);
    if (button != nullptr)
        return button;
    return nullptr;
}

void Dialog::toScreenCenter()
{
    QWidget* top_window = Utils::getTopWindow();
    if (top_window == nullptr)
        top_window = this;

    auto rect = Utils::screenRect(top_window->mapToGlobal(top_window->rect().center()));
    if (!rect.isNull())
        setGeometry(QStyle::alignedRect(Qt::LeftToRight, Qt::AlignCenter, size(), rect));

    if (top_window != this) {
        auto g = geometry();
        g.moveCenter(top_window->geometry().center());
        setGeometry(g);
    }
}

void Dialog::doLoad()
{
}

void Dialog::doClear()
{
}

Error Dialog::onApply()
{
    return Error();
}

bool Dialog::isApplying() const
{
    return _processing_button_role == QDialogButtonBox::AcceptRole || _processing_button_role == QDialogButtonBox::YesRole;
}

void Dialog::onCancel()
{
}

bool Dialog::isCanceling() const
{
    return _processing_button != QDialogButtonBox::NoButton
           && (_processing_button_role == QDialogButtonBox::RejectRole || _processing_button_role == QDialogButtonBox::NoRole);
}

void Dialog::onReset()
{
}

bool Dialog::isReseting() const
{
    return _processing_button_role == QDialogButtonBox::ResetRole;
}

QPushButton* Dialog::button(QDialogButtonBox::StandardButton buttonType) const
{
    Z_CHECK(buttonType != QDialogButtonBox::NoButton);
    return _button_codes.value(buttonType, nullptr);
}

QDialogButtonBox::StandardButton Dialog::role(QPushButton* button) const
{
    Z_CHECK_NULL(button);
    return _button_codes.key(button, QDialogButtonBox::NoButton);
}

QHBoxLayout* Dialog::buttonsLayout() const
{
    return _ui->zfla;
}

void Dialog::setButtonLayoutMode(QDialogButtonBox::ButtonLayout layout)
{
    _ui->buttonBoxDialog->setStyleSheet(QString("* { button-layout: %1 }").arg(layout));
}

Error Dialog::setUI(const QString& resource_name)
{
    return setUI(resource_name, workspace());
}

Error Dialog::setUI(const QString& resource_name, QWidget* target)
{
    Z_CHECK_NULL(target);
    Error error;
    QWidget* form = Utils::openUI(resource_name, error);
    if (error.isError())
        return error;

    // устанавливаем форму в рабочую область
    target->setLayout(new QVBoxLayout);
    target->layout()->setObjectName("zflui");
    target->layout()->setMargin(0);
    target->layout()->addWidget(form);

    return Error();
}

bool Dialog::onButtonClick(QDialogButtonBox::StandardButton button)
{
    Q_UNUSED(button)
    return false;
}

bool Dialog::onCloseByCross()
{
    return true;
}

QDialogButtonBox::StandardButton Dialog::processingButton() const
{
    return _processing_button;
}

QDialogButtonBox::ButtonRole Dialog::processingButtonRole() const
{
    return _processing_button_role;
}

void Dialog::beforePopup()
{
}

void Dialog::afterPopup()
{
}

void Dialog::beforeHide()
{
}

void Dialog::afterHide()
{
}

void Dialog::afterCreateDialog()
{
}

void Dialog::requestCheckButtonsEnabled()
{
    if (objectExtensionDestroyed())
        return;
    Framework::internalCallbackManager()->addRequest(this, Framework::DIALOG_CHECK_BUTTONS_KEY);
}

void Dialog::checkButtonsEnabled()
{
}

void Dialog::onCallbackManagerInternal(int key, const QVariant& data)
{
    Q_UNUSED(data)

    if (key == Framework::DIALOG_AFTER_CREATED_KEY) {
        afterCreatedInternal();

    } else if (key == Framework::DIALOG_CHECK_BUTTONS_KEY) {
        checkButtonsEnabled();

    } else if (key == Framework::DIALOG_INIT_TAB_ORDER_KEY) {
        initTabOrder();

    } else if (key == Framework::DIALOG_CHANGE_BOTTOM_LINE_VISIBLE) {
        onChangeBottomLineVisible();

    } else if (key == Framework::DIALOG_AFTER_LOAD_CONFIG_KEY) {
        afterLoadConfiguration();
    }
}

QDialogButtonBox::ButtonRole Dialog::customRole(QDialogButtonBox::StandardButton button) const
{
    return _role_mapping.value(button, QDialogButtonBox::InvalidRole);
}

void Dialog::setVisible(bool visible)
{
    if (isVisible() == visible)
        return;

    if (visible) {
        _is_loading = true;

        _objects_finder->setBaseName(metaObject()->className());
        zf::translate(workspace());

        beforePopup();
        doLoad();

        if (!_initialized) {
            _initialized = true;

            QObjectList ls;
            Utils::findObjectsByClass(ls, this, "QWidget", false, false);
            for (QObject* obj : qAsConst(ls)) {
                QWidget* w = qobject_cast<QWidget*>(obj);
                // Устанавливаем для всех виджетов фильтр событий
                w->installEventFilter(this);
            }
        }        

    } else {
        if (_auto_save_config)
            Core::fr()->dialogConfiguration()->load(this);
        beforeHide();
        _pure_modal = false;
    }

    QDialog::setVisible(visible);

    if (visible) {
        afterPopup();

        if (_disable_default_action)
            disableDefaultAction(); // Qt зачем то переназначает свойство default после показа диалога

        _is_loading = false;

        Utils::prepareWidgetScale(this);
        Utils::prepareWidgetScale(workspace());

        emit sg_show();

    } else {
        afterHide();
        doClear();

        emit sg_hide();
    }
}

bool Dialog::eventFilter(QObject* obj, QEvent* e)
{
    if (objectExtensionDestroyed())
        return QDialog::eventFilter(obj, e);

    if (e->type() == QEvent::KeyPress) {
        QKeyEvent* event = static_cast<QKeyEvent*>(e);
        if ((event->key() == Qt::Key_Escape) && event->modifiers() == Qt::NoModifier) {
            if (!_disable_escape_close) {                
                reject();
                setResult(static_cast<int>(QDialogButtonBox::Cancel));
            }
            return true;
        }
    }

    if (e->type() == QEvent::ChildAdded) {
        // инициализируем динамически добавляемые виджеты
        QChildEvent* chEvent = static_cast<QChildEvent*>(e);
        if (chEvent->child()->inherits("QWidget")) {
            QWidget* w = static_cast<QWidget*>(chEvent->child());
            Z_CHECK_NULL(w);
            prepareWidget(w);
            QObjectList ls;
            Utils::findObjectsByClass(ls, chEvent->child(), QStringLiteral("QWidget"), false, false);
            for (QObject* obj : qAsConst(ls)) {
                QWidget* c_w = static_cast<QWidget*>(obj);
                prepareWidget(c_w);
            }
        }
    }

    if (e->type() == QEvent::ParentChange || e->type() == QEvent::Show || e->type() == QEvent::ShowToParent) {
        requestInitTabOrder();
    }

    bool res = QDialog::eventFilter(obj, e);

    if (obj == _ui->buttonBoxDialog) {
        // борьба с идиотами из Qt, которые постоянно пытаются назначить кнопку по умолчанию внутри QDialogButtonBox
        if (isDisableDefaultAction()) {
            auto buttons = _ui->buttonBoxDialog->buttons();
            for (auto b : qAsConst(buttons)) {
                auto push_b = qobject_cast<QPushButton*>(b);
                if (push_b != nullptr)
                    push_b->setDefault(false);
            }
        }
    }

    return res;
}

void Dialog::moveEvent(QMoveEvent* e)
{
    QDialog::moveEvent(e);
}

void Dialog::resizeEvent(QResizeEvent* e)
{
    QDialog::resizeEvent(e);
}

bool Dialog::event(QEvent* e)
{
#if QT_VERSION == QT_VERSION_CHECK(5, 12, 4)
    // workaround for https://bugreports.qt.io/browse/QTBUG-76509 causing blank windows when window re-used
    if (e->type() == QEvent::PlatformSurface)
        return true;
#endif

    return QDialog::event(e);
}

void Dialog::showEvent(QShowEvent* e)
{
    QDialog::showEvent(e);

    if (!_is_adjusted) {
        adjustDialogSize();
        if (_auto_save_config)
            Core::fr()->dialogConfiguration()->apply(this);
        else
            toScreenCenter();
        _is_adjusted = true;
    }
}

void Dialog::closeEvent(QCloseEvent* e)
{
    if (_disable_cross_close || !onCloseByCross())
        e->ignore();
    else
        QDialog::closeEvent(e);
}

void Dialog::keyPressEvent(QKeyEvent* e)
{
    // диалог зачем то сам переназначает default у кнопок. этот код выдран из QDialog::keyPressEvent и запрещает
    // ему это непотребство
    if (!e->modifiers() || (e->modifiers() & Qt::KeypadModifier && e->key() == Qt::Key_Enter)) {
        switch (e->key()) {
            case Qt::Key_Enter:
            case Qt::Key_Return: {
                if (isDisableDefaultAction()) {
                    auto list = allButtons();
                    for (auto b : qAsConst(list)) {
                        if (b->isDefault() && b->isVisible()) {
                            if (b->isEnabled()) {
                                e->ignore();
                                return;
                            }
                        }
                    }
                }
            }
        }
    }

    QDialog::keyPressEvent(e);
}

bool Dialog::nativeEvent(const QByteArray& eventType, void* message, long* result)
{
    return QDialog::nativeEvent(eventType, message, result);
}

void Dialog::defaultButtonOptions(QDialogButtonBox::StandardButton button, QString& text,
                                  QDialogButtonBox::ButtonRole& role, QIcon& icon)
{
    icon = QIcon();

    switch (button) {
        case QDialogButtonBox::Ok:
            text = Core::isBootstraped() ? ZF_TR(ZFT_CLOSE) : "Close";
            role = QDialogButtonBox::AcceptRole;
            //            icon = QIcon(":/share_icons/blue/check.svg");
            break;
        case QDialogButtonBox::Save:
            text = Core::isBootstraped() ? ZF_TR(ZFT_SAVE) : "Save";
            role = QDialogButtonBox::AcceptRole;
            //            icon = QIcon(":/share_icons/blue/save.svg");
            break;
        case QDialogButtonBox::Open:
            text = Core::isBootstraped() ? ZF_TR(ZFT_OPEN) : "Open";
            role = QDialogButtonBox::AcceptRole;
            //            icon = QIcon(":/share_icons/blue/document.svg");
            break;
        case QDialogButtonBox::Cancel:
            text = Core::isBootstraped() ? ZF_TR(ZFT_CANCEL) : "Cancel";
            role = QDialogButtonBox::RejectRole;
            //            icon = QIcon(":/share_icons/red/close.svg");
            break;
        case QDialogButtonBox::Close:
            text = Core::isBootstraped() ? ZF_TR(ZFT_CLOSE) : "Close";
            role = QDialogButtonBox::AcceptRole;
            //            icon = QIcon(":/share_icons/red/close.svg");
            break;
        case QDialogButtonBox::Apply:
            text = Core::isBootstraped() ? ZF_TR(ZFT_APPLY) : "Apply";
            role = QDialogButtonBox::ApplyRole;
            //            icon = QIcon(":/share_icons/blue/check.svg");
            break;
        case QDialogButtonBox::Reset:
            text = Core::isBootstraped() ? ZF_TR(ZFT_RESET) : "Reset";
            role = QDialogButtonBox::ResetRole;
            //            icon = QIcon(":/share_icons/blue/reload.svg");
            break;
        case QDialogButtonBox::Help:
            text = Core::isBootstraped() ? ZF_TR(ZFT_HELP) : "Help";
            role = QDialogButtonBox::HelpRole;
            //            icon = QIcon(":/share_icons/help.svg");
            break;
        case QDialogButtonBox::Discard:
            text = Core::isBootstraped() ? ZF_TR(ZFT_DISCARD) : "Discard";
            role = QDialogButtonBox::DestructiveRole;
            //            icon = QIcon(":/share_icons/red/close.svg");
            break;
        case QDialogButtonBox::Yes:
            text = Core::isBootstraped() ? ZF_TR(ZFT_YES) : "Yes";
            role = QDialogButtonBox::AcceptRole;
            //            icon = QIcon(":/share_icons/blue/check.svg");
            break;
        case QDialogButtonBox::No:
            text = Core::isBootstraped() ? ZF_TR(ZFT_NO) : "No";
            role = QDialogButtonBox::RejectRole;
            //            icon = QIcon(":/share_icons/red/close.svg");
            break;
        case QDialogButtonBox::YesToAll:
            text = Core::isBootstraped() ? ZF_TR(ZFT_YES_ALL) : "Yes for all";
            role = QDialogButtonBox::AcceptRole;            
            break;
        case QDialogButtonBox::NoToAll:
            text = Core::isBootstraped() ? ZF_TR(ZFT_NO_ALL) : "No for all";
            role = QDialogButtonBox::RejectRole;            
            break;
        case QDialogButtonBox::SaveAll:
            text = Core::isBootstraped() ? ZF_TR(ZFT_SAVE_ALL) : "Save all";
            role = QDialogButtonBox::AcceptRole;            
            break;
        case QDialogButtonBox::Abort:
            text = Core::isBootstraped() ? ZF_TR(ZFT_ABORT) : "Abort";
            role = QDialogButtonBox::RejectRole;
            //            icon = QIcon(":/share_icons/red/important.svg");
            break;
        case QDialogButtonBox::Retry:
            text = Core::isBootstraped() ? ZF_TR(ZFT_RETRY) : "Retry";
            //            role = QDialogButtonBox::AcceptRole;
            icon = QIcon("");
            break;
        case QDialogButtonBox::Ignore:
            text = Core::isBootstraped() ? ZF_TR(ZFT_IGNORE) : "Ignore";
            role = QDialogButtonBox::AcceptRole;
            //            icon = QIcon("");
            break;
        case QDialogButtonBox::RestoreDefaults:
            text = Core::isBootstraped() ? ZF_TR(ZFT_RESTORE_DEFAULTS) : "Restore defaults";
            role = QDialogButtonBox::ResetRole;
            //            icon = QIcon(":/share_icons/blue/reload.svg");
            break;
        default:
            role = QDialogButtonBox::InvalidRole;
            Z_CHECK(false);
            break;
    }

    // кастомизация иконок в zf_proxy_style.h
    QStyle::StandardPixmap s_pixmap = QStyle::SP_CustomBase;

    // взято из QDialogButtonBoxPrivate::createButton
    switch (button) {
        case QDialogButtonBox::Ok:
            s_pixmap = QStyle::SP_DialogOkButton;
            break;
        case QDialogButtonBox::Save:
            s_pixmap = QStyle::SP_DialogSaveButton;
            break;
        case QDialogButtonBox::Open:
            s_pixmap = QStyle::SP_DialogOpenButton;
            break;
        case QDialogButtonBox::Cancel:
            s_pixmap = QStyle::SP_DialogCancelButton;
            break;
        case QDialogButtonBox::Close:
            s_pixmap = QStyle::SP_DialogCloseButton;
            break;
        case QDialogButtonBox::Apply:
            s_pixmap = QStyle::SP_DialogApplyButton;
            break;
        case QDialogButtonBox::Reset:
            s_pixmap = QStyle::SP_DialogResetButton;
            break;
        case QDialogButtonBox::Help:
            s_pixmap = QStyle::SP_DialogHelpButton;
            break;
        case QDialogButtonBox::Discard:
            s_pixmap = QStyle::SP_DialogDiscardButton;
            break;
        case QDialogButtonBox::Yes:
            s_pixmap = QStyle::SP_DialogYesButton;
            break;
        case QDialogButtonBox::No:
            s_pixmap = QStyle::SP_DialogNoButton;
            break;
        case QDialogButtonBox::YesToAll:
            s_pixmap = QStyle::SP_DialogYesToAllButton;
            break;
        case QDialogButtonBox::NoToAll:
            s_pixmap = QStyle::SP_DialogNoToAllButton;
            break;
        case QDialogButtonBox::SaveAll:
            s_pixmap = QStyle::SP_DialogSaveAllButton;
            break;
        case QDialogButtonBox::Abort:
            s_pixmap = QStyle::SP_DialogAbortButton;
            break;
        case QDialogButtonBox::Retry:
            s_pixmap = QStyle::SP_DialogRetryButton;
            break;
        case QDialogButtonBox::Ignore:
            s_pixmap = QStyle::SP_DialogIgnoreButton;
            break;
        case QDialogButtonBox::RestoreDefaults:
            s_pixmap = QStyle::SP_RestoreDefaultsButton;
            break;

        default:
            break;
    }

    if (s_pixmap != QStyle::SP_CustomBase)
        icon = qApp->style()->standardIcon(s_pixmap);
}

void Dialog::getDefaultButtonOptions(const QList<int>& buttonsList, QStringList& buttonsTexts, QList<int>& buttonsRoles)
{
    buttonsTexts.clear();
    buttonsRoles.clear();

    for (int i = 0; i < buttonsList.count(); i++) {
        QString text;
        QDialogButtonBox::ButtonRole role;
        QIcon icon;
        defaultButtonOptions(static_cast<QDialogButtonBox::StandardButton>(buttonsList.at(i)), text, role, icon);
        buttonsTexts << text;
    }

    for (int i = 0; i < buttonsList.count(); i++) {
        QString text;
        QDialogButtonBox::ButtonRole role;
        QIcon icon;
        defaultButtonOptions(static_cast<QDialogButtonBox::StandardButton>(buttonsList.at(i)), text, role, icon);
        buttonsRoles << role;
    }
}

bool Dialog::isSaveSplitterConfiguration(const QSplitter* splitter) const
{
    Q_UNUSED(splitter)
    return true;
}

void Dialog::embedViewAttached(View* view)
{
    Q_UNUSED(view)
}

void Dialog::embedViewDetached(View* view)
{
    Q_UNUSED(view)
}

void Dialog::onEmbedViewError(View* view, ObjectActionType type, const Error& error)
{
    Q_UNUSED(view)
    Q_UNUSED(type);

    // ошибку показываем только если она вне операций сохранения и т.п.
    if (_processing_button == QDialogButtonBox::NoButton && _processing_button_role == QDialogButtonBox::NoRole)
        Core::error(error);
}

void Dialog::onObjectAdded(QObject* obj)
{
    Q_UNUSED(obj)
}

void Dialog::onObjectRemoved(QObject* obj)
{
    Q_UNUSED(obj)
}

void Dialog::onChangeBottomLineVisible()
{
}

void Dialog::createButton(QDialogButtonBox::StandardButton button, const QString& text,
                          QDialogButtonBox::ButtonRole role)
{
    QString t;
    QDialogButtonBox::ButtonRole r;
    QIcon icon;
    defaultButtonOptions(button, t, r, icon);

    QPushButton* btn = new QPushButton(text);
    btn->setObjectName(QStringLiteral("button_%1_%2").arg(button).arg(role));
    _buttons << btn;
    _ui->buttonBoxDialog->addButton(btn, role);
    btn->setIcon(icon);
    _button_codes[button] = btn;

    btn->installEventFilter(this);
}

QList<QObject*> Dialog::findObjectsHelper(const QStringList& path, const char* class_name) const
{
    return _objects_finder->findObjects(path, class_name);
}

void Dialog::afterCreatedInternal()
{
    // инициализируем все виджеты
    QObjectList ls;
    Utils::findObjectsByClass(ls, workspace(), QStringLiteral("QWidget"), false, false);
    for (QObject* obj : qAsConst(ls)) {
        QWidget* w = static_cast<QWidget*>(obj);
        prepareWidget(w);
    }

    afterCreateDialog();
}

// префикс для хранения состояния сплиттеров
#define CONFIG_SPLIT_KEY_PREFIX QStringLiteral("SPLIT_")

QMap<QString, QVariant> Dialog::getConfigurationInternal() const
{
    QMap<QString, QVariant> config;

    // запоминаем положение всех сплиттеров
    QObjectList splitters;
    Utils::findObjectsByClass(splitters, workspace(), QStringLiteral("QSplitter"));
    for (QObject* obj : qAsConst(splitters)) {
        QSplitter* s = qobject_cast<QSplitter*>(obj);
        Z_CHECK_NULL(s);
        if (s->objectName().isEmpty() || !isSaveSplitterConfiguration(s))
            continue;

        config[CONFIG_SPLIT_KEY_PREFIX + s->objectName()] = QVariant::fromValue(Utils::toVariantList(s->sizes()));
    }

    return getConfiguration().unite(config);
}

Error Dialog::applyConfigurationInternal(const QMap<QString, QVariant>& config)
{
    if (!config.isEmpty()) {
        // восстанавливаем положение всех сплиттеров
        QObjectList splitters;
        Utils::findObjectsByClass(splitters, workspace(), QStringLiteral("QSplitter"));
        for (QObject* obj : qAsConst(splitters)) {
            QSplitter* s = qobject_cast<QSplitter*>(obj);
            Z_CHECK_NULL(s);
            if (s->objectName().isEmpty() || !isSaveSplitterConfiguration(s))
                continue;

            auto sizes = config.value(CONFIG_SPLIT_KEY_PREFIX + s->objectName()).value<QVariantList>();
            if (sizes.count() == s->sizes().count())
                s->setSizes(Utils::toIntList(sizes));
        }
    }

    auto error = applyConfiguration(config);

    Framework::internalCallbackManager()->addRequest(this, Framework::DIALOG_AFTER_LOAD_CONFIG_KEY);
    return error;
}

QList<int> Dialog::buttonsListForType(Dialog::Type t)
{
    switch (t) {
        case Dialog::Type::Edit:
            return {QDialogButtonBox::Ok, QDialogButtonBox::Cancel};
        case Dialog::Type::View:
            return {QDialogButtonBox::Ok};

        default:
            Z_HALT_INT;
            return {};
    }
}

QStringList Dialog::buttonsTextForType(Dialog::Type t)
{
    switch (t) {
        case Dialog::Type::Edit:
            return {ZF_TR(ZFT_SAVE), ZF_TR(ZFT_CANCEL)};
        case Dialog::Type::View:
            return {ZF_TR(ZFT_OK)};

        default:
            Z_HALT_INT;
            return {};
    }
}

QList<int> Dialog::buttonsRoleForType(Dialog::Type t)
{
    switch (t) {
        case Dialog::Type::Edit:
            return {QDialogButtonBox::AcceptRole, QDialogButtonBox::RejectRole};
        case Dialog::Type::View:
            return {QDialogButtonBox::AcceptRole};

        default:
            Z_HALT_INT;
            return {};
    }
}

void Dialog::embedViewAttachedHelper(View* view)
{
    embedViewAttached(view);
}

void Dialog::embedViewDetachedHelper(View* view)
{
    embedViewDetached(view);
}

void Dialog::requestInitTabOrder()
{
    if (objectExtensionDestroyed())
        return;
    // Автоматическая расстановка табуляции
    Framework::internalCallbackManager()->addRequest(this, Framework::DIALOG_INIT_TAB_ORDER_KEY);
}

void Dialog::initTabOrder()
{
    Z_CHECK_X(workspace()->layout() != nullptr, "workspace()->layout() not initialized");

    QList<QWidget*> tab_order;
    Utils::createTabOrder(workspace()->layout(), tab_order);
    setTabOrder(tab_order);
}

void Dialog::setTabOrder(const QList<QWidget*>& tab_order)
{
    _tab_order = tab_order;
    Utils::setTabOrder(_tab_order);
}

const QList<QWidget*>& Dialog::tabOrder() const
{
    return _tab_order;
}

} // namespace zf
