#pragma once

#include "zf_widget_replacer.h"

namespace zf
{
class DataWidgets;

//! Реализация WidgetReplacer для DataStructure
class ZCORESHARED_EXPORT WidgetReplacerDataStructure : public WidgetReplacer
{    
    //! Свойства виджетов для копирования
    static const QHash<QString, QSet<QString>> _properties;

public:
    //! Профикс виджетов замены из DataStructure
    static const QString PREFIX_DATA_PROPERTY; // _p_
    //! Профикс виджета поиска сущности
    static const QString PREFIX_ENTITY_SEARCH_WIDGET; // _sw_

    WidgetReplacerDataStructure(
        //! Виджеты, созданные на основе структуры данных
        const DataWidgets* data_widgets,
        //! Виджет поиска сущности
        QWidget* entity_search_widget = nullptr);

    //! Задать виджет поиска сущности
    void setEntitySearchWidget(QWidget* entity_search_widget);

protected:
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

    //! Иницализация стиля
    void initStyle(
        //! Созданный виджет
        QWidget* widget,
        //! Свойство данных (если есть)
        DataProperty& data_property) override;

private:
    const DataWidgets* _data_widgets = nullptr;
    //! Виджет поиска сущности
    QWidget* _entity_search_widget = nullptr;
};

} // namespace zf
