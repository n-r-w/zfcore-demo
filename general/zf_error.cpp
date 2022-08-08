#include "zf_error.h"
#include "zf_core.h"
#include "zf_html_tools.h"
#include "zf_translation.h"
#include "zf_logging.h"

#include <QDir>

namespace zf
{
int Error::_metatype_number = 0;

//! Разделяемые данные для Error
class Error_data : public QSharedData
{
public:
    Error_data();
    Error_data(const Error_data& d);
    ~Error_data();

    void copyFrom(const Error_data* d);

    static Error_data* shared_null();

    void clear();

    ErrorType type = ErrorType::No;
    int code = -1;
    QString text;
    QString detail;
    QString extended;
    QVariant data;
    bool is_main = false;
    bool is_close = false;
    bool is_modal = true;
    bool is_hidden = false;
    bool need_reload = false;
    ErrorList child_errors;
    QString child_error_text;
    QString hash_value;
    InformationType info_type = InformationType::Error;
    bool is_note = false;
    bool is_file_debug = false;
    QString file_debug_modifier;
    ErrorPtr non_critical_error;
    QList<Uid> uids;
};

Q_GLOBAL_STATIC(Error_data, nullResult)
Error_data* Error_data::shared_null()
{
    auto res = nullResult();
    if (res->ref == 0)
        res->ref.ref(); // чтобы не было удаления самого nullResult
    return res;
}

Error::Error()
    : _d(Error_data::shared_null())
{
}

Error::Error(const Error& e)
    : _d(e._d)
{
}

Error::Error(ErrorType type, int code, const QString& text, const QString& detail)
    : _d(new Error_data())
{
    _d->type = type;
    _d->code = code;
    _d->text = text.trimmed();
    _d->detail = detail.trimmed();

    Z_CHECK(!_d->text.isEmpty());
    init();
}

Error::Error(const QString& text, const QString& detail)
    : Error(ErrorType::Custom, -1, text, detail)
{
}

Error::Error(const char* text)
    : Error(utf(text))
{
}

Error::Error(int code, const QString& text, const QString& detail)
    : Error(ErrorType::Custom, code, text, detail)
{
}

Error::Error(const ErrorList& errors)
    : Error(fromList(errors))
{
}

Error::Error(int code, const ErrorList& errors)
    : _d(new Error_data())
{
    if (errors.isEmpty())
        *this = Error();
    else {
        *this = errors.at(0);

        QList<Error> e(errors);
        e.removeFirst();

        addChildError(e);
    }

    _d->code = code;
    init();
}

Error::Error(const Log& log)
    : Error(-1, log)
{
}

Error::Error(int code, const Log& log)
    : Error(logItemsError(log).setCode(code))
{    
}

Error::~Error()
{
}

Error& Error::operator=(const Error& e)
{    
    if (this != &e)
        _d = e._d;
    return *this;
}

QString Error::debString() const
{
    return isOk() ? "OK" : fullText();
}

Error Error::debPrint() const
{
    return Logging::debugFile("zf::Error", {}, debString());
}

Error& Error::clear()
{
    if (!isError())
        return *this;

    _d->clear();

    return *this;
}

ErrorType Error::type() const
{
    return _d->type;
}

Error& Error::setType(ErrorType type)
{
    if (_d->type != type) {
        Z_CHECK(isError());
        Z_CHECK(type != ErrorType::No);
        _d->type = type;
        changed();
    }
    return *this;
}

bool Error::isError() const
{
    return type() != ErrorType::No;
}

bool Error::isOk() const
{
    return type() == ErrorType::No;
}

QString Error::text() const
{
    return _d->text;
}

Error& Error::setText(const QString& text)
{
    Z_CHECK(isError());
    _d->text = text.trimmed();
    Z_CHECK(!_d->text.isEmpty());
    changed();
    return *this;
}

QString Error::textDetail() const
{
    return _d->detail;
}

Error& Error::setTextDetail(const QString& detail)
{
    Z_CHECK(isError());
    _d->detail = detail.trimmed();
    changed();
    return *this;
}

QString Error::fullText(int limit, bool is_html) const
{
    QString s = textDetailFull(is_html);

    QString text;

    if (!isLogItemsError())
        text = _d->text;
    else
        text = data().value<Log>().toString();

    if (!s.isEmpty() && s != text) {
        QString delimeter
                = is_html ? QStringLiteral("<br>") : ((s.at(0).isUpper()) ? QStringLiteral(".") : QStringLiteral(","));
        text = QStringLiteral("%1%2 %3").arg(text, delimeter, s);
    }

    return (limit <= 0 || text.length() <= limit) ? text : text.left(limit) + QStringLiteral("...");
}

QString Error::textDetailFull(bool is_html) const
{
    QString detail;
    if (_d->child_errors.count() > 0) {
        // Формируем текст ошибки с учетом дочерних
        if (_d->detail.isEmpty())
            detail = childErrorText(is_html);
        else
            detail = QStringLiteral("%1%2%3").arg(
                    _d->detail, is_html ? QStringLiteral("<br>") : QStringLiteral("\n"), childErrorText(is_html));
    } else {
        detail = _d->detail;
    }

    if (_d->text == detail)
        detail.clear();

    return detail;
}

QString Error::childErrorText(bool is_html) const
{
    if (_d->child_errors.isEmpty())
        return QString();

    if (_d->child_error_text.isEmpty()) {
        QString s;
        for (auto& err : qAsConst(_d->child_errors)) {
            QString childText;

            if (err.isLogItemsError()) {
                // Для таких ошибок текст надо формировать из журнала
                Log log = Log::fromVariant(data());
                if (log.isEmpty())
                    continue; // пустой журнал ошибко игнорируем

                childText = log.toString();
            } else {
                childText = err.textDetail();
                HtmlTools::plainIfHtml(childText, false);
            }

            if (childText.isEmpty())
                childText = err.text();
            else
                childText = QStringLiteral("%1 (%2)").arg(err.text(), childText);

            if (err.childErrors().count() > 0)
                childText = QStringLiteral("%1 - %2").arg(childText, err.childErrorText());

            if (s.isEmpty())
                s = childText;
            else
                s = QStringLiteral("%1%2%3").arg(s, is_html ? QStringLiteral("<br>") : QStringLiteral(" ,"), childText);
        }

        const_cast<Error*>(this)->_d->child_error_text = s;
    }
    return _d->child_error_text;
}

const char* Error::pritable() const
{
    if (_printable == nullptr)
        _printable = std::make_unique<QByteArray>(fullText().toLocal8Bit());

    return _printable->constData();
}

Error Error::nonCriticalError() const
{
    return _d->non_critical_error ? *_d->non_critical_error : Error();
}

Error& Error::setNonCriticalError(const Error& err)
{
    if (err.isError()) {
        if (_d->non_critical_error == nullptr)
            _d->non_critical_error = std::make_shared<Error>(err);
        else
            *_d->non_critical_error = err;
        changed();

    } else {
        if (_d->non_critical_error != nullptr) {
            _d->non_critical_error.reset();
             changed();
        }
    }
    return *this;
}

int Error::code() const
{
    return _d->code;
}

Error& Error::setCode(int code)
{
    if (_d->code != code) {
        Z_CHECK(isError());
        _d->code = code;
        changed();
    }
    return *this;
}

const QVariant& Error::data() const
{
    return _d->data;
}

Error& Error::setData(const QVariant& v)
{
    Z_CHECK(isError());
    _d->data = v;
    changed();
    return *this;
}

QString Error::extendedInfo() const
{
    return _d->extended;
}

Error& Error::setExtendedInfo(const QString& text)
{
    Z_CHECK(isError());
    _d->extended = text.trimmed();
    changed();
    return *this;
}

bool Error::isMain() const
{
    return _d->is_main;
}

Error& Error::setMain(bool is_main)
{
    Z_CHECK(isError());
    if (_d->is_main != is_main) {
        _d->is_main = is_main;
        changed();
    }
    return *this;
}

bool Error::isHidden() const
{
    return _d->is_hidden;
}

Error& Error::setHidden(bool is_hidden)
{
    Z_CHECK(isError());
    if (_d->is_hidden != is_hidden) {
        _d->is_hidden = is_hidden;
        changed();
    }
    return *this;
}

InformationType Error::informationType() const
{
    Error main = extractMainError();
    if (main.isError())
        return main._d->info_type;

    QList<InformationType> types;
    types << _d->info_type;

    for (auto& e : _d->child_errors) {
        types << e.informationType();
    }

    return Utils::getTopErrorLevel(types);
}

Error& Error::setInformationType(InformationType t)
{
    Z_CHECK(isError());
    if (_d->info_type != t) {
        _d->info_type = t;
        changed();
    }
    return *this;
}

bool Error::isNote() const
{
    return _d->is_note;
}

Error& Error::setNote(bool is_note)
{
    Z_CHECK(isError());
    if (_d->is_note != is_note) {
        _d->is_note = is_note;
        changed();
    }
    return *this;
}

QList<Uid> Error::uids() const
{
    return _d->uids;
}

Error& Error::setUids(const QList<Uid>& uids)
{
    _d->uids = uids;
    changed();
    return *this;
}

bool Error::isCloseWindow() const
{
    return _d->is_close;
}

Error& Error::setCloseWindow(bool b)
{
    Z_CHECK(isError());
    if (_d->is_close != b) {
        _d->is_close = b;
        changed();
    }
    return *this;
}

bool Error::isModalWindow() const
{
    return _d->is_modal;
}

Error& Error::setModalWindow(bool b)
{
    Z_CHECK(isError());
    if (_d->is_modal != b) {
        _d->is_modal = b;
        changed();
    }
    return *this;
}

bool Error::needReload() const
{
    return _d->need_reload;
}

Error& Error::setNeedReload(bool b)
{
    Z_CHECK(isError());
    if (_d->need_reload != b) {
        _d->need_reload = b;
        changed();
    }
    return *this;
}

bool Error::isFileDebug() const
{
    return _d->is_file_debug;
}

QString Error::fileDebugModifier() const
{
    return _d->file_debug_modifier;
}

Error& Error::setFileDebug(bool is_file_debug, const QString& modifier)
{
    Z_CHECK(isError());
    Z_CHECK(!modifier.isEmpty());
    if (_d->is_file_debug != is_file_debug || _d->file_debug_modifier != modifier) {
        _d->is_file_debug = is_file_debug;
        _d->file_debug_modifier = modifier;
        changed();
    }
    return *this;
}

Error Error::extractMainError() const
{
    if (isMain())
        return *this;

    for (const Error& e : _d->child_errors) {
        if (e.isMain())
            return e;
    }

    return Error();
}

Error Error::extractNotMainError() const
{
    Error res;

    if (!isMain())
        res << extractParentError();

    for (const Error& e : _d->child_errors) {
        if (!e.isMain())
            res << e.extractParentError();
    }

    return res;
}

Error Error::extractParentError() const
{
    if (_d->child_errors.isEmpty())
        return *this;

    Error res = *this;
    res._d->child_errors.clear();
    res.changed();
    return res;
}

QStringList Error::toStringList() const
{
    if (!isError())
        return QStringList();

    if (childErrors().isEmpty())
        return {fullText()};

    QStringList res;

    bool main_found = false;
    if (isMain()) {
        main_found = true;
    }
    res << extractParentError().fullText();

    for (const Error& e : _d->child_errors) {
        if (!main_found && e.isMain()) {
            res.prepend(e.fullText());
            main_found = true;
        } else {
            res << e.toStringList();
        }
    }

    res.removeDuplicates();

    return res;
}

QVariant Error::variant() const
{
    return QVariant::fromValue(*this);
}

Error Error::fromVariant(const QVariant& v)
{
    return v.value<Error>();
}

bool Error::isErrorVariant(const QVariant& v)
{
    return v.userType() == _metatype_number;
}

ErrorList Error::toList() const
{
    if (isOk())
        return {};

    ErrorList res;
    res << withoutChildErrors();
    res << _d->child_errors;

    return res;
}

Error Error::fromList(const ErrorList& e, bool auto_main)
{
    if (e.isEmpty())
        return Error();

    ErrorList list;
    int main_index = -1;
    for (auto& err : e) {
        if (!err.isError())
            continue;

        if (err.childErrors().isEmpty()) {
            list << err;
            if (main_index < 0 && err.isMain())
                main_index = list.count() - 1;
            continue;
        }

        list << err.extractParentError();
        for (auto& c_err : err.childErrors()) {
            list << c_err;
            if (main_index < 0 && c_err.isMain())
                main_index = list.count() - 1;
        }
    }

    if (list.isEmpty())
        return Error();

    Error res;
    if (auto_main) {
        if (main_index < 0)
            main_index = 0;

        list.move(main_index, 0);
    }

    res = list.constFirst();
    if (auto_main)
        res.setMain(true);
    for (int i = 1; i < list.count(); i++) {
        res.addChildError(list.at(i));
    }

    return res;
}

const ErrorList& Error::childErrors() const
{
    return _d->child_errors;
}

ErrorList Error::childErrors(ErrorType t) const
{
    ErrorList res;

    if (type() == t)
        res << *this;

    for (const Error& e : _d->child_errors) {
        if (e.type() == t)
            res << e;
    }

    return res;
}

bool Error::containsChildErrors(ErrorType t) const
{
    if (type() == t)
        return true;

    for (const Error& e : _d->child_errors) {
        if (e.type() == t)
            return true;
    }

    return false;
}

int Error::count() const
{
    return isError() ? (_d->child_errors.count() + 1) : 0;
}

Error& Error::addChildError(const Error& error)
{
    return addChildError(ErrorList {error});
}

Error& Error::addChildError(const ErrorList& errors)
{
    bool ch = false;
    for (const Error& err : errors) {
        if (err.isOk())
            continue;

        if (isOk()) {
            *this = err;
            ch = true;
            continue;
        }

        // без дочерних
        _d->child_errors << err.extractParentError();
        // дочерних добавляем отдельно
        addChildError(err.childErrors());
        ch = true;
    }

    if (ch)
        changed();

    return *this;
}

Error& Error::addChildError(const QString& error)
{
    return addChildError(Error(error));
}

Error& Error::addChildError(const char* error)
{
    return addChildError(Error(error));
}

Error& Error::operator<<(const Error& e)
{
    return addChildError(e);
}

Error& Error::operator<<(const QString& e)
{
    return addChildError(Error(e));
}

Error& Error::operator<<(const char* e)
{
    return addChildError(Error(e));
}

Error& Error::operator<<(const ErrorList& errors)
{
    return addChildError(errors);
}

Error& Error::operator+=(const Error& e)
{
    return addChildError(e);
}

Error& Error::operator+=(const QString& e)
{
    return addChildError(Error(e));
}

Error& Error::operator+=(const char* e)
{
    return addChildError(Error(e));
}

Error& Error::operator+=(const ErrorList& errors)
{
    return addChildError(errors);
}

Error Error::operator+(const Error& e) const
{
    return Error(*this).addChildError(e);
}

Error Error::operator+(const ErrorList& e) const
{
    return Error(*this).addChildError(e);
}

Error& Error::removeChildErrors()
{
    if (_d->child_errors.isEmpty())
        return *this;

    _d->child_errors.clear();
    changed();
    return *this;
}

Error Error::withoutChildErrors() const
{
    Error error(*this);
    return error.removeChildErrors();
}

static QString _combine(const QString& s1, const QString& s2)
{
    QString s1_t = s1.trimmed();
    QString s2_t = s2.trimmed();

    if (s2_t.isEmpty())
        return s1_t;
    else if (s1_t.isEmpty())
        return s2_t;
    else if (s1_t == s2_t) {
        return s1_t;
    } else {
        if (s1_t.right(1) == QStringLiteral(".") || s1_t.right(1) == QStringLiteral(","))
            return s1_t + QStringLiteral(" ") + s2_t;
        else
            return s1_t + QStringLiteral(", ") + s2_t;
    }
}

Error Error::compressChildErrors() const
{
    if (isOk() || childErrors().isEmpty())
        return *this;

    Error error;
    error << this->withoutChildErrors();
    QStringList rows({_combine(text(), textDetail())});
    for (const Error& err : childErrors()) {
        QString row = _combine(err.text(), err.textDetail());
        if (rows.contains(row))
            continue;

        rows << row;
        error << err;
    }

    return error;
}

bool Error::operator==(const Error& e) const
{
    if (_d == e._d)
        return true;

    if (type() != e.type() || code() != e.code() || childErrors().count() != e.childErrors().count())
        return false;

    if (e.code() != -1)
        return true; // не код по умолчанию

    return fullText() == e.fullText();
}

void Error::detach()
{
    _d.detach();
}

bool Error::isHiddenError() const
{
    return type() == ErrorType::Hidden;
}

bool Error::isCoreError() const
{
    return type() == ErrorType::Core;
}

bool Error::isSystemError() const
{
    return type() == ErrorType::System;
}

bool Error::isCancelError() const
{
    return type() == ErrorType::Cancel;
}

bool Error::isSyntaxError() const
{
    return type() == ErrorType::Syntax;
}

bool Error::isNeedConfirmationError() const
{
    return type() == ErrorType::NeedConfirmation;
}

bool Error::isApplicationHaltedError() const
{
    return type() == ErrorType::ApplicationHalted;
}

bool Error::isDatabaseDriverError() const
{
    return type() == ErrorType::DatabaseDriver;
}

bool Error::isModuleError() const
{
    return type() == ErrorType::Module;
}

bool Error::isViewNotFoundError() const
{
    return type() == ErrorType::ViewNotFound;
}

bool Error::isModuleNotFoundError() const
{
    return type() == ErrorType::ModuleNotFound;
}

bool Error::isDocumentError() const
{
    return type() == ErrorType::Document;
}

bool Error::isDatabaseError() const
{
    return type() == ErrorType::Database;
}

bool Error::isHasUnsavedDataError() const
{
    return type() == ErrorType::HasUnsavedData;
}

bool Error::isEntityNotFoundError() const
{
    return type() == ErrorType::EntityNotFound;
}

bool Error::isLogItemsError() const
{
    return type() == ErrorType::LogItems;
}

bool Error::isFileNotFoundError() const
{
    return type() == ErrorType::FileNotFound;
}

bool Error::isBadFileError() const
{
    return type() == ErrorType::BadFile;
}

bool Error::isFileIOError() const
{
    return type() == ErrorType::FileIO;
}

bool Error::isCopyFileError() const
{
    return type() == ErrorType::CopyFile;
}

bool Error::isRemoveFileError() const
{
    return type() == ErrorType::RemoveFile;
}

bool Error::isRenameFileError() const
{
    return type() == ErrorType::RenameFile;
}

bool Error::isCreateDirError() const
{
    return type() == ErrorType::CreateDir;
}

bool Error::isRemoveDirError() const
{
    return type() == ErrorType::RemoveDir;
}

bool Error::isTimeoutError() const
{
    return type() == ErrorType::Timeout;
}

bool Error::isUnsupportedError() const
{
    return type() == ErrorType::Unsupported;
}

bool Error::isJsonError() const
{
    return type() == ErrorType::Json;
}

bool Error::isCorruptedDataError() const
{
    return type() == ErrorType::CorruptedData;
}

bool Error::isForbiddenError() const
{
    return type() == ErrorType::Forbidden;
}

bool Error::isDataNotFound() const
{
    return type() == ErrorType::DataNotFound;
}

bool Error::isConversionError() const
{
    return type() == ErrorType::Conversion;
}

bool Error::isConnectionClosedError() const
{
    return type() == ErrorType::ConnectionClosed;
}

bool Error::isChangedByOtherUserError() const
{
    return type() == ErrorType::ChangedByOtherUser;
}

Error Error::hiddenError(const QString& text, const QString& detail)
{
    return Error(ErrorType::Hidden, -1, text, detail);
}

Error Error::coreError(const QString& text, const QString& detail)
{
    return Error(ErrorType::Core, -1, text, detail);
}

Error Error::systemError(const QString& text, const QString& detail)
{
    return Error(ErrorType::System, -1, text, detail);
}

Error Error::cancelError()
{
    return Error(ErrorType::Cancel, -1, QStringLiteral("cancel error"));
}

Error Error::syntaxError(const QString& text, const QString& detail)
{
    return Error(ErrorType::Syntax, -1, text, detail);
}

Error Error::needConfirmationError(const QString& text, const QString& detail)
{
    return Error(ErrorType::NeedConfirmation, -1, text, detail);
}

Error Error::applicationHaltedError()
{
    return Error(ErrorType::ApplicationHalted, -1, QStringLiteral("halted"));
}

Error Error::databaseDriverError(const QString& text)
{
    return Error(ErrorType::DatabaseDriver, -1, text);
}

Error Error::moduleError(const QString& text)
{
    return Error(ErrorType::Module, -1, text);
}

Error Error::moduleError(const EntityCode& code, const QString& text)
{
    return Error(ErrorType::Module, -1, Core::getModuleName(code), text);
}

Error Error::viewNotFoundError(const EntityCode& code)
{
    return Error(ErrorType::ViewNotFound, -1, Core::getModuleName(code), "view not found");
}

Error Error::moduleNotFoundError(const EntityCode& code)
{
    return Error(ErrorType::ModuleNotFound, -1, Core::getModuleName(code), "plugin not found");
}

Error Error::documentError(const Uid& uid, const QString& text, const QString& detail)
{
    Z_CHECK(uid.isValid());
    return Error(ErrorType::Document, -1, text, detail).setData(uid.variant()).setUids({uid});
}

Error Error::databaseError(const QString& text, const QString& detail)
{
    return Error(ErrorType::Database, -1, text, detail);
}

Error Error::hasUnsavedDataError(const Uid& uid)
{
    Z_CHECK(uid.isValid());
    return Error(ErrorType::HasUnsavedData, -1, ZF_TR_EX(ZFT_HAS_UNSAVED_DATA, "Has unsaved data")).setData(uid.variant()).setUids({uid});
}

Error Error::entityNotFoundError(const Uid& uid)
{
    Z_CHECK(uid.isValid());

    return Error(ErrorType::EntityNotFound, -1, ZF_TR_EX(ZFT_ENTITY_NOT_FOUND, "Entity not found") + ": " + uid.toPrintable()).setData(uid.variant()).setUids({uid});
}

Error Error::logItemsError(const Log& log)
{
    Z_CHECK(!log.isEmpty());

    Error error = Error(ErrorType::LogItems, -1, ZF_TR_EX(ZFT_ERROR_LOG, "Log error"));
    error.setData(log.variant());
    return error;
}

Error Error::fileNotFoundError(const QString& fileName, const QString& text)
{
    return Error(ErrorType::FileNotFound, -1,
        ZF_TR_EX(ZFT_FILE_NOT_FOUND, "File not found: %1")
            .arg(QDir::toNativeSeparators(QFileInfo(fileName).absoluteFilePath())),
        text);
}

Error Error::badFileError(const QString& fileName, const QString& text)
{
    return Error(ErrorType::BadFile, -1,
        ZF_TR_EX(ZFT_BAD_FILE, "Bad file: %1").arg(QDir::toNativeSeparators(QFileInfo(fileName).absoluteFilePath())),
        text);
}

Error Error::badFileError(const QIODevice* device, const QString& text)
{
    Z_CHECK_NULL(device);
    if (auto f = qobject_cast<const QFile*>(device))
        return badFileError(f->fileName(), text);

    if (!device->errorString().isEmpty())
        return Error(device->errorString(), text);

    return Error("IODevice error", text);
}

Error Error::fileIOError(const QString& fileName, const QString& text)
{
    return Error(ErrorType::FileIO, -1,
        ZF_TR_EX(ZFT_FILE_IO_ERROR, "File IO error: %1")
            .arg(QDir::toNativeSeparators(QFileInfo(fileName).absoluteFilePath())),
        text);
}

Error Error::fileIOError(const QIODevice* device, const QString& text)
{
    Z_CHECK_NULL(device);
    if (auto f = qobject_cast<const QFile*>(device))
        return fileIOError(f->fileName(), text);

    if (!device->errorString().isEmpty())
        return Error(device->errorString(), text);

    return Error("IODevice error", text);
}

Error Error::fileIOError(const QIODevice& device, const QString& text)
{
    return fileIOError(&device, text);
}

Error Error::copyFileError(const QString& sourceFile, const QString& destFile, const QString& text)
{
    return Error(ErrorType::CopyFile, -1,
        ZF_TR_EX(ZFT_FILE_COPY_ERROR, "File copy error: %1")
            .arg(QDir::toNativeSeparators(sourceFile), QDir::toNativeSeparators(destFile)),
        text);
}

Error Error::removeFileError(const QString& fileName, const QString& text)
{
    return Error(ErrorType::RemoveFile, -1,
        ZF_TR_EX(ZFT_FILE_REMOVE_ERROR, "File remove error: %1")
            .arg(QDir::toNativeSeparators(QFileInfo(fileName).absoluteFilePath())),
        text);
}

Error Error::renameFileError(const QString& old_file_name, const QString& new_file_name, const QString& text)
{
    return Error(ErrorType::RenameFile, -1,
                 ZF_TR_EX(ZFT_FILE_RENAME_ERROR, "File rename error: %1 -> %2")
                     .arg(QDir::toNativeSeparators(old_file_name), QDir::toNativeSeparators(new_file_name)),
                 text);
}

Error Error::createDirError(const QString& dir_name, const QString& text)
{
    return Error(ErrorType::CreateDir, -1,
        ZF_TR_EX(ZFT_FOLDER_CREATE_ERROR, "Folder create error: %1").arg(QDir::toNativeSeparators(dir_name)), text);
}

Error Error::removeDirError(const QString& dir_name, const QString& text)
{
    return Error(ErrorType::RemoveDir, -1,
        ZF_TR_EX(ZFT_FOLDER_REMOVE_ERROR, "Folder remove error: %1").arg(QDir::toNativeSeparators(dir_name)), text);
}

Error Error::timeoutError(const QString& text)
{
    QString err_text = ZF_TR_EX(ZFT_TIMEOUT_ERROR, "Timeout error");
    return Error(ErrorType::Timeout, -1, text.isEmpty() ? err_text : err_text + ": " + text);
}

Error Error::unsupportedError(const QString& text)
{
    QString err_text = ZF_TR_EX(ZFT_UNSUPPORTED_ERROR, "Unsupported");
    return Error(ErrorType::Unsupported, -1, text.isEmpty() ? err_text : err_text + ": " + text);
}

Error Error::jsonError(const QString& key)
{
    return jsonError(QStringList({key}));
}

Error Error::jsonError(const QStringList& key_path)
{
    Z_CHECK(!key_path.isEmpty());
    return Error(
        ErrorType::Json, -1, ZF_TR_EX(ZFT_JSON_ERROR, "JSON error") + QStringLiteral(": ") + key_path.join("->"));
}

Error Error::jsonError(const QStringList& key_path, const QString& last_key)
{
    return jsonError(key_path, QStringList {last_key});
}

Error Error::jsonError(const QStringList& key_path, const QStringList& last_keys)
{
    QStringList l(key_path);
    l << last_keys;
    return jsonError(l);
}

Error Error::corruptedDataError(const QString& text)
{
    return Error(ErrorType::CorruptedData, -1, text.isEmpty() ? ZF_TR_EX(ZFT_CORRUPTED_ERROR, "File corrupted") : text);
}

Error Error::forbiddenError(const QString& text)
{
    return Error(ErrorType::Forbidden, -1, text.isEmpty() ? ZF_TR_EX(ZFT_FORBIDDEN_ERROR, "Forbidden") : text);
}

Error Error::dataNotFoundrror(const QString& text)
{
    return Error(ErrorType::DataNotFound, -1, text.isEmpty() ? ZF_TR_EX(ZFT_NOT_FOUND_ERROR, "Not found") : text);
}

Error Error::conversionError(const QString& text)
{
    return Error(ErrorType::Conversion, -1, text.isEmpty() ? ZF_TR_EX(ZFT_BAD_VALUE, "Conversion error") : text);
}

Error Error::connectionClosedError(const QString& text, int code)
{
    return Error(ErrorType::ConnectionClosed, code, text.isEmpty() ? ZF_TR_EX(ZFT_CONNECTION_CLOSED, "Connection closed error") : text);
}

Error Error::connectionClosedError(int code)
{
    return connectionClosedError("", code);
}

Error Error::changedByOtherUserError(const QString& text)
{
    return Error(ErrorType::ChangedByOtherUser, -1,
                 text.isEmpty() ? ZF_TR_EX(ZFT_CHANGED_BY_OTHER_USER, "Object changed by other user") : text);
}

QString Error::hashValue() const
{
    if (_d->hash_value.isEmpty()) {
        const_cast<Error*>(this)->_d->hash_value = Utils::generateUniqueString(QCryptographicHash::Sha224,
                fullText() + Consts::KEY_SEPARATOR + QString::number(static_cast<int>(_d->type)) + Consts::KEY_SEPARATOR
                        + QString::number(_d->code));
    }
    return _d->hash_value;
}

void Error::init()
{
    if (!_d->detail.isEmpty()) {
        if (_d->text.simplified() == _d->detail.simplified())
            _d->detail.clear();
    }
}

void Error::changed()
{
    _d->child_error_text.clear();
    _d->hash_value.clear();
    _printable.reset();
}

Error_data::Error_data()
{
    Z_DEBUG_NEW("Error_data");
}

Error_data::Error_data(const Error_data& d)
    : QSharedData(d)
{
    copyFrom(&d);
    Z_DEBUG_NEW("Error_data");
}

Error_data::~Error_data()
{
    Z_DEBUG_DELETE("Error_data");
}

void Error_data::copyFrom(const Error_data* d)
{
    type = d->type;
    code = d->code;
    text = d->text;
    detail = d->detail;
    extended = d->extended;
    data = d->data;
    is_main = d->is_main;
    is_hidden = d->is_hidden;
    is_close = d->is_close;
    is_modal = d->is_modal;
    need_reload = d->need_reload;
    child_errors = d->child_errors;
    child_error_text = d->child_error_text;
    hash_value = d->hash_value;
    info_type = d->info_type;
    is_note = d->is_note;
    is_file_debug = d->is_file_debug;
    file_debug_modifier = d->file_debug_modifier;
    uids = d->uids;

    if (d->non_critical_error != nullptr)
        non_critical_error = std::make_shared<Error>(*d->non_critical_error);
    else
        non_critical_error.reset();
}

void Error_data::clear()
{
    type = ErrorType::No;
    code = -1;
    text.clear();
    detail.clear();
    extended.clear();
    data.clear();
    is_main = false;
    is_hidden = false;
    is_close = false;
    is_modal = true;
    need_reload = false;
    child_errors.clear();
    child_error_text.clear();
    hash_value.clear();
    info_type = InformationType::Error;
    is_note = false;
    is_file_debug = false;
    file_debug_modifier.clear();
    non_critical_error.reset();
    uids.clear();
}

static const int _error_stream_version = 9;
QDataStream& operator<<(QDataStream& s, const Error& e)
{
    s << _error_stream_version;
    toStreamInt(s, e._d->type);
    s << e._d->code;
    s << e._d->text;
    s << e._d->detail;
    s << e._d->extended;
    s << e._d->data;
    s << e._d->is_main;
    s << e._d->is_hidden;
    s << e._d->is_close;
    s << e._d->is_modal;
    s << e._d->need_reload;
    s << e._d->child_error_text;
    s << e._d->hash_value;
    toStreamInt(s, e._d->info_type);
    s << e._d->is_note;
    s << e._d->is_file_debug;
    s << e._d->file_debug_modifier;

    s << (e._d->non_critical_error != nullptr);
    if (e._d->non_critical_error)
        s << *e._d->non_critical_error;

    s << e._d->child_errors.count();
    for (auto& c : e._d->child_errors) {
        s << c;
    }
    return s;
}

QDataStream& operator>>(QDataStream& s, Error& e)
{
    e.clear();

    int version;
    s >> version;
    if (version != _error_stream_version) {
        if (s.status() == QDataStream::Ok)
            s.setStatus(QDataStream::ReadCorruptData);
        return s;
    }

    fromStreamInt(s, e._d->type);
    s >> e._d->code;
    s >> e._d->text;
    s >> e._d->detail;
    s >> e._d->extended;
    s >> e._d->data;
    s >> e._d->is_main;
    s >> e._d->is_hidden;
    s >> e._d->is_close;
    s >> e._d->is_modal;
    s >> e._d->need_reload;
    s >> e._d->child_error_text;
    s >> e._d->hash_value;
    fromStreamInt(s, e._d->info_type);
    s >> e._d->is_note;
    s >> e._d->is_file_debug;
    s >> e._d->file_debug_modifier;

    bool has_non_critical_error;
    s >> has_non_critical_error;
    if (has_non_critical_error) {
        e._d->non_critical_error = std::make_shared<Error>();
        s >> *e._d->non_critical_error;
    }

    int child_count;
    s >> child_count;
    for (int i = 0; i < child_count; i++) {
        Error ch;
        s >> ch;
        if (ch.isOk()) {
            if (s.status() == QDataStream::Ok)
                s.setStatus(QDataStream::ReadCorruptData);
            e.clear();
            return s;
        }

        e._d->child_errors << ch;
    }

    if (s.status() != QDataStream::Ok)
        e.clear();

    return s;
}

} // namespace zf

QDebug operator<<(QDebug debug, const zf::Error& c)
{
    QDebugStateSaver saver(debug);
    debug.nospace();
    debug.noquote();

    debug << c.debString();

    return debug;
}

QDebug operator<<(QDebug debug, const zf::ErrorList& c)
{
    QDebugStateSaver saver(debug);
    debug.nospace();
    debug.noquote();

    using namespace zf;

    if (c.isEmpty()) {
        debug << "empty";

    } else {
        Core::beginDebugOutput();
        for (int i = 0; i < c.count(); i++) {
            debug << "error: ";
            debug << c.at(i);
        }
        Core::endDebugOutput();
    }

    return debug;
}
