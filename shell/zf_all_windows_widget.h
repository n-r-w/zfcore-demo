#pragma once

#include <QWidget>
#include <QMdiSubWindow>
#include <QModelIndexList>

namespace zf
{
class ModuleWindow;
class View;
class MdiArea;
class TreeView;
class ItemModel;

class AllWindowsWidget : public QWidget
{
    Q_OBJECT

public:
    explicit AllWindowsWidget(MdiArea* mdi_area);
    ~AllWindowsWidget();

private slots:
    //! Новое окно
    void sl_openMdiWindow(zf::View* view);
    //! Окно закрыто
    void sl_closeMdiWindow(zf::View* view);
    //! Изменилось имя
    void sl_entityNameChanged();

    //! Активировано окно
    void sl_subWindowActivated(QMdiSubWindow* w);
    //! Смена текущей строки
    void sl_currentRowChanged(const QModelIndex& current, const QModelIndex& previous);
    //! Нажатие мыши на строку
    void sl_clicked(const QModelIndex& index);

    //! Сигнал - добавилась рабочая зона
    void sl_workZoneInserted(int id);
    //! Сигнал - удалена рабочая зона
    void sl_workZoneRemoved(int id);
    //! Сигнал - сменилась текущая рабочая зона
    void sl_currentWorkZoneChanged(int previous_id, int current_id);
    //! Сигнал - изменилась доступность рабочей зоны
    void sl_workZoneEnabledChanged(int id, bool enabled);

    //! Окно добавлено в рабочую зону
    void sl_windowAddedToWorkZone(int work_zone_id);
    //! Окно удалено из рабочей зоны
    void sl_windowRemovedFromWorkZone(int work_zone_id);

private:        
    //! Загрузить начальную информацию
    void initWindows();
    //! Окно из индекса
    static ModuleWindow* windowFromIndex(const QModelIndex& index);
    //! Смена текущего индекса
    void onCurrentChanged(const QModelIndex& current);

    //! Подключиться к сигналам окна
    void connectToWindow(ModuleWindow* w);
    //! Поиск строки для окна
    QModelIndex findWindowIndex(View* view, int work_zone_id) const;
    //! Поиск строк для окна во всех зонах
    QModelIndexList findWindowIndexesAllZones(View* view) const;
    //! Поиск строки для рабочей зоны
    QModelIndex findWorkZoneIndex(int id) const;
    //! Поиск всех строк, связанных с рабочей зоной (строка самой зоны и окна)
    QModelIndexList findAllWorkZoneIndexes(int id) const;

    //! Добавить строку с окном
    void addWindowRow(ModuleWindow* window, const QModelIndex& zone_index);

    //! На каком расстоянии от бортика располагать
    int borderSize() const;

    ItemModel* _windows_model = nullptr;
    MdiArea* _mdi_area = nullptr;
    TreeView* _view = nullptr;
    int _block = 0;

    friend class _WindowsListView;
};

} // namespace zf
