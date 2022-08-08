#include "zf_wizard_dialog.h"
#include "zf_core.h"
#include "zf_translation.h"

namespace zf
{
WizardDialog::WizardDialog(Type type, Options options)
    : Dialog(createButtons(type), createTexts(type), createRoles(type))
    , _type(type)
    , _options(options)
    , _widget(new WizardWidget)
{
    Core::registerNonParentWidget(this);

    workspace()->setLayout(new QVBoxLayout);
    workspace()->layout()->setContentsMargins(0, 0, 0, 0);
    workspace()->layout()->addWidget(_widget);

    connect(_widget, &WizardWidget::sg_currentStepChanged, this, &WizardDialog::sl_currentStepChanged);
    connect(_widget, &WizardWidget::sg_statesChanged, this, &WizardDialog::sl_statesChanged);

    //    setBottomLineVisible(false);
    disableEscapeClose();
    disableDefaultAction();
    setButtonLayoutMode(QDialogButtonBox::GnomeLayout);

    initButtonIcons();
}

WizardDialog::~WizardDialog()
{
}

void WizardDialog::addStep(const WizardStepID& id, QWidget* widget, const WizardStepStates& states)
{
    _widget->addStep(id, widget, states);
    updateButtonProperies();
}

Error WizardDialog::addStep(const WizardStepID& id, const QString& ui_file, const WizardStepStates& states)
{
    Z_CHECK(!ui_file.isEmpty());

    Error error;
    auto ui = Utils::openUI(ui_file, error);
    if (error.isError())
        return error;

    addStep(id, ui, states);

    return {};
}

Error WizardDialog::addStep(const WizardStepID& id, const QString& ui_file, const DataWidgets* widgets, const WizardStepStates& states)
{
    auto error = addStep(id, ui_file, states);
    if (error.isError())
        return error;

    zf::Core::replaceWidgets(widgets, widget(id));

    return {};
}

QList<WizardStepID> WizardDialog::steps() const
{
    return _widget->steps();
}

QWidget* WizardDialog::widget(const WizardStepID& id) const
{
    return _widget->widget(id);
}

WizardStepStates WizardDialog::stepStates(const WizardStepID& id) const
{
    return _widget->stepStates(id);
}

WizardStepID WizardDialog::currentStep() const
{
    return _widget->currentStep();
}

WizardStepID WizardDialog::nextStep() const
{
    return _widget->nextStep();
}

WizardStepID WizardDialog::previousStep() const
{
    return _widget->previousStep();
}

bool WizardDialog::isFirstStep() const
{
    return _widget->isFirstStep();
}

bool WizardDialog::isFirstStep(const WizardStepID& id) const
{
    return _widget->isFirstStep(id);
}

bool WizardDialog::isLastStep() const
{
    return _widget->isLastStep();
}

bool WizardDialog::isLastStep(const WizardStepID& id) const
{
    return _widget->isLastStep(id);
}

Error WizardDialog::toNextStep()
{
    auto step = nextStep();
    Z_CHECK(step.isValid());

    auto error = beforeSetStep(currentStep(), step);
    if (error.isError())
        return error;

    Z_CHECK(_widget->toNextStep());
    return {};
}

Error WizardDialog::toPreviousStep()
{
    auto step = previousStep();
    Z_CHECK(step.isValid());

    auto error = beforeSetStep(currentStep(), step);
    if (error.isError())
        return error;

    Z_CHECK(_widget->toPreviousStep());
    return error;
}

bool WizardDialog::tryNextStep()
{
    if (!nextStep().isValid())
        return false;

    auto error = toNextStep();
    if (error.isError()) {
        Core::error(error);
        return false;
    }

    return true;
}

bool WizardDialog::tryPreviousStep()
{
    if (!previousStep().isValid())
        return false;

    auto error = toPreviousStep();
    if (error.isError()) {
        Core::error(error);
        return false;
    }

    return true;
}

Error WizardDialog::setCurrentStep(const WizardStepID& id)
{
    Z_CHECK(id.isValid());
    if (id == currentStep())
        return {};

    auto error = beforeSetStep(currentStep(), id);
    if (error.isError())
        return error;

    Z_CHECK(_widget->setCurrentStep(id));
    return error;
}

void WizardDialog::setStepStates(const WizardStepID& id, const WizardStepStates& states)
{
    _widget->setStepStates(id, states);
}

Error WizardDialog::beforeSetStep(const WizardStepID& previous_id, const WizardStepID& new_id)
{
    Q_UNUSED(previous_id)
    Q_UNUSED(new_id)
    return Error();
}

bool WizardDialog::canSave()
{
    return true;
}

bool WizardDialog::canCancel()
{
    return true;
}

QString WizardDialog::getButtonText(WizardDialog::ButtonType button_type, const WizardStepID& step_id) const
{
    if (isLastStep(step_id) && button_type == NextButton) {
        if (_options.testFlag(LastStepInfo))
            return ZF_TR(ZFT_OK);

        return ZF_TR(ZFT_SAVE);
    }
    return getButtonTextDefault(button_type);
}

QIcon WizardDialog::getButtonIcon(WizardDialog::ButtonType button_type, const WizardStepID& step_id) const
{
    if (isLastStep(step_id) && button_type == NextButton) {
        if (_options.testFlag(LastStepInfo))
            return QIcon(":/share_icons/green/check.svg");

        return QIcon(":/share_icons/blue/save.svg");
    }
    return getButtonIconDefault(button_type);
}

void WizardDialog::onCurrentStepChanged(const WizardStepID& previous, const WizardStepID& current)
{
    Q_UNUSED(previous)
    Q_UNUSED(current)
}

void WizardDialog::onStatesChanged(const WizardStepID& id)
{
    Q_UNUSED(id)
}

bool WizardDialog::onButtonClick(QDialogButtonBox::StandardButton button)
{
    auto type = convertButtonType(button);

    if (type == PreviousButton) {
        auto s = previousStep();
        if (s.isValid()) {
            auto error = setCurrentStep(s);
            if (error.isError())
                zf::Core::error(error);
        }
        return true;

    } else if (type == NextButton) {
        if (isLastStep())
            return false; // вызовет apply

        auto s = nextStep();
        if (s.isValid()) {
            auto error = setCurrentStep(s);
            if (error.isError())
                zf::Core::error(error);
        }

        return true;

    } else if (type == CancelButton) {
        onCancel();
        reject();
        setResult(static_cast<int>(button));
        return true;
    }

    return zf::Dialog::onButtonClick(button);
}

Error WizardDialog::onApply()
{
    return Error();
}

void WizardDialog::sl_currentStepChanged(const WizardStepID& previous, const WizardStepID& current)
{
    updateButtonProperies();
    onCurrentStepChanged(previous, current);
}

void WizardDialog::sl_statesChanged(const WizardStepID& id)
{
    updateButtonProperies();
    onStatesChanged(id);
}

WizardDialog::ButtonType WizardDialog::convertButtonType(QDialogButtonBox::StandardButton button)
{
    switch (button) {
        case QDialogButtonBox::Cancel:
            return ButtonType::CancelButton;
        case QDialogButtonBox::No:
            return ButtonType::PreviousButton;
        case QDialogButtonBox::Yes:
            return ButtonType::NextButton;

        default:
            Z_HALT_INT;
            break;
    }
    return ButtonType::CancelButton;
}

QDialogButtonBox::StandardButton WizardDialog::convertButtonType(WizardDialog::ButtonType button)
{
    switch (button) {
        case ButtonType::CancelButton:
            return QDialogButtonBox::Cancel;
        case ButtonType::PreviousButton:
            return QDialogButtonBox::No;
        case ButtonType::NextButton:
            return QDialogButtonBox::Yes;

        default:
            Z_HALT_INT;
            break;
    }
    return QDialogButtonBox::NoButton;
}

QString WizardDialog::getButtonTextDefault(WizardDialog::ButtonType button_type)
{
    switch (button_type) {
        case ButtonType::CancelButton:
            return ZF_TR(ZFT_CANCEL);
        case ButtonType::PreviousButton:
            return ZF_TR(ZFT_PREV);
        case ButtonType::NextButton:
            return ZF_TR(ZFT_NEXT);
        default:
            Z_HALT_INT;
            break;
    }
    return {};
}

QIcon WizardDialog::getButtonIconDefault(WizardDialog::ButtonType button_type)
{
    switch (button_type) {
        case ButtonType::CancelButton:
            return QIcon(":/share_icons/red/close.svg");
        case ButtonType::PreviousButton:
            return QIcon(":/share_icons/previous.svg");
        case ButtonType::NextButton:
            return QIcon(":/share_icons/next.svg");
        default:
            Z_HALT_INT;
            break;
    }
    return {};
}

void WizardDialog::initButtonIcons()
{
    switch (_type) { //-V785
        case General:
            button(convertButtonType(CancelButton))->setIcon(getButtonIconDefault(CancelButton));
            button(convertButtonType(PreviousButton))->setIcon(getButtonIconDefault(PreviousButton));
            button(convertButtonType(NextButton))->setIcon(getButtonIconDefault(NextButton));
            break;

        default:
            Z_HALT_INT;
    }
}

void WizardDialog::updateButtonProperies()
{
    auto current = _widget->currentStep();

    switch (_type) { //-V785
        case General: {
            auto cancel = button(convertButtonType(CancelButton));
            auto prev = button(convertButtonType(PreviousButton));
            auto next = button(convertButtonType(NextButton));

            cancel->setIcon(getButtonIcon(CancelButton, current));
            prev->setIcon(getButtonIcon(PreviousButton, current));
            next->setIcon(getButtonIcon(NextButton, current));

            cancel->setText(getButtonText(CancelButton, current));
            prev->setText(getButtonText(PreviousButton, current));
            next->setText(getButtonText(NextButton, current));

            prev->setEnabled(_widget->previousStep().isValid());
            prev->setVisible(!_widget->isFirstStep());
            cancel->setVisible(true);

            if (isLastStep()) {
                if (_options.testFlag(LastStepInfo)) {
                    next->setVisible(true);
                    next->setEnabled(true);

                    cancel->setVisible(false);
                    prev->setVisible(false);

                } else {
                    next->setEnabled(canSave());
                    cancel->setEnabled(canCancel());
                }

            } else {
                next->setEnabled(_widget->nextStep().isValid());
            }

            break;
        }
        default:
            Z_HALT_INT;
    }
}

QList<int> WizardDialog::createButtons(WizardDialog::Type type)
{
    QList<int> res;
    switch (type) { //-V785
        case General:
            res = {QDialogButtonBox::Cancel, QDialogButtonBox::No, QDialogButtonBox::Yes};
            break;
        default:
            Z_HALT_INT;
            break;
    }

    return res;
}

QStringList WizardDialog::createTexts(WizardDialog::Type type)
{
    QStringList res;
    switch (type) { //-V785
        case General:
            res = QStringList {
                getButtonTextDefault(CancelButton),
                getButtonTextDefault(PreviousButton),
                getButtonTextDefault(NextButton),
            };
            break;
        default:
            Z_HALT_INT;
            break;
    }

    return res;
}

QList<int> WizardDialog::createRoles(WizardDialog::Type type)
{
    QList<int> res;
    switch (type) { //-V785
        case General:
            res = {QDialogButtonBox::ResetRole, QDialogButtonBox::NoRole, QDialogButtonBox::YesRole};
            break;
        default:
            Z_HALT_INT;
            break;
    }

    return res;
}

} // namespace zf
