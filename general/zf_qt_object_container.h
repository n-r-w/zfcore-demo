#ifndef ZF_QT_OBJECT_CONTAINER_H
#define ZF_QT_OBJECT_CONTAINER_H

#include "zf_global.h"
#include <QObject>
#include <QSet>
#include <QVariant>

namespace zf
{
/*! Контейнер для наследников QObject с автоматическим удалением объектов из контейнера при вызове их деструктора
 * Позволяет контролировать время жизни группы объектов */
class ZCORESHARED_EXPORT QtObjectContainer : public QObject
{
    Q_OBJECT
public:
    QtObjectContainer(QObject* parent = nullptr);

    //! Список объектов
    inline const QSet<QObject*>& objects() const { return _objects; }
    //! Содержит ли информацию о таком объекте
    bool contains(const QObject* o) const { return _data.contains(o); }
    //! Данные для объекта. HALT если такого объекта нет
    QVariant data(const QObject* o) const;

    //! Добавить объект. Возвращает true если раньше такого не было
    bool add(QObject* o,
            //! Произвольные данные
            const QVariant& data);
    //! Очистить данные по объекту и отключиться от его контроля
    //! Возвращает true если такой был
    bool clearData(QObject* o);

private slots:
    void sl_destroyed(QObject* o);

private:
    QSet<QObject*> _objects;
    QHash<const QObject*, QVariant> _data;
};
} // namespace zf
#endif // ZF_QT_OBJECT_CONTAINER_H
