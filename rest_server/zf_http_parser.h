#pragma once

#include "zf_global.h"
#include "zf_http_headers.h"
#include <QUrl>
#include <QAbstractSocket>

struct http_parser;

namespace zf
{
namespace http
{
/*! Парсер http (обертка вокруг https://github.com/nodejs/http-parser)
 * Часть кода выдрана из https://github.com/qt-labs/qthttpserver */
class ZCORESHARED_EXPORT HttpParser
{
public:
    //! Должен соответствовать https://github.com/nodejs/http-parser - http_parser_type
    enum class Type
    {
        HTTP_REQUEST,
        HTTP_RESPONSE,
        HTTP_BOTH,
    };

    HttpParser(Type type = Type::HTTP_REQUEST);
    ~HttpParser();

    //! До парсинга соответствует параметру конструктора, после парсинга может измениться в зависимости от содержимого заголовка
    Type type() const;

    /*! Парсинг из сокета. В случае ошибки возвращает текст ошибки, иначе пусто. Парсеру нужно кормить последовательные
     * входящие фрагменты, до тех пор пока isCompleted не будет true */
    QString parse(QAbstractSocket* socket);
    /*! Парсинг из готового пакета данных. В случае ошибки возвращает текст ошибки, иначе пусто. Парсеру нужно кормить последовательные
     * входящие фрагменты, до тех пор пока isCompleted не будет true */
    QString parse(const QByteArray& data, bool is_ssl);

    //! Сообщение загружено полностью
    bool isCompleted() const;

    const QByteArray& body() const;
    QUrl url() const;

    const QList<QPair<QString, QString>>& headers() const;
    QString header(const QString& key) const;

    Method method() const;
    StatusCode statusCode() const;

    quint16 major() const;
    quint16 minor() const;

    void clear();

private:    
    void init();

    static HttpParser* instance(http_parser* httpParser);
    static bool parseUrl(const char* at, size_t length, bool connect, QUrl* url);
    static void debug();

    static int onMessageBegin(http_parser* httpParser);
    static int onUrl(http_parser* httpParser, const char* at, size_t length);
    static int onStatus(http_parser* httpParser, const char* at, size_t length);
    static int onHeaderField(http_parser* httpParser, const char* at, size_t length);
    static int onHeaderValue(http_parser* httpParser, const char* at, size_t length);
    static int onHeadersComplete(http_parser* httpParser);
    static int onBody(http_parser* httpParser, const char* at, size_t length);
    static int onMessageComplete(http_parser* httpParser);
    static int onChunkHeader(http_parser* httpParser);
    static int onChunkComplete(http_parser* httpParser);

    enum class State
    {
        NotStarted,
        OnMessageBegin,
        OnUrl,
        OnStatus,
        OnHeaders,
        OnHeadersComplete,
        OnBody,
        OnMessageComplete,
        OnChunkHeader,
        OnChunkComplete
    };
    State _state = State::NotStarted;

    QByteArray _body;
    QUrl _url;

    QList<QPair<QString, QString>> _headers;
    QMap<QString, QString> _headers_lower;

    std::shared_ptr<http_parser> _http_parser;
    Type _type;

    StatusCode _status = StatusCode::Ok;
    Method _method = Method::Get;
    quint16 _http_major = 0;
    quint16 _http_minor = 0;

    friend struct HttpParcerInternal;
};

} // namespace http
} // namespace zf
