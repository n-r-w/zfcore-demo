#pragma once

#include "zf_script_player.h"
#include "zf_dialog.h"
#include <QRadioButton>

namespace Ui
{
class ScriptViewMain;
}

namespace zf
{
class DataWidgets;

//! Отображение скрипта. ВАЖНО: не модифицировать скрипт после создания данного объекта!
class ZCORESHARED_EXPORT ScriptView : public Dialog
{
    Q_OBJECT
public:
    explicit ScriptView(ScriptPlayer* player);
    ~ScriptView();

    enum class ResultType
    {
        Error,
        Cancelled,
        Finished,
        Saved,
    };

    ResultType run(Error& error);

protected:
    //! Была нажата кнопка. Если событие обработано и не требуется стандартная реакция - вернуть true
    bool onButtonClick(QDialogButtonBox::StandardButton button) override;

private slots:
    //! Начато выполнение скрипта
    void sl_player_started();
    //! Выполнение скрипта закончено
    void sl_player_finished();

    //! Ушли с шага
    void sl_player_stepLeft(const zf::ScriptStepID& id);
    //! Перешли на шаг
    void sl_player_stepEntered(
        //! id предыдущего шага (< 0, если перешли на первый шаг)
        const zf::ScriptStepID& prev_id,
        //! id нового шага
        const zf::ScriptStepID& current_id);

    //! Плеер готов к переходу на следующий шаг (заполнены все необходимые данные)
    void sl_player_nextStepReady(const zf::ScriptStepID& id);

    //! Активирован выбор
    void sl_player_choiseActivated(const zf::ScriptStepID& step_id, const zf::ScriptChoiseID& choise_id);

    //! Пользователь активировал выбор
    void sl_choiseUserActivated(bool checked);

    void sl_splitterMoved(int pos, int index);

private:
    struct StepInfo;
    typedef std::shared_ptr<ScriptView::StepInfo> StepInfoPtr;

    void init();
    //! Обновить параметры UI, сохраненные между сессиями
    void updateSavedConfig();
    //! Привести UI к текущему шагу
    Error updateUI();
    //! Получить интерфейс текущего шага. Для шагов/функций nullptr
    StepInfoPtr getStepUI(ScriptStep* step, ScriptChoise* choise, Error& error);
    //! Информация о текущем шаге
    StepInfoPtr currentStepInfo() const;
    //! Обновить кнопки диалога
    void updateDialogButtons();

    ScriptPlayer* _player;
    //! Автоматически сгенерированные виджеты
    DataWidgets* _widgets;

    //! Данные для шагов
    struct StepInfo
    {
        StepInfo();
        ~StepInfo();

        //! Шаг
        ScriptStep* step = nullptr;

        //! Виджет шага
        QPointer<QWidget> widget;
        //! UI шага
        Ui::ScriptViewMain* ui;

        //! Группировка вариантов выбора
        QButtonGroup* radio_group;
        //! Варианты выбора - кнопки
        QList<QPointer<QRadioButton>> choise_radio;
        //! Варианты выбора
        QList<ScriptChoise*> choises;
        //! Состояние сплиттера
        Utils::SplitterSavedData splitter_data;
    };
    QMap<ScriptStepID, std::shared_ptr<StepInfo>> _steps;

    ScriptStep* _current_step = nullptr;
    ScriptChoise* _current_choise = nullptr;
};

} // namespace zf
