#pragma once

#include <QMdiSubWindow>
#include "zf_mdi_area.h"
#include "zf_view.h"

namespace Ui
{
class ModuleWindow;
}

namespace zf
{
class ZCORESHARED_EXPORT ModuleWindow : public QMdiSubWindow, public I_ObjectExtension, public I_ModuleWindow
{
    Q_OBJECT

public:
    explicit ModuleWindow(const std::weak_ptr<View>& view, MdiArea* mdi_area, const ModuleWindowOptions& options);
    ~ModuleWindow();

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
    //! Представление
    ViewPtr view() const;
    //! Модель
    ModelPtr model() const;
    //! Закрыто
    bool isClosed() const;

    //! Идентификаторы рабочих зон
    QList<int> workZoneIds() const;
    //! Добавить в рабочую зону
    void addToWorkZone(int work_zone_id);
    //! Удалить из рабочей зоны
    void removeFromWorkZone(int work_zone_id);

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

    //! Показать окно
    void show(
        //! Надо ли показать открытое окно или показать но оставить в статусе минимизировано если оно в нем
        bool raise);

    void setVisible(bool b) override;

    //! Параметры
    ModuleWindowOptions options() const override;

protected:
    bool eventFilter(QObject* object, QEvent* event) override;
    void closeEvent(QCloseEvent* closeEvent) override;
    void showEvent(QShowEvent* showEvent) override;
    void hideEvent(QHideEvent* hideEvent) override;
    void resizeEvent(QResizeEvent* resizeEvent) override;
    void moveEvent(QMoveEvent* moveEvent) override;
    void keyPressEvent(QKeyEvent* e) override;

    void focusOutEvent(QFocusEvent* focusOutEvent) override;
    void focusInEvent(QFocusEvent* focusInEvent) override;

    void dragEnterEvent(QDragEnterEvent* event) override;
    void dragMoveEvent(QDragMoveEvent* event) override;
    void dropEvent(QDropEvent* event) override;
    void dragLeaveEvent(QDragLeaveEvent* event) override;

signals:
    //! Окно закрыто
    void sg_closed(const zf::Uid& entity_uid);
    //! Изменился идентификатор
    void sg_entityChanged(const zf::Uid& old_entity, const zf::Uid& new_entity);
    //! Окно добавлено в рабочую зону
    void sg_addedToWorkZone(int work_zone_id);
    //! Окно удалено из рабочей зоны
    void sg_removedFromWorkZone(int work_zone_id);

private slots:
    //! Изменилось состояние окна
    void sl_windowStateChanged(Qt::WindowStates oldState, Qt::WindowStates newState);

    //! Входящие сообщения от диспетчера
    void sl_message_dispatcher_inbound(const zf::Uid& sender, const zf::Message& message, zf::SubscribeHandle subscribe_handle);
    //! Изменился признак существования объекта в БД
    void sl_existsChanged();
    //! Изменилось имя сущности
    void sl_entityNameChanged();
    //! Обработчик менеджера обратных вызовов
    void sl_callbackManager(int key, const QVariant& data);
    //! Изменился текущий выбор виджета поиска searchWidget (действие пользователя)
    void sl_searchWidgetEdited(
        //! Какой идентификатор был выбран
        const zf::Uid& entity);

    //! Сигнал - удалена рабочая зона
    void sl_workZoneRemoved(int id);
    //! Сигнал - сменилась текущая рабочая зона
    void sl_currentWorkZoneChanged(int previous_id, int current_id);

    //! Изменилась видимость свойства
    void sl_fieldVisibleChanged(const zf::DataProperty& property, QWidget* widget);

    //! Завершена загрузка
    void sl_model_finishLoad(const zf::Error& error,
        //! Параметры загрузки
        const zf::LoadOptions& load_options,
        //! Какие свойства обновлялись
        const zf::DataPropertySet& properties);

    //! Завершилась обработка операции
    void sl_operationProcessed(const zf::Operation& op, const zf::Error& error);

private:
    //! Загрузить конфигурацию. Возвращает истину, если размер окна был изменен
    bool loadConfiguration();
    void saveConfiguration();
    void requestSaveConfiguration();

    //! Подключение к представлению
    void connectToView();

    //! Подключение к модели
    void connectToModel();
    //! Отключение от модели
    void disconnectFromModel();

    //! Создать тулбар
    void createToolbar();
    void removeToolbars();

    //! Проверка доступности поля поиска
    void checkSearchEnabled();

    //! Изменение размера
    void resizeHelper();
    void requestResize(const QSize& new_size);

    //! Разместить окно (алгоритм позиционирования окон)
    void placeWindow();

    //! Нужно ли активировать поле поиска в наборах данных
    bool needSearch() const;

    //! Набор данных для поиска на уровне всего окна
    DataProperty searchDataset() const;

    QSize mdiSize() const;

    Ui::ModuleWindow* _ui = nullptr;
    std::weak_ptr<View> _view;
    MdiArea* _mdi_area = nullptr;
    //! Поле поиска
    QFrame* _search_widget = nullptr;
    LineEdit* _search_editor = nullptr;

    bool _closed = false;
    bool _size_initialized = false;
    bool _toolbar_created = false;
    bool _destroying = false;
    Uid _entity_uid;
    QList<int> _work_zone_ids;

    //! Параметры
    ModuleWindowOptions _options;

    //! Подписка на удаление
    SubscribeHandle _remove_handle;

    //! Потокобезопасное расширение возможностей QObject
    ObjectExtension* _object_extension = nullptr;
};

} // namespace zf
