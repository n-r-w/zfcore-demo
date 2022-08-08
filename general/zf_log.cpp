#include "zf_log.h"
#include "zf_core.h"
#include "zf_html_tools.h"
#include "zf_translation.h"
#include <QStandardItemModel>

namespace zf
{

//! Разделяемые данные для LogRecord
class LogRecord_data : public QSharedData
{
public:
    LogRecord_data();
    LogRecord_data(const LogRecord_data& d);
    ~LogRecord_data();

    void copyFrom(const LogRecord_data* d)
    {
        num = d->num;
        type = d->type;
        info = d->info;
        detail = d->detail;
    }

    int num;
    InformationType type;
    QString info;
    QString detail;
};

//! Разделяемые данные для Log
class Log_data : public QSharedData
{
public:
    Log_data();
    Log_data(const Log_data& d);
    ~Log_data();

    QString name;
    QString detail;
    LogRecordList records;
    QMap<InformationType, LogRecordList*> recordsByType;
};

Log::Log()
    : QObject()
    , _d(new Log_data)
{
}

Log::Log(const QString& name, const QString& detail)
    : QObject()
    , _d(new Log_data)
{
    _d->name = name.trimmed();
    _d->detail = detail.simplified();
    Z_CHECK(!_d->name.isEmpty());
}

Log::Log(const Log& log)
    : QObject(nullptr)
    , _d(log._d)
{
}

Log::~Log()
{
}

Log& Log::add(InformationType type, const QString& info, const QString& detail)
{
    Z_CHECK(isValid());

    _d->records << LogRecord(_d->records.count(), type, info, detail);

    LogRecordList* ls = _d->recordsByType.value(type);
    if (!ls) {
        ls = new LogRecordList;
        _d->recordsByType[type] = ls;
    }
    ls->append(_d->records.last());

#if !defined(RELEASE_MODE) && defined(QT_DEBUG)
    if (detail.isEmpty())
        Core::logInfo(QStringLiteral("Type: %1. %2").arg(static_cast<int>(type)).arg(info));
    else
        Core::logInfo(QStringLiteral("Type: %1. %2 (%3)").arg(static_cast<int>(type)).arg(info, detail));
#endif

    emit sg_recordAdded(_d->records.last());
    return *this;
}

Log& Log::add(const QString& error, InformationType type)
{
    return add(Error(error), type);
}

Log& Log::add(const QStringList& error, InformationType type)
{
    for (auto& e : qAsConst(error)) {
        add(e, type);
    }
    return *this;
}

Log& Log::add(const char* error, InformationType type)
{
    return add(utf(error), type);
}

Log& Log::add(const Error& error, InformationType type)
{
    Z_CHECK(error.isError());
    return add(type, error.text(), error.textDetailFull());
}

Log& Log::add(const ErrorList& error, InformationType type)
{
    for (auto& e : qAsConst(error)) {
        add(e, type);
    }
    return *this;
}

Log& Log::add(const LogRecord& r)
{
    return add(r.type(), r.info(), r.detail());
}

Log& Log::add(const LogRecordList& recs, const QString& prefix)
{
    for (const auto& r : recs) {
        if (prefix.isEmpty())
            add(r);
        else {
            LogRecord rNew(r.num(), r.type(), prefix + " " + r.info(), r.detail());
            add(rNew);
        }
    }
    return *this;
}

Log& Log::add(const Log& log, const QString& prefix)
{
    if (log.isValid()) {
        if (!isValid()) {
            _d->name = log.name();
            _d->detail = log.detail();
        }

        for (const auto& r : qAsConst(_d->records)) {
            if (prefix.isEmpty())
                add(r);
            else {
                LogRecord rNew(r.num(), r.type(), prefix + " " + r.info(), r.detail());
                add(rNew);
            }
        }
    }
    return *this;
}

Log& Log::operator<<(const LogRecord& r)
{
    add(r);
    return *this;
}

Log& Log::operator<<(const Log& log)
{
    return add(log);
}

Log& Log::operator<<(const Error& error)
{
    add(error);
    return *this;
}

Log& Log::operator=(const Log& log)
{
    if (this != &log)
        _d = log._d;
    return *this;
}

LogRecordList Log::records() const
{
    return _d->records;
}

LogRecordList Log::records(InformationType type) const
{
    LogRecordList* res = _d->recordsByType.value(type);
    return res ? *res : LogRecordList();
}

bool Log::isValid() const
{
    return !_d->name.isEmpty();
}

bool Log::isEmpty() const
{
    return _d->records.isEmpty();
}

bool Log::contains(InformationType type) const
{
    return count(type) > 0;
}

int Log::typesCount() const
{
    int i = 0;
    for (LogRecordList* r : _d->recordsByType) {
        if (!r->isEmpty())
            i++;
    }
    return i;
}

int Log::count() const
{
    return _d->records.count();
}

int Log::count(InformationType type) const
{
    LogRecordList* ls = _d->recordsByType.value(type);
    return (ls) ? ls->count() : 0;
}

LogRecord Log::record(int i) const
{
    Z_CHECK(i >= 0 && i < _d->records.count());
    return _d->records.at(i);
}

LogRecord Log::record(int i, InformationType type) const
{
    LogRecordList* ls = _d->recordsByType.value(type);
    Z_CHECK_NULL(ls);
    Z_CHECK(i >= 0 && i < ls->count());

    return ls->at(i);
}

void Log::clear()
{
    _d->records.clear();
    qDeleteAll(_d->recordsByType);
    _d->recordsByType.clear();
    emit sg_cleared();
}

QString Log::name() const
{
    return _d->name;
}

QString Log::detail() const
{
    return _d->detail;
}

QString Log::toString(InformationType type) const
{
    QString s;
    for (const auto& r : _d->records) {
        if (type != InformationType::Invalid && r.type() != type)
            continue;

        if (!s.isEmpty())
            s += QStringLiteral("; ");

        s += r.info();
        if (!r.detail().isEmpty() && r.detail() != r.info())
            s += QStringLiteral(" (") + r.detail() + QStringLiteral(")");
    }
    return s.simplified();
}

Error Log::saveCSV(QIODevice* device, InformationType type) const
{
    // Если всего одна категория, то колонка "категория" не нужна
    bool saveCategory = (type == InformationType::Invalid && typesCount() > 1);

    QStandardItemModel* im = new QStandardItemModel;
    QMap<int, QString> headerMap;
    if (saveCategory) {
        im->setColumnCount(3);
        headerMap[0] = ZF_TR(ZFT_CATEGORY);
        headerMap[1] = ZF_TR(ZFT_MESSAGE);
        headerMap[2] = ZF_TR(ZFT_DETAIL);
    } else {
        im->setColumnCount(2);
        headerMap[0] = ZF_TR(ZFT_MESSAGE);
        headerMap[1] = ZF_TR(ZFT_DETAIL);
    }

    for (const auto& r : _d->records) {
        if (type != InformationType::Invalid && r.type() != type)
            continue;

        int row = im->rowCount();
        im->insertRow(row);

        if (saveCategory)
            im->setItem(row, 0, new QStandardItem(r.typeText()));

        im->setItem(row, saveCategory ? 1 : 0, new QStandardItem(r.info().simplified()));
        im->setItem(row, saveCategory ? 2 : 1, new QStandardItem(r.detail().simplified()));
    }

    Error error = Utils::itemModelToExcel(im, device, nullptr, headerMap);
    delete im;
    return error;
}

LogRecord::LogRecord()
    : _d(new LogRecord_data)
{
}

LogRecord::LogRecord(const LogRecord& record)
    : _d(record._d)
{
}

LogRecord::LogRecord(int num, InformationType type, const QString& info, const QString& detail)
    : _d(new LogRecord_data)
{
    _d->num = num;
    _d->type = type;

    _d->info = info.trimmed();
    HtmlTools::plainIfHtml(_d->info, false);

    _d->detail = detail.trimmed();
    HtmlTools::plainIfHtml(_d->detail, false);

    Z_CHECK(_d->num >= 0 && _d->type != InformationType::Invalid && !_d->info.isEmpty());
}

LogRecord::~LogRecord()
{
}

LogRecord& LogRecord::operator=(const LogRecord& r)
{
    if (this != &r)
        _d = r._d;
    return *this;
}

bool LogRecord::isValid() const
{
    return _d->type != InformationType::Invalid;
}

int LogRecord::num() const
{
    return _d->num;
}

InformationType LogRecord::type() const
{
    return _d->type;
}

QString LogRecord::typeText() const
{
    return Utils::informationTypeText(_d->type);
}

QString LogRecord::info() const
{
    return _d->info;
}

QString LogRecord::detail() const
{
    return _d->detail;
}

Log_data::Log_data()
{
}

Log_data::Log_data(const Log_data& d)
    : QSharedData(d)
    , name(d.name)
    , detail(d.detail)
    , records(d.records)
{
    for (auto& r : qAsConst(records)) {
        LogRecordList* rList = recordsByType.value(r.type(), nullptr);
        if (!rList) {
            rList = new LogRecordList;
            recordsByType[r.type()] = rList;
        }
        rList->append(r);
    }
}

Log_data::~Log_data()
{
    qDeleteAll(recordsByType);
}

LogRecord_data::LogRecord_data()
    : num(-1)
    , type(InformationType::Invalid)
{
}

LogRecord_data::LogRecord_data(const LogRecord_data& d)
    : QSharedData(d)    
{
    copyFrom(&d);
}

LogRecord_data::~LogRecord_data()
{
}

} // namespace zf
