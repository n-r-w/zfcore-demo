#pragma once

#include "zf_global.h"

#include <QList>
#include <QUrl>
#include <QString>
#include <QScopedPointer>
#include "zf_version.h"
#include "zf_database_credentials.h"

namespace zf
{
namespace http
{
//! Метод (номера должны соответстовать https://github.com/nodejs/http-parser/blob/master/http_parser.h HTTP_METHOD_MAP)
enum class Method
{
    Delete = 0,
    Get = 1,
    Head = 2,
    Post = 3,
    Put = 4,
    Connect = 5,
    Options = 6,
    Trace = 7,
    Copy = 8,
    Lock = 9,
    Mkcol = 10,
    Move = 11,
    Propfind = 12,
    Proppatch = 13,
    Search = 14,
    Unlock = 15,
    Bind = 16,
    Rebind = 17,
    Unbind = 18,
    Acl = 19,
    Report = 20,
    Mkactivity = 21,
    Checkout = 22,
    Merge = 23,
    Msearch = 24,
    Notify = 25,
    Subscribe = 26,
    Unsubscribe = 27,
    Patch = 28,
    Purge = 29,
    Mkcalendar = 30,
    Link = 31,
    Unlink = 32,
    Source = 33,
};

enum class StatusCode
{
    // 1xx: Informational
    Continue = 100,
    SwitchingProtocols,
    Processing,

    // 2xx: Success
    Ok = 200,
    Created,
    Accepted,
    NonAuthoritativeInformation,
    NoContent,
    ResetContent,
    PartialContent,
    MultiStatus,
    AlreadyReported,
    IMUsed = 226,

    // 3xx: Redirection
    MultipleChoices = 300,
    MovedPermanently,
    Found,
    SeeOther,
    NotModified,
    UseProxy,
    // 306: not used, was proposed as "Switch Proxy" but never standardized
    TemporaryRedirect = 307,
    PermanentRedirect,

    // 4xx: Client Error
    BadRequest = 400,
    Unauthorized,
    PaymentRequired,
    Forbidden,
    NotFound,
    MethodNotAllowed,
    NotAcceptable,
    ProxyAuthenticationRequired,
    RequestTimeout,
    Conflict,
    Gone,
    LengthRequired,
    PreconditionFailed,
    PayloadTooLarge,
    UriTooLong,
    UnsupportedMediaType,
    RequestRangeNotSatisfiable,
    ExpectationFailed,
    ImATeapot,
    MisdirectedRequest = 421,
    UnprocessableEntity,
    Locked,
    FailedDependency,
    UpgradeRequired = 426,
    PreconditionRequired = 428,
    TooManyRequests,
    RequestHeaderFieldsTooLarge = 431,
    UnavailableForLegalReasons = 451,

    // 5xx: Server Error
    InternalServerError = 500,
    NotImplemented,
    BadGateway,
    ServiceUnavailable,
    GatewayTimeout,
    HttpVersionNotSupported,
    VariantAlsoNegotiates,
    InsufficientStorage,
    LoopDetected,
    NotExtended = 510,
    NetworkAuthenticationRequired,
    NetworkConnectTimeoutError = 599,
};

//! Виды заголовков
enum class HeaderType
{
    // Request and response headers
    CacheControl,
    Connection,
    ContentLength,
    ContentMD5,
    ContentType,
    Date,
    Pragma,
    TransferEncoding,
    Via,
    Warning,

    // Request headers
    Accept,
    AcceptCharset,
    AcceptEncoding,
    AcceptLanguage,
    Authorization,
    Cookie,
    Expect,
    From,
    Host,
    IfMatch,
    IfModifiedSince,
    IfNoneMatch,
    IfRange,
    IfUnmodifiedSince,
    MaxForwards,
    ProxyAuthorization,
    Range,
    Referer,
    TE,
    Upgrade,
    UserAgent,

    // Response headers
    AcceptRanges,
    Age,
    Allow,
    ContentEncoding,
    ContentLanguage,
    ContentLocation,
    ContentDisposition,
    ContentRange,
    ContentSecurityPolicy,
    ETag,
    Expires,
    LastModified,
    Link,
    Location,
    P3P,
    ProxyAuthenticate,
    Refresh,
    RetryAfter,
    Server,
    SetCookie,
    Trailer,
    Vary,
    WWWAuthenticate
};

//! Виды контента
enum class ContentType
{
    Unknown,
    All,
    ApplicationAtomXml,
    ApplicationFormUrlencoded,
    ApplicationJson,
    ApplicationOctetStream,
    ApplicationSvgXml,
    ApplicationXhtmlXml,
    ApplicationXml,
    MultipartFormData,
    ApplicationZip,
    ApplicationPdf,
    ApplicationMicrosoftWordDoc,
    ApplicationMicrosoftWordDocx,
    ApplicationMicrosoftExcelXls,
    ApplicationMicrosoftExcelXlsx,
    ImagePng,
    ImageJpeg,
    ImageGif,
    TextHtml,
    TextPlain,
    TextXml,

    MicrosoftWord,
    MicrosoftWord_OpenXML,
    MicrosoftExcel,
    MicrosoftExcel_OpenXML,
};

class HttpParser;
class HttpPrivate;
class HttpHeaderPrivate;
class HttpResponseHeaderPrivate;
class HttpRequestHeaderPrivate;

/*! Заголовок HTTP
 * Реализация частично выдрана из Qt4, т.к. в Qt5 подобные классы были удалены */
class ZCORESHARED_EXPORT HttpHeader
{
public:
    virtual ~HttpHeader();

    virtual void clear() = 0;

    HttpHeader& operator=(const HttpHeader& h);

    //! Уникальный номер запроса для внутреннего использования
    QString uniqueId() const;

    void setValue(const QString &key, const QString &value);
    void setValue(HeaderType key, const QString& value);

    void setValues(const QList<QPair<QString, QString>>& values);

    void addValue(const QString& key, const QString& value);
    void addValue(HeaderType key, const QString& value);

    QList<QPair<QString, QString>> values() const;

    bool hasKey(const QString& key) const;
    bool hasKey(HeaderType key) const;

    QStringList keys() const;

    QString value(const QString& key) const;
    QString value(HeaderType key) const;

    QStringList allValues(const QString& key) const;
    QStringList allValues(HeaderType key) const;

    void removeValue(const QString& key);
    void removeValue(HeaderType key);

    void removeAllValues(const QString& key);
    void removeAllValues(HeaderType key);

    bool hasContentLength() const;
    qint64 contentLength() const;
    void setContentLength(int len);

    bool hasContentType() const;
    ContentType contentType() const;
    QString contentTypeString() const;
    void setContentType(const QString& type);
    void setContentType(const ContentType& type);

    QByteArray content() const;
    void setContent(const QByteArray& content,
                    //! Это полное тело запроса
                    bool final_size = true);
    void setContent(const QString& content,
        //! Это полное тело запроса
        bool final_size = true);
    void setContent(const char* content,
        //! Это полное тело запроса
        bool final_size = true);
    void appendContent(const QByteArray& content);
    void updateContentLength();
    qint64 realContentLength() const;
    bool isContentCompleted() const;

    virtual QByteArray toByteArray(bool header_only = false) const;

    bool isValid() const;

    virtual int majorVersion() const = 0;
    virtual int minorVersion() const = 0;

    static QString statusToString(StatusCode status);
    static StatusCode statusFromString(const QString& status);

    static QString methodToString(Method method);
    static Method methodFromString(const QString& method);

    static QString headerTypeToString(HeaderType header);

    static QString contentTypeToString(ContentType content);
    static ContentType contentTypeFromString(const QString& method);

protected:
    HttpHeader();
    HttpHeader(const HttpHeader& header);

    void setValid(bool);
    void fromParser(const HttpParser* parser);

    HttpHeader(HttpHeaderPrivate& dd);
    HttpHeader(HttpHeaderPrivate& dd, const HttpHeader& header);
    QScopedPointer<HttpHeaderPrivate> d_ptr;

private:
    Q_DECLARE_PRIVATE(HttpHeader)
};

/*! Ответ HTTP
 * Реализация частично выдрана из Qt4, т.к. в Qt5 подобные классы были удалены */
class ZCORESHARED_EXPORT HttpResponseHeader : public HttpHeader
{
public:
    HttpResponseHeader();
    HttpResponseHeader(const HttpResponseHeader& header);
    explicit HttpResponseHeader(StatusCode code, int majorVer = 1, int minorVer = 1);
    //! Парсинг из заранее полученных данных
    explicit HttpResponseHeader(const QByteArray& data);
    //! Результат берется прямо из парсера
    explicit HttpResponseHeader(const HttpParser& parser);
    ~HttpResponseHeader();

    HttpResponseHeader& operator=(const HttpResponseHeader& header);

    void clear() override;

    void setStatusLine(StatusCode code, int majorVer = 1, int minorVer = 1);

    StatusCode statusCode() const;
    bool isError() const;
    bool isClientError() const;
    bool isServerError() const;

    QString reasonPhrase() const;

    int majorVersion() const override;
    int minorVersion() const override;

    QByteArray toByteArray(bool header_only = false) const override;

private:
    void fromParser(const HttpParser* parser);

    Q_DECLARE_PRIVATE(HttpResponseHeader)
    friend class HttpPrivate;
};

/*! Запрос HTTP
 * Реализация частично выдрана из Qt4, т.к. в Qt5 подобные классы были удалены */
class ZCORESHARED_EXPORT HttpRequestHeader : public HttpHeader
{
public:
    HttpRequestHeader();
    HttpRequestHeader(const HttpRequestHeader& header);
    explicit HttpRequestHeader(Method method, const QUrl& path = QUrl(), int majorVer = 1, int minorVer = 1);
    //! Парсинг из заранее полученных данных
    explicit HttpRequestHeader(const QByteArray& data);
    //! Результат берется прямо из парсера
    explicit HttpRequestHeader(const HttpParser& parser);
    ~HttpRequestHeader();

    HttpRequestHeader& operator=(const HttpRequestHeader& header);

    void clear() override;

    void setRequest(Method method, const QUrl& path, int majorVer = 1, int minorVer = 1);

    Method method() const;
    QString methodString() const;

    QUrl path() const;

    //! Установить заголовок Accept с параметром version
    void setProtocolVersion(const Version& version);
    //! Ищет заголовок Accept и извлекает из него значение параметра version. Параметры должны быть разделены ';'
    Version protocolVersion() const;

    //! Установить заголовок Authorization: basic login password или basic hash
    void setBasicAuthorization(const QString& login, const QString& password);
    void setBasicAuthorization(const QString& hash);
    void setBasicAuthorization(const zf::Credentials& c);
    QString hash() const;
    QString login() const;
    QString password() const;
    zf::Credentials cridentials() const;

    int majorVersion() const override;
    int minorVersion() const override;

    QByteArray toByteArray(bool header_only = false) const override;

private:
    void fromParser(const HttpParser* parser);

    QByteArray uri(bool throughProxy) const;

    Q_DECLARE_PRIVATE(HttpRequestHeader)
};

} // namespace http
} // namespace zf

Q_DECLARE_METATYPE(zf::http::HttpResponseHeader)
Q_DECLARE_METATYPE(zf::http::HttpRequestHeader)
