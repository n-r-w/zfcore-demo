#include "zf_utils.h"
#include "zf_core.h"
#include <QMimeDatabase>
#include <QMimeType>

namespace zf
{
QStringList Utils::fileExtensions(FileType type, bool halt_if_not_found)
{
    if (halt_if_not_found)
        Z_CHECK(type != FileType::Undefined);
    else if (type == FileType::Undefined)
        return {};

    switch (type) {
        case FileType::Docx:
            return {"docx"};
        case FileType::Doc:
            return {"doc"};
        case FileType::Xlsx:
            return {"xlsx"};
        case FileType::Xls:
            return {"xls"};
        case FileType::Html:
            return {"html", "htm"};
        case FileType::Plain:
            return {"txt"};
        case FileType::Xml:
            return {"xml"};
        case FileType::Pdf:
            return {"pdf"};
        case FileType::Json:
            return {"json"};
        case FileType::Zip:
            return {"zip"};
        case FileType::Rtf:
            return {"rtf"};
        case FileType::Png:
            return {"png"};
        case FileType::Jpg:
            return {"jpg", "jpeg", "jpe"};
        case FileType::Bmp:
            return {"bmp", "dib"};
        case FileType::Ico:
            return {"ico"};
        case FileType::Tif:
            return {"tif", "tiff"};
        case FileType::Gif:
            return {"gif"};
        case FileType::Svg:
            return {"svg"};
        default: {
            if (halt_if_not_found)
                Z_HALT_INT;
            return {};
        }
    }
}

QString Utils::fileExtension(FileType type, bool prepend_point)
{
    auto e = fileExtensions(type, false);
    return e.isEmpty() ? QString() : (prepend_point ? "." : "") + e.at(0);
}

FileType Utils::fileTypeFromExtension(const QString& ext)
{
    if (ext.isEmpty())
        return FileType::Undefined;

    QString fake_file_name = QStringLiteral("fake_file.") + ext;
    QMimeDatabase md;
    return fileTypeFromMime(md.mimeTypeForFile(fake_file_name, QMimeDatabase::MatchExtension).name());
}

FileType Utils::fileTypeFromMime(const QString& mime)
{
    Z_CHECK(!mime.simplified().isEmpty());

    static const QMap<QString, FileType> mime_map = {
        {"application/vnd.openxmlformats-officedocument.wordprocessingml.document", FileType::Docx},
        {"application/msword", FileType::Doc},
        {"application/vnd.openxmlformats-officedocument.spreadsheetml.sheet", FileType::Xlsx},
        {"application/vnd.ms-excel", FileType::Xls},
        {"application/xhtml+xml", FileType::Html},
        {"text/plain", FileType::Plain},
        {"application/xml", FileType::Xml},
        {"application/pdf", FileType::Pdf},
        {"application/json", FileType::Json},
        {"application/rtf", FileType::Rtf},
        {"image/png", FileType::Png},
        {"image/jpeg", FileType::Jpg},
        {"image/bmp", FileType::Bmp},
        {"image/vnd.microsoft.icon", FileType::Ico},
        {"image/tiff", FileType::Tif},
        {"image/gif", FileType::Gif},
        {"image/svg+xml", FileType::Svg},
    };

    return mime_map.value(mime.toLower(), FileType::Undefined);
}

FileType Utils::fileTypeFromBody(const QByteArray& data, FileType expected_type)
{
    QMimeDatabase md;

    // сначала определяем по содержимому
    QMimeType mime = md.mimeTypeForData(data);
    auto type = fileTypeFromMime(mime.name());

    if (type == FileType::Undefined && expected_type != FileType::Undefined) {
        // по содержимому не удалось - по расширению
        QString fake_file_name = QStringLiteral("fake_file.") + fileExtensions(expected_type).constFirst();
        type = fileTypeFromMime(md.mimeTypeForFile(fake_file_name).name());
    }

    return type;
}

FileType Utils::fileTypeFromFile(const QString& file_name, Error& error, FileType expected_type)
{
    if (!QFile::exists(file_name)) {
        error = Error::fileNotFoundError(file_name);
        return FileType::Undefined;
    }

    QFile file(file_name);
    if (!file.open(QFile::ReadOnly)) {
        error = Error::fileIOError(file_name);
        return FileType::Undefined;
    }

    error.clear();
    return fileTypeFromBody(file.readAll(), expected_type == FileType::Undefined
                                                    ? fileTypeFromExtension(QFileInfo(file_name).suffix())
                                                    : expected_type);
};

bool Utils::isImage(FileType type, bool supported_only)
{
    Q_UNUSED(supported_only)

    switch (type) {
        case FileType::Png:
        case FileType::Jpg:
        case FileType::Bmp:
        case FileType::Ico:
        case FileType::Tif:
        case FileType::Gif:
        case FileType::Svg:
            return true;
        default:
            return false;
    }
}

} // namespace zf
