#include "zf_html_tools.h"
#include <QSet>
#include <zf_core.h>
#include <zf_utils.h>

namespace zf
{
Z_RECURSIVE_MUTEX HtmlTools::_mutex;
std::shared_ptr<QString> HtmlTools::_parsedCorrect;
std::shared_ptr<QString> HtmlTools::_parsedPlain;
std::shared_ptr<HtmlNodes> HtmlTools::_parsed;

struct HtmlNode
{
    enum Type
    {
        Tag, //! Корректный html тэг
        BadTag, //! Некорректный html тэг
        Escaped, //! Необходимо экранировать при выводе в html
        Data //! обычные данные
    };

    HtmlNode(Type type, const QString& text);
    //! Тип
    Type type;
    //! Содержимое
    QString text;
};

class HtmlNodes
{
    friend class HtmlTools;

public:
    HtmlNodes()
        : _isCorrect(true)
        , _hasHtmlNodes(false)
    {
    }
    ~HtmlNodes() {}

    //! Преобразовать в строку исключая все корректные html тэги
    QString toPlain() const;

    const QList<std::shared_ptr<HtmlNode>>& nodes() const { return _nodes; }

    //! Содержит хотя бы один некорректный тэг, то FALSE
    bool isCorrect() const { return _isCorrect; }
    //! Содержит хотя бы один корректный HTML тег
    bool isHasHtmlNodes() const { return _hasHtmlNodes; }

private:
    QList<std::shared_ptr<HtmlNode>> _nodes;
    bool _isCorrect;
    bool _hasHtmlNodes;
};

HtmlTools::HtmlTools()
{
}

// Известные html теги
const QSet<QString> HtmlTools::_supportedHtmlTags
        = {"!doctype", "style", "ag", "a", "address", "b", "big", "blockquote", "body", "br", "center", "cite", "code",
                "dd", "dfn", "div", "dl", "dt", "em", "font", "h1", "h2", "h3", "h4", "h5", "h6", "head", "hr", "html",
                "i", "img", "kbd", "meta", "li", "nobr", "ol", "p", "pre", "qt", "s", "samp", "small", "span", "strong",
                "sub", "sup", "table", "tbody", "td", "tfoot", "th", "thead", "title", "tr", "tt", "u", "ul", "var"};

// Известные теги, которые надо экранировать
const QSet<QString> HtmlTools::_escapedTags = {"zuid"};

// Заменяем '<' и '>' на ковычки, чтобы не было проблем при установке текста в html
#define Z_HTML_REPLACE_OPEN QStringLiteral("\"")
#define Z_HTML_REPLACE_CLOSE QStringLiteral("\"")

QString HtmlTools::plain(const QString& s, bool keepNewLine, bool remove_quotes)
{
    QMutexLocker lock(&_mutex);

    QString sTmp = s;
    sTmp.replace(QStringLiteral("<b>'"), QStringLiteral("<b>"), Qt::CaseInsensitive); //-V567
    sTmp.replace(QStringLiteral("'</b>"), QStringLiteral("</b>"), Qt::CaseInsensitive); //-V567
    sTmp.replace(QStringLiteral("<b>"), QStringLiteral("<b>'"), Qt::CaseInsensitive); //-V567
    sTmp.replace(QStringLiteral("</b>"), QStringLiteral("'</b>"), Qt::CaseInsensitive); //-V567
    parse(sTmp);
    return plain(keepNewLine, remove_quotes);
}

bool HtmlTools::plainIfHtml(QString& s, bool keepNewLine, bool remove_quotes)
{
    QMutexLocker lock(&_mutex);

    if (isHtml(s)) {
        s = HtmlTools::plain(keepNewLine, remove_quotes);
        return true;
    }

    return false;
}

QString HtmlTools::plain(bool keepNewLine, bool remove_quotes)
{        
    /* Корректные html теги исключаются
     * Некорректные теги остаются без изменений */

    Z_CHECK_NULL(_parsed);
    if (!_parsedPlain) {
        _parsedPlain = Z_MAKE_SHARED(QString);
        for (const std::shared_ptr<HtmlNode>& n : _parsed->nodes()) {
            if (n->type == HtmlNode::Tag) {
                if (n->text == QStringLiteral("<br>") || n->text == QStringLiteral("<hr>")) {
                    if (keepNewLine)
                        *_parsedPlain += '\n';
                    else
                        *_parsedPlain += ',';
                }

            } else if (n->type == HtmlNode::Escaped) {
                *_parsedPlain += n->text;

            } else if (n->type == HtmlNode::BadTag) {
                *_parsedPlain += n->text;

            } else
                *_parsedPlain += n->text;
        }

        if (remove_quotes)
            _parsedPlain->replace("'", "");

    }
    return *_parsedPlain;
}

bool HtmlTools::isHtml(const QString& s)
{
    QMutexLocker lock(&_mutex);

    parse(s);
    return isHtml();
}

bool HtmlTools::isHtml()
{        
    Z_CHECK_NULL(_parsed);
    return _parsed->isCorrect() && _parsed->isHasHtmlNodes();
}

QString HtmlTools::correct(const QString& s)
{
    QMutexLocker lock(&_mutex);

    parse(s);
    return correct();
}

bool HtmlTools::correctIfHtml(QString& s)
{
    QMutexLocker lock(&_mutex);

    if (isHtml(s)) {
        s = correct(s);
        return true;
    }

    return false;
}

QString HtmlTools::correct()
{        
    /* Корректные html теги остаются
     * Некорректные теги исключаются */

    Z_CHECK_NULL(_parsed);
    _parsedCorrect = Z_MAKE_SHARED(QString);
    for (const std::shared_ptr<HtmlNode>& n : _parsed->nodes()) {
        QString s;

        if (n->type == HtmlNode::Escaped) {
            s = n->text.toHtmlEscaped();

        } else if (n->type == HtmlNode::BadTag) {
            s = Z_HTML_REPLACE_OPEN + n->text.mid(1, n->text.length() - 2) + Z_HTML_REPLACE_CLOSE;

        } else
            s = n->text;

        *_parsedCorrect += s;
    }

    _parsedCorrect->replace(QStringLiteral("\r\n"), "<br>");
    _parsedCorrect->replace(QStringLiteral("\r"), "<br>");
    _parsedCorrect->replace(QStringLiteral("\n"), "<br>");

    return *_parsedCorrect;
}

QString HtmlTools::color(const QString& s, const QColor& color)
{
    return QStringLiteral(R"(<font color="%2">%1</font>)").arg(s, color.name());
}

QString HtmlTools::bold(const QString& s)
{
    return QStringLiteral("<b>%1</b>").arg(s);
}

QString HtmlTools::italic(const QString& s)
{
    return QStringLiteral("<i>%1</i>").arg(s);
}

QString HtmlTools::font(const QString& s, int size)
{
    return QStringLiteral(R"(<span style="font-size:%2pt;">%1</span>)").arg(s).arg(size);
}

QString HtmlTools::table(const QStringList& rows, bool bold)
{
    if (rows.isEmpty())
        return QString();

    if (rows.count() == 1)
        return rows.at(0);

    QString res;
    for (QString row : qAsConst(rows)) {
        if (!res.isEmpty())
            res += QStringLiteral("<hr>");

        if (row.startsWith(QStringLiteral("<?xml version"), Qt::CaseInsensitive)) {
            row = row.toHtmlEscaped();
        } else {
            if (row.contains(QStringLiteral("<table"), Qt::CaseInsensitive) || row.contains(QStringLiteral("<tr>"), Qt::CaseInsensitive)
                || row.contains(QStringLiteral("<td>"), Qt::CaseInsensitive))
                HtmlTools::plain(row);
        }
        res += QStringLiteral("<tr><td>%1</td></tr>").arg(bold ? HtmlTools::bold(row) : row);
    }

    return QStringLiteral(R"(<table width="100%" cellspacing="0" cellpadding="0">%1</table>)").arg(res);
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

QString HtmlTools::table(const Error& errors, bool bold)
{
    Z_CHECK(errors.isError());

    if (errors.childErrors().isEmpty())
        return _combine(errors.text(), errors.textDetail());

    QStringList rows({_combine(errors.text(), errors.textDetail())});
    for (const Error& err : errors.childErrors()) {
        QString row = _combine(err.text(), err.textDetail());
        if (bold)
            row = HtmlTools::bold(row);

        if (rows.contains(row))
            continue;

        rows << row;
    }

    if (rows.count() == 0)
        return rows.at(0);

    return table(rows);
}

QString HtmlTools::table(const QList<Error>& errors, bool bold)
{
    return table(Error(errors), bold);
}

QString HtmlTools::iconTable(const QStringList& rows, const QStringList& icon_paths, const QSize& icon_size)
{
    Z_CHECK(rows.count() == icon_paths.count());

    if (rows.isEmpty())
        return QString();

    QString res;
    for (int i = 0; i < rows.count(); i++) {
        QString row = rows.at(i);
        QString icon = icon_paths.at(i);

        if (row.startsWith(QStringLiteral("<?xml version"), Qt::CaseInsensitive)) {
            row = row.toHtmlEscaped();
        } else {
            if (row.contains(QStringLiteral("<table"), Qt::CaseInsensitive) || row.contains(QStringLiteral("<tr>"), Qt::CaseInsensitive)
                || row.contains(QStringLiteral("<td>"), Qt::CaseInsensitive))
                HtmlTools::plain(row);
        }
        res += QStringLiteral("<tr><td>%1</td></tr>").arg(textIcon(row, icon, icon_size));
    }

    static QString table_begin = R"(<table cellspacing="0" cellpadding="-3">)";
    static QString table_end = R"(</table>)";

    return table_begin + res + table_end;
}

QString HtmlTools::iconTable(const QStringList& rows, const QStringList& icon_paths, int icon_size)
{
    return iconTable(rows, icon_paths, {icon_size, icon_size});
}

QString HtmlTools::textIcon(const QString& text, const QString& icon_path, const QSize& icon_size)
{
    qreal dpr = 1.0;
    bool use_png = true; // тут важно использовать PNG, т.к. он масштабируется внутри html с антиалиасингом в отличие от SVG
    if (QCoreApplication::testAttribute(Qt::AA_UseHighDpiPixmaps)) {
        // иначе будет алиасинг. интеллектуалы из Qt сами это сделать внутри отрисовки html забыли
        // сначала увеличиваем размер, а потом уменьшаем принудительным заданием width/height в img
        dpr = qApp->devicePixelRatio();
    }

    QString file_name = icon_size.isEmpty() ? icon_path : Utils::resizeSvgFileName(icon_path, icon_size * dpr, use_png);
    QString icon;

    if (icon_size.isEmpty() || fuzzyCompare(dpr, 1))
        icon = QStringLiteral(R"(<img src="%1">)").arg(file_name);
    else
        icon = QStringLiteral(R"(<img src="%1" width="%2" height="%3">)").arg(file_name).arg(icon_size.width()).arg(icon_size.height());

    if (text.isEmpty())
        return icon;

    return QStringLiteral(
               R"(<table cellspacing="7" cellpadding="0"><tr><td style="vertical-align: middle">%2</td><td style="vertical-align: middle">%1</td></tr></table>)")
        .arg(text, icon);
}

QString HtmlTools::textIcon(const QString& text, const QString& icon_path, int icon_size)
{
    return textIcon(text, icon_path, {icon_size, icon_size});
}

std::shared_ptr<HtmlNodes> HtmlTools::parse(const QString& html)
{
    QMutexLocker lock(&_mutex);

    _parsedCorrect.reset();
    _parsedPlain.reset();
    _parsed = Z_MAKE_SHARED(HtmlNodes);

    QString text;
    bool tagFound = false;

    if ( // Это результат QTextEdit::toHtml для пустого содержимого
            html != QStringLiteral("p, li { white-space: pre-wrap; }")) {
        for (int i = 0; i < html.length(); i++) {
            auto& c = html.at(i);
            if (c == '<') {
                if (tagFound) {
                    // Подряд две открывающие - это не html
                    text = QStringLiteral("<") + text + c;
                    _parsed->_isCorrect = false;
                    tagFound = false;

                } else {
                    // Начало тега. Все что было ранее рассматриваем как текст
                    if (!text.isEmpty()) {
                        auto node = Z_MAKE_SHARED(HtmlNode, HtmlNode::Data, text);
                        _parsed->_nodes << node;

                        text.clear();
                    }

                    tagFound = true;
                }

            } else if (c == '>') {
                if (!tagFound) {
                    // закрывающая без открывающего - это не html
                    text += c;
                    _parsed->_isCorrect = false;

                } else {
                    // Обнаружен какой-то тег
                    QString tag = text;
                    if (!tag.isEmpty()) {
                        if (tag.at(0) == '/')
                            tag = tag.right(tag.length() - 1);

                        int spacePos = tag.indexOf(' ', 0);
                        if (spacePos >= 0)
                            tag = tag.left(spacePos);
                    }
                    QString tag_prepared = tag.toLower().trimmed();

                    if (!tag.isEmpty() && _supportedHtmlTags.contains(tag_prepared)) {
                        // Корректный html тег
                        auto node = Z_MAKE_SHARED(HtmlNode, HtmlNode::Tag, QStringLiteral("<") + text + QStringLiteral(">"));
                        _parsed->_nodes << node;
                        _parsed->_hasHtmlNodes = true;

                    } else if (!tag.isEmpty() && _escapedTags.contains(tag_prepared)) {
                        // Надо экранировать содержимое
                        auto node = Z_MAKE_SHARED(HtmlNode, HtmlNode::Escaped, QStringLiteral("<") + text + QStringLiteral(">"));
                        _parsed->_nodes << node;

                    } else {
                        // Некорректный html тег
                        auto node = Z_MAKE_SHARED(HtmlNode, HtmlNode::BadTag, QStringLiteral("<") + text + QStringLiteral(">"));
                        _parsed->_nodes << node;
                        _parsed->_isCorrect = false;
                    }

                    text.clear();
                    tagFound = false;
                }
            } else
                text += c;
        }
    }

    if (tagFound)
        // Была открывающая скобка, но закрывающей не было
        _parsed->_isCorrect = false;

    if (!text.isEmpty()) {
        auto node = Z_MAKE_SHARED(HtmlNode, HtmlNode::Data, text);
        _parsed->_nodes << node;
    }

    return _parsed;
}

QString HtmlTools::underline(const QString& s)
{
    return QStringLiteral(R"(<span style="text-decoration: underline;">%1</span>)").arg(s);
}

QString HtmlTools::align(const QString& s, Qt::Alignment align)
{
    QString a;
    if (align == Qt::AlignLeft)
        a = "left";
    else if (align == Qt::AlignRight)
        a = "right";
    else if (align == Qt::AlignCenter)
        a = "center";
    else
        Z_HALT(QString::number(static_cast<int>(align)));

    return QStringLiteral(R"(<p align="%1">%2</p>)").arg(a, s);
}

HtmlNode::HtmlNode(HtmlNode::Type type, const QString& text)
    : type(type)
    , text(text)
{
    if (this->type == Data)
        Z_CHECK(!this->text.isEmpty());
}

} // namespace zf
