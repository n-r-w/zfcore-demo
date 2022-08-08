#ifndef ZF_MAIN_WINDOW_H
#define ZF_MAIN_WINDOW_H

#include "zf_error.h"
#include "zf_progress_dialog.h"
#include "zf_relaxed_progress.h"
#include "zf_view.h"
#include "zf_all_windows_widget.h"
#include "zf_mdi_area.h"
#include "zf_work_zones.h"
#include "zf_i_work_zones.h"

#include <QMainWindow>

namespace Ui
{
class MainWindow;
}

class QWinTaskbarButton;

namespace zf
{
class WindowManager;


//! Главное окно
class ZCORESHARED_EXPORT MainWindow : public QMainWindow, public I_WorkZones, public I_ObjectExtension
{
    Q_OBJECT

public:
    MainWindow(const QString& app_name);
    ~MainWindow() override;

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
#ifdef Q_OS_WIN
    QWinTaskbarButton* taskbarButton() const;
#endif

    //! Добавить меню (вставляется в начало сервисного меню)
    void addMenu(QMenu* menu);
    //! Добавить экшены в меню (вставляется в начало сервисного меню)
    void addMenu(const QList<QAction*> actions);

public: // I_WorkZones
    //! Есть ли такая рабочая зона
    bool isWorkZoneExists(int id) const override;

    //! Рабочие зоны
    QList<WorkZone*> workZones() const override;
    //! Рабочая зона по id
    WorkZone* workZone(int id, bool halt_if_not_found = true) const override;
    //! Вставить рабочую зону
    WorkZone* insertWorkZone(
        //! Перед какой зоной вставить. Если -1, то в конец
        int before_id,
        //! Идентификатор
        int id,
        //! Константа перевода
        const QString& translation_id,
        //! Рисунок (для таббара)
        const QIcon& icon,
        //! Виджет
        QWidget* widget = nullptr,
        //! Тулбары
        const QList<QToolBar*>& toolbars = {}) override;
    //! Заменить виджет в рабочей зоне. Возвращает старый виджет
    QWidget* replaceWorkZoneWidget(int id, QWidget* w) override;
    //! Заменить тулбары в рабочей зоне. Возвращает старые тулбары
    QList<QToolBar*> replaceWorkZoneToolbars(int id, const QList<QToolBar*>& toolbars) override;

    //! Удалить рабочую зону
    void removeWorkZone(int id) override;

    //! Сделать рабочую зону (не)доступной. Возвращает текущую зону после выполнения операции (зона может поменяться если текущую сделали
    //! недоступной)
    WorkZone* setWorkZoneEnabled(int id, bool enabled) override;
    //! Доступна ли рабочая зона
    bool isWorkZoneEnabled(int id) const override;
    //! Доступна ли хоть одна рабочая зона
    bool hasWorkZoneEnabled() const override;

    //! Текущая рабочая зона
    WorkZone* currentWorkZone() const override;
    //! Текущая рабочая зона (id)
    int currentWorkZoneId() const override;
    //! Сменить текущую рабочую зону. Возвращает новую рабочую зону. Если зона заблокирована, то nullptr
    WorkZone* setCurrentWorkZone(int id) override;

protected:
    bool eventFilter(QObject* obj, QEvent* event) override;
    void showEvent(QShowEvent* event) override;
    void hideEvent(QHideEvent* event) override;
    void closeEvent(QCloseEvent* event) override;

signals:
    //! Окно скрыто
    void sg_hidden();

    //! Сигнал - добавилась рабочая зона
    void sg_workZoneInserted(int id) override;
    //! Сигнал - удалена рабочая зона
    void sg_workZoneRemoved(int id) override;
    //! Сигнал - сменилась текущая рабочая зона
    void sg_currentWorkZoneChanged(int previous_id, int current_id) override;
    //! Сигнал - изменилась доступность рабочей зоны
    void sg_workZoneEnabledChanged(int id, bool enabled) override;

private slots:
    //! Входящие сообщения
    void sl_message_dispatcher_inbound(const zf::Uid& sender, const zf::Message& message, zf::SubscribeHandle subscribe_handle);
    //! Менеджер обратных вызово
    void sl_callbackManager(int key, const QVariant& data);

    //! Новое окно
    void sl_openMdiWindow(zf::View* view);
    //! Окно будет закрыто
    void sl_closeMdiWindow(zf::View* view);

    //! Сменилась текущая закладка рабочей зоны
    void sl_work_zones_tab_changed(int index);

private:
    void configureStatusBar();
    void configureMainToolbar();
    void configureServiceToolbar();
    void configureModuleToolbar();
    void currentWorkZoneChangeHelper(int pos);
    void onWorkZonesChanged();

    Q_SLOT void updateWindowsActions();

    Ui::MainWindow* _ui;
    MdiArea* _mdi_area;        
    QLabel* _status_message = nullptr;
    //! Виджет со списком открытых окон
    AllWindowsWidget* _all_win_widget;

    //! Меню "прочее"
    QMenu* _service_menu = nullptr;
    //! Меню "окна"
    QMenu* _windows_menu = nullptr;

    // кнопка "свернуть/развернуть окна"
    QAction* _a_min_max_windows = nullptr;
    QAction* _a_close_all = nullptr;
    QAction* _a_tile = nullptr;
    QAction* _a_cascade = nullptr;
    QAction* _a_minimize = nullptr;
    QAction* _a_restore = nullptr;

    ProgressDialog* _progress_dialog = nullptr;
#ifdef Q_OS_WIN
    QWinTaskbarButton* _taskbarButton = nullptr;
#endif

    //! Таб рабочих зон
    QTabBar* _work_zones_tab;
    //! Рабочие зоны
    QMap<int, std::shared_ptr<WorkZone>> _work_zones;
    //! Рабочие зоны по порядку отображения
    QList<std::shared_ptr<WorkZone>> _work_zones_ordered;
    //! Текущая рабочая зона
    std::shared_ptr<WorkZone> _current_work_zone;
    //! Блокировка реакции на смену закладки рабочей зоны
    bool _work_zones_tab_changing = false;

    //! Потокобезопасное расширение возможностей QObject
    ObjectExtension* _object_extension = nullptr;
};
} // namespace zf

#endif // ZF_MAIN_WINDOW_H
