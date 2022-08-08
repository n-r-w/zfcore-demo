#pragma once
#include "zf.h"

namespace zf
{
class ChildObjectsContainer;

//! Поиск объектов по имени
class ZCORESHARED_EXPORT ObjectsFinder : public QObject
{
    Q_OBJECT
public:
    ObjectsFinder(
        //! Имя для диагностики
        const QString& base_name = {}, const QObjectList& sources = {});

    //! Есть ли такой источник
    bool hasSource(QObject* source) const;
    //! Добавить источник
    void addSource(QObject* source);

    //! Список всех дочерних объектов для каждого источника
    QHash<QObject*, QObjectList> childObjects() const;
    //! Список всех дочерних виджетов для каждого источника
    QHash<QObject*, QWidgetList> childWidgets() const;

    /*! Найти на форме объект класса T по имени */
    template <class T>
    T* object(
        //! Путь к объекту. Последний элемент - имя самого объекта, далее его родитель и т.д.
        //! Родителей можно пропускать. Главное чтобы на форме не было двух объектов, подходящих по критерию поиска
        const QString& name) const
    {
        return object<T>(QStringList(name));
    }
    /*! Найти на форме объект класса T по пути(списку имен самого объекта и его родителей) к объекту */
    template <class T>
    T* object(
        //! Путь к объекту. Последний элемент - имя самого объекта, далее его родитель и т.д.
        //! Родителей можно пропускать. Главное чтобы на форме не было двух объектов, подходящих по критерию поиска
        const QStringList& path) const
    {
        QList<QObject*> objects = findObjects(path, T::staticMetaObject.className());
        Z_CHECK_X(!objects.isEmpty(),
                  QString("Object %1:%2:%3 not found").arg(_base_name).arg(T::staticMetaObject.className()).arg(path.join("\\")));
        Z_CHECK_X(objects.count() == 1, QString("Object %1:%2:%3 found more then once (%4)")
                                            .arg(_base_name)
                                            .arg(T::staticMetaObject.className())
                                            .arg(path.join("\\"))
                                            .arg(objects.count()));
        T* res = qobject_cast<T*>(objects.first());
        Z_CHECK_X(res != nullptr,
                  QString("Casting error %1:%2:%3").arg(_base_name).arg(T::staticMetaObject.className()).arg(path.join("\\")));

        return res;
    }

    /*! Существует ли на форме объект класса T по имени */
    template <class T> bool objectExists(const QString& name) const { return objectExists<T>(QStringList(name)); }
    /*! Существует ли на форме объект класса T по пути(списку имен самого объекта и его родителей) к объекту */
    template <class T> bool objectExists(const QStringList& path) const
    {
        return !findObjects(path, reinterpret_cast<T>(0)->staticMetaObject.className()).isEmpty();
    }

    //! Поиск элемента по имени
    QList<QObject*> findObjects(
        //! Путь к объекту. Последний элемент - имя самого объекта, далее его родитель и т.д.
        const QStringList& path, const char* class_name) const;
    // Проверка совпадения пути к объекту
    static bool compareObjectName(const QStringList& path, QObject* object, bool path_only);

    //! Найти все объекты, наследники class_name в source()
    void findObjectsByClass(QObjectList& list,
                            //! Имя класса
                            const QString& class_name,
                            //! Проверять ли source()
                            bool is_check_source = false,
                            //! Только у которых есть имена
                            bool has_names = true) const;

    //! Очистка кэша поиска объекта по имени
    void clearCache();

    //! Имя для диагностики
    void setBaseName(const QString& s);

signals:
    //! Смена имени
    void sg_objectNameChanged(QObject* object);
    //! Вызывается когда поменялось содержимое контейнера (удалился или добавился объект)
    void sg_contentChanged();
    //! Вызывается когда добавлен новый объект
    void sg_objectAdded(QObject* obj);
    //! Вызывается когда объект удален из контейнера (изъят из контроля или удален из памяти)
    //! ВАЖНО: при удалении объекта из памяти тут будет только указатель на QObject, т.к. деструкторы наследника уже сработают, поэтому нельзя делать qobject_cast
    void sg_objectRemoved(QObject* obj);

private slots:
    //! Смена имени
    void sl_objectNameChanged(QObject* object);
    //! Вызывается когда поменялось содержимое контейнера (удалился или добавился объект)
    void sl_contentChanged();

private:
    struct SourceInfo
    {
        SourceInfo(QObject* obj);
        ~SourceInfo();
        QPointer<QObject> object;
        ChildObjectsContainer* container = nullptr;
    };

    //! Поиск объекта по имени
    static void findObjectByName(const std::shared_ptr<SourceInfo>& source,
                                 //! Путь к объекту. Последний элемент - имя самого объекта, далее его родитель и т.д.
                                 const QStringList& path, const char* class_name, bool is_check_parent, QList<QObject*>& objects);

    QList<std::shared_ptr<SourceInfo>> _sources;
    //! Хэш для поиска элементов формы по имени
    mutable QHash<QString, QList<QPointer<QObject>>> _find_element_hash;
    //! Имя для диагностики
    QString _base_name;
};

} // namespace zf
