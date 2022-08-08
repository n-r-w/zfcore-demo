#pragma once

#include "zf_data_structure.h"
#include "zf_highlight_model.h"
#include "zf_i_hightlight_view_controller.h"
#include "zf_object_extension.h"

namespace zf
{
class DataWidgets;
class HighlightView;
class HighlightMapping;
class View;

//! Класс для координации поведения виджетов и панели ошибок
class ZCORESHARED_EXPORT HightlightViewController : public QObject, public I_ObjectExtension
{
    Q_OBJECT
public:
    HightlightViewController(I_HightlightViewController* interface, HighlightView* highlight_view,
                             //! Связь между виджетами и свойствами для обработки ошибок
                             HighlightMapping* mapping);
    ~HightlightViewController();

public: // реализация I_ObjectExtension
    //! Удален ли объект
    bool objectExtensionDestroyed() const final;
    ;
    //! Безопасно удалить объект
    void objectExtensionDestroy() override;

    //! Получить данные
    QVariant objectExtensionGetData(
        //! Ключ данных
        const QString& data_key) const final;
    //! Записать данные
    //! Возвращает признак - были ли записаны данные
    bool objectExtensionSetData(
        //! Ключ данных
        const QString& data_key, const QVariant& value,
        //! Замещение. Если false, то при наличии такого ключа, данные не замещаются и возвращается false
        bool replace) const final;

    //! Другой объект сообщает этому, что будет хранить указатель на него
    void objectExtensionRegisterUseExternal(I_ObjectExtension* i) final;
    //! Другой объект сообщает этому, что перестает хранить указатель на него
    void objectExtensionUnregisterUseExternal(I_ObjectExtension* i) final;

    //! Другой объект сообщает этому, что будет удален и надо перестать хранить указатель на него
    //! Реализация этого метода, кроме вызова ObjectExtension::objectExtensionDeleteInfoExternal должна
    //! очистить все ссылки на указанный объект
    void objectExtensionDeleteInfoExternal(I_ObjectExtension* i) final;

    //! Этот объект начинает использовать другой объект
    void objectExtensionRegisterUseInternal(I_ObjectExtension* i) final;
    //! Этот объект прекращает использовать другой объект
    void objectExtensionUnregisterUseInternal(I_ObjectExtension* i) final;

public:
    //! Перейти на виджет с ошибкой
    void focusError(View* form);
    //! Перейти на следующий виджет с ошибкой
    void focusNextError(View* form, InformationTypes info_type);

private slots:
    //! Изменился фокус виджета на форме
    void sl_focusChanged(QWidget* previous_widget, const zf::DataProperty& previous_property, QWidget* current_widget,
                         const zf::DataProperty& current_property);

    //! Смена текущего элемента пользователем или программно на панели ошибок
    void sl_currentChanged(const zf::DataProperty& previous, const zf::DataProperty& current);
    //! Смена текущего элемента пользователем на панели ошибок
    void sl_currentEdited(const zf::DataProperty& previous, const zf::DataProperty& current);

    //! Менеджер обратных вызовов
    void sl_callbackManager(int key, const QVariant& data);

    //! Добавлен элемент
    void sl_itemInserted(const zf::HighlightItem& item);
    //! Удален элемент
    void sl_itemRemoved(const zf::HighlightItem& item);
    //! Изменен элемент
    void sl_itemChanged(const zf::HighlightItem& item);
    //! Начато групповое изменение
    void sl_beginUpdate();
    //! Закончено групповое изменение
    void sl_endUpdate();

private:
    //! Синхронизация панели ошибок с формой
    void synchronize();
    void synchronizeHelper();
    //! Обработка запроса на смену фокуса
    void processFocusRequest();
    //! Установка фокуса на view
    void setFocus(const zf::DataProperty& property);
    //! Модель ошибок
    HighlightModel* highlightModel(bool execute_check) const;

    I_HightlightViewController* _interface;
    HighlightView* _highlight_view;
    //! Связь между виджетами и свойствами для обработки ошибок
    HighlightMapping* _mapping;

    int _block_counter = 0;
    DataPropertyPtr _focus_request;

    //! Потокобезопасное расширение возможностей QObject
    ObjectExtension* _object_extension = nullptr;
};

} // namespace zf
