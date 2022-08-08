#pragma once

#include "zf_data_widgets.h"

namespace zf
{
//! Связь между виджетами и свойствами для обработки ошибок
class HighlightMapping : public QObject
{
    Q_OBJECT
public:
    HighlightMapping(
        //! Виджеты
        DataWidgets* widgets, QObject* parent = nullptr);

    //! Виджеты
    DataWidgets* widgets() const;

    //! Виджет, который соответствует свойству
    QWidget* getHighlightWidget(const DataProperty& property) const;
    //! Свойство, которое соответствует виджету
    DataProperty getHighlightProperty(QWidget* w) const;

    //! Свойство, которое было замаплено в другое свойство через setHighlightPropertyMapping
    DataProperty getSourceHighlightProperty(const DataProperty& dest_property) const;
    //! Свойство, которое было замаплено в другое свойство через setHighlightPropertyMapping
    DataProperty getDestinationHighlightProperty(const DataProperty& source_property) const;

    //! Задать нестандартную связь между виджетами и свойствами для обработки ошибок
    //! Нужно в случае когда ошибке соответствует произвольный виджет на форме, а не тот, которы был сгенерирован по свойству
    void setHighlightWidgetMapping(const DataProperty& property, QWidget* w);
    void setHighlightWidgetMapping(const PropertyID& property_id, QWidget* w);

    //! Задать соответствие между свойством в котором находится ошибка и свойством на котором ее нужно показать
    void setHighlightPropertyMapping(const DataProperty& source_property, const DataProperty& dest_property);
    void setHighlightPropertyMapping(const PropertyID& source_property_id, const PropertyID& dest_property_id);

signals:
    void sg_mappingChanged(const zf::DataProperty& source_property);

private:
    //! Ленивая инициализация маппинга виджетов
    void initHighlightWidgetMappingLazy() const;

    //! Виджеты
    DataWidgets* _widgets;

    //! Нестандартная связь между виджетами и свойствами для обработки ошибок
    //! Нужно в случае когда ошибке соответствует произвольный виджет на форме, а не тот, которы был сгенерирован по свойству
    QMap<DataProperty, QPointer<QWidget>> _highlight_widget_mapping;
    QMap<DataProperty, DataProperty> _highlight_property_mapping;
    bool _highlight_widget_mapping_initialized = false;
};
} // namespace zf
