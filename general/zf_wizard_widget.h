#pragma once

#include "zf_basic_types.h"
#include <QWidget>
#include <QStackedLayout>
#include "zf_error.h"

namespace zf
{
//! Идентификатор шага мастера
class ZCORESHARED_EXPORT WizardStepID : public ID_Wrapper
{
public:
    WizardStepID();
    WizardStepID(int id);

    //! Преобразовать в QVariant
    QVariant variant() const;
    //! Восстановить из QVariant
    static WizardStepID fromVariant(const QVariant& v);
};

inline uint qHash(const zf::WizardStepID& d)
{
    return ::qHash(d.value());
}

//! Виджет мастера
class ZCORESHARED_EXPORT WizardWidget : public QWidget
{
    Q_OBJECT
public:
    WizardWidget(QWidget* parent = nullptr);
    ~WizardWidget();

    //! Добавить шаг. Порядок шагов определяется порядком добавления, а не их ID
    void addStep(const WizardStepID& id,
                 //! Виджет. За удаление отвечает WizardWidget
                 QWidget* widget,
                 //! Состояние шага
                 const WizardStepStates& states = WizardStepState::Enabled | WizardStepState::Visible);

    //! Список шагов
    QList<WizardStepID> steps() const;
    //! Виджет для шага
    QWidget* widget(const WizardStepID& id) const;
    //! Сотсояние шага
    WizardStepStates stepStates(const WizardStepID& id) const;

    //! Текущий шаг
    WizardStepID currentStep() const;

    //! Следующий доступный шаг
    WizardStepID nextStep() const;
    //! Предыдущий доступный шаг
    WizardStepID previousStep() const;

    //! Находимся на первом шаге (с учетом видимости)
    bool isFirstStep() const;
    //! Является ли данный шаг первым (с учетом видимости)
    bool isFirstStep(const WizardStepID& id) const;

    //! Находимся на последнем шаге (с учетом видимости)
    bool isLastStep() const;
    //! Является ли данный шаг последним (с учетом видимости)
    bool isLastStep(const WizardStepID& id) const;

    //! Перейти на следующий доступный шаг
    bool toNextStep();
    //! Перейти на предыдущий доступный шаг
    bool toPreviousStep();

    //! Задать текущий шаг
    bool setCurrentStep(const WizardStepID& id);
    //! Задать состояние для шага
    void setStepStates(const WizardStepID& id, const WizardStepStates& states);

signals:
    //! Сменился текущий шаг
    void sg_currentStepChanged(const zf::WizardStepID& previous, const zf::WizardStepID& current);
    //! Сменилось состояние шага
    void sg_statesChanged(const zf::WizardStepID& id);

private:
    //! Информация о шаге
    struct StepInfo
    {
        WizardStepID id;
        QPointer<QWidget> widget;
        WizardStepStates states;
    };
    typedef std::shared_ptr<StepInfo> StepInfoPtr;
    StepInfoPtr info(const WizardStepID& id) const;

    void bootsrap();

    //! Шаги
    QList<StepInfoPtr> _steps;
    QMap<WizardStepID, StepInfoPtr> _step_map;
    //! Текущий шаг
    StepInfoPtr _current;
    //! Лайаут для управления переключениями текущего шага
    QStackedLayout* _stacked;
};

} // namespace zf

Q_DECLARE_METATYPE(zf::WizardStepID)
