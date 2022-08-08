#pragma once

#include "zf_widget_replacer.h"

namespace zf
{
//! Реализация WidgetReplacer для ручной замены произвольных виджетов
class ZCORESHARED_EXPORT WidgetReplacerManual : public WidgetReplacer
{
public:
    WidgetReplacerManual(QWidget* source, QWidget* target,
                         //! Какое имя надо присвоить target
                         const QString& new_name,
                         //! Копировать ли свойства
                         bool copy_properties);

    //! Замена виджета на новый
    static void replaceWidget(QWidget* source, QWidget* target,
                              //! Какое имя надо присвоить target
                              const QString& new_name,
                              //! Копировать ли свойства
                              bool copy_properties);

protected:
    using WidgetReplacer::replace;

    //! Проанализировать виджет на предмет необходимости подмены. Если надо подменять, то
    //! поместить в tag особый признак подмены (например префикс) и вернуть истину
    bool isNeedReplace(const QWidget* widget,
                       //! Особый признак подмены (например префикс)
                       QString& tag,
                       //! Имя виджета после замены
                       QString& new_name) override;

    //! Создать виджет на подмену. Если нет подмены, то nullptr
    void createReplacement(
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
        QHash<QString, QSet<QString>>*& properties,
        //! Свойство данных (если есть)
        DataProperty& data_property) override;

private:
    QWidget* _source;
    QWidget* _target;
    QString _new_name;
    bool _copy_properties;
};

} // namespace zf
