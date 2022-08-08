#include "zf_modal_window.h"
#include "ui_zf_modal_window.h"
#include "zf_core.h"
#include "zf_translation.h"
#include "zf_framework.h"
#include "zf_highlight_view.h"
#include "zf_highlight_view_controller.h"
#include "zf_colors_def.h"

#include <QCloseEvent>
#include <QPushButton>
#include <QScrollArea>

namespace zf
{
ModalWindow::ModalWindow(View* view, const ModuleWindowOptions& options)
    : HighlightDialog(HighlightDialogOption::NoAutoShowHighlight | HighlightDialogOption::SimpleHighlight, view, view, buttonsList(view), buttonsText(view),
        buttonsRole(view))
    , _ui(new Ui::ModalWindow)
    , _view(view)
    , _options(options)
{
    Z_CHECK_NULL(view);

    setDialogCode(view->configurationCode());

    _entity_uid = view->entityUid();

    //    Core::logInfo(QString("modal window start load: %1").arg(view->entityUid().toPrintable()));

    setModal(true);
    disableDefaultAction();
    disableEscapeClose();

    if (options.testFlag(ModuleWindowOption::EditWindowBottomLineHidden))
        setBottomLineVisible(false);
    else if (options.testFlag(ModuleWindowOption::EditWindowBottomLineVisible))
        setBottomLineVisible(false);
    else
        setBottomLineVisible(!view->isViewWindow());

    _ui->setupUi(body());

    const int default_margin = 3;
    body()->layout()->setContentsMargins(options.testFlag(ModuleWindowOption::LeftMargin) ? default_margin : 0,
        options.testFlag(ModuleWindowOption::TopMargin) ? default_margin : 0, options.testFlag(ModuleWindowOption::RightMargin) ? default_margin : 0,
        options.testFlag(ModuleWindowOption::BottomMargin) ? default_margin : 0);

    if (view->isViewWindow())
        setWindowIcon(view->moduleInfo().icon());

    auto win_flags = windowFlags();
    win_flags.setFlag(Qt::WindowMaximizeButtonHint);
    win_flags.setFlag(Qt::WindowMinimizeButtonHint, false);

    setWindowFlags(win_flags);
    setAcceptDrops(true);

    bootstrapViewActions();

    if (!view->isViewWindow()) {
        // подключение к информации об ошибках
        connect(view->highlight(false), &HighlightModel::sg_itemInserted, this, &ModalWindow::checkEnabled);
        connect(view->highlight(false), &HighlightModel::sg_itemRemoved, this, &ModalWindow::checkEnabled);
        connect(view->highlight(false), &HighlightModel::sg_itemChanged, this, &ModalWindow::checkEnabled);
        connect(view->highlight(false), &HighlightModel::sg_endUpdate, this, &ModalWindow::checkEnabled);
    }

    connect(view, &View::sg_operationProcessed, this, &ModalWindow::sl_operationProcessed);

    // откладываем отрисовку тулбара, чтобы не было "мельтешения" видимости/доступности
    if (!view->toolbarsWithActions(true).isEmpty()) {
        _ui->toolbar_widget->setUpdatesEnabled(false);
        createToolbar();
        Framework::internalCallbackManager()->addRequest(this, Framework::MODAL_WINDOW_CREATE_TOOLBAR_KEY);
    }

    connect(view, &View::sg_toolbarAdded, this, [&]() { createToolbar(); });
    connect(view, &View::sg_toolbarRemoved, this, [&]() { createToolbar(); });

    _ui->body_layout->addWidget(view->mainWidget());

    connect(view, &View::sg_uiBlocked, this, &ModalWindow::checkEnabled);
    connect(view, &View::sg_uiUnBlocked, this, &ModalWindow::checkEnabled);
    connect(view, &View::sg_readOnlyChanged, this, [&]() { checkEnabled(); });
    connect(view, &View::sg_editWindowOkSaveEnabledChanged, this, [&]() { checkEnabled(); });
    connect(view, &View::sg_entityNameChanged, this, &ModalWindow::sl_entityNameChanged);

    if (view->isLoading())
        connect(view, &View::sg_finishLoad, this, [&]() { requestHighlightShow(); });

    checkEnabled();
    sl_entityNameChanged();

    view->setModalWindowInterface(this);

    auto deb_print = new QPushButton("DebPrint");
    view->connect(deb_print, &QPushButton::clicked, view, [view]() { view->debPrint(); });
#ifndef QT_DEBUG
    deb_print->setHidden(true);
#endif
    buttonsLayout()->insertWidget(0, deb_print);
}

ModalWindow::~ModalWindow()
{
    if (!_view.isNull())
        _view->setModalWindowInterface(nullptr);

    if (Core::isBootstraped() && !_view.isNull()) {
        removeToolbars();
        _view->mainWidget()->setParent(nullptr);
    }

    delete _ui;

    //    Core::logInfo(QString("modal window closed: %1").arg(_entity_uid.toPrintable()));
}

Error ModalWindow::operationError() const
{
    return _operation_error;
}

ModuleWindowOptions ModalWindow::options() const
{
    return _options;
}

Error ModalWindow::invokeModalWindowOkSave(bool force_not_save)
{
    _operation_error.clear();

    if (force_not_save || (_view != nullptr && _view->isReadOnly())) {
        accept();
        return {};
    }

    // Надо зафиксировать изменения, которые есть в представлении, но не получены моделью
    Utils::fixUnsavedData();

    // если есть ошибки, то надо перейти на поле с ошибкой
    if (isErrorExists(true)) {
        checkEnabled();
        focusError();
        return _view->highlight(false)->toError(InformationType::Error);
    }

    setOkButtonEnabled(false);

    Uid persistent;
    _operation_error = _view->model()->saveSync(persistent, 0, true);
    if (_operation_error.isOk()) {
        _view->afterModalWindowSave();
        accept();

    } else {
        // ошибку отобразит представление, тут же необходимо решить надо ли нам закрыть окно при данной ошибке
        if (_operation_error.isCloseWindow()) {
            // надо принудительно закрыть окно несмотря на ошибку сохранения
            reject();
        }
    }

    setOkButtonEnabled(true);

    return _operation_error;
}

void ModalWindow::invokeModalWindowCancel(bool force)
{
    if (force) {
        reject();

    } else {
        if (buttonRoles().testFlag(QDialogButtonBox::StandardButton::Cancel))
            onButtonClick(QDialogButtonBox::Cancel);
        else
            reject(); // кнопки calcel нет, так что просто закрываем
    }
}

QDialogButtonBox* ModalWindow::modalWindowButtonBox() const
{
    return buttonBox();
}

QDialogButtonBox::StandardButtons ModalWindow::modalWindowButtonRoles() const
{
    return buttonRoles();
}

QList<QPushButton*> ModalWindow::modalWindowButtons() const
{
    return allButtons();
}

QPushButton* ModalWindow::modalWindowButton(QDialogButtonBox::StandardButton buttonType) const
{
    return button(buttonType);
}

QPushButton* ModalWindow::modalWindowButton(const OperationID& op) const
{
    Z_CHECK_NULL(_view);
    return _view->modalWindowOperationButton(op);
}

QHBoxLayout* ModalWindow::modalWindowButtonsLayout() const
{
    return buttonsLayout();
}

void ModalWindow::modalWindowSetButtonHighlightFrame(QDialogButtonBox::StandardButton button, InformationType info_type)
{
    setButtonHighlightFrame(button, info_type);
}

void ModalWindow::modalWindowSetButtonHighlightFrame(const OperationID& op, InformationType info_type)
{
    Z_CHECK(op.isValid());
    Z_CHECK(info_type != InformationType::Invalid);

    if (_view == nullptr)
        return;

    if (modalWindowButtonHighlightFrame(op) == info_type)
        return;

    QPushButton* b = _view->modalWindowOperationButton(op);
    Z_CHECK_NULL(b);

    Utils::setHighlightWidgetFrame(b, info_type);
    _op_color_frames[_view->operation(op)] = info_type;
}

InformationType ModalWindow::modalWindowButtonHighlightFrame(QDialogButtonBox::StandardButton button) const
{
    return buttonHighlightFrame(button);
}

InformationType ModalWindow::modalWindowButtonHighlightFrame(const OperationID& op) const
{
    Z_CHECK(op.isValid());

    if (_view == nullptr)
        return InformationType::Invalid;

    return _op_color_frames.value(_view->operation(op), InformationType::Invalid);
}

void ModalWindow::modalWindowRemoveButtonHighlightFrame(QDialogButtonBox::StandardButton button)
{
    removeButtonHighlightFrame(button);
}

void ModalWindow::modalWindowRemoveButtonHighlightFrame(const OperationID& op)
{
    Z_CHECK(op.isValid());

    if (_view == nullptr)
        return;

    QPushButton* b = _view->modalWindowOperationButton(op);
    Z_CHECK_NULL(b);

    Utils::removeHighlightWidgetFrame(b);
    _op_color_frames.remove(_view->operation(op));
}

void ModalWindow::beforePopup()
{
    _operation_error.clear();
    HighlightDialog::beforePopup();

    _view->model()->onActivatedInternal(true);
    _view->onActivatedInternal(true);
}

void ModalWindow::afterPopup()
{
    HighlightDialog::afterPopup();
    loadConfiguration();

    _view->model()->onActivatedInternal(false);
    _view->onActivatedInternal(false);
}

void ModalWindow::beforeHide()
{
    HighlightDialog::beforeHide();
    saveConfiguration();
}

void ModalWindow::afterHide()
{
    HighlightDialog::afterHide();
    _view = nullptr;
}

bool ModalWindow::onButtonClick(QDialogButtonBox::StandardButton button)
{
    if (_view.isNull())
        return false;

    // Надо зафиксировать изменения, которые есть в представлении, но не получены моделью
    Utils::fixUnsavedData();

    bool processed = processModalWindowButtonClickResult(_view->onModalWindowButtonClick(button, Operation(), false));
    if (processed)
        return true;

    if (_view->isViewWindow())
        return HighlightDialog::onButtonClick(button);

    if (button == QDialogButtonBox::Save) {
        // нестандартное сохранение
        Error error;
        bool res = _view->onModalWindowCustomSave(error);
        if (error.isError()) {
            Z_CHECK(res == false);
            _operation_error = error;
            Core::error(error);
            return true;
        }

        if (res) {
            // нестандартное сохранение выполнено, просто закрываем окно
            if (_view != nullptr)
                _view->afterModalWindowSave();
            accept();
            return true;
        }

        if (_view != nullptr && !_view->onModalWindowOkSave())
            return true;

        if (_options.testFlag(ModuleWindowOption::EditWindowNoSave)) {
            accept();
            return true;
        }

        invokeModalWindowOkSave(false); // ошибка будет отображена в методе View::sl_model_finishSave -> View::processErrorHelper

        return true;

    } else if (button == QDialogButtonBox::Cancel) {
        if (_view != nullptr && !_view->onModalWindowCancel())
            return true;
    }

    return false;
}

void ModalWindow::adjustDialogSize()
{
    if (_view.isNull())
        return;

    Utils::resizeWindowToScreen(this, QSize(_view->moduleUI()->baseSize().width() > 0 ? _view->moduleUI()->baseSize().width() : qMax(width(), 640),
                                          _view->moduleUI()->baseSize().height() > 0 ? _view->moduleUI()->baseSize().height() : qMax(height(), 480)));
}

void ModalWindow::onCallbackManagerInternal(int key, const QVariant& data)
{
    HighlightDialog::onCallbackManagerInternal(key, data);

    if (key == Framework::MODAL_WINDOW_CREATE_TOOLBAR_KEY) {
        _ui->toolbar_widget->setUpdatesEnabled(true);

    } else if (key == Framework::MODAL_WINDOW_CHECK_ENABLED_KEY) {
        checkEnabledHelper();
    }
}

bool ModalWindow::onCloseByCross()
{
    if (_view == nullptr)
        return true;

    if (_options.testFlag(ModuleWindowOption::EditWindowNoSave) || (_view != nullptr && _view->isReadOnly()))
        return true;

    // Надо зафиксировать изменения, которые есть в представлении, но не получены моделью
    Utils::fixUnsavedData();

    if (!processModalWindowButtonClickResult(_view->onModalWindowButtonClick(QDialogButtonBox::StandardButton::NoButton, Operation(), true))) {
        Error error;
        bool res = hasUnsavedData(error);
        if (error.isError()) {
            Core::error(error);
            return true; // при ошибке даем закрыть, иначе просто никогда не закроем окно
        }
        if (res)
            return Core::ask(ZF_TR(ZFT_CLOSE_NO_SAVE));

        return true;
    }

    return false;
}

void ModalWindow::dragEnterEvent(QDragEnterEvent* event)
{
    if (!_view.isNull() && _view->onDragEnterHelper(this, event))
        return;

    HighlightDialog::dragEnterEvent(event);
}

void ModalWindow::dragMoveEvent(QDragMoveEvent* event)
{
    if (!_view.isNull() && _view->onDragMoveHelper(this, event))
        return;

    HighlightDialog::dragMoveEvent(event);
}

void ModalWindow::dropEvent(QDropEvent* event)
{
    if (!_view.isNull() && _view->onDropHelper(this, event))
        return;

    HighlightDialog::dropEvent(event);
}

void ModalWindow::dragLeaveEvent(QDragLeaveEvent* event)
{
    if (!_view.isNull() && _view->onDragLeaveHelper(this, event))
        return;

    HighlightDialog::dragLeaveEvent(event);
}

void ModalWindow::keyPressEvent(QKeyEvent* e)
{
    HighlightDialog::keyPressEvent(e);

    if (_view != nullptr && e->modifiers().testFlag(Qt::ShiftModifier) && e->modifiers().testFlag(Qt::AltModifier) && e->key() == Qt::Key_F12) {
        // тестовый просмотр содержимого данных
        zf::Utils::debugDataShow(_view->model());
    }
}

void ModalWindow::checkEnabled()
{
    if (objectExtensionDestroyed())
        return;
    Framework::internalCallbackManager()->addRequest(this, Framework::MODAL_WINDOW_CHECK_ENABLED_KEY);
}

void ModalWindow::sl_entityNameChanged()
{
    if (_view.isNull())
        return;

    QString name = _view->entityName();
    setWindowTitle(name);
}

void ModalWindow::sl_operationProcessed(const Operation& op, const Error& error)
{
    Q_UNUSED(op);

    _operation_error = error;

    if (error.isError()) {
        if (error.isCloseWindow())
            reject(); // ошибка ушла в _operation_error и будет обработана после закрытия окна
        else
            Core::error(error);
    }
}

QList<int> ModalWindow::buttonsList(View* view)
{
    Z_CHECK_NULL(view);

    if (view->isViewWindow())
        return {QDialogButtonBox::Ok};

    return {QDialogButtonBox::Save, QDialogButtonBox::Cancel};
}

QStringList ModalWindow::buttonsText(View* view)
{
    Z_CHECK_NULL(view);
    if (view->isViewWindow())
        return {ZF_TR(ZFT_OK)};

    return {ZF_TR(ZFT_SAVE), ZF_TR(ZFT_CANCEL)};
}

QList<int> ModalWindow::buttonsRole(View* view)
{
    Z_CHECK_NULL(view);
    if (view->isViewWindow())
        return {QDialogButtonBox::AcceptRole};

    return {QDialogButtonBox::AcceptRole, QDialogButtonBox::RejectRole};
}

void ModalWindow::checkEnabledHelper()
{
    if (_view.isNull())
        return;

    setOkButtonEnabled(isOkEnabled());

    if (!_view->isViewWindow()) {
        if (!_options.testFlag(ModuleWindowOption::EditWindowNoHighlightSave)) {
            QMap<QDialogButtonBox::StandardButton, InformationType> buttons_info;
            QMap<OperationID, InformationType> operations_info;
            if (!_view->getModalWindowShowHightlightFrame(_view->highlight(false), buttons_info, operations_info)) {
                // кнопки
                auto all_roles = buttonRoles();
                for (auto i = buttons_info.constBegin(); i != buttons_info.constEnd(); ++i) {
                    if (i.value() == InformationType::Invalid)
                        removeButtonHighlightFrame(i.key());
                    else
                        setButtonHighlightFrame(i.key(), i.value());

                    all_roles.setFlag(i.key(), false);
                }
                // очищаем для всех остальных кнопок
                auto buttons = this->buttons();
                for (auto& b : qAsConst(buttons)) {
                    auto r = role(b);
                    if (r == QDialogButtonBox::NoButton || !all_roles.testFlag(r))
                        continue;

                    removeButtonHighlightFrame(r);
                }

                // операции
                auto all_ops = _view->modalWindowOperations();
                for (auto i = operations_info.constBegin(); i != operations_info.constEnd(); ++i) {
                    if (i.value() == InformationType::Invalid)
                        modalWindowRemoveButtonHighlightFrame(i.key());
                    else
                        modalWindowSetButtonHighlightFrame(i.key(), i.value());

                    all_ops.removeOne(_view->operation(i.key()));
                }
                // очищаем для всех остальных операций
                for (auto& o : qAsConst(all_ops)) {
                    modalWindowRemoveButtonHighlightFrame(o.id());
                }

            } else {
                Z_CHECK(buttons_info.isEmpty() && operations_info.isEmpty());
                if (!_view->isReadOnly() && isErrorExists(false))
                    setButtonHighlightFrame(QDialogButtonBox::Save, InformationType::Error);
                else
                    removeButtonHighlightFrame(QDialogButtonBox::Save);
            }
        }
    }

    if (!_view->isLoading())
        requestHighlightShow();
}

void ModalWindow::createToolbar()
{
    if (_view.isNull())
        return;

    removeToolbars();
    bool has_toolbars = !_view->isToolbarsHidden() && _view->configureToolbarsLayout(_ui->toolbar_layout);

    if (has_toolbars)
        _ui->toolbar_layout->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Fixed));
}

void ModalWindow::removeToolbars()
{
    while (_ui->toolbar_layout->count() > 0) {
        QLayoutItem* item = _ui->toolbar_layout->takeAt(0);
        if (item->widget() != nullptr) {
            QToolBar* toolbar = qobject_cast<QToolBar*>(item->widget());
            if (toolbar != nullptr)
                toolbar->setParent(nullptr);
            else
                delete item->widget();
        }
        delete item;
    }
}

void ModalWindow::bootstrapViewActions()
{
    auto operations = _view->modalWindowOperations();
    if (operations.isEmpty())
        return;

    buttonBox()->setSizePolicy(QSizePolicy::Maximum, buttonBox()->sizePolicy().verticalPolicy());

    for (int i = operations.count() - 1; i >= 0; i--) {
        buttonsLayout()->insertWidget(0, _view->modalWindowOperationButton(operations.at(i).id()));
    }

    buttonsLayout()->insertSpacerItem(0, new QSpacerItem(0, 0, QSizePolicy::Expanding));
}

bool ModalWindow::isOkEnabled() const
{
    if (_view.isNull())
        return false;

    if (_view->isViewWindow())
        return true;
    else
        return !_view->isReadOnly() && !_view->isBlockedUi() && _view->isModalWindowOkSaveEnabled();
}

bool ModalWindow::isErrorExists(bool force_check) const
{
    return !_view.isNull() && _view->highlight(force_check)->hasErrorsWithoutOptions(HighlightOption::NotBlocked);
}

void ModalWindow::focusError()
{
    if (_view.isNull())
        return;

    // если уже находится в поле с ошибкой, то ничего не делаем
    QWidget* focused = focusWidget();
    if (focused != nullptr) {
        DataProperty current_property = _view->widgets()->widgetProperty(focused);
        if (current_property.isValid())
            if (_view->highlight()->contains(current_property, InformationType::Error | InformationType::Critical | InformationType::Fatal))
                return;
    }

    auto items = _view->highlight(true)->items(InformationType::Error | InformationType::Critical | InformationType::Fatal);
    if (items.isEmpty())
        return;

    setFocus(items.first().property());
}

void ModalWindow::focusNextError(InformationTypes info_type)
{
    if (_view == nullptr)
        return;

    DataProperty current_property;
    QWidget* focused = focusWidget();
    if (focused != nullptr) {
        current_property = _view->widgets()->widgetProperty(focused);
        if (current_property.propertyType() == PropertyType::Dataset && _view->currentIndex(current_property).isValid())
            current_property = _view->propertyCell(_view->currentIndex(current_property));
    }

    DataPropertyList properties = _view->highlight(false)->properties(info_type);
    Z_CHECK(!properties.isEmpty());

    DataProperty next_property;

    if (current_property.isValid()) {
        int pos = properties.indexOf(current_property);
        if (pos >= 0) {
            if (pos == properties.count() - 1)
                pos = 0;
            else
                pos++;

            next_property = properties.at(pos);
        }
    }
    if (!next_property.isValid())
        next_property = properties.first();

    if (next_property == current_property)
        return;

    setFocus(next_property);
}

void ModalWindow::modalWindowSetBottomLineVisible(bool b)
{
    setBottomLineVisible(b);
}

bool ModalWindow::beforeModalWindowOperationExecuted(const Operation& op)
{
    if (_view.isNull())
        return false;

    // Надо зафиксировать изменения, которые есть в представлении, но не получены моделью
    Utils::fixUnsavedData();

    Z_CHECK(op.isValid() && op.entityCode() == _view->entityCode());
    return !processModalWindowButtonClickResult(_view->onModalWindowButtonClick(QDialogButtonBox::NoButton, op, false));
}

void ModalWindow::afterModalWindowOperationExecuted(const Operation& op, const Error& error)
{
    Q_UNUSED(op);
    Q_UNUSED(error);
}

void ModalWindow::loadConfiguration()
{
    if (_view.isNull())
        return;

    _view->loadConfiguration();
}

void ModalWindow::saveConfiguration()
{
    if (_view.isNull())
        return;

    _view->saveConfiguration();
}

void ModalWindow::setFocus(const DataProperty& property)
{
    _view->beforeFocusHighlight(property);
    _view->setFocus(property);
    _view->afterFocusHighlight(property);
}

bool ModalWindow::hasUnsavedData(Error& error) const
{
    if (_view.isNull())
        return false;
    return _view->hasUnsavedData(error);
}

bool ModalWindow::processModalWindowButtonClickResult(View::ModalWindowButtonAction action)
{
    switch (action) {
        case View::ModalWindowButtonAction::Close:
            accept();
            return true;

        case View::ModalWindowButtonAction::Cancel:
            return true;

        case View::ModalWindowButtonAction::Continue:
            return false;

        case View::ModalWindowButtonAction::CloseSave: {
            invokeModalWindowOkSave(false);
            return true;
        }

        case View::ModalWindowButtonAction::CloseCheck: {
            Error error;
            bool res = hasUnsavedData(error);
            if (error.isError()) {
                reject(); // при ошибке закрываем, иначе просто никогда не закроем окно
                Core::error(error);
                return true;
            }
            if (res && Core::ask(ZF_TR(ZFT_CLOSE_NO_SAVE)))
                reject();

            return true;
        }
    }

    Z_HALT_INT;
    return false;
}

} // namespace zf
