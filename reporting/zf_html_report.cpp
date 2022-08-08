#include "zf_html_report.h"

#include <QFile>

// Поле: начало тэга
#define HTML_FIELD_BEGIN QStringLiteral("{{")
// Поле: конец тэга
#define HTML_FIELD_END QStringLiteral("}}")

// Ячейка: начало тэга
#define HTML_CELL_BEGIN QStringLiteral("{<")
// Ячейка: конец тэга
#define HTML_CELL_END QStringLiteral(">}")

// Начало блока: начало тэга
#define HTML_BLOCK_START_BEGIN QStringLiteral("{[")
// Начало блока: конец тэга
#define HTML_BLOCK_START_END QStringLiteral("]}")

// Конец блока: начало тэга
#define HTML_BLOCK_FINISH_BEGIN QStringLiteral("{#")
// Конец блока: конец тэга
#define HTML_BLOCK_FINISH_END QStringLiteral("#}")

namespace zf
{
HtmlReportGenerator::HtmlReportGenerator()
{
}

Error HtmlReportGenerator::unpackTemplate(const QByteArray& template_file, const QString& temp_folder, QStringList& to_parse)
{
    Q_UNUSED(template_file)
    Q_UNUSED(temp_folder)
    Q_UNUSED(to_parse);
    return Error();
}

Error HtmlReportGenerator::packTarget(const QString& temp_folder, const QString& target_file)
{
    Q_UNUSED(temp_folder)
    Q_UNUSED(target_file)
    return Error();
}

Error HtmlReportGenerator::openDocument(const QString& template_file, XMLDocumentPtr& document)
{
    document = XMLDocument::readHtmlFile(template_file, XML_ParserOption::HTML_PARSE_DEFAULT | XML_ParserOption::XML_PARSE_HUGE);
    if (document == nullptr)
        return Error::badFileError(template_file);

    return Error();
}

Error HtmlReportGenerator::saveDocument(const QString& target_file, const XMLDocumentPtr& document)
{
    if (document->saveToFile(target_file, XML_SaveOption::XML_SAVE_AS_HTML | XML_SaveOption::XML_SAVE_WSNONSIG) < 0)
        return Error::fileIOError(target_file);
    return Error();
}

Error HtmlReportGenerator::extractText(
    const XMLNodePtr& node, QString& text, QString& text_before, ReportGenerator::TagType& type, bool& is_scan_next)
{
    static const QList<QPair<QString, QString>> marks = {
        {HTML_FIELD_BEGIN, HTML_FIELD_END},
        {HTML_CELL_BEGIN, HTML_CELL_END},
        {HTML_BLOCK_START_BEGIN, HTML_BLOCK_START_END},
        {HTML_BLOCK_FINISH_BEGIN, HTML_BLOCK_FINISH_END},
    };
    static const QList<ReportGenerator::TagType> types = {
        TagType::Field,
        TagType::Cell,
        TagType::BlockStart,
        TagType::BlockFinish,
    };

    QString text_after;
    SplitTextResult result;
    Error error = splitText(node->content(), false, marks, types, text_before, text, text_after, type, result);
    if (error.isError())
        return error;

    if (result == SplitTextResult::FullTag)
        is_scan_next = false;
    else if (result == SplitTextResult::TagEnd)
        return createError(QString("found tag end mark without start mark <%1>").arg(node->content()));
    else if (result == SplitTextResult::TagBegin)
        is_scan_next = true;

    return Error();
}

Error HtmlReportGenerator::extractTextNext(const XMLNodePtr& start_node, ReportGenerator::TagType type,
    const QString& previous_text, const XMLNodePtr& node, QString& text, QString& text_after, bool& is_scan_next)
{    
    Q_UNUSED(start_node)
    Q_UNUSED(previous_text)
    is_scan_next = true;

    static const QList<QPair<QString, QString>> marks = {
        {HTML_FIELD_BEGIN, HTML_FIELD_END},
        {HTML_CELL_BEGIN, HTML_CELL_END},
        {HTML_BLOCK_START_BEGIN, HTML_BLOCK_START_END},
        {HTML_BLOCK_FINISH_BEGIN, HTML_BLOCK_FINISH_END},
    };
    static const QList<ReportGenerator::TagType> types = {
        TagType::Field,
        TagType::Cell,
        TagType::BlockStart,
        TagType::BlockFinish,
    };

    QString text_before;
    SplitTextResult result;
    Error error = splitText(node->content(), true, marks, types, text_before, text, text_after, type, result);
    if (error.isError())
        return error;

    if (result == SplitTextResult::FullTag || result == SplitTextResult::TagBegin) {
        return createError(QString("found tag start mark after another start mark <%1>").arg(node->content()));

    } else if (result == SplitTextResult::TagEnd) {
        is_scan_next = false;

    } else {
        is_scan_next = true;
        text = node->content();
    }

    return Error();
}

Error HtmlReportGenerator::getTagMainNode(
    const XMLNodePtr& node, ReportGenerator::TagType type, const QString& tag_text, XMLNodePtr& main_node)
{    
    if (type == TagType::BlockStart || type == TagType::BlockFinish) {
        // для блоков родителем должна являться строка таблицы
        main_node = node->parentByName("tr");
        if (main_node == nullptr)
            return createError(QString("Block <%1> error - parent 'TR' not found").arg(tag_text));
    } else {
        main_node = node;
    }
    return Error();
}

Error HtmlReportGenerator::setText(const QString& text, const XMLNodePtr& element)
{    
    element->setContent(text);
    return Error();
}

Error HtmlReportGenerator::setImage(const XMLNodePtr& set_to, const QString& text_before, const QString& text_after, const QImage& value,
                                    const QString& image_id)
{
    Q_UNUSED(set_to);
    Q_UNUSED(text_before);
    Q_UNUSED(text_after);
    Q_UNUSED(value);
    Q_UNUSED(image_id);
    return Error("HtmlReportGenerator: image fields not supported");
}

} // namespace zf
