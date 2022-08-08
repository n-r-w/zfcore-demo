#include "zf_wizard_widget.h"
#include "zf_core.h"

#include <QScrollArea>

namespace zf
{
WizardWidget::WizardWidget(QWidget* parent)
    : QWidget(parent)

{
    bootsrap();
}

WizardWidget::~WizardWidget()
{
    for (auto& s : qAsConst(_steps)) {
        if (!s->widget.isNull())
            delete s->widget;
    }
}

void WizardWidget::addStep(const WizardStepID& id, QWidget* widget, const WizardStepStates& states)
{
    Z_CHECK_X(!_step_map.contains(id), id.string());
    Z_CHECK_NULL(widget);

    auto step = Z_MAKE_SHARED(StepInfo);
    step->id = id;
    step->widget = widget;
    step->states = states;

    _steps << step;
    _step_map[id] = step;

    auto scroll = new QScrollArea;
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    auto scroll_area_content = new QWidget;
    auto la = new QVBoxLayout;
    la->setMargin(6);
    scroll_area_content->setLayout(la);
    la->addWidget(widget);
    scroll->setWidget(scroll_area_content);

    _stacked->addWidget(scroll);

    if (_current == nullptr && states.testFlag(WizardStepState::Visible))
        setCurrentStep(id);
}

QList<WizardStepID> WizardWidget::steps() const
{
    QList<WizardStepID> res;
    for (auto& s : qAsConst(_steps)) {
        res << s->id;
    }

    return res;
}

QWidget* WizardWidget::widget(const WizardStepID& id) const
{
    auto w = info(id)->widget;
    Z_CHECK_NULL(w);
    return w;
}

WizardStepStates WizardWidget::stepStates(const WizardStepID& id) const
{
    return info(id)->states;
}

WizardStepID WizardWidget::currentStep() const
{
    if (_current == nullptr)
        return {};
    return _current->id;
}

WizardStepID WizardWidget::nextStep() const
{
    if (_steps.isEmpty() || _current == _steps.constLast())
        return WizardStepID();

    StepInfoPtr step = (_current == nullptr ? _steps.first() : _current);
    int index = _steps.indexOf(step);
    Z_CHECK(index >= 0);
    index++;
    int next = -1;
    for (int i = index; i < _steps.count(); i++) {
        if (!_steps.at(i)->states.testFlag(WizardStepState::Visible))
            continue;
        next = i;
        break;
    }

    if (next < 0)
        return WizardStepID();

    return _steps.at(next)->states.testFlag(WizardStepState::Enabled) ? _steps.at(next)->id : WizardStepID();
}

WizardStepID WizardWidget::previousStep() const
{
    if (_steps.isEmpty() || _current == _steps.constFirst())
        return WizardStepID();

    StepInfoPtr step = (_current == nullptr ? _steps.first() : _current);
    int index = _steps.indexOf(step);
    Z_CHECK(index >= 0);
    index--;
    int next = -1;
    for (int i = index; i >= 0; i--) {
        if (!_steps.at(i)->states.testFlag(WizardStepState::Visible))
            continue;
        next = i;
        break;
    }

    if (next < 0)
        return WizardStepID();

    return _steps.at(next)->states.testFlag(WizardStepState::Enabled) ? _steps.at(next)->id : WizardStepID();
}

bool WizardWidget::isFirstStep() const
{
    if (_current == nullptr)
        return false;

    return isFirstStep(_current->id);
}

bool WizardWidget::isFirstStep(const WizardStepID& id) const
{
    auto step = info(id);

    for (int i = 0; i < _steps.indexOf(step); i++) {
        if (_steps.at(i)->states.testFlag(WizardStepState::Visible))
            return false;
    }

    return true;
}

bool WizardWidget::isLastStep() const
{
    if (_current == nullptr)
        return false;

    return isLastStep(_current->id);
}

bool WizardWidget::isLastStep(const WizardStepID& id) const
{
    auto step = info(id);

    for (int i = _steps.count() - 1; i > _steps.indexOf(step); i--) {
        if (_steps.at(i)->states.testFlag(WizardStepState::Visible))
            return false;
    }

    return true;
}

bool WizardWidget::toNextStep()
{
    auto s = nextStep();
    if (!s.isValid())
        return false;

    return setCurrentStep(s);
}

bool WizardWidget::toPreviousStep()
{
    auto s = previousStep();
    if (!s.isValid())
        return false;

    Z_CHECK(setCurrentStep(s));
    return true;
}

bool WizardWidget::setCurrentStep(const WizardStepID& id)
{
    if (currentStep() == id)
        return true;

    if (!stepStates(id).testFlag(WizardStepState::Visible) || !stepStates(id).testFlag(WizardStepState::Enabled))
        return false;

    auto prev = currentStep();
    auto i = info(id);
    _current = i;

    int index = _steps.indexOf(i);
    Z_CHECK(index >= 0 && index < _stacked->count());
    _stacked->setCurrentIndex(index);

    emit sg_currentStepChanged(prev, _current->id);
    return true;
}

void WizardWidget::setStepStates(const WizardStepID& id, const WizardStepStates& states)
{
    auto i = info(id);
    if (i->states == states)
        return;

    i->states = states;
    emit sg_statesChanged(id);

    if (_current == nullptr && states.testFlag(WizardStepState::Visible))
        setCurrentStep(id);
}

WizardWidget::StepInfoPtr WizardWidget::info(const WizardStepID& id) const
{
    auto info = _step_map.value(id);
    if (info == nullptr)
        Z_HALT(id.string());
    return info;
}

void WizardWidget::bootsrap()
{
    _stacked = new QStackedLayout;
    _stacked->setMargin(0);
    setLayout(_stacked);
}

WizardStepID::WizardStepID()
{
}

WizardStepID::WizardStepID(int id)
    : ID_Wrapper(id)
{
}

QVariant WizardStepID::variant() const
{
    return QVariant::fromValue(*this);
}

WizardStepID WizardStepID::fromVariant(const QVariant& v)
{
    return v.value<WizardStepID>();
}

} // namespace zf
