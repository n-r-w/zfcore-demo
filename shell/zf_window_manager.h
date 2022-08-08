#ifndef ZF_WINDOW_MANAGER_H
#define ZF_WINDOW_MANAGER_H

#include "zf.h"
#include "zf_callback.h"
#include "zf_error.h"
#include "zf_model.h"
#include "zf_uid.h"
#include "zf_mdi_area.h"
#include "zf_i_work_zones.h"

namespace zf
{
class ModuleWindow;
class AllWindowsDialog;
class View;
typedef std::shared_ptr<View> ViewPtr;

class ZCORESHARED_EXPORT WindowManager : public QObject, public I_ObjectExtension
{
    Q_OBJECT
public:
    WindowManager(MdiArea* mdi_area);
    ~WindowManager();

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
    //! Список открытых окон
    QList<ModuleWindow*> windows() const;
    //! Список открытых окон текущей рабочей зоны
    QList<ModuleWindow*> windowsCurrentWorkZone() const;
    //! Найти все окна с указанным идентификатором сущности
    QList<ModuleWindow*> findModuleWindows(const Uid& entity_uid) const;
    //! Найти все окна по указателю модели
    QList<ModuleWindow*> findModuleWindows(const ModelPtr& model) const;

    //! Текущее активное окно
    ModuleWindow* activeWindow() const;
    //! Убрать активное окно
    void setNoActiveWindow();

    //! Открыть окно просмотра сущности по ее идентификатору. Если окно с таким uid уже отрыто, то открывается оно.
    //! ВАЖНО: если модель с таким идетификатором уже загружена, то будет открыт именно этот экземпляр без содания копии.
    Error openWindow(
        //! Идентификатор сущности
        const Uid& uid,
        //! Параметры открытия модального диалога
        const ModuleWindowOptions& options = ModuleWindowOption::NoOptions,
        //! Код для открытия представления
        int code = 0,
        //! Произвольные данные, передаваемые в метод View::onDataReady
        const QMap<QString, QVariant>& custom_data = {});
    //! Открыть окно просмотра сущности по ее идентификатору. Если окно с таким uid уже отрыто, то открывается оно.
    //! ВАЖНО: если модель с таким идетификатором уже загружена, то будет открыт именно этот экземпляр без содания копии.
    Error openWindow(
        //! Идентификатор сущности
        const Uid& uid,
        //! Модель, созданная при открытии окна
        ModelPtr& model,
        //! Представление, созданное при открытии окна
        ViewPtr& view,
        //! Параметры открытия модального диалога
        const ModuleWindowOptions& options = ModuleWindowOption::NoOptions,
        //! Код для открытия представления
        int code = 0,
        //! Произвольные данные, передаваемые в метод View::onDataReady
        const QMap<QString, QVariant>& custom_data = {});
    //! Открыть окно просмотра сущности по указателю на модель. Если окно с такой моделью уже отрыто, то открывается оно.
    //! Открывается именно переданная модель без создания копии.
    Error openWindow(
        //! Модель
        const ModelPtr& model,
        //! Представление, созданное при открытии окна
        ViewPtr& view,
        //! Параметры открытия модального диалога
        const ModuleWindowOptions& options = ModuleWindowOption::NoOptions,
        //! Код для открытия представления
        int code = 0,
        //! Произвольные данные, передаваемые в метод View::onDataReady
        const QMap<QString, QVariant>& custom_data = {});

    //! Открыть окно редактирования (в модальном режиме) для произвольного указателя не модель.
    //! ВАЖНО: Копии данных модели не производится и все правки идут прямо в указанной модели.
    bool editWindow(
        //! Модель
        const ModelPtr& model,
        //! Ошибка
        Error& error,
        //! Параметры открытия модального диалога
        const ModuleWindowOptions& options = ModuleWindowOption::NoOptions,
        //! Код для открытия представления
        int code = 0,
        //! Произвольные данные, передаваемые в метод View::onDataReady
        const QMap<QString, QVariant>& custom_data = {});
    //! Открыть окно редактирования (в модальном режиме) по идентификатору сущности.
    //! Если модель с таким идентификатором уже загружена, то перед редактированием создается ее копия.
    bool editWindow(
        //! Идентификатор сущности
        const Uid& uid,
        //! Ошибка
        Error& error,
        //! Параметры открытия модального диалога
        const ModuleWindowOptions& options = ModuleWindowOption::NoOptions,
        //! Код для открытия представления
        int code = 0,
        //! Произвольные данные, передаваемые в метод View::onDataReady
        const QMap<QString, QVariant>& custom_data = {});
    //! Открыть окно редактирования (в модальном режиме) по идентификатору сущности.
    //! Если модель с таким идентификатором уже загружена, то перед редактированием создается ее копия.
    bool editWindow(
        //! Идентификатор сущности
        const Uid& uid,
        //! Модель, созданная при открытии окна
        ModelPtr& model,
        //! Ошибка
        Error& error,
        //! Параметры открытия модального диалога
        const ModuleWindowOptions& options = ModuleWindowOption::NoOptions,
        //! Код для открытия представления
        int code = 0,
        //! Произвольные данные, передаваемые в метод View::onDataReady
        const QMap<QString, QVariant>& custom_data = {});
    /*! Открыть окно редактирования сущности (в модальном режиме) с вызовом callback функции
     *  Пример лямбда CallbackModel функции:
     *  [this](zf::ModelPtr model) -> zf::Error { return model->setValue(DocAttach::PID::OWNER_UID, entityUid()); }
     *   Пример лямбда CallbackView функции:
     *  [this](zf::ViewPtr view) -> zf::Error { return view->object<QWidget>("xxx")->setHidden(true); } */
    template <typename CallbackModel, typename CallbackView>
    bool editWindow(
        //! Идентификатор сущности
        const Uid& uid,
        //! Функция, которая будет вызвана перед передачей модели на редактирование
        CallbackModel callback_model,
        //! Функция, которая будет вызвана перед передачей модели на редактирование
        CallbackView callback_view,
        //! Ошибка
        Error& error,
        //! Параметры открытия модального диалога
        const ModuleWindowOptions& options = ModuleWindowOption::NoOptions,
        //! Код для открытия представления
        int code = 0,
        //! Произвольные данные, передаваемые в метод View::onDataReady
        const QMap<QString, QVariant>& custom_data = {})
    {
        error.clear();
        auto model = editWindowHelperModelGet(uid, options, error);
        if (error.isError())
            return false;

        error = callback_model(model);
        if (error.isError())
            return false;

        auto view = editWindowHelperViewGet(model, code, options, error);
        if (error.isError())
            return false;

        error = callback_view(view);
        if (error.isError())
            return false;

        return editNewWindowHelper(view, options, custom_data, error);
    }

    /*! Открыть окно редактирования сущности (в модальном режиме) с вызовом callback функции
     *  Пример лямбда CallbackModel функции:
     *  [this](zf::ModelPtr model) -> zf::Error { return model->setValue(DocAttach::PID::OWNER_UID, entityUid()); }
     *   Пример лямбда CallbackView функции:
     *  [this](zf::ViewPtr view) -> zf::Error { return view->object<QWidget>("xxx")->setHidden(true); } */
    template <typename CallbackModel, typename CallbackView>
    bool editWindow(
        //! Идентификатор сущности
        const Uid& uid,
        //! Функция, которая будет вызвана перед передачей модели на редактирование
        CallbackModel callback_model,
        //! Функция, которая будет вызвана перед передачей модели на редактирование
        CallbackView callback_view,
        //! Модель, созданная при открытии окна
        ModelPtr& model,
        //! Ошибка
        Error& error,
        //! Параметры открытия модального диалога
        const ModuleWindowOptions& options = ModuleWindowOption::NoOptions,
        //! Код для открытия представления
        int code = 0,
        //! Произвольные данные, передаваемые в метод View::onDataReady
        const QMap<QString, QVariant>& custom_data = {})
    {
        error.clear();
        model = editWindowHelperModelGet(uid, options, error);
        if (error.isError())
            return false;

        error = callback_model(model);
        if (error.isError())
            return false;

        auto view = editWindowHelperViewGet(model, code, options, error);
        if (error.isError())
            return false;

        error = callback_view(view);
        if (error.isError())
            return false;

        return editNewWindowHelper(view, options, custom_data, error);
    }

    //! Открыть окно создания новой сущности (в модальном режиме)
    bool editNewWindow(
        //! Идентификатор сущности
        const EntityCode& entity_code,
        //! Ошибка
        Error& error,
        //! Параметры открытия модального диалога
        const ModuleWindowOptions& options = ModuleWindowOption::NoOptions,
        //! Код для открытия представления
        int code = 0,
        //! Идентификатор БД
        const DatabaseID& database_id = {},
        //! Произвольные данные, передаваемые в метод View::onDataReady
        const QMap<QString, QVariant>& custom_data = {});
    //! Открыть окно создания новой сущности (в модальном режиме)
    bool editNewWindow(
        //! Идентификатор сущности
        const EntityCode& entity_code,
        //! Модель, созданная при открытии окна
        ModelPtr& model,
        //! Ошибка
        Error& error,
        //! Параметры открытия модального диалога
        const ModuleWindowOptions& options = ModuleWindowOption::NoOptions,
        //! Код для открытия представления
        int code = 0,
        //! Идентификатор БД
        const DatabaseID& database_id = {},
        //! Произвольные данные, передаваемые в метод View::onDataReady
        const QMap<QString, QVariant>& custom_data = {});
    /*! Открыть окно создания новой сущности (в модальном режиме) с вызовом callback функции
     *  Пример лямбда CallbackModel функции:
     *  [this](zf::ModelPtr model) -> zf::Error { return model->setValue(DocAttach::PID::OWNER_UID, entityUid()); }
     *   Пример лямбда CallbackView функции:
     *  [](zf::ViewPtr view) -> zf::Error { view->object<QWidget>("xxx")->setHidden(true); return zf::Error(); }*/
    template <typename CallbackModel, typename CallbackView>
    bool editNewWindow(
        //! Идентификатор сущности
        const EntityCode& entity_code,
        //! Функция, которая будет вызвана перед передачей модели на редактирование
        CallbackModel callback_model,
        //! Функция, которая будет вызвана перед передачей модели на редактирование
        CallbackView callback_view,
        //! Ошибка
        Error& error,
        //! Параметры открытия модального диалога
        const ModuleWindowOptions& options = ModuleWindowOption::NoOptions,
        //! Код для открытия представления
        int code = 0,
        //! Идентификатор БД
        const DatabaseID& database_id = {},
        //! Произвольные данные, передаваемые в метод View::onDataReady
        const QMap<QString, QVariant>& custom_data = {})
    {
        error.clear();
        auto model = editNewWindowHelperModelGet(entity_code, database_id, options, error);
        if (error.isError())
            return false;

        error = callback_model(model);
        if (error.isError())
            return false;

        auto view = editNewWindowHelperViewGet(model, code, options, error);
        if (error.isError())
            return false;

        error = callback_view(view);
        if (error.isError())
            return false;

        return editNewWindowHelper(view, options, custom_data, error);
    }

    //! Открыть окно просмотра (в модальном режиме) для произвольного указателя не модель
    bool viewWindow(
        //! Модель
        const ModelPtr& model,
        //! Ошибка
        Error& error,
        //! Параметры открытия модального диалога
        const ModuleWindowOptions& options = ModuleWindowOption::NoOptions,
        //! Код для открытия представления
        int code = 0,
        //! Произвольные данные, передаваемые в метод View::onDataReady
        const QMap<QString, QVariant>& custom_data = {});
    //! Открыть окно просмотра (в модальном режиме) по идентификатору сущности
    bool viewWindow(
        //! Идентификатор сущности
        const Uid& uid,
        //! Ошибка
        Error& error,
        //! Параметры открытия модального диалога
        const ModuleWindowOptions& options = ModuleWindowOption::NoOptions,
        //! Код для открытия ппредставления
        int code = 0);
    /*! Открыть окно просмотра сущности (в модальном режиме) с вызовом callback функции
     *  Пример лямбда CallbackModel функции:
     *  [this](zf::ModelPtr model) -> zf::Error { return model->setValue(DocAttach::PID::OWNER_UID, entityUid()); }
     *   Пример лямбда CallbackView функции:
     *  [this](zf::ViewPtr view) -> zf::Error { return view->object<QWidget>("xxx")->setHidden(true); } */
    template <typename CallbackModel, typename CallbackView>
    bool viewWindow(
        //! Идентификатор сущности
        const Uid& uid,
        //! Функция, которая будет вызвана перед передачей модели на редактирование
        CallbackModel callback_model,
        //! Функция, которая будет вызвана перед передачей модели на редактирование
        CallbackView callback_view,
        //! Ошибка
        Error& error,
        //! Параметры открытия модального диалога
        const ModuleWindowOptions& options = ModuleWindowOption::NoOptions,
        //! Код для открытия представления
        int code = 0,
        //! Произвольные данные, передаваемые в метод View::onDataReady
        const QMap<QString, QVariant>& custom_data = {})
    {
        error.clear();
        auto model = viewWindowHelperModelGet(uid, options, error);
        if (error.isError())
            return false;

        error = callback_model(model);
        if (error.isError())
            return false;

        auto view = viewWindowHelperViewGet(model, code, options, error);
        if (error.isError())
            return false;

        error = callback_view(view);
        if (error.isError())
            return false;

        return viewWindowHelper(view, options, custom_data, error);
    }

    /*! Открыть окно просмотра сущности (в модальном режиме) с вызовом callback функции
     *  Пример лямбда CallbackModel функции:
     *  [this](zf::ModelPtr model) -> zf::Error { return model->setValue(DocAttach::PID::OWNER_UID, entityUid()); }
     *   Пример лямбда CallbackView функции:
     *  [this](zf::ViewPtr view) -> zf::Error { return view->object<QWidget>("xxx")->setHidden(true); } */
    template <typename CallbackModel, typename CallbackView>
    bool viewWindow(
        //! Идентификатор сущности
        const Uid& uid,
        //! Функция, которая будет вызвана перед передачей модели на редактирование
        CallbackModel callback_model,
        //! Функция, которая будет вызвана перед передачей модели на редактирование
        CallbackView callback_view,
        //! Модель, созданная при открытии окна
        ModelPtr& model,
        //! Ошибка
        Error& error,
        //! Параметры открытия модального диалога
        const ModuleWindowOptions& options = ModuleWindowOption::NoOptions,
        //! Код для открытия представления
        int code = 0,
        //! Произвольные данные, передаваемые в метод View::onDataReady
        const QMap<QString, QVariant>& custom_data = {})
    {
        error.clear();
        model = viewWindowHelperModelGet(uid, options, error);
        if (error.isError())
            return false;

        error = callback_model(model);
        if (error.isError())
            return false;

        auto view = viewWindowHelperViewGet(model, code, options, error);
        if (error.isError())
            return false;

        error = callback_view(view);
        if (error.isError())
            return false;

        return viewWindowHelper(view, options, custom_data, error);
    }

    //! Рабочие зоны
    I_WorkZones* workZones() const;

    //! Найти идентификатор рабочей зоны по идентификатору сущности, установленной через setEntityToWorkZone
    //! Если не найдено: -1
    int entityWorkZone(const Uid& uid) const;
    //! Найти идентификатор сущности, установленной через setEntityToWorkZone по идентификатор рабочей зоны
    Uid entityWorkZone(int id) const;
    //! Список идентификаторов сущностей всех рабочих зон, установленных через setEntityToWorkZone
    UidList entityWorkZones() const;

    //! Все представления рабочих зон
    QList<View*> entityWorkZoneViews() const;
    //! Представление рабочей зоны
    View* entityWorkZoneView(const Uid& uid) const;
    View* entityWorkZoneView(int id) const;

    bool isWorkZoneEnabled(const Uid& uid) const;
    bool isWorkZoneEnabled(int id) const;

    //! Представление текущей рабочей зоны
    View* currentEntityWorkZoneView() const;

    //! Установить представление сущности в указанную рабочую зону. В эту рабочую зону ранее не должно быть установлено ничего либо другая
    //! сущность через этот метод. Если до этого туда был установлен виджет минуя этот метод, то он удаляется.
    Error setEntityToWorkZone(
        //! Идентификатор рабочей зоны
        int work_zone_id,
        //! Идентификатор сущности
        const Uid& uid,
        //! Какие свойства надо загружать
        const DataPropertyList& properties,
        //! Код для открытия представления
        int code = 0);
    Error setEntityToWorkZone(
        //! Идентификатор рабочей зоны
        int work_zone_id,
        //! Идентификатор сущности
        const Uid& uid,
        //! Какие свойства надо загружать
        const PropertyIDList& properties,
        //! Код для открытия представления
        int code = 0);

    //! Устновить фоновый виджет для области MDI. Возвращает предыдущий. За удаление отвечает вызывающий.
    //! Эта функция не должна использоваться совместно с рабочими зонами, т.к. рабочие зоны устанавливают свои виджеты именно через нее.
    QWidget* setMdiBackground(QWidget* w);

    //! Открыть существующее окно модуля
    void openExistWindow(ModuleWindow* w);

    //! Запросить закрытие приложения
    bool askClose();

    //! Закрыть активное окно
    void closeActiveWindow();
    //! Закрыть все окна
    void closeAllWindows();
    //! Расположить рядом
    void tileWindows();
    //! Расположить друг за другом
    void cascadeWindows();
    //! Выбрать следующее окно
    void activateNextWindow();
    //! Выбрать предыдущее окно
    void activatePreviousWindow();

    //! Свернуть все окна
    void minimizeWindows();
    //! Развернуть все окна
    void restoreWindows();
    //! Есть ли хотябы одно не минимизированное окно в текущей рабочей зоне
    bool hasNotMimimized() const;
    //! Есть ли хотябы одно минимизированное окно в текущей рабочей зоне
    bool hasMimimized() const;

    //! Сделать невалидными модели всех видимых окон
    void invalidateVisibleWindows();

    //! Открыть окно отладки
    void openDebugWindow();

    //! Последнее место размещения окна
    static QPoint lastPlace();

signals:
    //! Новое окно
    void sg_openMdiWindow(zf::View* view);
    //! Окно закрыто
    void sg_closeMdiWindow(zf::View* view);

    //! Новое окно добавлено в рабочую зону
    void sg_mdiWindowAddedToWorkZone(zf::View* view);

    //! Сигнал - добавилась рабочая зона
    void sg_workZoneInserted(int id);
    //! Сигнал - удалена рабочая зона
    void sg_workZoneRemoved(int id);
    //! Сигнал - сменилась текущая рабочая зона
    void sg_currentWorkZoneChanged(int previous_id, int current_id);
    //! Сигнал - изменилась доступность рабочей зоны
    void sg_workZoneEnabledChanged(int id, bool enabled);

    //! Изменилось состояние окна
    void sg_windowStateChanged(QMdiSubWindow* window, zf::View* view, Qt::WindowStates old_state, Qt::WindowStates new_state);

private slots:
    //! Изменилось состояние окна
    void sl_windowStateChanged(Qt::WindowStates old_state, Qt::WindowStates new_state);
    //! Окно закрыто
    void sl_closeMdiWindow(const zf::Uid& entity_uid);
    //! Окончание выполнения задачи окна
    void sl_progressEndMdiWindow();
    //! Менеджер обратных вызовов
    void sl_callbackManager(int key, const QVariant& data);

    //! Завершилась обработка операции
    void sl_operationProcessed(const zf::Operation& operation, const zf::Error& error);
    //! Изменился идентификатор окна
    void sl_entityChanged(const zf::Uid& old_entity, const zf::Uid& new_entity);

    //! Сменилась текущая рабочая зона
    void sl_currentWorkZoneChanged(int previous_id, int current_id);

private:
    void requestRemove(ViewPtr view);
    void processRemove();

    Error checkAccessRights(const EntityCode& entity_code, const AccessRights& rights) const;

    //! Найти представление
    ViewPtr* findView(const Uid& uid, View* view) const;

    //! Перед началом перерасположения окон
    void beforeRearange();
    //! После окончания перерасположения окон
    void afterRearange();

    //! Дополнительный метод для editNewWindow
    ModelPtr editNewWindowHelperModelGet(
        //! Идентификатор сущности
        const EntityCode& entity_code,
        //! Идентификатор БД
        const DatabaseID& database_id,
        //! Параметры открытия модального диалога
        const ModuleWindowOptions& options, Error& error);
    //! Дополнительный метод для editNewWindow
    ViewPtr editNewWindowHelperViewGet(const ModelPtr& model, int code, const ModuleWindowOptions& options, Error& error);
    //! Дополнительный метод для editNewWindow
    bool editNewWindowHelper(
        //! Идентификатор сущности
        const ViewPtr& view, const ModuleWindowOptions& options,
        //! Произвольные данные, передаваемые в метод View::onDataReady
        const QMap<QString, QVariant>& custom_data,
        //! Ошибка
        Error& error);

    //! Дополнительный метод для editWindow
    ModelPtr editWindowHelperModelGet(
        //! Идентификатор сущности
        const Uid& entity, const ModuleWindowOptions& options, Error& error);
    //! Дополнительный метод для editWindow
    ViewPtr editWindowHelperViewGet(const ModelPtr& model, int code, const ModuleWindowOptions& options, Error& error);
    //! Дополнительный метод для editWindow
    bool editWindowHelper(
        //! Идентификатор сущности
        const ViewPtr& view, const ModuleWindowOptions& options,
        //! Произвольные данные, передаваемые в метод View::onDataReady
        const QMap<QString, QVariant>& custom_data,
        //! Ошибка
        Error& error);

    //! Дополнительный метод для viewWindow
    ModelPtr viewWindowHelperModelGet(
        //! Идентификатор сущности
        const Uid& entity, const ModuleWindowOptions& options, Error& error);
    //! Дополнительный метод для viewWindow
    ViewPtr viewWindowHelperViewGet(const ModelPtr& model, int code, const ModuleWindowOptions& options, Error& error);
    //! Дополнительный метод для viewWindow
    bool viewWindowHelper(
        //! Идентификатор сущности
        const ViewPtr& view, const ModuleWindowOptions& options,
        //! Произвольные данные, передаваемые в метод View::onDataReady
        const QMap<QString, QVariant>& custom_data,
        //! Ошибка
        Error& error);

    //! Исправить потерю фокуса фоновым виджетом
    void fixWorkZoneViewFocus();

    MdiArea* _mdi_area = nullptr;
    //! Открытые окна
    QMultiMap<Uid, ViewPtr*> _windows;
    //! Запросы на удаление окон
    QMultiMap<Uid, ViewPtr*> _remove_requests;
    //! Представления, встроенные в рабочие зоны. Ключ - id рабочей зоны
    QMap<int, ViewPtr> _work_zone_views;
    //! Для решения проблемы совместного использования shared_ptr и Qt-шного удаления родителей
    //! При встраивании
    QMap<int, QPointer<View>> _work_zone_views_ptr;

    //! Потокобезопасное расширение возможностей QObject
    ObjectExtension* _object_extension = nullptr;

    //! Последнее место размещения окна
    static QPoint _last_place;
};
} // namespace zf

#endif // ZF_WINDOW_MANAGER_H
