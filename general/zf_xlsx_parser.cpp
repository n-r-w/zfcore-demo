#include "zf_xlsx_parser.h"
#include "zf_core.h"
#include "zf_utils.h"
#include "zf_item_model.h"

#include <QFile>
#include <xlsxdocument.h>

namespace zf
{
Error XlsxParser::parse(const QString& file_name, const QMap<int, QString>& column_map, Options options, QList<int>& columns,
                        QList<QVariantList>& data)
{
    columns.clear();
    data.clear();

    if (!QFile::exists(file_name))
        return Error::fileNotFoundError(file_name);

    QFile f(file_name);
    if (!f.open(QFile::ReadOnly))
        return Error::fileIOError(file_name);

    Error error = parse(&f, column_map, options, columns, data);
    f.close();
    return error;
}

Error XlsxParser::parse(QIODevice* file_device, const QMap<int, QString>& column_map, Options options, QList<int>& columns,
                        QList<QVariantList>& data)
{
    using namespace QXlsx;

    Z_CHECK_NULL(file_device);
    columns.clear();
    data.clear();

    Document doc(file_device);

    if (!doc.isLoadPackage())
        return Error::badFileError(file_device);

    QMap<int, QString> column_map_prepared;
    for (auto it = column_map.constBegin(); it != column_map.constEnd(); ++it) {
        column_map_prepared[it.key()] = prepareString(it.value());
    }

    // ключ - фактическая колонка, значение - код колонки
    QMap<int, int> columns_found;

    CellRange range = doc.dimension();
    for (int col = 1; col <= range.lastColumn(); col++) {
        QString header = prepareString(doc.read(1, col).toString());
        if (header.isEmpty())
            continue;
        for (auto it = column_map_prepared.constBegin(); it != column_map_prepared.constEnd(); ++it) {
            if (header != it.value())
                continue;

            columns_found[col] = it.key();
        }
    }

    if (columns_found.isEmpty())
        return {};

    for (auto it = columns_found.constBegin(); it != columns_found.constEnd(); ++it) {
        columns << it.value();
    }

    // содержание
    for (int row = range.lastRow(); row >= 2; row--) {
        QVariantList row_values;
        bool has_data = false;
        for (auto it = columns_found.constBegin(); it != columns_found.constEnd(); ++it) {
            row_values << doc.read(row, it.key());
            if (!has_data && !row_values.last().isNull() && row_values.last().isValid())
                has_data = true;
        }

        // пустые строки пропускаем всегда в конце или в середине при условии skip_empty_rows
        if (!has_data && ((options & SkipEmptyRows) || data.isEmpty()))
            continue;

        data.prepend(row_values);
    }

    return {};
}

Error XlsxParser::parse(const QString& file_name, const QStringList& columns, ItemModel* item_model, const QVariantList& default_values,
                        Options options)
{
    if (!QFile::exists(file_name))
        return Error::fileNotFoundError(file_name);

    QFile f(file_name);
    if (!f.open(QFile::ReadOnly))
        return Error::fileIOError(file_name);

    Error error = parse(&f, columns, item_model, default_values, options);
    f.close();
    return error;
}

Error XlsxParser::parse(QIODevice* file_device, const QStringList& columns, ItemModel* item_model, const QVariantList& default_values,
                        Options options)
{
    Z_CHECK_NULL(item_model);
    Z_CHECK(default_values.isEmpty() || default_values.count() == columns.count());

    QMap<int, QString> column_map;
    for (int i = 0; i < columns.count(); i++) {
        column_map[i] = columns.at(i);
    }

    QList<int> columns_found;
    QList<QVariantList> data_found;

    Error error = parse(file_device, column_map, options, columns_found, data_found);
    if (error.isError()) {
        item_model->setRowCount(0);
        item_model->setColumnCount(0);
        return error;
    }
    Z_CHECK(columns_found.count() <= columns.count());

    item_model->setRowCount(0); // очищаем
    item_model->setColumnCount(columns_found.count());
    item_model->setRowCount(data_found.count());

    for (int row = 0; row < data_found.count(); row++) {
        const QVariantList& row_data = data_found.at(row);
        for (int col_index = 0; col_index < columns_found.count(); col_index++) {
            int col = columns_found.at(col_index);

            QVariant value = row_data.at(col_index);
            if (!default_values.isEmpty() && (value.isNull() || !value.isValid()))
                value = default_values.at(col);

            if (!value.isNull() && value.isValid())
                item_model->setData(row, col, value);
        }
    }

    return {};
}

QString XlsxParser::prepareString(const QString& value)
{
    // используем prepareRussianString для текста на любом языке, т.к. нам важно просто сравнить
    return Utils::removeAmbiguities(Utils::prepareRussianString(value.simplified().trimmed()), false).toUpper();
}

} // namespace zf
