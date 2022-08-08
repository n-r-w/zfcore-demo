#include "zf_utils.h"

#include <QAbstractItemModel>
#include <QApplication>
#include <QDynamicPropertyChangeEvent>
#include <QCryptographicHash>
#include <QDate>
#include <QFormLayout>
#include <QIcon>
#include <QLayout>
#include <QMainWindow>
#include <QScrollArea>
#include <QStyleFactory>
#include <QSplitter>
#include <QStylePainter>
#include <QTextCodec>
#include <QToolBox>
#include <QUuid>
#include <QtConcurrent>
#include <QAbstractSocket>
#include <QUiLoader>
#include <QChart>
#include <QGraphicsScene>
#include <QPdfWriter>
#include <QSvgGenerator>
#include <QMovie>
#include <QDir>
#include <QEventLoop>
#include <QSet>
#include <QtMath>
#include <QCryptographicHash>
#include <QLocale>
#include <QMutex>
#include <QSplitter>
#include <QTabWidget>
#include <QApplication>
#include <QElapsedTimer>
#include <QMdiSubWindow>
#include <QProxyStyle>
#include <QGridLayout>
#include <QStackedWidget>
#include <QLayoutItem>
#include <QDesktopServices>
#include <QPdfDocument>
#include <QPdfDocumentRenderOptions>

#if QT_VERSION < QT_VERSION_CHECK(5, 10, 0)
#include <QDesktopWidget>
#else
#include <QScreen>
#endif

#include "zf_callback.h"
#include "zf_core.h"
#include "zf_exception.h"
#include "zf_framework.h"
#include "zf_header_config_dialog.h"
#include "zf_header_view.h"
#include "zf_html_tools.h"
#include "zf_itemview_header_item.h"
#include "zf_progress_object.h"
#include "zf_translation.h"
#include "zf_utils.h"
#include "zf_core_messages.h"
#include "zf_dbg_break.h"
#include "zf_picture_selector.h"
#include "zf_conditions_dialog.h"
#include "zf_console_utils.h"
#include "zf_numeric.h"
#include <texthyphenationformatter.h>
#include "private/zf_item_view_p.h"
#include "zf_image_list.h"
#include "zf_request_selector.h"
#include "zf_shared_ptr_deleter.h"
#include "zf_cumulative_error.h"
#include "zf_graphics_effects.h"
#include "zf_highlight_mapping.h"
#include "zf_colors_def.h"
#include "zf_data_container.h"

#include <cstdlib>

#ifdef Q_OS_LINUX
#include <unistd.h>
#include <sys/types.h>
#endif

#ifdef Q_OS_WIN
#include <Windows.h>
// сохранять порядок
#include <Psapi.h>
#include <QWinTaskbarProgress>
#include "mso/MSOControl.h"

#ifdef _MSC_VER
#define PATH_MAX MAX_PATH
#endif

#ifdef __MINGW32__
#include <limits>
#endif

#else
#include <linux/limits.h>
#include <unistd.h>
#endif

namespace zf
{
Error Utils::convertToPdf(const QString& source_file_name, const QString& dest_file_name, ConvertToPdfType type)
{
    Error error;

#ifdef Q_OS_LINUX
    Q_UNUSED(type)

    // под линукс конвертируем с помощью libreofice

    QString temp_dir = generateTempDirName();
    error = Utils::makeDir(temp_dir);
    if (error.isError())
        return error;

    QString temp_file_name = temp_dir + "/" + QFileInfo(source_file_name).completeBaseName() + ".pdf";

    QProcess converter;
    converter.setWorkingDirectory(temp_dir);

    converter.start("soffice", {"--headless", "--convert-to", "pdf", source_file_name});
    bool is_error = false;
    if (converter.waitForStarted(3000)) {
        converter.waitForFinished();
        if (converter.exitCode() == 0) {
            error = copyFile(temp_file_name, dest_file_name, true, true);
        } else {
            is_error = true;
        }

    } else {
        is_error = true;
    }

    if (is_error) {
        switch (converter.error()) {
            case QProcess::FailedToStart:
                error = Error::fileNotFoundError("soffice");
                break;
            case QProcess::Timedout:
                error = Error::timeoutError();
                break;
            case QProcess::ReadError:
            case QProcess::WriteError:
            case QProcess::UnknownError: {
                QString error_text = converter.readAllStandardError();
                if (error_text.isEmpty())
                    error_text = converter.readAllStandardOutput();
                if (error_text.isEmpty())
                    error_text = "undefined error";
                error = Error(error_text);
                break;
            }
            default:
                Z_HALT_INT;
                break;
        }
    }
    removeFile(temp_file_name);
    removeDir(temp_dir);

#else
    error = Utils::makeDir(QFileInfo(dest_file_name).absolutePath());
    if (error.isError())
        return error;

    bool res = false;
    if (type == ConvertToPdfType::MSWord)
        res = MSOControl::saveWordAsPdf(source_file_name, dest_file_name);
    else
        res = MSOControl::saveExcelAsPdf(source_file_name, dest_file_name);

    if (!res)
        error = Error::fileIOError(source_file_name);
#endif

    return error;
}

Error Utils::wordToPdf(const QString& source_file_name, const QString& dest_file_name)
{
    return convertToPdf(source_file_name, dest_file_name, ConvertToPdfType::MSWord);
}

Error Utils::excelToPdf(const QString& source_file_name, const QString& dest_file_name)
{
    return convertToPdf(source_file_name, dest_file_name, ConvertToPdfType::MSExcel);
}

QString Utils::getOpenFileName(const QString& caption, const QString& default_name, const QString& filter)
{
    QString f_name;
    if (default_name.isEmpty())
        f_name = Core::lastUsedDirectory();
    else if (default_name.contains(QDir::separator()))
        f_name = default_name;
    else
        f_name = Core::lastUsedDirectory() + "/" + default_name;

    QString res = QFileDialog::getOpenFileName(getTopWindow(), caption.isEmpty() ? QApplication::applicationName() : caption, f_name,
                                               filter, nullptr,
#ifdef Q_OS_LINUX
                                               QFileDialog::Options() //& (~QFileDialog::DontUseNativeDialog)
#else
                                               QFileDialog::Options()
#endif
    );

    if (!res.isEmpty())
        Core::updateLastUsedDirectory(res);

    return res;
}

QStringList Utils::getOpenFileNames(const QString& caption, const QString& default_name, const QString& filter)
{
    QString f_name;
    if (default_name.isEmpty())
        f_name = Core::lastUsedDirectory();
    else if (default_name.contains(QDir::separator()))
        f_name = default_name;
    else
        f_name = Core::lastUsedDirectory() + "/" + default_name;

    QStringList res = QFileDialog::getOpenFileNames(getTopWindow(), caption.isEmpty() ? QApplication::applicationName() : caption, f_name,
                                                    filter, nullptr,
#ifdef Q_OS_LINUX
                                                    QFileDialog::Options() //& (~QFileDialog::DontUseNativeDialog)
#else
                                                    QFileDialog::Options()
#endif
    );

    if (!res.isEmpty())
        Core::updateLastUsedDirectory(res.at(0));

    return res;
}

//! Убрать лишние пробелы из имени файла
static QString fixFileName(const QString& f_name,
                           //! Исправить на указанное расширение файла
                           const QString& f_ext = {})
{
    QFileInfo fi(f_name);
    QString path = fi.absolutePath();
    QString name = fi.completeBaseName().trimmed();
    QString ext = fi.suffix().trimmed();
    if (!f_ext.isEmpty() && !comp(ext, f_ext))
        ext = f_ext;

    return path + "/" + name + (!ext.isEmpty() ? QString(".") + ext : QString());
}

//! Получить список расширений из строки фильтра диалога
static QStringList extractExtentions(const QString& filters)
{
    QStringList res;
    QRegularExpression r(R"(\*\.[\w\s]+)");
    QRegularExpressionMatch match = r.match(filters);
    return match.capturedTexts();
}

QString Utils::getSaveFileName(const QString& caption, const QString& default_name, const QString& filter)
{
    QString f_name;
    if (default_name.isEmpty())
        f_name = Core::lastUsedDirectory();
    else if (default_name.contains(QDir::separator()))
        f_name = default_name;
    else
        f_name = Core::lastUsedDirectory() + "/" + default_name;

    QString res = QFileDialog::getSaveFileName(getTopWindow(), caption.isEmpty() ? QApplication::applicationName() : caption, f_name,
                                               filter, nullptr,
#ifdef Q_OS_LINUX
                                               QFileDialog::Options() //& (~QFileDialog::DontUseNativeDialog)
#else
                                               QFileDialog::Options()
#endif
    );

    if (!res.isEmpty()) {
        Core::updateLastUsedDirectory(res);

        if (!default_name.isEmpty() && !default_name.contains(QDir::separator())) {
            // берем требуемое расширение из имени оригинального файла
            QFileInfo fi_orig(default_name);
            res = fixFileName(res, fi_orig.suffix());

        } else if (!filter.isEmpty()) {
            // берем требуемое расширение из фильтров
            auto extentions = extractExtentions(filter);
            if (extentions.count() == 1 && extentions.at(0).startsWith("*.")) {
                res = fixFileName(res, extentions.at(0).right(extentions.at(0).length() - 2));
            }
        }
    }

    return res;
}

QString Utils::getExistingDirectory(const QString& caption, const QString& dir, QFileDialog::Options options)
{
    QString res = QFileDialog::getExistingDirectory(getTopWindow(), caption.isEmpty() ? QApplication::applicationName() : caption,
                                                    dir.isEmpty() ? Core::lastUsedDirectory() : dir,
#ifdef Q_OS_LINUX
                                                    options // | QFileDialog::DontUseNativeDialog
#else
                                                    options
#endif
    );

    if (!res.isEmpty())
        Core::updateLastUsedDirectory(res);

    return res;
}

Error Utils::openFile(const QString& file_dir)
{
    Z_CHECK(!file_dir.isEmpty());
    if (!QDesktopServices::openUrl(QUrl::fromLocalFile(file_dir)))
        return Error::fileNotFoundError(file_dir);
    return {};
}

Error Utils::openFile(const QByteArray& file_content, const QString& extension)
{
    Z_CHECK(!extension.isEmpty());
    QString file_name = generateTempDocFileName(extension);
    auto error = saveByteArray(file_name, file_content);
    if (error.isError())
        return error;

    return openFile(file_name);
}

Error Utils::openDir(const QString& file_dir)
{
    QFileInfo info(file_dir);
    if (info.isDir())
        return openFile(file_dir);

    return openFile(info.absolutePath());
}

QString Utils::validFileName(const QString& s, const QChar& replace)
{
    QString res = s.simplified()
                      .replace('\\', " ")
                      .replace('/', " ")
                      .replace(':', " ")
                      .replace('*', " ")
                      .replace('?', " ")
                      .replace('"', " ")
                      .replace('<', " ")
                      .replace('>', " ")
                      .replace('^', " ")
                      .replace('%', " ")
                      .replace('$', " ")
                      .replace('#', " ")
                      .replace('@', " ")
                      .replace('!', " ")
                      .replace('~', " ")
                      .replace('&', " ")
                      .replace('+', " ")
                      .replace('\\', " ")
                      .replace('\'', " ")
                      .replace('|', " ")
                      .replace('_', " ")
                      .replace(',', " ")
                      .replace(';', " ")
                      .replace('-', " ")
                      .simplified()
                      .replace(' ', replace);

    int max_len;
#ifdef Q_OS_WIN
    max_len = qMax(10,
#ifdef __MINGW32__
                   MAX_PATH
#else
                   PATH_MAX
#endif
                       - documentsLocation().length() - 1);
#else
    max_len = NAME_MAX - 5;
#endif

    QFileInfo fi(res);
    QString basename = fi.baseName();
    QString ext = fi.completeSuffix();

    // Обрезаем длину файла до максимально разрешенного
    QByteArray ba = QString(basename + QString(ext.isEmpty() ? "" : ".") + ext).toUtf8();
    while (ba.size() > max_len) {
        basename = basename.left(basename.length() - 1);
        ba = QString(basename + QString(ext.isEmpty() ? "" : ".") + ext).toUtf8();
    }

    return basename + QString(ext.isEmpty() ? "" : ".") + ext;
}

Error Utils::saveByteArray(const QString& file_name, const QByteArray& ba)
{
    QSaveFile f(file_name);
    if (!f.open(QFile::WriteOnly | QFile::Truncate))
        return Error::fileIOError(f);

    if (f.write(ba) != ba.size())
        return Error::fileIOError(f);

    if (!f.commit())
        return Error::fileIOError(f);

    return {};
}

Error Utils::loadByteArray(const QString& file_name, QByteArray& ba)
{
    if (!QFile::exists(file_name))
        return Error::fileNotFoundError(file_name);

    QFile f(file_name);
    if (!f.open(QFile::ReadOnly))
        return Error::fileIOError(f);

    ba = f.readAll();
    return {};
}

QByteArray Utils::loadByteArray(const QString& file_name)
{
    QByteArray res;
    if (loadByteArray(file_name, res).isOk())
        return res;
    return QByteArray();
}

QJsonDocument Utils::openJson(const QByteArray& source, Error& error)
{
    QJsonParseError p_error;
    auto doc = QJsonDocument::fromJson(source, &p_error);
    if (p_error.error == QJsonParseError::NoError)
        return doc;

    error = Error(QStringLiteral("JSON parser: ") + p_error.errorString());
    return QJsonDocument();
}

QJsonDocument Utils::openJson(const QString& file_name, Error& error)
{
    Z_CHECK(!file_name.isEmpty());

    QFile f(file_name);
    if (!f.open(QFile::ReadOnly)) {
        error = Error::fileIOError(file_name);
        return QJsonDocument();
    }

    auto doc = openJson(f.readAll(), error);
    f.close();

    return doc;
}

bool Utils::dirCd(const QDir& dir, const QString& dirName, QDir& newDir, bool check_exists)
{
    if (check_exists) {
        // родной метод Qt проверяет на существование
        QDir d(dir);
        if (!d.cd(dirName))
            return false;
        newDir = dir;
        return true;
    }

    // код выдран из QDir::cd
    if (dirName.isEmpty() || dirName == QLatin1String("."))
        return true;
    QString newPath;
    if (QDir::isAbsolutePath(dirName)) {
        newPath = QDir::cleanPath(dirName);
    } else {
        newPath = dir.path();
        if (!newPath.endsWith(QLatin1Char('/')))
            newPath += QLatin1Char('/');
        newPath += dirName;
        if (dirName.indexOf(QLatin1Char('/')) >= 0 || dirName == QLatin1String("..") || dir.path() == QLatin1String(".")) {
            newPath = QDir::cleanPath(newPath);

            if (newPath.startsWith(QLatin1String(".."))) {
                newPath = QFileInfo(newPath).absoluteFilePath();
            }
        }
    }

    newDir = QDir(newPath);
    return true;
}

bool Utils::dirCdUp(const QDir& path, QDir& new_path, bool check_exists)
{
    return dirCd(path, QLatin1String(".."), new_path, check_exists);
}

Error Utils::makeDir(const QString& path
#ifdef Q_OS_LINUX
                     ,
                     const QString& root_password
#endif
)
{
    if (make_dir_helper(path
#ifdef Q_OS_LINUX
                        ,
                        root_password
#endif

                        )) {
        return Error();
    } else
        return Error::createDirError(path);
}

Error Utils::removeDir(const QString& path
#ifdef Q_OS_LINUX
                       ,
                       const QString& root_password
#endif
)
{
    if (remove_dir_helper(path
#ifdef Q_OS_LINUX
                          ,
                          root_password
#endif

                          ))
        return Error();
    else
        return Error::removeDirError(path);
}

Error Utils::clearDir(const QString& path, const QSet<QString>& keep
#ifdef Q_OS_LINUX
                      ,
                      const QString& root_password
#endif
)
{
    QStringList files;
    Error error = getDirContent(path, files, true, false);
    if (error.isError())
        return error;

    for (auto& file : qAsConst(files)) {
        QFileInfo f(file);
        if (f.isDir() && QDir(file).exists()) {
            if (keep.contains(file))
                continue;

            error = error.isError() ? error
                                    : removeDir(file
#ifdef Q_OS_LINUX
                                                ,
                                                root_password
#endif
                                    );
        } else {
            if (f.exists())
                error = error.isError() ? error
                                        : removeFile(file
#ifdef Q_OS_LINUX
                                                     ,
                                                     root_password
#endif
                                        );
        }
    }

    return error;
}

Error Utils::removeFile(const QString& fileName
#ifdef Q_OS_LINUX
                        ,
                        const QString& root_password
#endif
)
{
    if (!remove_file_helper(fileName
#ifdef Q_OS_LINUX
                            ,
                            root_password
#endif
                            ))
        return Error::removeFileError(fileName);
    else
        return Error();
}

Error Utils::renameFile(const QString& oldFileName, const QString& newFileName, bool createDir, bool overwrite
#ifdef Q_OS_LINUX
                        ,
                        const QString& root_password
#endif
)
{
    QFileInfo fiTarget(newFileName);
    Error error;

    if (createDir) {
        error = makeDir(fiTarget.absolutePath()
#ifdef Q_OS_LINUX
                            ,
                        root_password
#endif
        );
        if (error.isError())
            return error;
    }

    if (overwrite && fiTarget.exists()) {
        error = removeFile(newFileName
#ifdef Q_OS_LINUX
                           ,
                           root_password
#endif
        );
        if (error.isError())
            return error;
    }

    if (!rename_file_helper(oldFileName, newFileName
#ifdef Q_OS_LINUX
                            ,
                            root_password
#endif
                            ))
        return Error::renameFileError(oldFileName, newFileName);
    else
        return Error();
}

Error Utils::copyFile(const QString& fileName, const QString& newName, bool createDir, bool overwrite
#ifdef Q_OS_LINUX
                      ,
                      const QString& root_password
#endif
)
{
    QFileInfo fiTarget(newName);
    Error error;

    if (createDir) {
        error = makeDir(fiTarget.absolutePath()
#ifdef Q_OS_LINUX
                            ,
                        root_password
#endif
        );
        if (error.isError())
            return error;
    }

    if (overwrite && fiTarget.exists()) {
        error = removeFile(newName
#ifdef Q_OS_LINUX
                           ,
                           root_password
#endif
        );
        if (error.isError())
            return error;
    }

    if (!copy_file_helper(fileName, newName
#ifdef Q_OS_LINUX
                          ,
                          root_password
#endif
                          ))
        error = Error::copyFileError(fileName, newName);

    return error;
}

Error Utils::moveFile(const QString& fileName, const QString& newName, bool createDir, bool overwrite
#ifdef Q_OS_LINUX
                      ,
                      const QString& root_password
#endif
)
{
    return renameFile(fileName, newName, createDir, overwrite
#ifdef Q_OS_LINUX
                      ,
                      root_password
#endif
    );
}

Utils::CopyDirInfo Utils::copyDir(const QString& sourcePath, const QString& destinationPath, bool overWriteDirectory,
                                  const QStringList& excludedFiles, ProgressObject* progress, const QString& progressText

#ifdef Q_OS_LINUX
                                  ,
                                  const QString& root_password
#endif
)
{
    QString p_text = progressText.isEmpty() ? ZF_TR(ZFT_FILE_COPY) : progressText;

    ProgressHelper ph;
    if (progress) {
        ph.reset(progress);
        progress->startProgress(p_text);
    }

    Error error;

    // Подготовливаем строку папки
    QString dPath = QDir::fromNativeSeparators(destinationPath);
    if (!dPath.isEmpty() && dPath.at(dPath.length() - 1) == '/')
        dPath = dPath.left(dPath.length() - 1);
    // Подготовливаем строку папки
    QString sPath = QDir::fromNativeSeparators(sourcePath);
    if (!sPath.isEmpty() && sPath.at(sPath.length() - 1) == '/')
        sPath = sPath.left(sPath.length() - 1);

    QDir originDirectory(sPath);

    if (!originDirectory.exists())
        return Error(ZF_TR(ZFT_FOLDER_NOT_EXISTS).arg(QDir::toNativeSeparators(sPath)));

    // Получаем список файлов для копирования
    QStringList sFilesRelative;
    error = getDirContent(sPath, sFilesRelative, true, true);
    if (error.isError())
        return error;

    // Формируем список для очистки без удаления
    QSet<QString> filesAbsolute;
    for (auto& s : qAsConst(sFilesRelative)) {
        filesAbsolute << dPath + "/" + s;
    }

    QDir destinationDirectory(dPath);

    if (destinationDirectory.exists() && !overWriteDirectory)
        return Error(ZF_TR(ZFT_FOLDER_ALREADY_EXISTS).arg(QDir::toNativeSeparators(dPath)));
    else if (destinationDirectory.exists() && overWriteDirectory) {
        error = clearDir(dPath, filesAbsolute
#ifdef Q_OS_LINUX
                         ,
                         root_password
#endif
        );
        if (error.isError())
            return error;
    }

    CopyDirInfo info;

    bool destDirCreated = false;
    QSet<QString> excludedFilesSet = toSet(excludedFiles);
    for (int i = 0; i < sFilesRelative.count(); i++) {
        QString file = sFilesRelative.at(i);

        QString sourceFile = sPath + "/" + file;
        QFileInfo sourceInfo(sourceFile);

        QString destinationFile = dPath + "/" + file;

        if (!excludedFilesSet.contains(sourceFile) && !excludedFilesSet.contains(sourceInfo.absoluteDir().absolutePath())) {
            if (sourceInfo.isDir()) {
                // Это папка, просто создаем
                error = makeDir(destinationFile
#ifdef Q_OS_LINUX
                                ,
                                root_password
#endif
                );
                if (error.isError())
                    return error;

            } else {
                // Это файл
                error = copyFile(sourceFile, destinationFile, true, true
#ifdef Q_OS_LINUX
                                 ,
                                 root_password
#endif
                );
                if (error.isError())
                    return error;

                info.source_files << sourceFile;
                info.dest_files << destinationFile;
                info.size += sourceInfo.size();
            }

            destDirCreated = true;
        }

        if (progress)
            progress->setProgressPercent((i + 1) * 100 / sFilesRelative.count());
    }

    if (!destDirCreated) {
        // Целевая папка не была создана - создаем
        error = makeDir(dPath
#ifdef Q_OS_LINUX
                        ,
                        root_password
#endif
        );
        if (error.isError())
            return error;
    }

    return info;
}

Error Utils::getDirContentHelper(const QString& path, QStringList& files, bool isRecursive, bool isRelative, const QString& relativePath)
{
    Error error;

    QDir directory(path);
    if (!directory.exists())
        return Error(ZF_TR(ZFT_FOLDER_NOT_EXISTS).arg(QDir::toNativeSeparators(path)));

    QString rPath = (isRelative && !relativePath.isEmpty() ? relativePath + QStringLiteral("/") : QString());

    if (isRecursive) {
        for (auto& directoryName : directory.entryList(QDir::Dirs | QDir::NoDotAndDotDot | QDir::Hidden)) {
            if (isRelative)
                files << rPath + directoryName + "/";
            else
                files << path + "/" + directoryName + "/";

            error = getDirContentHelper(path + QStringLiteral("/") + directoryName, files, isRecursive, isRelative,
                                        isRelative ? rPath + directoryName : QString());
            if (error.isError())
                return error;
        }
    }

    for (auto& fileName : directory.entryList(QDir::Files)) {
        if (isRelative)
            files << rPath + fileName;
        else
            files << path + "/" + fileName;
    }

    return Error();
}

Error Utils::getDirContent(const QString& path, QStringList& files, bool isRecursive, bool isRelative)
{
    files.clear();

    // Подготовливаем строку папки
    QString dPath = QDir::fromNativeSeparators(path);
    if (!dPath.isEmpty() && dPath.at(dPath.length() - 1) == '/')
        dPath = dPath.left(dPath.length() - 1);

    return getDirContentHelper(dPath, files, isRecursive, isRelative, "");
}

bool Utils::remove_file_helper(const QString& fileName

#ifdef Q_OS_LINUX
                               ,
                               const QString& root_password
#endif
)
{
    QString name_prepared = QDir::fromNativeSeparators(fileName);
    if (!QFile::exists(name_prepared))
        return true;

#ifdef Q_OS_LINUX
    if (!root_password.trimmed().isEmpty() && !isRoot()) {
        return execute_sudo_comand_helper(QStringLiteral("rm %1").arg(name_prepared), root_password);
    } else {
#endif
        return QFile::remove(name_prepared);
#ifdef Q_OS_LINUX
    }
#endif
}

bool Utils::rename_file_helper(const QString& oldName, const QString& newName
#ifdef Q_OS_LINUX
                               ,
                               const QString& root_password
#endif
)
{
    QString old_name_prepared = QDir::fromNativeSeparators(oldName);
    QString new_name_prepared = QDir::fromNativeSeparators(newName);

    if (!QFile::exists(old_name_prepared) || QFile::exists(new_name_prepared)) {
#ifdef Z_DEBUG_MODE
        ZCore::logError(utf("Utils::rename_file_helper error: %1 -> %2, %3, %4")
                            .arg(old_name_prepared, new_name_prepared)
                            .arg(QFile::exists(old_name_prepared))
                            .arg(QFile::exists(new_name_prepared)));
#endif
        return false;
    }

    if (old_name_prepared == new_name_prepared)
        return true;

#ifdef Q_OS_LINUX
    if (!root_password.trimmed().isEmpty() && !isRoot()) {
        return execute_sudo_comand_helper(QStringLiteral("mv %1 %2").arg(old_name_prepared, new_name_prepared), root_password);
    } else {
#endif
        return QFile::rename(old_name_prepared, new_name_prepared);
#ifdef Q_OS_LINUX
    }
#endif
}

bool Utils::copy_file_helper(const QString& fileName, const QString& newName
#ifdef Q_OS_LINUX
                             ,
                             const QString& root_password
#endif
)
{
    QString old_name_prepared = QDir::fromNativeSeparators(fileName);
    QString new_name_prepared = QDir::fromNativeSeparators(newName);

    if (!QFile::exists(old_name_prepared) || QFile::exists(new_name_prepared))
        return false;

#ifdef Q_OS_LINUX
    if (!root_password.trimmed().isEmpty() && !isRoot()) {
        return execute_sudo_comand_helper(QStringLiteral("install -m 755 %1 %2").arg(old_name_prepared, new_name_prepared), root_password);
    } else {
#endif
        return QFile::copy(old_name_prepared, new_name_prepared);
#ifdef Q_OS_LINUX
    }
#endif
}

bool Utils::make_dir_helper(const QString& path
#ifdef Q_OS_LINUX
                            ,
                            const QString& root_password
#endif
)
{
    QDir dir(QDir::fromNativeSeparators(path));
    if (dir.exists())
        return true;

    QDir dir_up;
    Z_CHECK(dirCdUp(dir, dir_up, false));

    if (!dir_up.exists()) {
        if (!make_dir_helper(dir_up.absolutePath()
#ifdef Q_OS_LINUX
                                 ,
                             root_password
#endif
                             ))
            return false;
    }

#ifdef Q_OS_LINUX
    if (!root_password.isEmpty() && !isRoot()) {
        return execute_sudo_comand_helper(QStringLiteral("mkdir -m 755 '%1'").arg(dir.absolutePath()), root_password);
    } else {
#endif
        return dir.mkdir(dir.absolutePath());
#ifdef Q_OS_LINUX
    }
#endif
}

bool Utils::remove_dir_helper(const QString& path
#ifdef Q_OS_LINUX
                              ,
                              const QString& root_password
#endif
)
{
    QDir dir(QDir::fromNativeSeparators(path));
#ifdef Q_OS_LINUX
    if (!root_password.isEmpty() && !isRoot()) {
        return execute_sudo_comand_helper(QStringLiteral("rm -f -r -d '%1'").arg(dir.absolutePath()), root_password);
    } else {
#endif
        return dir.removeRecursively();
#ifdef Q_OS_LINUX
    }
#endif
}

#ifdef Q_OS_LINUX
bool Utils::execute_sudo_comand_helper(const QString& command, const QString& root_password)
{
    Z_CHECK(!command.isEmpty() && !root_password.trimmed().isEmpty());

    QString script_code = QStringLiteral("if echo %2 | sudo -S %1; then exit 0; else exit 1; fi")
                              .arg(command, QStringLiteral("\"") + root_password + QStringLiteral("\n\""));

    QProcess p;
    p.start(QStringLiteral("/bin/bash"), QStringList {QStringLiteral("-c"), QStringLiteral("(") + script_code + QStringLiteral(")")});
    if (!p.waitForStarted(3000))
        return false;

    p.waitForFinished();

    return p.exitCode() == 0;
}
#endif

QString Utils::location(const QString& path, const QString& default_path)
{
    QMutexLocker lock(&_location_mutex);

    if (_location_created == nullptr)
        _location_created = std::make_unique<QHash<QString, QString>>();

    auto p = _location_created->find(path);

    if (p != _location_created->end()) {
        return p.value();
    }

    if (QDir::root().mkpath(path)) {
        _location_created->insert(path, path);
        return path;
    } else {
        Z_CHECK(QDir(default_path).exists());
        _location_created->insert(path, default_path);
        return default_path;
    }
}

static QString _userLocationPrefix()
{
    QString user = Core::currentUserLogin();
    if (!user.isEmpty())
        return QStringLiteral("/") + user;
    return QString();
}

QString Utils::instanceLocation(const QString& instance_key)
{
    QString s = QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation);
    QString instance_key_prep;
    if (!instance_key.isEmpty())
        instance_key_prep = QStringLiteral("/") + instance_key;
    return location(
        s + QStringLiteral("/") + Consts::CORE_INTERNAL_CODE
            + (!Core::coreInstanceKey(false).isEmpty() ? QStringLiteral("/") + Core::coreInstanceKey(false) : instance_key_prep),
        s);
}

QString Utils::instanceTempLocation(const QString& instance_key)
{
    QString s = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    QString instance_key_prep;
    if (!instance_key.isEmpty())
        instance_key_prep = QStringLiteral("/") + instance_key;
    return location(
        s + QStringLiteral("/") + Consts::CORE_INTERNAL_CODE
            + (!Core::coreInstanceKey(false).isEmpty() ? QStringLiteral("/") + Core::coreInstanceKey(false) : instance_key_prep),
        s);
}

QString Utils::instanceCacheLocation(const QString& instance_key)
{
    QString s = QStandardPaths::writableLocation(QStandardPaths::GenericCacheLocation);
    QString instance_key_prep;
    if (!instance_key.isEmpty())
        instance_key_prep = QStringLiteral("/") + instance_key;
    return location(
        s + QStringLiteral("/") + Consts::CORE_INTERNAL_CODE
            + (!Core::coreInstanceKey(false).isEmpty() ? QStringLiteral("/") + Core::coreInstanceKey(false) : instance_key_prep),
        s);
}

QString Utils::dataLocation()
{
    return location(instanceLocation() + _userLocationPrefix());
}

QString Utils::tempLocation()
{
    return location(instanceTempLocation() + _userLocationPrefix());
}

QString Utils::generateTempFileName(const QString& extention)
{
    makeDir(tempLocation());
    return tempLocation() + QStringLiteral("/") + generateUniqueString(QCryptographicHash::Sha224, QString(), true)
           + (!extention.isEmpty() ? QStringLiteral(".") + extention : QString());
}

QString Utils::generateTempDirName(const QString& sub_folder)
{
    makeDir(tempLocation());
    return tempLocation() + QStringLiteral("/") + (sub_folder.isEmpty() ? QString() : sub_folder + QStringLiteral("/"))
           + generateUniqueString(QCryptographicHash::Sha224, QString(), true);
}

QString Utils::tempDocsLocation()
{
    return tempLocation() + QStringLiteral("/docs");
}

QString Utils::generateTempDocFileName(const QString& extention)
{
    makeDir(tempDocsLocation());
    return tempDocsLocation() + QStringLiteral("/") + generateUniqueString(QCryptographicHash::Sha224, QString(), true)
           + (!extention.isEmpty() ? QStringLiteral(".") + extention : QString());
}

QString Utils::documentsLocation()
{
    return QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
}

QString Utils::cacheLocation()
{
    return location(instanceCacheLocation() + _userLocationPrefix());
}

QString Utils::dataSharedLocation()
{
    QString s = QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation);
    return location(s + QStringLiteral("/") + Consts::CORE_INTERNAL_CODE + QStringLiteral("/shared"), s);
}

QString Utils::moduleDataLocation(const EntityCode& entity_code)
{
    Z_CHECK(entity_code.isValid());
    QString s = QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation);
    return location(dataLocation() + QStringLiteral("/entities/%1").arg(entity_code.string()), s);
}

QString Utils::logLocation()
{
    QString s = QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation);
    return location(_log_folder.isEmpty() ? dataLocation() + QStringLiteral("/logs") : _log_folder, s);
}

void Utils::setLogLocation(const QString& path)
{
    Z_CHECK(isMainThread());
    _log_folder = path;
}

QString Utils::driverCacheLocation()
{
    return cacheLocation() + QStringLiteral("/srv");
}

} // namespace zf
