#include "zf_http_parser.h"
#include <QLoggingCategory>
#include <QSslSocket>
#include <array>
#include "http-parser/http_parser.h"
#include "zf_core.h"

#if defined(qCDebug)
#undef qCDebug
#endif

#if (false)
Q_LOGGING_CATEGORY(lc, "zf.http.HttpParcer.request")
#if !defined(QT_NO_DEBUG_OUTPUT)
#define qCDebug(category, ...)                                                                                                             \
    for (bool qt_category_enabled = category().isDebugEnabled(); qt_category_enabled; qt_category_enabled = false)                         \
    QMessageLogger(QT_MESSAGELOG_FILE, QT_MESSAGELOG_LINE, QT_MESSAGELOG_FUNC, category().categoryName()).debug(__VA_ARGS__)
#else
#define qCDebug(category, ...) QT_NO_QDEBUG_MACRO()
#endif
#else
#define qCDebug(category, ...) QT_NO_QDEBUG_MACRO()
#endif

namespace zf
{
namespace http
{
static const std::array<void (*)(const QString&, QUrl*), UF_MAX> parseUrlFunctions {
    [](const QString& string, QUrl* url) { url->setScheme(string); },
    [](const QString& string, QUrl* url) { url->setHost(string); },
    [](const QString& string, QUrl* url) { url->setPort(string.toInt()); },
    [](const QString& string, QUrl* url) { url->setPath(string, QUrl::TolerantMode); },
    [](const QString& string, QUrl* url) { url->setQuery(string); },
    [](const QString& string, QUrl* url) { url->setFragment(string); },
    [](const QString& string, QUrl* url) { url->setUserInfo(string); },
};

struct HttpParcerInternal
{
    static http_parser_settings httpParserSettings;
};

http_parser_settings HttpParcerInternal::httpParserSettings {
    &HttpParser::onMessageBegin,
    &HttpParser::onUrl,
    &HttpParser::onStatus,
    &HttpParser::onHeaderField,
    &HttpParser::onHeaderValue,
    &HttpParser::onHeadersComplete,
    &HttpParser::onBody,
    &HttpParser::onMessageComplete,
    &HttpParser::onChunkHeader,
    &HttpParser::onChunkComplete,
};

HttpParser::HttpParser(Type type)
    : _type(type)
{
    init();
    Z_DEBUG_NEW("HttpParser");
}

HttpParser::~HttpParser()
{
    Z_DEBUG_DELETE("HttpParser");
}

HttpParser::Type HttpParser::type() const
{
    return _type;
}

QString HttpParser::header(const QString& key) const
{
    return _headers_lower.value(key.toLower());
}

Method HttpParser::method() const
{
    return _method;
}

StatusCode HttpParser::statusCode() const
{
    return _status;
}

quint16 HttpParser::major() const
{
    return _http_major;
}

quint16 HttpParser::minor() const
{
    return _http_minor;
}

void HttpParser::clear()
{
    _url.clear();    
    _headers.clear();
    _headers_lower.clear();
    _body.clear();
    _state = State::NotStarted;
    _method = Method::Get;

    init();
}

void HttpParser::init()
{
    _http_parser = Z_MAKE_SHARED(http_parser);
    _http_parser->data = this;
    http_parser_init(_http_parser.get(), static_cast<http_parser_type>(_type));
}

QString HttpParser::parse(QAbstractSocket* socket)
{
    const auto fragment = socket->readAll();
    auto ssl_socket = qobject_cast<QSslSocket*>(socket);
    return parse(fragment, ssl_socket != nullptr && ssl_socket->isEncrypted());
}

QString HttpParser::parse(const QByteArray& data, bool is_ssl)
{
    if (data.size()) {
        _url.setScheme(is_ssl ? QStringLiteral("https") : QStringLiteral("http"));

        const auto parsed
            = http_parser_execute(_http_parser.get(), &HttpParcerInternal::httpParserSettings, data.constData(), size_t(data.size()));
        if (int(parsed) < data.size()) {
            QString error = http_errno_name(static_cast<http_errno>(_http_parser->http_errno));
            return error.isEmpty() ? QString::number(_http_parser->http_errno) : error;
        }
    }
    return QString();
}

bool HttpParser::isCompleted() const
{
    return _state == State::OnMessageComplete;
}

const QByteArray& HttpParser::body() const
{
    return _body;
}

QUrl HttpParser::url() const
{
    return _url;
}

const QList<QPair<QString, QString>>& HttpParser::headers() const
{
    return _headers;
}

HttpParser* HttpParser::instance(http_parser* httpParser)
{
    return static_cast<HttpParser*>(httpParser->data);
}

bool HttpParser::parseUrl(const char* at, size_t length, bool connect, QUrl* url)
{
    struct http_parser_url u;
    if (http_parser_parse_url(at, length, connect ? 1 : 0, &u) != 0)
        return false;

    for (auto i = 0U; i < UF_MAX; i++) {
        if (u.field_set & (1 << i)) {
            parseUrlFunctions.at(i)(QString::fromUtf8(at + u.field_data[i].off, u.field_data[i].len), url);
        }
    }
    return true;
}

int HttpParser::onMessageBegin(http_parser* httpParser)
{
    qCDebug(lc) << static_cast<void*>(httpParser);

    auto i = instance(httpParser);
    i->_state = State::OnMessageBegin;
    i->_method = static_cast<Method>(httpParser->method);
    return 0;
}

int HttpParser::onUrl(http_parser* httpParser, const char* at, size_t length)
{
    qCDebug(lc) << httpParser << QString::fromUtf8(at, int(length));
    auto instance = static_cast<HttpParser*>(httpParser->data);
    instance->_state = State::OnUrl;
    parseUrl(at, length, false, &instance->_url);
    return 0;
}

int HttpParser::onStatus(http_parser* httpParser, const char* at, size_t length)
{
    qCDebug(lc) << httpParser << QString::fromUtf8(at, int(length));
    instance(httpParser)->_state = State::OnStatus;
    return 0;
}

int HttpParser::onHeaderField(http_parser* httpParser, const char* at, size_t length)
{
    qCDebug(lc) << httpParser << QString::fromUtf8(at, int(length));
    auto i = instance(httpParser);
    i->_state = State::OnHeaders;
    const auto key = QString::fromUtf8(at, int(length));
    i->_headers << qMakePair(key, QString());
    i->_headers_lower[key.toLower()] = QString();

    return 0;
}

int HttpParser::onHeaderValue(http_parser* httpParser, const char* at, size_t length)
{
    qCDebug(lc) << httpParser << QString::fromUtf8(at, int(length));
    auto i = instance(httpParser);
    i->_state = State::OnHeaders;
    Z_CHECK(!i->_headers.isEmpty() && i->_headers.last().second.isEmpty());
    const auto value = QString::fromUtf8(at, int(length));
    i->_headers.last().second = value;
    i->_headers_lower[i->_headers.last().first.toLower()] = value;
    if (i->_headers.last().first.compare(QByteArrayLiteral("host"), Qt::CaseInsensitive) == 0)
        parseUrl(at, length, true, &i->_url);    
    return 0;
}

int HttpParser::onHeadersComplete(http_parser* httpParser)
{
    qCDebug(lc) << httpParser;
    auto i = instance(httpParser);
    i->_state = State::OnHeadersComplete;
    i->_http_major = httpParser->http_major;
    i->_http_minor = httpParser->http_minor;
    i->_status = static_cast<StatusCode>(httpParser->status_code);
    i->_type = static_cast<Type>(httpParser->type);
    return 0;
}

int HttpParser::onBody(http_parser* httpParser, const char* at, size_t length)
{
    qCDebug(lc) << httpParser << QString::fromUtf8(at, int(length));
    auto i = instance(httpParser);
    i->_state = State::OnBody;
    if (i->_body.isEmpty()) {
        i->_body.reserve(static_cast<int>(httpParser->content_length) + static_cast<int>(length));
    }

    i->_body.append(at, int(length));
    return 0;
}

int HttpParser::onMessageComplete(http_parser* httpParser)
{
    qCDebug(lc) << httpParser;
    instance(httpParser)->_state = State::OnMessageComplete;
    return 0;
}

int HttpParser::onChunkHeader(http_parser* httpParser)
{
    qCDebug(lc) << httpParser;
    instance(httpParser)->_state = State::OnChunkHeader;
    return 0;
}

int HttpParser::onChunkComplete(http_parser* httpParser)
{
    qCDebug(lc) << httpParser;
    instance(httpParser)->_state = State::OnChunkComplete;
    return 0;
}

} // namespace http
} // namespace zf
