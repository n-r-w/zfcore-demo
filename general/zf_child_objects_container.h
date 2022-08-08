#pragma once

#include "zf_defs.h"
#include "zf_basic_types.h"

namespace zf
{

/*! Cодержимое дочерних объектов для объекта
 * Поддерживает список всех дочерних объектов, чтобы не надо было их искать (это медленно) */
class ZCORESHARED_EXPORT ChildObjectsContainer : public QObject
{
    Q_OBJECT
public:
    ChildObjectsContainer(QObject* source,
                          //! Код группы, в рамках которой будет работать ChildObjectsContainer
                          int group_id,
                          //! Хранить до тех пор, пока дочерний объект не будет удален
                          //! Если истина, то объект не будет удален из списка, даже если сменил родителя
                          bool keep_until_destroyed, QObject* parent = nullptr);

    QObject* source() const;

    //! Все QObject
    QObjectList childObjects() const;
    //! Все виджеты
    QWidgetList childWidgets() const;

    //! Найти все объекты, наследники className в source(), которые имеют objectName
    void findObjectsByClass(QObjectList& list,
                            //! Имя класса
                            const QString& class_name,
                            //! Проверять ли source()
                            bool is_check_source = false,
                            //! Только у которых есть имена
                            bool has_names = true) const;

    bool eventFilter(QObject* watched, QEvent* event) override;

    //! Проверка, находится ли уже объект в контейнере
    static bool inContainer(QObject* obj);
    //! Пометить объект, как игнорируемый при отслеживании в контейнере
    static void markIgnored(QObject* obj);

signals:
    //! Сменилось имя объекта
    void sg_objectNameChanged(QObject* obj);
    //! Вызывается когда поменялось содержимое контейнера (удалился или добавился объект)
    void sg_contentChanged();
    //! Вызывается когда добавлен новый объект
    void sg_objectAdded(QObject* obj);
    //! Вызывается когда объект удален из контейнера (изъят из контроля или удален из памяти)
    //! ВАЖНО: при удалении объекта из памяти тут будет только указатель на QObject, т.к. деструкторы наследника уже сработают, поэтому нельзя делать qobject_cast
    void sg_objectRemoved(QObject* obj);

private slots:
    void sl_destroyed(QObject* obj);

private:
    void clearCache();
    void fillCache() const;
    bool prepareObject(QObject* object);

    static uint objectContainerGroup(QObject* object);
    static uint objectContainerID(QObject* object);
    static bool objectContainerIgnored(QObject* object);

    //! Имя свойства Qt в котором сохраняется код группы
    static const char* _group_id_property_name;
    //! Имя свойства Qt в котором сохраняется код владельца
    static const char* _id_property_name;
    //! Имя свойства Qt в котором сохраняется информация об игнорировании
    static const char* _ignore_property_name;

    //! Код группы
    uint _group_id;
    //! Уникальный код владельца
    Handle<uint> _code;

    QPointer<QObject> _source;
    bool _keep_until_destroyed;

    mutable QObjectList _objects_cache;
    mutable QWidgetList _widgets_cache;
};

} // namespace zf
