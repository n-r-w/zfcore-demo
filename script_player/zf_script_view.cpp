#include "zf_script_view.h"
#include "zf_core.h"
#include "ui_zf_script_view_form.h"
#include "zf_data_widgets.h"
#include "zf_translation.h"

#include <QButtonGroup>
#include <QFormLayout>

#define DEBUG_SAVE_STATE 0

namespace zf
{
ScriptView::ScriptView(ScriptPlayer* player)
    : Dialog(
        {
            QDialogButtonBox::Cancel, QDialogButtonBox::Save, QDialogButtonBox::No, QDialogButtonBox::Yes
#if DEBUG_SAVE_STATE
                ,
                QDialogButtonBox::SaveAll, QDialogButtonBox::Open
#endif
        },
        {
            ZF_TR(ZFT_CANCEL), ZF_TR(ZFT_SAVE), ZF_TR(ZFT_PREV), ZF_TR(ZFT_NEXT)
#if DEBUG_SAVE_STATE
                                                                     ,
                "Сохранить состояние", "Восстановить состояние"
#endif
        },
        {
            QDialogButtonBox::ResetRole, QDialogButtonBox::AcceptRole, QDialogButtonBox::AcceptRole, QDialogButtonBox::AcceptRole
#if DEBUG_SAVE_STATE
                ,
                QDialogButtonBox::HelpRole, QDialogButtonBox::HelpRole
#endif
        })
    , _player(player)

{
    Z_CHECK_NULL(_player);
    init();
}

ScriptView::~ScriptView()
{
    delete _widgets;
}

ScriptView::ResultType ScriptView::run(Error& error)
{
    error = updateUI();
    if (error.isError())
        return ResultType::Error;

    int res = exec();

    switch (res) {
        case QDialogButtonBox::Yes:
            return ResultType::Finished;
        case QDialogButtonBox::Save:
            return ResultType::Saved;

        default:
            return ResultType::Cancelled;
    }
}

bool ScriptView::onButtonClick(QDialogButtonBox::StandardButton button)
{
    if (button == QDialogButtonBox::Yes) {
        if (!_player->isFinalStep()) {
            Error error = _player->toNextStep();
            if (error.isError())
                Core::error(error);

            return true;
        } else {
            if (!Core::ask(ZF_TR(ZFT_SCRIPT_PLAYER_DONE)))
                return true;
        }

    } else if (button == QDialogButtonBox::No) {
        if (!_player->isFirstStep()) {
            Error error = _player->toPreviousStep();
            if (error.isError())
                Core::error(error);
        }
        return true;

    } else if (button == QDialogButtonBox::Cancel) {
        if (!Core::ask(ZF_TR(ZFT_SCRIPT_PLAYER_CANCEL), InformationType::Warning))
            return true;

        reject();
        setResult(static_cast<int>(button));
        return true;

    } else if (button == QDialogButtonBox::Save) {
        if (!Core::ask(ZF_TR(ZFT_SCRIPT_PLAYER_SAVE_STATE)))
            return true;

        accept();
        setResult(static_cast<int>(button));
        return true;

    }
#if DEBUG_SAVE_STATE
    else if (button == QDialogButtonBox::SaveAll) {
        QString file_name = Utils::getSaveFileName("Сохранить состояние", "", "JSON (*.json);;");
        if (!file_name.isEmpty()) {
            auto json = _player->stateToJSON();
            auto error = Utils::saveByteArray(file_name, json);
            if (error.isError())
                Core::error(error);
        }
        return true;

    } else if (button == QDialogButtonBox::Open) {
        QString file_name = Utils::getOpenFileName("Сохранить состояние", "", "JSON (*.json);;");
        if (!file_name.isEmpty()) {
            QByteArray json;
            auto error = Utils::loadByteArray(file_name, json);
            if (error.isError())
                Core::error(error);

            error = _player->setStateFromJSON(json);
            if (error.isError())
                Core::error(error);
        }
        return true;
    }
#endif

    return Dialog::onButtonClick(button);
}

void ScriptView::sl_player_started()
{
    Error error = updateUI();
    if (error.isError())
        Core::error(error);
}

void ScriptView::sl_player_finished()
{
    Error error = updateUI();
    if (error.isError())
        Core::error(error);
}

void ScriptView::sl_player_stepLeft(const ScriptStepID& id)
{
    Q_UNUSED(id)
}

void ScriptView::sl_player_stepEntered(const ScriptStepID& prev_id, const ScriptStepID& current_id)
{
    Q_UNUSED(prev_id)
    Q_UNUSED(current_id)

    Error error = updateUI();
    if (error.isError())
        Core::error(error);
}

void ScriptView::sl_player_nextStepReady(const ScriptStepID& id)
{
    Q_UNUSED(id)

    updateDialogButtons();
}

void ScriptView::sl_player_choiseActivated(const ScriptStepID& step_id, const ScriptChoiseID& choise_id)
{
    Q_UNUSED(step_id)
    Q_UNUSED(choise_id)

    Error error = updateUI();
    if (error.isError())
        Core::error(error);
}

void ScriptView::sl_choiseUserActivated(bool checked)
{
    if (!checked)
        return;

    auto rb = qobject_cast<QRadioButton*>(sender());
    auto info = currentStepInfo();
    Z_CHECK_NULL(info);
    int choise_index = info->choise_radio.indexOf(rb);
    Z_CHECK(choise_index >= 0);
    auto choises = info->step->choises();
    Z_CHECK(choise_index < choises.count());
    _player->activateChoise(choises.at(choise_index)->id());
}

void ScriptView::sl_splitterMoved(int, int)
{
    auto splitter = qobject_cast<QSplitter*>(sender());
    Z_CHECK_NULL(splitter);

    // выравниваем все сплиттеры
    for (auto it = _steps.constBegin(); it != _steps.constEnd(); ++it) {
        if (it.value()->ui->splitter == splitter)
            continue;

        if (splitter->sizes().constLast() > 0)
            it.value()->ui->splitter->setSizes(splitter->sizes());
    }
}

void ScriptView::init()
{
    Core::registerNonParentWidget(this);

    setDefaultSize({600, 400});
    setButtonLayoutMode(QDialogButtonBox::WinLayout);
    disableDefaultAction();

    connect(_player, &ScriptPlayer::sg_started, this, &ScriptView::sl_player_started);
    connect(_player, &ScriptPlayer::sg_finished, this, &ScriptView::sl_player_finished);
    connect(_player, &ScriptPlayer::sg_stepLeft, this, &ScriptView::sl_player_stepLeft);
    connect(_player, &ScriptPlayer::sg_stepEntered, this, &ScriptView::sl_player_stepEntered);
    connect(_player, &ScriptPlayer::sg_nextStepReady, this, &ScriptView::sl_player_nextStepReady);
    connect(_player, &ScriptPlayer::sg_choiseActivated, this, &ScriptView::sl_player_choiseActivated);

    _widgets = new DataWidgets(Core::defaultDatabase(), _player->data());
    _widgets->updateWidgets(DataWidgets::UpdateReason::NoReason);

    workspace()->setLayout(new QVBoxLayout);
    workspace()->layout()->setObjectName(QStringLiteral("zfla"));
    workspace()->layout()->setMargin(0);

    button(QDialogButtonBox::No)->setIcon(QIcon(":/share_icons/previous.svg"));

    updateDialogButtons();
}

void ScriptView::updateSavedConfig()
{
    if (_current_step == nullptr)
        return;
}

Error ScriptView::updateUI()
{
    if (!_player->isStarted())
        return _player->start();

    if (_current_step == _player->currentStep() && _current_choise == _player->currentChoise()) {
        updateDialogButtons();
        return Error();
    }

    Error error;
    auto info = getStepUI(_player->currentStep(), _player->currentChoise(), error);
    if (error.isError()) {
        updateDialogButtons();
        return error;
    }

    _current_step = _player->currentStep();
    _current_choise = _player->currentChoise();

    if (workspace()->layout()->count() > 0) {
        // изымаем предыдущий виджет
        Z_CHECK(workspace()->layout()->count() == 1);
        workspace()->layout()->takeAt(0);
    }

    if (info != nullptr) {
        workspace()->layout()->addWidget(info->widget);
        info->widget->setHidden(false);
    }

    updateDialogButtons();
    return Error();
}

ScriptView::StepInfoPtr ScriptView::getStepUI(ScriptStep* step, ScriptChoise* choise, Error& error)
{
    Z_CHECK_NULL(step);
    error.clear();

    if (_current_step != nullptr && _current_step != step) {
        auto old_info = _steps.value(_current_step->id());
        if (old_info != nullptr) {
            old_info->widget->setHidden(true);
            old_info->widget->setParent(nullptr);
        }
    }

    if (step->type() == ScriptStep::Function)
        return nullptr;

    auto step_info = _steps.value(step->id());
    // варианты выбора
    auto choises = step->choises();

    if (step_info == nullptr) {
        step_info = Z_MAKE_SHARED(StepInfo);
        step_info->step = step;

        // устанваливаем размер на основании ранее заданного
        for (auto it = _steps.constBegin(); it != _steps.constEnd(); ++it) {
            if (it.value()->ui->splitter->sizes().constLast() > 0) {
                step_info->ui->splitter->setSizes(it.value()->ui->splitter->sizes());
                break;
            }
        }

        connect(step_info->ui->splitter, &QSplitter::splitterMoved, this, &ScriptView::sl_splitterMoved);

        // варианты выбора
        if (choises.isEmpty()) {
            // скрываем
            step_info->ui->choise->setHidden(true);

        } else {
            for (int i = 0; i < choises.count(); i++) {
                auto cr = new QRadioButton;
                step_info->choise_radio << cr;
                step_info->radio_group->addButton(cr);
                step_info->ui->choise_la->addWidget(cr);
                step_info->choises << choises.at(i);

                connect(cr, &QRadioButton::toggled, this, &ScriptView::sl_choiseUserActivated);
            }
        }

        _steps[step->id()] = step_info;
    }

    // текст сообщения и вариантов выбора надо обновить в любом случае, т.к. могли измениться данные
    QString message = step->text(true, error);
    if (error.isError())
        return nullptr;
    step_info->ui->message->setHtml(message);

    Z_CHECK(step_info->choise_radio.count() == choises.count());
    for (int i = 0; i < choises.count(); i++) {
        Z_CHECK_NULL(step_info->choise_radio.at(i));

        QString choise_text = choises.at(i)->text(false, error);
        if (error.isError())
            return nullptr;

        step_info->choise_radio.at(i)->setText(choise_text);

        if (_current_choise != choise)
            step_info->choise_radio.at(i)->setChecked(choises.at(i)->isCurrent());
    }

    // Данные для заполнения добавляем динамически, т.к. едиторы общие для всех шагов

    auto properties = step->targetProperties();
    if (choise != nullptr) {
        for (auto& p : choise->targetProperties()) {
            if (properties.contains(p))
                continue;
            properties << p;
        }
    }

    if (properties.isEmpty()) {
        // нечего показывать
        step_info->ui->data_top->setHidden(true);
        if (step_info->ui->splitter->sizes().constLast() > 0)
            step_info->splitter_data = Utils::hideSplitterPart(step_info->ui->splitter, 1);
        step_info->ui->info_layout->setContentsMargins(0, 0, 0, 0);
        step_info->ui->data_layout->setContentsMargins(0, 0, 0, 0);

    } else {
        // если ранее был добавлен QFormLayout, то его надо убрать, сохранив при этом эдиторы
        if (step_info->ui->data->layout() != nullptr) {
            QFormLayout* layout = qobject_cast<QFormLayout*>(step_info->ui->data->layout());
            Z_CHECK_NULL(layout);

            for (int i = layout->rowCount() - 1; i >= 0; i--) {
                auto row = layout->takeRow(i);
                // текстовое поле удаляем, а редактор оставляем
                delete row.labelItem->widget();
                if (row.fieldItem != nullptr && row.fieldItem->widget() != nullptr)
                    row.fieldItem->widget()->setHidden(true);
            }
            delete layout;
        }

        QFormLayout* layout = new QFormLayout;
        layout->setObjectName(QStringLiteral("zfla"));
        layout->setMargin(0);

        step_info->ui->data->setLayout(layout);
        step_info->ui->data_top->setVisible(true);

        for (auto& p_id : properties) {
            DataProperty p = _player->data()->property(p_id);

            QLabel* label = new QLabel(p.name(false));
            QWidget* editor = _widgets->field(p);
            layout->addRow(label, editor);
            editor->setHidden(false);
        }

        step_info->ui->info_layout->setContentsMargins(0, 0, 3, 0);
        step_info->ui->data_layout->setContentsMargins(3, 0, 0, 0);
        if (step_info->splitter_data.isValid()) {
            Utils::showSplitterPart(step_info->ui->splitter, step_info->splitter_data);
            step_info->splitter_data = Utils::SplitterSavedData();
        }
    }

    return step_info;
}

ScriptView::StepInfoPtr ScriptView::currentStepInfo() const
{
    return _steps.value(_player->currentStep()->id());
}

void ScriptView::updateDialogButtons()
{
    auto done_next = button(QDialogButtonBox::Yes);
    auto prev = button(QDialogButtonBox::No);
    auto save = button(QDialogButtonBox::Save);

    Error error;
    auto steps = _player->nextStep(false, error);
    // если ошибка, то разрешаем перейти, чтобы она могла отобразиться
    bool allow_save_next;
    if (_player->isFinalStep())
        allow_save_next = true;
    else
        allow_save_next = error.isError() || !steps.isEmpty();

    if (_player->currentStep() != nullptr && !_player->currentStep()->choises().isEmpty()
        && _player->currentStep()->selectedChoise() == nullptr)
        allow_save_next = false;

    if (_player->isFinalStep()) {
        done_next->setText(" " + ZF_TR(ZFT_DONE));
        done_next->setIcon(QIcon(":/share_icons/green/check.svg"));
        save->setHidden(true);
    } else {
        done_next->setText(ZF_TR(ZFT_NEXT));
        done_next->setIcon(QIcon(":/share_icons/next.svg"));
        save->setHidden(false);
    }

    prev->setEnabled(_player->previousStep() != nullptr);
    done_next->setEnabled(allow_save_next);
}

ScriptView::StepInfo::StepInfo()
    : widget(new QWidget)
    , ui(new Ui::ScriptViewMain)
    , radio_group(new QButtonGroup(widget))
{
    ui->setupUi(widget);
}

ScriptView::StepInfo::~StepInfo()
{
    delete ui;

    if (!widget.isNull())
        delete widget;

    for (auto& c : qAsConst(choise_radio)) {
        if (!c.isNull())
            delete c;
    }
}

} // namespace zf
