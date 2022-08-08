#pragma once

#include "zf.h"
#include "zf_highlight_model.h"
#include "zf_object_extension.h"

namespace zf
{
class DataProperty;
class PropertyID;
class DataWidgets;
class WidgetHighlighter;
class HighlightProcessor;
class HighlightMapping;

//! Реация на изменение в списке ошибок и передача их в WidgetHighlighter
class WidgetHighligterController : public QObject, public I_ObjectExtension
{
    Q_OBJECT
public:
    WidgetHighligterController(
        //! Реализация обработки реакции на изменение данных и заполнения модели ошибок
        HighlightProcessor* processor,
        //! Виджеты
        DataWidgets* widgets,
        //! Индикация ошибок
        WidgetHighlighter* highlighter,
        //! Связь между виджетами и свойствами для обработки ошибок
        HighlightMapping* mapping);
    ~WidgetHighligterController();

    //! Виджеты
    DataWidgets* widgets() const;

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

private slots:
    //! Информация об ошибках: Добавлен элемент
    void sl_highlight_itemInserted(const zf::HighlightItem& item);
    //! Информация об ошибках: Удален элемент
    void sl_highlight_itemRemoved(const zf::HighlightItem& item);
    //! Информация об ошибках: Изменен элемент
    void sl_highlight_itemChanged(const zf::HighlightItem& item);
    //! Информация об ошибках: Начато групповое изменение
    void sl_highlight_beginUpdate();
    //! Информация об ошибках: Закончено групповое изменение
    void sl_highlight_endUpdate();

    //! Менеджер обратных вызовов
    void sl_callbackManager(int key, const QVariant& data);

    void sl_mappingChanged(const zf::DataProperty& source_property);
    void sl_itemSelectorReadOnlyChanged();

private:
    //! Обновить отрисовку ошибки на виджете
    void updateWidgetHighlight(const HighlightItem& item);

    //! Обновить состояние статус бара с ошибками
    void updateStatusBarErrorInfo();
    //! Загрузить всю информацию об ошибках из модели ошибок
    void loadAllHighlights();

    //! Реализация обработки реакции на изменение данных и заполнения модели ошибок
    HighlightProcessor* _highlight_processor = nullptr;
    //! Виджеты
    DataWidgets* _widgets = nullptr;
    //! Индикация ошибок
    WidgetHighlighter* _widget_highlighter = nullptr;
    //! Связь между виджетами и свойствами для обработки ошибок
    HighlightMapping* _mapping;

    //! Список элементов Highlight, отображение которых, надо обновить
    QSet<HighlightItem> _items_to_update;

    //! Потокобезопасное расширение возможностей QObject
    ObjectExtension* _object_extension = nullptr;
};
} // namespace zf
