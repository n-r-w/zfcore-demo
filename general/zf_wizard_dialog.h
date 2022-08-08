#pragma once

#include "zf_dialog.h"
#include "zf_wizard_widget.h"
#include "zf_data_widgets.h"

namespace zf
{
//! Диалог мастера
class ZCORESHARED_EXPORT WizardDialog : public Dialog
{
    Q_OBJECT
public:
    //! Тип мастера
    enum Type
    {
        General, // Кнопки Cancel, Next, Previous
    };

    //! Виды кнопок
    enum ButtonType
    {
        CancelButton,
        NextButton,
        PreviousButton,
    };

    //! Параметры
    enum Option
    {
        //! Последний шаг является информационным. Т.е. сохранение выполняется раньше
        LastStepInfo
    };
    Q_DECLARE_FLAGS(Options, Option);

    WizardDialog(Type type = General, Options options = {});
    ~WizardDialog();

    //! Добавить шаг. Порядок шагов определяется порядком добавления, а не их ID
    void addStep(const WizardStepID& id,
                 //! Виджет. За удаление отвечает WizardWidget
                 QWidget* widget,
                 //! Состояние шага
                 const WizardStepStates& states = WizardStepState::Enabled | WizardStepState::Visible);
    //! Добавить шаг. Порядок шагов определяется порядком добавления, а не их ID
    Error addStep(const WizardStepID& id,
                  //! Файл с UI
                  const QString& ui_file,
                  //! Состояние шага
                  const WizardStepStates& states = WizardStepState::Enabled | WizardStepState::Visible);
    //! Добавить шаг. Порядок шагов определяется порядком добавления, а не их ID
    Error addStep(const WizardStepID& id,
                  //! Файл с UI
                  const QString& ui_file,
                  //! Виджеты, которые должны быть сгенерированы для UI файла
                  const DataWidgets* widgets,
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
    Error toNextStep();
    //! Перейти на предыдущий доступный шаг
    Error toPreviousStep();

    //! Попробовать перейти на следующий шаг и выдать ошибку при необходимости
    bool tryNextStep();
    //! Попробовать перейти на предыдущий шаг и выдать ошибку при необходимости
    bool tryPreviousStep();

    //! Задать текущий шаг
    Error setCurrentStep(const WizardStepID& id);
    //! Задать состояние для шага
    void setStepStates(const WizardStepID& id, const WizardStepStates& states);

protected:
    //! Можно ли перейти на указанный шаг. Вызывается если шаг доступен.
    virtual Error beforeSetStep(const WizardStepID& previous_id, const WizardStepID& new_id);
    //! Можно ли сохранить изменения. Вызывается на последнем видимом шаге
    virtual bool canSave();
    //! Можно ли отменить изменения. Вызывается на последнем видимом шаге
    virtual bool canCancel();

    //! Получить имя кнопки на указанном шаге
    virtual QString getButtonText(ButtonType button_type, const WizardStepID& step_id) const;
    //! Получить рисунок кнопки на указанном шаге
    virtual QIcon getButtonIcon(ButtonType button_type, const WizardStepID& step_id) const;

    //! Сменился текущий шаг
    virtual void onCurrentStepChanged(const WizardStepID& previous, const WizardStepID& current);
    //! Сменилось состояние шага
    virtual void onStatesChanged(const WizardStepID& id);

protected:
    //! Была нажата кнопка. Если событие обработано и не требуется стандартная реакция - вернуть true
    bool onButtonClick(QDialogButtonBox::StandardButton button) override;
    //! Вызывается после нажатия ОК для последнего видимого шага диалога
    zf::Error onApply() override;

private slots:
    //! Сменился текущий шаг
    void sl_currentStepChanged(const zf::WizardStepID& previous, const zf::WizardStepID& current);
    //! Сменилось состояние шага
    void sl_statesChanged(const zf::WizardStepID& id);

private:
    //! Инициализация рисунков на кнопках
    void initButtonIcons();
    //! Обновить свойства кнопок
    void updateButtonProperies();

    //! Преобразование типа кнопки Qt в ButtonType
    static ButtonType convertButtonType(QDialogButtonBox::StandardButton button);
    static QDialogButtonBox::StandardButton convertButtonType(ButtonType button);

    //! Получить имя кнопки на указанном шаге
    static QString getButtonTextDefault(ButtonType button_type);
    //! Получить рисунок кнопки на указанном шаге
    static QIcon getButtonIconDefault(ButtonType button_type);

    //! Список кнопок (QDialogButtonBox::StandardButton приведенные к int)
    static QList<int> createButtons(Type type);
    //! Список текстовых названий кнопок
    static QStringList createTexts(Type type);
    //! Список ролей кнопок (QDialogButtonBox::ButtonRole приведенные к int)
    static QList<int> createRoles(Type type);

    //! Тип
    Type _type;
    //! Параметры
    Options _options;
    //! Виджет визарда
    WizardWidget* _widget;
};

} // namespace zf

Q_DECLARE_OPERATORS_FOR_FLAGS(zf::WizardDialog::Options);
