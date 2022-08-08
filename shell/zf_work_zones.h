#pragma once

#include "zf_global.h"
#include <QPointer>
#include <QIcon>

class QToolBar;

namespace zf
{
//! Рабочая зона
class ZCORESHARED_EXPORT WorkZone
{
public:
    //! Идентификатор
    int id() const;
    //! Название (для таббара)
    QString name() const;
    //! Виджет
    QWidget* widget() const;
    //! Тулбары
    QList<QToolBar*> toolbars() const;
    //! Рисунок (для таббара)
    QIcon icon() const;

private:
    WorkZone(int id, const QString& translation_id, QWidget* widget, const QList<QToolBar*>& toolbars, const QIcon& icon);

    //! Заменить виджет
    QWidget* replaceWidget(QWidget* widget);
    //! Заменить тулбары
    QList<QToolBar*> replaceToolbars(const QList<QToolBar*>& toolbars);

    int _id;
    QString _translation_id;
    QPointer<QWidget> _widget;
    QList<QPointer<QToolBar>> _toolbars;
    QIcon _icon;

    friend class MainWindow;
};

} // namespace zf
