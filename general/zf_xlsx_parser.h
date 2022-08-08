#pragma once

#include "zf_data_structure.h"

namespace zf
{
class ItemModel;

/*! Парсинг файлов XLSX
 * Подразумевается что первая строка содержит текстовые метки с названием колонок */
class ZCORESHARED_EXPORT XlsxParser
{
public:
    enum Options
    {
        NoOptions = 0,
        //! Пропускать ли пустые строки
        SkipEmptyRows = 1,
    };

    //! Парсинг файла
    static Error parse(
        //! Имя файла
        const QString& file_name,
        //! Соответствие между кодами и текстовой меткой колонки
        const QMap<int, QString>& column_map,
        //! Параметры
        Options options,
        //! Результат. Коды найденных колонок
        QList<int>& columns,
        //! Результат. Данные: список строк, каждая строка - список данных для каждой колонки
        QList<QVariantList>& data);
    //! Парсинг из открытого QIODevice
    static Error parse(
        //! Открытый файл
        QIODevice* file_device,
        //! Соответствие между кодами и текстовой меткой колонки
        const QMap<int, QString>& column_map,
        //! Параметры
        Options options,
        //! Результат. Коды найденных колонок
        QList<int>& columns,
        //! Результат. Данные: список строк, каждая строка - список данных для каждой колонки
        QList<QVariantList>& data);

    //! Парсинг файла
    static Error parse(
        //! Имя файла
        const QString& file_name,
        //! Список текстовых меток колонок
        const QStringList& columns,
        //! Полученные данные
        ItemModel* item_model,
        //! Значения по умолчанию. Должно быть быть либо пустым, либо совпадать по количеству с columns
        const QVariantList& default_values = QVariantList(),
        //! Параметры
        Options options = Options::SkipEmptyRows);
    //! Парсинг из открытого QIODevice
    static Error parse(
        //! Открытый файл
        QIODevice* file_device,
        //! Список текстовых меток колонок
        const QStringList& columns,
        //! Полученные данные
        ItemModel* item_model,
        //! Значения по умолчанию. Должно быть быть либо пустым, либо совпадать по количеству с columns
        const QVariantList& default_values = QVariantList(),
        //! Параметры
        Options options = Options::SkipEmptyRows);

private:
    //! Устранить неоднозначности из строки
    static QString prepareString(const QString& value);
};

} // namespace zf
