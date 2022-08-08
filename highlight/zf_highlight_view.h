#pragma once

#include "zf_highlight_processor.h"
#include "zf_i_highlight_view.h"

class QTableView;
class QLabel;

namespace zf
{
class ItemModel;
class HighlightMapping;

//! Отображение модели ошибок
class ZCORESHARED_EXPORT HighlightView : public QWidget, public I_ObjectExtension
{
    Q_OBJECT
public:
    HighlightView(HighlightProcessor* processor, I_HighlightView* interface,
                  //! Связь между виджетами и свойствами для обработки ошибок
                  HighlightMapping* mapping, QWidget* parent = nullptr);
    ~HighlightView();

public: // реализация I_ObjectExtension
    //! Удален ли объект
    bool objectExtensionDestroyed() const final;

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
    //! Текущий элемент
    DataProperty currentItem() const;
    //! Задать текущий элемент
    void setCurrentItem(const DataProperty& item);

    //! Обработка ошибок
    HighlightProcessor* highlightProcessor() const;

    //! Принудительная синхронизация
    void sync();

signals:
    //! Смена текущего элемента пользователем или программно
    void sg_currentChanged(const zf::DataProperty& previous, const zf::DataProperty& current);
    //! Смена текущего элемента пользователем
    void sg_currentEdited(const zf::DataProperty& previous, const zf::DataProperty& current);

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private slots:
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

    //! Сменилась текущая строка
    void sl_currentRowChanged(const QModelIndex& current, const QModelIndex& previous);
    //! Пользователь кликнул мышью
    void sl_clicked(const QModelIndex& index);

    void sl_resizeToContent();

    //! Менеджер обратных вызовов
    void sl_callbackManager(int key, const QVariant& data);

    //! Произошло изменение порядка отображения
    void sl_highlightSortOrderChanged();

private:
    //! Полная загрузка из модели ошибок
    void reload();
    //! Установить элемент в строку
    void setItem(const DataProperty& prop, int row);

    //! Задать текущий элемент
    void setCurrentItemHelper();

    //! Расчитать порядок отображения
    void calcPositions();

    DataProperty rowProperty(int row) const;

    void beginUpdate();
    void endUpdate();

    //! Принудительная синхронизация
    void syncInternal();

    //! Обновить доступность и видимость контролов
    void updateControls();

    //! Перехват нажатия клавишь
    void onStartKeyPressEvent(QKeyEvent* event);
    void onEndKeyPressEvent(QKeyEvent* event);
    //! Перехват нажатия клавишь мыши
    void onStartMousePressEvent(QMouseEvent* event);
    void onEndMousePressEvent(QMouseEvent* event);

    //! Обработка ошибок
    HighlightProcessor* _highlight_processor;
    //! Интерфейс для сортировки и т.п.
    I_HighlightView* _interface;
    //! Связь между виджетами и свойствами для обработки ошибок
    HighlightMapping* _mapping;

    //! Предыдущий элемент
    DataProperty _previous_item;
    //! Текущий элемент
    DataProperty _current_item;
    //! Отложенный запрос на смену текущего элемента
    bool _change_current_requested = false;

    //! Информация для отображения
    ItemModel* _view_model;
    //! Отображение ошибок
    QTableView* _view;
    //! Картинка ОК
    QLabel* _ok_image;

    //! Сохраненная информация о порядке отображения
    mutable QHash<DataProperty, int> _position_hash;

    //! Счетчик пользовательских событий
    int _user_event_counter = 0;

    //! Счетчик обновлений
    int _update_counter = 0;
    //! Текущий элемент до начала обновления
    DataProperty _current_before_update;

    //! Потокобезопасное расширение возможностей QObject
    ObjectExtension* _object_extension = nullptr;

    friend class HighlightTableView;
};

} // namespace zf
