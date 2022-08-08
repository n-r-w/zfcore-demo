#pragma once

#include "zf_highlight_dialog.h"
#include "zf_view.h"
#include "zf_i_modal_window.h"

namespace Ui
{
class ModalWindow;
}

namespace zf
{
class HighlightView;
class HightlightViewController;

class ModalWindow : public HighlightDialog, public I_ModalWindow
{
    Q_OBJECT

public:
    explicit ModalWindow(View* view, const ModuleWindowOptions& options);
    ~ModalWindow() override;

    //! Ошибка сохранения или другой операции
    Error operationError() const;

public: // I_ModalWindow
    //! Параметры
    ModuleWindowOptions options() const override;
    //! В режиме модального редактирования/просмотра, вызывает нажатие на кнопку Ок или Сохранить
    Error invokeModalWindowOkSave(
        //! Если истина, то закрывать с ОК, но без сохранения (если оно нужно)
        bool force_not_save) override;
    //! В режиме модального редактирования, вызывает нажатие на кнопку Отменить
    void invokeModalWindowCancel(
        //! Игнорировать не сохраненные данные
        bool force) override;
    //! В режиме модального редактирования/просмотра: Управляющий блок кнопок
    QDialogButtonBox* modalWindowButtonBox() const override;
    //! В режиме модального редактирования/просмотра: Набор управляющих кнопок
    QDialogButtonBox::StandardButtons modalWindowButtonRoles() const override;
    //! В режиме модального редактирования/просмотра: Все кнопки
    QList<QPushButton*> modalWindowButtons() const override;
    //! В режиме модального редактирования/просмотра: Кнопка по типу
    QPushButton* modalWindowButton(QDialogButtonBox::StandardButton buttonType) const override;
    //! В режиме модального редактирования/просмотра: Кнопка по операции
    QPushButton* modalWindowButton(const OperationID& op) const override;
    //! В режиме модального редактирования/просмотра: Layout в котором находится QDialogButtonBox
    QHBoxLayout* modalWindowButtonsLayout() const override;

    //! Подсветить кнопку цветом (по типу)
    void modalWindowSetButtonHighlightFrame(QDialogButtonBox::StandardButton button, InformationType info_type) override;
    //! Подсветить кнопку цветом (по операции)
    void modalWindowSetButtonHighlightFrame(const OperationID& op, InformationType info_type) override;

    //! Убрать подсветку кнопки цветом (по типу)
    void modalWindowRemoveButtonHighlightFrame(QDialogButtonBox::StandardButton button) override;
    //! Убрать подсветку кнопки цветом (по операции)
    void modalWindowRemoveButtonHighlightFrame(const OperationID& op) override;

    //! Цвет подсветки кнопки (по типу)
    InformationType modalWindowButtonHighlightFrame(QDialogButtonBox::StandardButton button) const override;
    //! Цвет подсветки кнопки (по операции)
    InformationType modalWindowButtonHighlightFrame(const OperationID& op) const override;

    //! Перейти на виджет с первой ошибкой
    void focusError() override;
    //! Перейти на следующий виджет с ошибкой
    void focusNextError(InformationTypes info_type) override;

    //! Линия разделения кнопок и рабочей области
    void modalWindowSetBottomLineVisible(bool b) override;

    //! Вызывается представлением перед активацией операции назначенной на кнопку через setModalWindowOperations
    //! Если возвращает false, то операция не вызывается
    //! Не вызывать вручную!
    bool beforeModalWindowOperationExecuted(const Operation& op) override;
    //! Вызывается представлением перед активацией операции назначенной на кнопку через setModalWindowOperations
    //! Если возвращает false, то операция не вызывается
    //! Не вызывать вручную!
    void afterModalWindowOperationExecuted(const Operation& op, const zf::Error& error) override;

protected:
    void beforePopup() override;
    //! После отображения диалога
    void afterPopup() override;
    //! Перед скрытием диалога
    void beforeHide() override;
    //! После скрытия диалога
    void afterHide() override;
    //! Была нажата кнопка. Если событие обработано и не требуется стандартная реакция - вернуть true
    bool onButtonClick(QDialogButtonBox::StandardButton button) override;
    //! Изменение размера при запуске
    void adjustDialogSize() override;
    //! Служебный обработчик менеджера обратных вызовов (не использовать)
    void onCallbackManagerInternal(int key, const QVariant& data) override;
    //! Закрытие через крестик. Если возвращает false, то закрытие отменяется
    bool onCloseByCross() override;

    void dragEnterEvent(QDragEnterEvent* event) override;
    void dragMoveEvent(QDragMoveEvent* event) override;
    void dropEvent(QDropEvent* event) override;
    void dragLeaveEvent(QDragLeaveEvent* event) override;
    void keyPressEvent(QKeyEvent* e) override;

private slots:
    void checkEnabled();
    //! Изменилось имя сущности
    void sl_entityNameChanged();
    //! Завершилась обработка операции
    void sl_operationProcessed(const zf::Operation& op, const zf::Error& error);

private:
    //! Создать список кнопок
    static QList<int> buttonsList(View* view);
    //! Создать список текстовых названий кнопок
    static QStringList buttonsText(View* view);
    //! Создать список ролей кнопок
    static QList<int> buttonsRole(View* view);

    void checkEnabledHelper();

    //! Создать тулбар
    void createToolbar();
    void removeToolbars();

    //! Настроить кнопки экшенов view
    void bootstrapViewActions();

    //! Доступна ли кнопка ОК
    bool isOkEnabled() const;
    //! Есть ли ошибки
    bool isErrorExists(bool force_check) const;

    //! Загрузить настройки
    void loadConfiguration();
    //! Сохранить настройки
    void saveConfiguration();

    //! Установка фокуса на view
    void setFocus(const zf::DataProperty& property);

    //! Есть ли несохраненные данные
    bool hasUnsavedData(
        //! Ошибка
        Error& error) const;

    //! Обработка результата вызова onModalWindowButtonClick. Если возвращается true, то дельнейшие стандартные действия отменяются
    bool processModalWindowButtonClickResult(View::ModalWindowButtonAction action);

    Ui::ModalWindow* _ui;
    QPointer<View> _view;
    Uid _entity_uid;

    //! Ошибка сохранения или другой операции
    Error _operation_error;

    //! Параметры
    ModuleWindowOptions _options;

    //! Цвет подсветки кнопки для операций
    QMap<Operation, InformationType> _op_color_frames;
};

} // namespace zf
