#ifndef ZF_WIDGETREPLACER_H
#define ZF_WIDGETREPLACER_H

#include "zf_global.h"
#include <QSet>

namespace zf
{
class DataProperty;

//! Подмена виджетов на форме на основании их префикса
class ZCORESHARED_EXPORT WidgetReplacer
{
public:
    WidgetReplacer();
    virtual ~WidgetReplacer();

    //! Подмена виджетов на форме на основании их префикса
    //! Возвращает список замененных виджетов
    QList<QWidget*> replace(
        //! Родительский виджет, внутри которого будет идти поиск и замена
        QWidget* widget);

protected:
    //! Проанализировать виджет на предмет необходимости подмены. Если надо подменять, то
    //! поместить в tag особый признак подмены (например префикс) и вернуть истину
    virtual bool isNeedReplace(const QWidget* widget,
                               //! Особый признак подмены (например префикс)
                               QString& tag,
                               //! Имя виджета после замены
                               QString& new_name)
        = 0;

    //! Создать виджет на подмену. Если нет подмены, то nullptr
    virtual void createReplacement(
        //! Признак подмены (например префикс) в нижнем регистре
        const QString& tag,
        //! Название виджета с удаленным префиксом
        const QString& new_name,
        //! Виджет на замену
        const QWidget* widget,
        //! Скрыть оригинальный виджет и не заменять его
        bool& is_hide,
        //! Виджет - замена
        QWidget*& replacement,
        //! Контейнер виджета - замены. Может быть равен null
        QWidget*& replacement_container,
        //! Имена свойств виджета, которые надо копировать
        //! Ключ - имя класса, значение - набор имен свойст
        //! Если null, то копировать все
        QHash<QString, QSet<QString>>*& properties,
        //! Свойство данных (если есть)
        DataProperty& data_property)
        = 0;

    //! Иницализация стиля
    virtual void initStyle(
        //! Созданный виджет
        QWidget* widget,
        //! Свойство данных (если есть)
        DataProperty& data_property);

private:
    static void copyProperties(QWidget* from, QWidget* to,
                               //! Если null, то копировать все
                               const QHash<QString, QSet<QString>>* properties, const DataProperty& data_property);
};

} // namespace zf

#endif // ZF_WIDGETREPLACER_H
