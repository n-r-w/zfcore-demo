#pragma once

#include "zf_error.h"

#include <QDialogButtonBox>
#include <QHBoxLayout>

namespace zf
{
class Operation;

//! Интерфейс управления модальным окном. Встраивается в представление
class I_ModalWindow
{
public:
    virtual ~I_ModalWindow() { }

    //! Параметры
    virtual ModuleWindowOptions options() const = 0;
    //! В режиме модального редактирования/просмотра, вызывает нажатие на кнопку Ок или Сохранить
    virtual Error invokeModalWindowOkSave(
        //! Если истина, то закрывать с ОК, но без сохранения (если оно нужно)
        bool force_not_save = false)
        = 0;
    //! В режиме модального редактирования, вызывает нажатие на кнопку Отменить
    virtual void invokeModalWindowCancel(
        //! Игнорировать не сохраненные данные
        bool force = false)
        = 0;

    //! В режиме модального редактирования/просмотра: Управляющий блок кнопок
    virtual QDialogButtonBox* modalWindowButtonBox() const = 0;
    //! В режиме модального редактирования/просмотра: Набор управляющих кнопок
    virtual QDialogButtonBox::StandardButtons modalWindowButtonRoles() const = 0;
    //! В режиме модального редактирования/просмотра: Все кнопки. Включает вообще все кнопки на форме, не только относящиеся к нижней панели
    virtual QList<QPushButton*> modalWindowButtons() const = 0;
    //! В режиме модального редактирования/просмотра: Кнопка по типу
    virtual QPushButton* modalWindowButton(QDialogButtonBox::StandardButton buttonType) const = 0;
    //! В режиме модального редактирования/просмотра: Кнопка по операции
    virtual QPushButton* modalWindowButton(const OperationID& op) const = 0;
    //! В режиме модального редактирования/просмотра: Layout в котором находится QDialogButtonBox
    virtual QHBoxLayout* modalWindowButtonsLayout() const = 0;

    //! Подсветить кнопку цветом (по типу)
    virtual void modalWindowSetButtonHighlightFrame(QDialogButtonBox::StandardButton button, InformationType info_type) = 0;
    //! Подсветить кнопку цветом (по операции)
    virtual void modalWindowSetButtonHighlightFrame(const OperationID& op, InformationType info_type) = 0;

    //! Убрать подсветку кнопки цветом (по типу)
    virtual void modalWindowRemoveButtonHighlightFrame(QDialogButtonBox::StandardButton button) = 0;
    //! Убрать подсветку кнопки цветом (по операции)
    virtual void modalWindowRemoveButtonHighlightFrame(const OperationID& op) = 0;

    //! Цвет подсветки кнопки (по типу)
    virtual InformationType modalWindowButtonHighlightFrame(QDialogButtonBox::StandardButton button) const = 0;
    //! Цвет подсветки кнопки (по операции)
    virtual InformationType modalWindowButtonHighlightFrame(const OperationID& op) const = 0;

    //! Перейти на виджет с первой ошибкой
    virtual void focusError() = 0;
    //! Перейти на следующий виджет с ошибкой
    virtual void focusNextError(InformationTypes info_type) = 0;

    //! Линия разделения кнопок и рабочей области
    virtual void modalWindowSetBottomLineVisible(bool b) = 0;

    //! Вызывается представлением перед активацией операции назначенной на кнопку через setModalWindowOperations
    //! Если возвращает false, то операция не вызывается
    //! Не вызывать вручную!
    virtual bool beforeModalWindowOperationExecuted(const Operation& op) = 0;
    //! Вызывается представлением перед активацией операции назначенной на кнопку через setModalWindowOperations
    //! Если возвращает false, то операция не вызывается
    //! Не вызывать вручную!
    virtual void afterModalWindowOperationExecuted(const Operation& op, const zf::Error& error) = 0;
};

} // namespace zf
