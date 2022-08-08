#include <QSet>
#include <map>

#include "http-parser/http_parser.h"
#include "zf_http_headers.h"
#include "zf_core.h"
#include "zf_http_parser.h"

namespace zf::http
{
class HttpHeaderPrivate
{
    Q_DECLARE_PUBLIC(HttpHeader)

public:
    HttpHeaderPrivate();
    virtual ~HttpHeaderPrivate();

    QList<QPair<QString, QString> > values;
    bool valid;
    HttpHeader* q_ptr;
    QByteArray content;
    //! Уникальный номер запроса для внутреннего использования
    QString id;
};

HttpHeaderPrivate::HttpHeaderPrivate()
{
    id = Utils::generateUniqueString();
}

HttpHeaderPrivate::~HttpHeaderPrivate()
{    
}

class HttpResponseHeaderPrivate : public HttpHeaderPrivate
{
    Q_DECLARE_PUBLIC(HttpResponseHeader)

public:
    StatusCode statCode = StatusCode::Ok;
    QString reasonPhr;
    quint16 majVer = 0;
    quint16 minVer = 0;
};

class HttpRequestHeaderPrivate : public HttpHeaderPrivate
{
    Q_DECLARE_PUBLIC(HttpRequestHeader)

public:
    Method method = Method::Get;
    QUrl path;
    quint16 majVer = 0;
    quint16 minVer = 0;
};

HttpHeader::HttpHeader()
    : d_ptr(new HttpHeaderPrivate)
{
    Q_D(HttpHeader);
    d->q_ptr = this;
    d->valid = true;
}

HttpHeader::HttpHeader(const HttpHeader& header)
    : d_ptr(new HttpHeaderPrivate)
{
    Q_D(HttpHeader);
    d->q_ptr = this;
    d->valid = header.d_func()->valid;
    d->values = header.d_func()->values;
    d->content = header.d_func()->content;
}

HttpHeader::HttpHeader(HttpHeaderPrivate& dd, const HttpHeader& header)
    : d_ptr(&dd)
{
    Q_D(HttpHeader);
    d->q_ptr = this;
    d->valid = header.d_func()->valid;
    d->values = header.d_func()->values;
    d->content = header.d_func()->content;
}

HttpHeader::~HttpHeader()
{
}

HttpHeader& HttpHeader::operator=(const HttpHeader& h)
{
    Q_D(HttpHeader);
    d->values = h.d_func()->values;
    d->valid = h.d_func()->valid;
    d->content = h.d_func()->content;
    return *this;
}

QString HttpHeader::uniqueId() const
{
    Q_D(const HttpHeader);
    return d->id;
}

bool HttpHeader::isValid() const
{
    Q_D(const HttpHeader);
    return d->valid;
}

void HttpHeader::setValid(bool v)
{
    Q_D(HttpHeader);
    d->valid = v;
}

void HttpHeader::fromParser(const HttpParser* parser)
{
    setValues(parser->headers());

    if (!parser->body().isEmpty()) {
        setContentLength(parser->body().size());
        setContent(parser->body());
    }
}

HttpHeader::HttpHeader(HttpHeaderPrivate& dd)
    : d_ptr(&dd)
{
    Q_D(HttpHeader);
    d->q_ptr = this;
    d->valid = true;
}

QString HttpHeader::value(const QString& key) const
{
    Q_D(const HttpHeader);
    QString lowercaseKey = key.toLower();
    QList<QPair<QString, QString> >::ConstIterator it = d->values.constBegin();
    while (it != d->values.constEnd()) {
        if ((*it).first.toLower() == lowercaseKey)
            return (*it).second;
        ++it;
    }
    return QString();
}

QString HttpHeader::value(HeaderType key) const
{
    return value(headerTypeToString(key));
}

QStringList HttpHeader::allValues(const QString& key) const
{
    Q_D(const HttpHeader);
    QString lowercaseKey = key.toLower();
    QStringList valueList;
    QList<QPair<QString, QString> >::ConstIterator it = d->values.constBegin();
    while (it != d->values.constEnd()) {
        if ((*it).first.toLower() == lowercaseKey)
            valueList.append((*it).second);
        ++it;
    }
    return valueList;
}

QStringList HttpHeader::allValues(HeaderType key) const
{
    return allValues(headerTypeToString(key));
}

QStringList HttpHeader::keys() const
{
    Q_D(const HttpHeader);
    QStringList keyList;
    QSet<QString> seenKeys;
    QList<QPair<QString, QString> >::ConstIterator it = d->values.constBegin();
    while (it != d->values.constEnd()) {
        const QString &key = (*it).first;
        QString lowercaseKey = key.toLower();
        if (!seenKeys.contains(lowercaseKey)) {
            keyList.append(key);
            seenKeys.insert(lowercaseKey);
        }
        ++it;
    }
    return keyList;
}

bool HttpHeader::hasKey(const QString& key) const
{
    Q_D(const HttpHeader);
    QString lowercaseKey = key.toLower();
    QList<QPair<QString, QString> >::ConstIterator it = d->values.constBegin();
    while (it != d->values.constEnd()) {
        if ((*it).first.toLower() == lowercaseKey)
            return true;
        ++it;
    }
    return false;
}

bool HttpHeader::hasKey(HeaderType key) const
{
    return hasKey(headerTypeToString(key));
}

void HttpHeader::setValue(const QString& key, const QString& value)
{
    Q_D(HttpHeader);
    QString lowercaseKey = key.toLower();
    QList<QPair<QString, QString> >::Iterator it = d->values.begin();
    while (it != d->values.end()) {
        if ((*it).first.toLower() == lowercaseKey) {
            (*it).second = value;
            return;
        }
        ++it;
    }
    // not found so add
    addValue(key, value);
}

void HttpHeader::setValue(HeaderType key, const QString& value)
{
    setValue(headerTypeToString(key), value);
}

void HttpHeader::setValues(const QList<QPair<QString, QString>>& values)
{
    Q_D(HttpHeader);
    d->values = values;
}

void HttpHeader::addValue(const QString& key, const QString& value)
{
    Q_D(HttpHeader);
    d->values.append(qMakePair(key, value));
}

void HttpHeader::addValue(HeaderType key, const QString& value)
{
    addValue(headerTypeToString(key), value);
}

QList<QPair<QString, QString>> HttpHeader::values() const
{
    Q_D(const HttpHeader);
    return d->values;
}

void HttpHeader::removeValue(const QString& key)
{
    Q_D(HttpHeader);
    QString lowercaseKey = key.toLower();
    QList<QPair<QString, QString> >::Iterator it = d->values.begin();
    while (it != d->values.end()) {
        if ((*it).first.toLower() == lowercaseKey) {
            d->values.erase(it);
            return;
        }
        ++it;
    }
}

void HttpHeader::removeValue(HeaderType key)
{
    removeValue(headerTypeToString(key));
}

void HttpHeader::removeAllValues(const QString& key)
{
    Q_D(HttpHeader);
    QString lowercaseKey = key.toLower();
    QList<QPair<QString, QString> >::Iterator it = d->values.begin();
    while (it != d->values.end()) {
        if ((*it).first.toLower() == lowercaseKey) {
            it = d->values.erase(it);
            continue;
        }
        ++it;
    }
}

void HttpHeader::removeAllValues(HeaderType key)
{
    removeAllValues(headerTypeToString(key));
}

QByteArray HttpHeader::toByteArray(bool header_only) const
{
    Q_D(const HttpHeader);
    if (!isValid() || !isContentCompleted())
        return QByteArray("");

    QByteArray ret = QByteArray("");

    QList<QPair<QString, QString> >::ConstIterator it = d->values.constBegin();
    while (it != d->values.constEnd()) {
        ret += (*it).first + QLatin1String(": ") + (*it).second + QLatin1String("\r\n");
        ++it;
    }

    if (!header_only && hasContentLength() && hasContentType()) {
        ret += QStringLiteral("\r\n");
        ret += d->content;
    }

    return ret;
}

bool HttpHeader::hasContentLength() const
{
    return hasKey(headerTypeToString(HeaderType::ContentLength));
}

qint64 HttpHeader::contentLength() const
{
    if (!hasContentLength())
        return 0;

    return value(headerTypeToString(HeaderType::ContentLength)).toLongLong();
}

void HttpHeader::setContentLength(int len)
{
    setValue(headerTypeToString(HeaderType::ContentLength), QString::number(len));
}

bool HttpHeader::hasContentType() const
{
    return hasKey(headerTypeToString(HeaderType::ContentType));
}

QString HttpHeader::contentTypeString() const
{
    QString type = value(headerTypeToString(HeaderType::ContentType));
    if (type.isEmpty())
        return QString();

    int pos = type.indexOf(QLatin1Char(';'));
    if (pos == -1)
        return type;

    return type.left(pos).trimmed();
}

ContentType HttpHeader::contentType() const
{
    return contentTypeFromString(contentTypeString());
}

void HttpHeader::setContentType(const QString& type)
{
    setValue(headerTypeToString(HeaderType::ContentType), type);
}

void HttpHeader::setContentType(const ContentType& type)
{
    setContentType(contentTypeToString(type));
}

QByteArray HttpHeader::content() const
{
    Q_D(const HttpHeader);
    return d->content;
}

void HttpHeader::setContent(const QByteArray& content, bool final_size)
{
    Q_D(HttpHeader);
    d->content = content;

    if (final_size)
        updateContentLength();

    setValid(isContentCompleted());
}

void HttpHeader::setContent(const QString& content, bool final_size)
{
    setContent(content.toUtf8(), final_size);
}

void HttpHeader::setContent(const char* content, bool final_size)
{
    setContent(QString::fromUtf8(content), final_size);
}

void HttpHeader::appendContent(const QByteArray& body)
{
    Q_D(HttpHeader);
    d->content.append(body);
    setValid(isContentCompleted());
}

qint64 HttpHeader::realContentLength() const
{
    Q_D(const HttpHeader);
    return d->content.size();
}

void HttpHeader::updateContentLength()
{
    setContentLength(realContentLength());
}

bool HttpHeader::isContentCompleted() const
{
    if (!hasContentLength())
        return true;

    return realContentLength() == contentLength();
}

HttpResponseHeader::HttpResponseHeader()
    : HttpHeader(*new HttpResponseHeaderPrivate)
{
    setValid(false);
    Z_DEBUG_NEW("HttpResponseHeader");
}

HttpResponseHeader::HttpResponseHeader(const HttpResponseHeader& header)
    : HttpHeader(*new HttpResponseHeaderPrivate, header)
{
    Q_D(HttpResponseHeader);
    d->statCode = header.d_func()->statCode;
    d->reasonPhr = header.d_func()->reasonPhr;
    d->majVer = header.d_func()->majVer;
    d->minVer = header.d_func()->minVer;

    Z_DEBUG_NEW("HttpResponseHeader");
}

HttpResponseHeader& HttpResponseHeader::operator=(const HttpResponseHeader& header)
{
    Q_D(HttpResponseHeader);
    HttpHeader::operator=(header);
    d->statCode = header.d_func()->statCode;
    d->reasonPhr = header.d_func()->reasonPhr;
    d->majVer = header.d_func()->majVer;
    d->minVer = header.d_func()->minVer;
    return *this;
}

void HttpResponseHeader::clear()
{
    *this = HttpResponseHeader();
}

HttpResponseHeader::HttpResponseHeader(const QByteArray& data)
    : HttpHeader(*new HttpResponseHeaderPrivate)
{
    setValid(false);

    HttpParser parser(HttpParser::Type::HTTP_RESPONSE);
    QString error = parser.parse(data, false);
    if (!error.isEmpty()) {
        Core::logError("HttpResponseHeader - parsing error");
        return;
    }

    if (!parser.isCompleted()) {
        Core::logError("HttpResponseHeader - data not completed");
        return;
    }

    fromParser(&parser);

    Z_DEBUG_NEW("HttpResponseHeader");
}

HttpResponseHeader::HttpResponseHeader(const HttpParser& parser)
    : HttpHeader(*new HttpResponseHeaderPrivate)
{
    fromParser(&parser);

    Z_DEBUG_NEW("HttpResponseHeader");
}

HttpResponseHeader::~HttpResponseHeader()
{
    Z_DEBUG_DELETE("HttpResponseHeader");
}

HttpResponseHeader::HttpResponseHeader(StatusCode code, int majorVer, int minorVer)
    : HttpHeader(*new HttpResponseHeaderPrivate)
{
    setStatusLine(code, majorVer, minorVer);

    Z_DEBUG_NEW("HttpResponseHeader");
}

void HttpResponseHeader::setStatusLine(StatusCode code, int majorVer, int minorVer)
{
    Q_D(HttpResponseHeader);
    setValid(true);
    d->statCode = code;
    d->reasonPhr = statusToString(d->statCode);
    d->majVer = majorVer;
    d->minVer = minorVer;
}

StatusCode HttpResponseHeader::statusCode() const
{
    Q_D(const HttpResponseHeader);
    return d->statCode;
}

bool HttpResponseHeader::isError() const
{
    return isClientError() || isServerError();
}

bool HttpResponseHeader::isClientError() const
{
    return QString::number(static_cast<int>(statusCode())).left(1) == QStringLiteral("4");
}

bool HttpResponseHeader::isServerError() const
{
    return QString::number(static_cast<int>(statusCode())).left(1) == QStringLiteral("5");
}

QString HttpResponseHeader::reasonPhrase() const
{
    Q_D(const HttpResponseHeader);
    return d->reasonPhr;
}

int HttpResponseHeader::majorVersion() const
{
    Q_D(const HttpResponseHeader);
    return d->majVer;
}

int HttpResponseHeader::minorVersion() const
{
    Q_D(const HttpResponseHeader);
    return d->minVer;
}

QByteArray HttpResponseHeader::toByteArray(bool header_only) const
{
    Q_D(const HttpResponseHeader);
    QByteArray ret;

    ret += "HTTP/";
    ret += QString::number(d->majVer).toUtf8();
    ret += ".";
    ret += QString::number(d->minVer).toUtf8();
    ret += " " + QString::number(static_cast<int>(d->statCode)) + " ";
    ret += d->reasonPhr.toUtf8();

    ret += "\r\n" + HttpHeader::toByteArray(header_only) + "\r\n";

    return ret;
}

void HttpResponseHeader::fromParser(const HttpParser* parser)
{
    Q_D(HttpResponseHeader);

    setValid(false);
    if (!parser->isCompleted() || parser->type() != HttpParser::Type::HTTP_RESPONSE)
        return;

    HttpHeader::fromParser(parser);

    d->majVer = parser->major();
    d->minVer = parser->minor();
    d->statCode = parser->statusCode();
    d->reasonPhr = statusToString(d->statCode);

    setValid(true);
}

HttpRequestHeader::HttpRequestHeader()
    : HttpHeader(*new HttpRequestHeaderPrivate)
{
    setValid(false);
    Z_DEBUG_NEW("HttpRequestHeader");
}

HttpRequestHeader::HttpRequestHeader(Method method, const QUrl& path, int majorVer, int minorVer)
    : HttpHeader(*new HttpRequestHeaderPrivate)
{
    Q_D(HttpRequestHeader);
    d->method = method;
    d->path = path;
    d->majVer = majorVer;
    d->minVer = minorVer;

    Z_DEBUG_NEW("HttpRequestHeader");
}

HttpRequestHeader::HttpRequestHeader(const HttpRequestHeader& header)
    : HttpHeader(*new HttpRequestHeaderPrivate, header)
{
    Q_D(HttpRequestHeader);
    d->method = header.d_func()->method;
    d->path = header.d_func()->path;
    d->majVer = header.d_func()->majVer;
    d->minVer = header.d_func()->minVer;

    Z_DEBUG_NEW("HttpRequestHeader");
}

HttpRequestHeader& HttpRequestHeader::operator=(const HttpRequestHeader& header)
{
    Q_D(HttpRequestHeader);
    HttpHeader::operator=(header);
    d->method = header.d_func()->method;
    d->path = header.d_func()->path;
    d->majVer = header.d_func()->majVer;
    d->minVer = header.d_func()->minVer;
    return *this;
}

void HttpRequestHeader::clear()
{
    *this = HttpRequestHeader();
}

HttpRequestHeader::HttpRequestHeader(const QByteArray& data)
    : HttpHeader(*new HttpRequestHeaderPrivate)
{
    setValid(false);

    HttpParser parser(HttpParser::Type::HTTP_REQUEST);
    QString error = parser.parse(data, false);
    if (!error.isEmpty()) {
        Core::logError("HttpRequestHeader - parsing error");
        return;
    }

    if (!parser.isCompleted()) {
        Core::logError("HttpRequestHeader - data not completed");
        return;
    }

    fromParser(&parser);

    Z_DEBUG_NEW("HttpRequestHeader");
}

HttpRequestHeader::HttpRequestHeader(const HttpParser& parser)
    : HttpHeader(*new HttpRequestHeaderPrivate)
{
    fromParser(&parser);

    Z_DEBUG_NEW("HttpRequestHeader");
}

HttpRequestHeader::~HttpRequestHeader()
{
    Z_DEBUG_DELETE("HttpRequestHeader");
}

void HttpRequestHeader::setRequest(Method method, const QUrl& path, int majorVer, int minorVer)
{
    Q_D(HttpRequestHeader);
    setValid(true);
    d->method = method;
    d->path = path;
    d->majVer = majorVer;
    d->minVer = minorVer;
}

Method HttpRequestHeader::method() const
{
    Q_D(const HttpRequestHeader);
    return d->method;
}

QString HttpRequestHeader::methodString() const
{
    return methodToString(method());
}

QUrl HttpRequestHeader::path() const
{
    Q_D(const HttpRequestHeader);
    return d->path;
}

void HttpRequestHeader::setProtocolVersion(const Version& version)
{
    QString accept = value(HeaderType::Accept);
    if (!accept.isEmpty())
        accept += QStringLiteral("; ");

    accept += QStringLiteral("version=") + version.text();

    setValue(HeaderType::Accept, accept);
}

Version HttpRequestHeader::protocolVersion() const
{
    QString v = value(HeaderType::Accept).simplified();
    if (v.isEmpty())
        return Version();

    auto split = v.splitRef(';', QString::SkipEmptyParts);
    for (auto& s : split) {
        if (!s.trimmed().startsWith(QStringLiteral("version")))
            continue;

        auto split_v = s.split('=', QString::SkipEmptyParts);
        if (split_v.isEmpty() || split_v.count() > 2)
            continue;

        auto split_f = split_v.at(1).split('.', QString::SkipEmptyParts);
        if (split_f.isEmpty())
            return Version();

        int major = 0;
        int minor = 0;
        int build = 0;
        bool ok;

        if (split_f.count() >= 1) {
            major = split_f.at(0).toInt(&ok);
            if (!ok)
                return Version();
        }
        if (split_f.count() >= 2) {
            minor = split_f.at(1).toInt(&ok);
            if (!ok)
                return Version();
        }
        if (split_f.count() >= 3) {
            build = split_f.at(2).toInt(&ok);
            if (!ok)
                return Version();
        }

        return Version(major, minor, build);
    }

    return Version();
}

void HttpRequestHeader::setBasicAuthorization(const QString& login, const QString& password)
{
    if (login.isEmpty() && password.isEmpty()) {
        removeValue(HeaderType::Authorization);
        return;
    }
    setBasicAuthorization(Credentials(login, password));
}

void HttpRequestHeader::setBasicAuthorization(const QString& hash)
{
    setBasicAuthorization(Credentials(hash));
}

void HttpRequestHeader::setBasicAuthorization(const Credentials& c)
{
    if (!c.isValid()) {
        removeValue(HeaderType::Authorization);
        return;
    }

    if (c.isHashBased())
        setValue(HeaderType::Authorization, QStringLiteral("basic %1").arg(c.hash()));
    else
        setValue(HeaderType::Authorization, QStringLiteral("basic %1 %2").arg(c.login(), c.password()));
}

QString HttpRequestHeader::hash() const
{
    return cridentials().hash();
}

QString HttpRequestHeader::login() const
{
    return cridentials().login();
}

QString HttpRequestHeader::password() const
{
    return cridentials().password();
}

int HttpRequestHeader::majorVersion() const
{
    Q_D(const HttpRequestHeader);
    return d->majVer;
}

int HttpRequestHeader::minorVersion() const
{
    Q_D(const HttpRequestHeader);
    return d->minVer;
}

QByteArray HttpRequestHeader::toByteArray(bool header_only) const
{
    Q_D(const HttpRequestHeader);

    QByteArray ret;
    ret += methodToString(d->method) + " ";
    ret += d->path.isEmpty() ? "*" : uri(false);
    ret += " HTTP/";
    ret += QString::number(d->majVer) + ".";
    ret += QString::number(d->minVer) + "\r\n";
    ret += HttpHeader::toByteArray(header_only) + "\r\n";

    return ret;
}

Credentials HttpRequestHeader::cridentials() const
{
    QStringList authorization
        = value(zf::http::HeaderType::Authorization).simplified().split(' ', QString::SkipEmptyParts);

    if ((authorization.count() != 2 && authorization.count() != 3)
        || authorization.at(0).toLower() != QStringLiteral("basic"))
        return zf::Credentials();

    if (authorization.count() == 3)
        return Credentials(authorization.at(1), authorization.at(2));
    else
        return Credentials(authorization.at(1));
}

void HttpRequestHeader::fromParser(const HttpParser* parser)
{
    Q_D(HttpRequestHeader);

    setValid(false);
    if (!parser->isCompleted() || parser->type() != HttpParser::Type::HTTP_REQUEST)
        return;

    HttpHeader::fromParser(parser);

    d->majVer = parser->major();
    d->minVer = parser->minor();
    d->method = parser->method();
    d->path = parser->url();

    setValid(true);
}

QByteArray HttpRequestHeader::uri(bool throughProxy) const
{
    // взято из QHttpNetworkRequest::uri

    Q_D(const HttpRequestHeader);

    QUrl::FormattingOptions format(QUrl::RemoveFragment | QUrl::RemoveUserInfo | QUrl::FullyEncoded);

    // for POST, query data is sent as content
    if (method() == Method::Post)
        format |= QUrl::RemoveQuery;
    // for requests through proxy, the Request-URI contains full url
    if (!throughProxy)
        format |= QUrl::RemoveScheme | QUrl::RemoveAuthority;
    QUrl copy = d->path;
    if (copy.path().isEmpty())
        copy.setPath(QStringLiteral("/"));
    else
        format |= QUrl::NormalizePathSegments;
    QByteArray uri = copy.toEncoded(format);
    return uri;
}

static const std::map<StatusCode, QByteArray> _status_string_by_code {
#define XX(num, name, string) {static_cast<StatusCode>(num), QByteArrayLiteral(#string)},
    HTTP_STATUS_MAP(XX)
#undef XX
};

static const std::map<QByteArray, StatusCode> _status_string_by_raw {
#define XX(num, name, string) {QByteArrayLiteral(#string), static_cast<StatusCode>(num)},
    HTTP_STATUS_MAP(XX)
#undef XX
};

static const std::map<Method, QByteArray> _method_string_by_code {
#define XX(num, name, string) {static_cast<Method>(num), QByteArrayLiteral(#string)},
    HTTP_METHOD_MAP(XX)
#undef XX
};

static const std::map<QByteArray, Method> _method_string_by_raw {
#define XX(num, name, string) {QByteArrayLiteral(#string), static_cast<Method>(num)},
    HTTP_METHOD_MAP(XX)
#undef XX
};

QString HttpHeader::statusToString(StatusCode status)
{
    auto i = _status_string_by_code.find(status);
    if (i != _status_string_by_code.end())
        return QString::fromUtf8(i->second);

    zf::Core::logError(QString("statusToString: wrong status code %1").arg(static_cast<int>(status)));
    return "unknown";
}

StatusCode HttpHeader::statusFromString(const QString& status)
{
    auto i = _status_string_by_raw.find(status.toUtf8());
    if (i != _status_string_by_raw.end())
        return i->second;

    zf::Core::logError(QString("statusFromString: wrong status %1").arg(status));
    return StatusCode::Ok;
}

QString HttpHeader::methodToString(Method method)
{
    auto i = _method_string_by_code.find(method);
    if (i != _method_string_by_code.end())
        return QString::fromUtf8(i->second);

    zf::Core::logError(QString("methodToString: wrong method code %1").arg(static_cast<int>(method)));
    return "unknown";
}

Method HttpHeader::methodFromString(const QString& method)
{
    auto i = _method_string_by_raw.find(method.toUtf8());
    if (i != _method_string_by_raw.end())
        return i->second;

    zf::Core::logError(QString("methodFromString: wrong method %1").arg(method));
    return Method::Get;
}

static QMap<HeaderType, QString> _headerNameMap = {
    {HeaderType::CacheControl, "Cache-Control"},
    {HeaderType::Connection, "Connection"},
    {HeaderType::ContentLength, "Content-Length"},
    {HeaderType::ContentMD5, "Content-MD5"},
    {HeaderType::ContentType, "Content-Type"},
    {HeaderType::Date, "Date"},
    {HeaderType::Pragma, "Pragma"},
    {HeaderType::TransferEncoding, "Transfer-Encoding"},
    {HeaderType::Via, "Via"},
    {HeaderType::Warning, "Warning"},

    {HeaderType::Accept, "Accept"},
    {HeaderType::AcceptCharset, "Accept-Charset"},
    {HeaderType::AcceptEncoding, "Accept-Encoding"},
    {HeaderType::AcceptLanguage, "Accept-Language"},
    {HeaderType::Authorization, "Authorization"},
    {HeaderType::Cookie, "Cookie"},
    {HeaderType::Expect, "Expect"},
    {HeaderType::From, "From"},
    {HeaderType::Host, "Host"},
    {HeaderType::IfMatch, "If-Match"},
    {HeaderType::IfModifiedSince, "If-Modified-Since"},
    {HeaderType::IfNoneMatch, "If-None-Match"},
    {HeaderType::IfRange, "If-Range"},
    {HeaderType::IfUnmodifiedSince, "If-Unmodified-Since"},
    {HeaderType::MaxForwards, "Max-Forwards"},
    {HeaderType::ProxyAuthorization, "Proxy-Authorization"},
    {HeaderType::Range, "Range"},
    {HeaderType::Referer, "Referer"},
    {HeaderType::TE, "TE"},
    {HeaderType::Upgrade, "Upgrade"},
    {HeaderType::UserAgent, "User-Agent"},

    {HeaderType::AcceptRanges, "Accept-Ranges"},
    {HeaderType::Age, "Age"},
    {HeaderType::Allow, "Allow"},
    {HeaderType::ContentEncoding, "Content-Encoding"},
    {HeaderType::ContentLanguage, "Content-Language"},
    {HeaderType::ContentLocation, "Content-Location"},
    {HeaderType::ContentDisposition, "Content-Disposition"},
    {HeaderType::ContentRange, "Content-Range"},
    {HeaderType::ContentSecurityPolicy, "Content-Security-Policy"},
    {HeaderType::ETag, "ETag"},
    {HeaderType::Expires, "Expires"},
    {HeaderType::LastModified, "Last-Modified"},
    {HeaderType::Link, "Link"},
    {HeaderType::Location, "Location"},
    {HeaderType::P3P, "P3P"},
    {HeaderType::ProxyAuthenticate, "Proxy-Authenticate"},
    {HeaderType::Refresh, "Refresh"},
    {HeaderType::RetryAfter, "Retry-After"},
    {HeaderType::Server, "Server"},
    {HeaderType::SetCookie, "Set-Cookie"},
    {HeaderType::Trailer, "Trailer"},
    {HeaderType::Vary, "Vary"},
    {HeaderType::WWWAuthenticate, "WWW-Authenticate"},
};

static QMap<ContentType, QString> _contentTypeNameMap = {
    {ContentType::All, "*/*"},
    {ContentType::ApplicationAtomXml, "application/atom+xml"},
    {ContentType::ApplicationFormUrlencoded, "application/x-www-form-urlencoded"},
    {ContentType::ApplicationJson, "application/json"},
    {ContentType::ApplicationOctetStream, "application/octet-stream"},
    {ContentType::ApplicationSvgXml, "application/svg+xml"},
    {ContentType::ApplicationXhtmlXml, "application/xhtml+xml"},
    {ContentType::ApplicationXml, "application/xml"},
    {ContentType::MultipartFormData, "multipart/form-data"},
    {ContentType::ApplicationZip, "application/zip"},
    {ContentType::ApplicationPdf, "application/pdf"},
    {ContentType::ApplicationMicrosoftWordDoc, "application/msword"},
    {ContentType::ApplicationMicrosoftWordDocx, "application/vnd.openxmlformats-officedocument.wordprocessingml.document"},
    {ContentType::ApplicationMicrosoftExcelXls, "application/vnd.ms-excel"},
    {ContentType::ApplicationMicrosoftExcelXlsx, "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet"},
    {ContentType::ImagePng, "image/png"},
    {ContentType::ImageJpeg, "image/jpeg"},
    {ContentType::ImageGif, "image/gif"},
    {ContentType::TextHtml, "text/html"},
    {ContentType::TextPlain, "text/plain"},
    {ContentType::TextXml, "text/xml"},

};

QString HttpHeader::headerTypeToString(HeaderType header)
{
    return _headerNameMap.value(header);
}

QString HttpHeader::contentTypeToString(ContentType content)
{
    QString result = _contentTypeNameMap.value(content);
    if (result.isEmpty()) {
        Core::logError(QString("contentTypeToString - bad content %1").arg(static_cast<int>(content)));
        return _contentTypeNameMap.value(ContentType::All);
    }
    return result;
}

ContentType HttpHeader::contentTypeFromString(const QString& method)
{
    return _contentTypeNameMap.key(method.toLower(), ContentType::Unknown);
}

} // namespace zf::http
