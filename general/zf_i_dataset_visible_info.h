#pragma once

#include <QObject>
#include <memory>
#include "zf.h"

class QAbstractItemModel;

namespace zf {
class Model;
class Error;
typedef std::shared_ptr<Model> ModelPtr;

//! Интерфейс для преобразования значений наборов данных
class I_DatasetVisibleInfo
{
public:
    //! Преобразовать отображаемое в ячейке значение
    virtual Error convertDatasetItemValue(const QModelIndex& index,
                                          //! Оригинальное значение
                                          const QVariant& original_value,
                                          //! Параметры
                                          const VisibleValueOptions& options,
                                          //! Значение после преобразования
                                          QString& value,
                                          //! Получение данных не возможно - требуется дождаться загрузки моделей
                                          QList<ModelPtr>& data_not_ready) const = 0;
    //! Информация о видимости строк набора данных
    virtual void getDatasetRowVisibleInfo(
        //! Оригинальный индекс
        const QModelIndex& index,
        //! Индекс прокси (тот что виден)
        QModelIndex& visible_index) const = 0;
    //! Информация о видимости колонок набора данных
    virtual void getDatasetColumnVisibleInfo(
        //! Источник данных
        const QAbstractItemModel* model,
        //! Колонка
        int column,
        //! Колонка сейчас видима
        bool& visible_now,
        //! Колонка может быть видима, даже если сейчас скрыта
        bool& can_be_visible,
        //! Порядковый номер с точки зрения отображения
        //! Если колонка не видна, то -1
        int& visible_pos) const = 0;
    //! Имя колонки набора данных
    virtual QString getDatasetColumnName(
        //! Источник данных
        const QAbstractItemModel* model,
        //! Колонка
        int column) const = 0;
};
}

QT_BEGIN_NAMESPACE
Q_DECLARE_INTERFACE(zf::I_DatasetVisibleInfo, "ru.ancor.metastaff.I_DatasetVisibleInfo/1.0")
QT_END_NAMESPACE
