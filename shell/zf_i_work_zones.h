#pragma once

#include "zf_global.h"

class QToolBar;

namespace zf
{
class WorkZone;

class I_WorkZones
{
public:
    //! Рабочие зоны
    virtual QList<WorkZone*> workZones() const = 0;

    //! Есть ли такая рабочая зона
    virtual bool isWorkZoneExists(int id) const = 0;

    //! Рабочая зона по id
    virtual WorkZone* workZone(int id, bool halt_if_not_found = true) const = 0;
    //! Вставить рабочую зону
    virtual WorkZone* insertWorkZone(
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
        const QList<QToolBar*>& toolbars = {})
        = 0;
    //! Заменить виджет в рабочей зоне. Возвращает старый виджет
    virtual QWidget* replaceWorkZoneWidget(int id, QWidget* w) = 0;
    //! Заменить тулбары в рабочей зоне. Возвращает старые тулбары
    virtual QList<QToolBar*> replaceWorkZoneToolbars(int id, const QList<QToolBar*>& toolbars) = 0;

    //! Удалить рабочую зону
    virtual void removeWorkZone(int id) = 0;

    //! Сделать рабочую зону (не)доступной. Возвращает текущую зону после выполнения операции (зона может поменяться если текущую сделали
    //! недоступной)
    virtual WorkZone* setWorkZoneEnabled(int id, bool enabled) = 0;
    //! Доступна ли рабочая зона
    virtual bool isWorkZoneEnabled(int id) const = 0;
    //! Доступна ли хоть одна рабочая зона
    virtual bool hasWorkZoneEnabled() const = 0;

    //! Текущая рабочая зона
    virtual WorkZone* currentWorkZone() const = 0;
    //! Текущая рабочая зона (id)
    virtual int currentWorkZoneId() const = 0;
    //! Сменить текущую рабочую зону. Возвращает новую рабочую зону. Если зона заблокирована, то nullptr
    virtual WorkZone* setCurrentWorkZone(int id) = 0;

    //! Сигнал - добавилась рабочая зона
    virtual void sg_workZoneInserted(int id) = 0;
    //! Сигнал - удалена рабочая зона
    virtual void sg_workZoneRemoved(int id) = 0;
    //! Сигнал - сменилась текущая рабочая зона
    virtual void sg_currentWorkZoneChanged(int previous_id, int current_id) = 0;
    //! Сигнал - изменилась доступность рабочей зоны
    virtual void sg_workZoneEnabledChanged(int id, bool enabled) = 0;
};

}; // namespace zf
