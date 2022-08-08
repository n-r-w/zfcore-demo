#include "zf_utils.h"
#include <QTextCodec>
#include "zf_core_consts.h"
#include "zf_core.h"
#include "zf_translation.h"

#include <xlsxdocument.h>
#include <xlsxworksheet.h>
#include <xlsxcell.h>

namespace zf
{
Error Utils::itemModelToCSV(const QAbstractItemModel* model, const QString& file_name, const I_DatasetVisibleInfo* visible_info,
                            const QMap<int, QString>& header_map, bool original_uid, int timeout_ms)
{
    Z_CHECK_NULL(model);
    Z_CHECK(!file_name.isEmpty());

    QFile f(file_name);
    if (!f.open(QFile::WriteOnly | QFile::Truncate))
        return Error::fileIOError(f);

    return itemModelToCSV(model, &f, visible_info, header_map, original_uid, timeout_ms);
}

Error Utils::itemModelToCSV(const QAbstractItemModel* model, QIODevice* device, const I_DatasetVisibleInfo* visible_info,
                            const QMap<int, QString>& header_map, bool original_uid, int timeout_ms)
{
    Z_CHECK_NULL(model);
    Z_CHECK_NULL(device);
    Z_CHECK(device->isOpen() && device->isWritable());
    Z_CHECK(isMainThread());

    QTextCodec* codec = QTextCodec::codecForName(Consts::FILE_ENCODING.toLatin1());

    // Заголовок
    QMap<int, QString> hMap;
    if (header_map.isEmpty()) {
        for (int col = 0; col < model->columnCount(); col++) {
            hMap[col] = QString::number(col + 1);
        }
    } else
        hMap = header_map;

    bool started = false;
    // Заголовок
    for (int col = 0; col < model->columnCount(); col++) {
        if (!hMap.contains(col))
            continue;

        if (started)
            device->write(codec->fromUnicode(QString(Consts::CSV_SEPARATOR)));

        QString val = hMap.value(col).simplified();
        // CSV заключает в ковычки каждую строку данных
        // Если в ней есть ковычки, они заменяются на двойные
        val = putInQuotes(val, '"');

        device->write(codec->fromUnicode(val));
        started = true;
    }
    device->write(codec->fromUnicode(QString('\n')));

    // Строки
    itemModelToCSV_helper(model, device, hMap, codec, visible_info, QModelIndex(), timeout_ms, original_uid);

    return Error();
}

Error Utils::itemModelFromCSV(FlatItemModel* model, const QString& file_name, const QMap<QString, int>& columns_mapping,
                              const QString& codec)
{
    Z_CHECK_NULL(model);
    Z_CHECK(!file_name.isEmpty());

    QFile f(file_name);
    if (!f.open(QFile::ReadOnly))
        return Error::fileIOError(f);

    return itemModelFromCSV(model, &f, columns_mapping, codec);
}

void Utils::itemModelToCSV_helper(const QAbstractItemModel* model, QIODevice* device, const QMap<int, QString>& hMap, QTextCodec* codec,
                                  const I_DatasetVisibleInfo* visible_info, const QModelIndex& parent, bool original_uid, int timeout_ms)
{
    bool started = false;
    for (int row = 0; row < model->rowCount(parent); row++) {
        started = false;
        for (int col = 0; col < model->columnCount(); col++) {
            if (!hMap.contains(col))
                continue;

            if (started)
                device->write(codec->fromUnicode(QString(Consts::CSV_SEPARATOR)));

            QModelIndex index = model->index(row, col, parent);
            QVariant data;
            if (index.data(Qt::DisplayRole).isValid()) {
                data = index.data(Qt::DisplayRole);

                // Вдруг это идентификатор?
                Uid uid = Uid::deserialize(data);
                Error err;
                if (uid.isValid()) {
                    if (original_uid)
                        data = uid.serializeToString();
                    else {
                        data = Core::getEntityNameSync(uid, err, PropertyID(), Core::defaultLanguage(), timeout_ms); // Ошибку игнорируем
                        if (err.isError() || data.toString().isEmpty())
                            data = uid.serializeToString();
                    }
                }

            } else if (index.data(Qt::CheckStateRole).isValid()) {
                if (index.data(Qt::CheckStateRole).toInt() == Qt::Checked)
                    data = ZF_TR(ZFT_YES).toLower();
                else if (index.data(Qt::CheckStateRole).toInt() == Qt::PartiallyChecked)
                    data = ZF_TR(ZFT_PARTIAL).toLower();
                else
                    data.clear();
            }

            QString val = Utils::variantToString(data).simplified();

            // CSV заключает в ковычки каждую строку данных
            // Если в ней есть ковычки, они заменяются на двойные
            putInQuotes(val, '"');

            device->write(codec->fromUnicode(val));
            started = true;
        }
        device->write(codec->fromUnicode(QString('\n')));

        itemModelToCSV_helper(model, device, hMap, codec, visible_info, model->index(row, 0, parent), timeout_ms, original_uid);
    }
}

Error Utils::itemModelFromCSV(FlatItemModel* model, QIODevice* device, const QMap<QString, int>& columns_mapping, const QString& codec)
{
    Z_CHECK_NULL(model);
    Z_CHECK_NULL(device);
    Z_CHECK(model->columnCount() > 0);
    Z_CHECK(!columns_mapping.isEmpty());
    Z_CHECK(device->isOpen() && device->isReadable());

    int model_column_count = 0;
    QMap<QString, int> columns_mapping_prepared;
    for (auto i = columns_mapping.begin(); i != columns_mapping.end(); ++i) {
        columns_mapping_prepared[i.key().toUpper()] = i.value();
        model_column_count = qMax(model_column_count, i.value() + 1);
    }

    bool header_processed = false;
    int file_row = 0;
    int file_column_count = 0;
    QMap<int, int> file_column_mapping;

    model->setRowCount(0);
    model->setColumnCount(model_column_count);

    QTextStream in(device);
    in.setCodec(QTextCodec::codecForName(codec.toLatin1()));

    while (!in.atEnd()) {
        file_row++;
        QString line = in.readLine().trimmed();

        if (line.isEmpty())
            continue;

        QStringList data = extractFromDelimeted(line, '"', Consts::CSV_SEPARATOR, false);

        if (!header_processed) {
            // Первая строка - заголовок
            header_processed = true;
            file_column_count = data.count();
            if (file_column_count == 0)
                return {};

            for (int col = 0; col < file_column_count; col++) {
                int model_col = columns_mapping_prepared.value(data.at(col).toUpper(), -1);
                if (model_col < 0)
                    continue;

                file_column_mapping[col] = model_col;
            }

            continue;
        }

        for (auto i = file_column_mapping.begin(); i != file_column_mapping.end(); ++i) {
            QString cell_text = data.at(i.key()).trimmed();
            QVariant v = variantFromString(cell_text);
            if (v.type() == QVariant::String) {
                // CSV заключает в ковычки каждую строку данных
                // Если в ней есть ковычки, они заменяются на двойные
                v = cell_text.replace("\"\"", "\"").simplified();
            }

            model->setData(model->rowCount() - 1, i.value(), v);
        }

        model->appendRow();
    }

    return Error();
}

Error Utils::itemModelToExcel(const QAbstractItemModel* model, const QString& file_name, const I_DatasetVisibleInfo* visible_info,
                              const QMap<int, QString>& header_map, bool original_uid, int timeout_ms)
{
    Z_CHECK_NULL(model);
    Z_CHECK(!file_name.isEmpty());

    QFile f(file_name);
    if (!f.open(QFile::WriteOnly | QFile::Truncate))
        return Error::fileIOError(f);

    auto error = itemModelToExcel(model, &f, visible_info, header_map, original_uid, timeout_ms);
    f.close();
    return error;
}

Error Utils::itemModelToExcel(const QAbstractItemModel* model, QIODevice* device, const I_DatasetVisibleInfo* visible_info,
                              const QMap<int, QString>& header_map, bool original_uid, int timeout_ms)
{
    Z_CHECK_NULL(model);
    Z_CHECK_NULL(device);
    Z_CHECK(device->isOpen() && device->isWritable());
    Z_CHECK(isMainThread());

    QXlsx::Document document;

    // Заголовок
    QMap<int, QPair<QString, int>> hMap;
    if (header_map.isEmpty() && visible_info == nullptr) {
        for (int col = 0; col < model->columnCount(); col++) {
            hMap[col] = {QString::number(col + 1), col};
        }

    } else {
        if (visible_info != nullptr) {
            for (int col = 0; col < model->columnCount(); col++) {
                if (header_map.contains(col)) {
                    hMap[col] = {header_map.value(col), col};
                    continue;
                }

                bool visible_now;
                bool can_be_visible;
                int visible_pos;
                visible_info->getDatasetColumnVisibleInfo(model, col, visible_now, can_be_visible, visible_pos);
                if (!visible_now)
                    continue;

                hMap[col] = {visible_info->getDatasetColumnName(model, col), visible_pos};
            }

        } else {
            for (int col = 0; col < model->columnCount(); col++) {
                if (header_map.contains(col))
                    hMap[col] = {header_map.value(col), col};
                else
                    hMap[col] = {QString::number(col + 1), col};
            }
        }
    }

    // Заголовок
    for (int col = 0; col < model->columnCount(); col++) {
        if (!hMap.contains(col))
            continue;

        auto info = hMap.constFind(col);
        QXlsx::Format format;
        format.setNumberFormatIndex(xlsxFormat(QVariant::String));
        format.setBorderStyle(QXlsx::Format::BorderStyle::BorderThin);
        format.setBorderColor(Qt::black);
        format.setPatternBackgroundColor(QColor(207, 231, 255));
        document.currentWorksheet()->write(1, info.value().second + 1, info.value().first, format);
    }

    // Строки
    int export_row = 1;
    itemModelToExcel_helper(model, document.currentWorksheet(), visible_info, export_row, hMap, QModelIndex(), timeout_ms, original_uid);

    if (!document.saveAs(device))
        return Error::fileIOError(device);

    return Error();
}

int Utils::xlsxFormat(QVariant::Type type)
{
    static const QMap<QVariant::Type, int> XLSX_FORMAT_VARIANT = {
        {QVariant::Type::Invalid, 0},   {QVariant::Type::Bool, 1},      {QVariant::Type::Int, 1},    {QVariant::Type::UInt, 1},
        {QVariant::Type::LongLong, 1},  {QVariant::Type::ULongLong, 1}, {QVariant::Type::Double, 2}, {QVariant::Type::Char, 49},
        {QVariant::Type::String, 49},   {QVariant::Type::Date, 14},     {QVariant::Type::Time, 20},  {QVariant::Type::DateTime, 22},
        {QVariant::Type::UserType, 49},
    };

    return XLSX_FORMAT_VARIANT.value(type, 0);
}

int Utils::xlsxFormat(DataType type)
{
    return xlsxFormat(dataTypeToVariant(type));
}

void Utils::itemModelToExcel_helper(const QAbstractItemModel* model, QXlsx::Worksheet* workbook, const I_DatasetVisibleInfo* visible_info,
                                    int& export_row, const QMap<int, QPair<QString, int>>& hMap, const QModelIndex& parent,
                                    bool original_uid, int timeout_ms)
{
    for (int row = 0; row < model->rowCount(parent); row++) {
        processEvents();
        for (int col = 0; col < model->columnCount(); col++) {
            auto info = hMap.constFind(col);
            if (info == hMap.constEnd())
                continue;

            QVariant data;
            QModelIndex index = model->index(row, col, parent);
            if (index.data(Qt::DisplayRole).isValid()) {
                data = index.data(Qt::DisplayRole);

                // Вдруг это идентификатор?
                Uid uid = Uid::deserialize(data);
                Error err;
                if (uid.isValid()) {
                    if (original_uid)
                        data = uid.serializeToString();
                    else {
                        data = Core::getEntityNameSync(uid, err, PropertyID(), Core::defaultLanguage(), timeout_ms); // Ошибку игнорируем
                        if (err.isError() || data.toString().isEmpty())
                            data = uid.serializeToString();
                    }
                }

            } else if (index.data(Qt::CheckStateRole).isValid()) {
                if (index.data(Qt::CheckStateRole).toInt() == Qt::Checked)
                    data = ZF_TR(ZFT_YES).toLower();
                else if (index.data(Qt::CheckStateRole).toInt() == Qt::PartiallyChecked)
                    data = ZF_TR(ZFT_PARTIAL).toLower();
                else
                    data.clear();
            }

            if (visible_info != nullptr) {
                QString converted;

                QList<ModelPtr> data_not_ready;
                auto error = visible_info->convertDatasetItemValue(index, data, VisibleValueOption::Export, converted, data_not_ready);

                Core::waitForLoadModel(data_not_ready); // ошибки не проверяем
                // попытка №2. если не удалось, то и хрен с ним
                data_not_ready.clear();
                error = visible_info->convertDatasetItemValue(index, data, VisibleValueOption::Export, converted, data_not_ready);

                if (error.isOk() && data_not_ready.isEmpty()) {
                    // была конвертация в отображаемую строку. надо попытаться определить тип данных
                    data = Utils::variantFromString(converted, LocaleType::UserInterface);
                    if (!data.isValid() && !converted.isEmpty())
                        data = converted;
                }
            }

            QXlsx::Format format;
            format.setNumberFormatIndex(xlsxFormat(data.type()));
            workbook->write(export_row + 1, info.value().second + 1, data, format);
        }

        export_row++;
        itemModelToExcel_helper(model, workbook, visible_info, export_row, hMap, model->index(row, 0, parent), timeout_ms, original_uid);
    }
}

void Utils::itemModelToStream(QDataStream& s, const FlatItemModel* model)
{
    Z_CHECK_NULL(model);
    Z_CHECK(s.version() == Consts::DATASTREAM_VERSION);

    s << model->columnCount();
    itemModelToStreamHelper(s, model, model->columnCount(), QModelIndex());
}

Error Utils::itemModelFromStream(QDataStream& s, FlatItemModel* model)
{
    Z_CHECK_NULL(model);
    Z_CHECK(s.version() == Consts::DATASTREAM_VERSION);

    int column_count;
    s >> column_count;
    if (s.status() != QDataStream::Ok || column_count < 0)
        return Error::corruptedDataError("itemModelFromStream corrupted data");

    model->setRowCount(0);
    model->setColumnCount(column_count);
    if (!itemModelFromStreamHelper(s, model, column_count, QModelIndex()))
        return Error::corruptedDataError("itemModelFromStream corrupted data");

    return Error();
}

bool Utils::itemModelToStreamHelper(QDataStream& s, const FlatItemModel* model, int column_count, const QModelIndex& parent)
{
    int row_count = model->rowCount(parent);
    s << row_count;

    for (int row = 0; row < row_count; row++) {
        for (int col = 0; col < column_count; col++) {
            s << model->dataHelperLanguageRoleMap(model->index(row, col, parent));
            if (s.status() != QDataStream::Ok)
                return false;
        }

        if (!itemModelToStreamHelper(s, model, column_count, model->index(row, 0, parent)))
            return false;
    }

    return true;
}

bool Utils::itemModelFromStreamHelper(QDataStream& s, FlatItemModel* model, int column_count, const QModelIndex& parent)
{
    int row_count;
    s >> row_count;
    if (s.status() != QDataStream::Ok || row_count < 0)
        return false;

    model->setRowCount(row_count, parent);

    for (int row = 0; row < row_count; row++) {
        for (int col = 0; col < column_count; col++) {
            QMap<int, LanguageMap> data;
            s >> data;
            if (s.status() != QDataStream::Ok)
                return false;
            if (!model->setDataHelperLanguageRoleMap(model->index(row, col, parent), data))
                return false;
        }
        if (!itemModelFromStreamHelper(s, model, column_count, model->index(row, 0, parent)))
            return false;
    }

    return true;
}
} // namespace zf
