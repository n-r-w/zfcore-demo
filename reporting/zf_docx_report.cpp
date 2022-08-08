#include "zf_docx_report.h"
#include "zf_core.h"
#include "zf_utils.h"
#include "zf_textsource.h"

#include <QDomDocument>
#include <JlCompress.h>
#include <QFile>
#include <QBuffer>

// Поле: начало тэга
#define DOCX_FIELD_BEGIN QStringLiteral("{{")
// Поле: конец тэга
#define DOCX_FIELD_END QStringLiteral("}}")

// Ячейка: начало тэга
#define DOCX_CELL_BEGIN QStringLiteral("{<")
// Ячейка: конец тэга
#define DOCX_CELL_END QStringLiteral(">}")

// Начало блока: начало тэга
#define DOCX_BLOCK_START_BEGIN QStringLiteral("{[")
// Начало блока: конец тэга
#define DOCX_BLOCK_START_END QStringLiteral("]}")

// Конец блока: начало тэга
#define DOCX_BLOCK_FINISH_BEGIN QStringLiteral("{#")
// Конец блока: конец тэга
#define DOCX_BLOCK_FINISH_END QStringLiteral("#}")

namespace zf
{
const QList<QPair<QString, QString>> DocxReportGenerator::_MARKS = {
    {DOCX_FIELD_BEGIN, DOCX_FIELD_END},
    {DOCX_CELL_BEGIN, DOCX_CELL_END},
    {DOCX_BLOCK_START_BEGIN, DOCX_BLOCK_START_END},
    {DOCX_BLOCK_FINISH_BEGIN, DOCX_BLOCK_FINISH_END},
};

DocxReportGenerator::DocxReportGenerator()
{
}

Error DocxReportGenerator::unpackTemplate(const QByteArray& template_file, const QString& temp_folder, QStringList& to_parse)
{
    QBuffer buffer;
    buffer.setData(template_file);
    Z_CHECK(buffer.open(QBuffer::ReadOnly));

    QStringList files = JlCompress::extractDir(&buffer, temp_folder);
    if (files.isEmpty())
        return Error("DocxReportGenerator - unzip error");

    for (auto& f : files) {
        if (comp(QFileInfo(f).suffix(), "xml"))
            to_parse << f;
    }

    return Error();
}

Error DocxReportGenerator::packTarget(const QString& temp_folder, const QString& target_file)
{
    auto error = prepareFiles(temp_folder);
    if (error.isError())
        return error;

    if (!JlCompress::compressDir(target_file, temp_folder, true
#ifndef Q_OS_WIN
                                 ,
                                 QDir::Hidden
#endif
                                 ))
        return Error::fileIOError(target_file);

    return Error();
}

QString DocxReportGenerator::prepareTargetFileName(const QString& file_name) const
{
    QFileInfo fi(file_name);
    if (!comp(fi.suffix(), "docx"))
        return file_name + ".docx";
    else
        return file_name;
}

Error DocxReportGenerator::openDocument(const QString& template_file, XMLDocumentPtr& document)
{
    QFile file(template_file);
    if (!file.open(QFile::ReadOnly))
        return Error::fileIOError(&file);

    QString source = QString::fromUtf8(file.readAll());
    file.close();

    QString content;
    auto error = compress(source, content);
    if (error.isError())
        return error;

    document = XMLDocument::readXmlMemory(content, /*XML_ParserOption::XML_PARSE_NOBLANKS |*/ XML_ParserOption::XML_PARSE_NONET
                                                       | XML_ParserOption::XML_PARSE_HUGE);
    if (document == nullptr)
        return Error::badFileError(template_file);

    return Error();
}

Error DocxReportGenerator::saveDocument(const QString& target_file, const XMLDocumentPtr& document)
{    
    if (document->saveToFile(target_file, /*XML_SaveOption::XML_SAVE_NO_EMPTY |*/ XML_SaveOption::XML_SAVE_NO_XHTML) < 0)
        return Error::fileIOError(target_file);
    return Error();
}

Error DocxReportGenerator::extractText(const XMLNodePtr& node, QString& text, QString& text_before, ReportGenerator::TagType& type,
                                       bool& is_scan_next)
{
    static const QList<ReportGenerator::TagType> types = {
        TagType::Field,
        TagType::Cell,
        TagType::BlockStart,
        TagType::BlockFinish,
    };

    QString text_after;
    SplitTextResult result;
    Error error = splitText(node->content(), false, _MARKS, types, text_before, text, text_after, type, result);
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

Error DocxReportGenerator::extractTextNext(const XMLNodePtr& start_node, ReportGenerator::TagType type,
    const QString& previous_text, const XMLNodePtr& node, QString& text, QString& text_after, bool& is_scan_next)
{
    Q_UNUSED(start_node)
    Q_UNUSED(previous_text)
    is_scan_next = true;

    if (node->compare("p", "w")) {
        // найден конец параграфа - дальше искать нет смысла
        is_scan_next = false;
        return Error();
    }

    static const QList<ReportGenerator::TagType> types = {
        TagType::Field,
        TagType::Cell,
        TagType::BlockStart,
        TagType::BlockFinish,
    };

    QString text_before;
    SplitTextResult result;
    Error error = splitText(previous_text + node->content(), true, _MARKS, types, text_before, text, text_after, type, result);
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

Error DocxReportGenerator::getTagMainNode(
    const XMLNodePtr& node, ReportGenerator::TagType type, const QString& tag_text, XMLNodePtr& main_node)
{
    Q_UNUSED(type)
    Q_UNUSED(tag_text)

    main_node = node->parentByName("p", "w");
    return Error();
}

Error DocxReportGenerator::setText(const QString& text, const XMLNodePtr& element)
{
    auto parent = element->parent();
    Z_CHECK(parent != nullptr);

    if (!parent->containsProp("xml:space"))
        parent->newProp("xml:space", "preserve");

    element->setContent(text);

    return Error();
}

int DocxReportGenerator::pixelsToEMU(int n)
{
    return n * 9525;
}

XMLNodePtr DocxReportGenerator::createNode(const XMLNodePtr& parent, const QString& name, const QMap<QString, QVariant>& values)
{
    auto node = document()->newNode(name, "");

    for (auto i = values.constBegin(); i != values.constEnd(); ++i) {
        node->newProp(i.key(), Utils::variantToString(i.value()));
    }

    parent->addChild(node);
    return node;
}

Error DocxReportGenerator::prepareFiles(const QString& temp_folder) const
{
    if (_image_info.isEmpty())
        return {};

    // для работы с картинками надо кое что сделать

    // добавляем файлы картинок в папку media
    QString media_folder = temp_folder + "/word/media";
    auto error = Utils::makeDir(media_folder);
    if (error.isError())
        return error;
    for (auto i = _image_info.constBegin(); i != _image_info.constEnd(); ++i) {
        QString file_name = media_folder + "/" + i.key() + ".jpg";
        i.value().save(file_name, "jpg");
    }

    QStringList files;
    error = Utils::getDirContent(temp_folder, files);
    if (error.isError())
        return error;

    for (auto& s : qAsConst(files)) {
        QString file_name = QFileInfo(s).fileName();
        if (file_name == "document.xml.rels") {
            // добавляем ссылки на картинки
            for (auto i = _image_info.constBegin(); i != _image_info.constEnd(); ++i) {
                appendToXML(s, "Relationships", "Relationship",
                            {
                                {"Id", i.key()},
                                {"Type", R"(http://schemas.openxmlformats.org/officeDocument/2006/relationships/image)"},
                                {"Target", "media/" + i.key() + ".jpg"},
                            },
                            {});
            }
        } else if (file_name == "[Content_Types].xml") {
            // добавляем тип файла jpg
            appendToXML(s, "Types", "Default",
                        {
                            {"Extension", "jpg"},
                            {"ContentType", "image/jpeg"},
                        },
                        {{"Default", {"Extension", "jpg"}}});
        }
    }

    return {};
}

Error DocxReportGenerator::appendToXML(const QString& xml_file, const QString& target_node, const QString& name,
                                       const QMap<QString, QString>& params, const QMap<QString, QPair<QString, QString>>& exclude) const
{
    QDomDocument doc;
    QFile file(xml_file);
    if (!file.open(QFile::ReadOnly))
        return Error::fileIOError(xml_file);
    doc.setContent(file.readAll());
    file.close();

    for (auto i = exclude.constBegin(); i != exclude.constEnd(); ++i) {
        QDomNodeList list = doc.elementsByTagName(i.key());
        for (int n = 0; n < list.count(); n++) {
            if (!list.at(n).isElement())
                continue;

            if (list.at(n).toElement().attribute(i.value().first) == i.value().second)
                return {};
        }
    }

    QDomNodeList target = doc.elementsByTagName(target_node);
    if (target.isEmpty())
        return {};

    QDomElement added = doc.createElement(name);

    for (auto i = params.constBegin(); i != params.constEnd(); ++i) {
        added.setAttribute(i.key(), i.value());
    }

    target.at(0).appendChild(added);

    if (!file.open(QFile::WriteOnly))
        return Error::fileIOError(xml_file);
    QTextStream st(&file);
    doc.save(st, 4);
    file.close();

    return {};
}

Error DocxReportGenerator::compress(const QString& source_data, QString& result_data)
{
    //! поиск xml тега
    static const QString reg_tag = QStringLiteral(R"((<%1>|<%1 )((?:(?!<\/?%1[ >]).)*)<\/%1>)");
    //    static const QString xml_body = QStringLiteral("w:body"); // тело документа
    static const QString xml_p = QStringLiteral("w:p"); // параграф
    //    static const QString xml_r = QStringLiteral("w:r"); // рекорд
    static const QString xml_t = QStringLiteral("w:t"); // текст

    /*! Сжать переменные и признаки начала/окончания секций
     * Переменные содержатся в элементах w:r
     * Начало нового параграфа w:p разрывает непрерывный текст
     * Несколько составных частей одной переменной помещаются в самый первый w:r */

    result_data = source_data;

    // параграфы
    QString p_exp_str = reg_tag.arg(xml_p);
    QRegularExpression p_exp(p_exp_str, QRegularExpression::CaseInsensitiveOption | QRegularExpression::DotMatchesEverythingOption
                                            | QRegularExpression::MultilineOption);
    // текст
    QString t_exp_str = reg_tag.arg(xml_t);
    QRegularExpression t_exp(t_exp_str, QRegularExpression::CaseInsensitiveOption | QRegularExpression::DotMatchesEverythingOption
                                            | QRegularExpression::MultilineOption);

    //TODO перебирать в цикле все варианты DOCX_FIELD_BEGIN/DOCX_FIELD_END и прочие
    // переменные
    QString var_reg_str = QStringLiteral("%1[^%2]+%2").arg(DOCX_FIELD_BEGIN, DOCX_FIELD_END);
    QRegularExpression var_exp(var_reg_str, QRegularExpression::CaseInsensitiveOption | QRegularExpression::DotMatchesEverythingOption
                                                | QRegularExpression::MultilineOption);    

    // *** сжатие переменнх
    // выявляем все параграфы
    QRegularExpressionMatchIterator reg_it_p = p_exp.globalMatch(result_data, 0, QRegularExpression::NormalMatch);
    while (reg_it_p.hasNext()) {
        // reg_it_p.next().capturedRef() делать нельзя, т.к. оптимизатор убъет reg_it_p.next() и capturedRef
        // будет указывать на удаленную область памяти
        QRegularExpressionMatch match = reg_it_p.next();
        QStringRef r = match.capturedRef();
        auto source = result_data.midRef(r.position(), r.length()); // иначе ссылка станет неопределенной

        // В рамках каждого параграфа выявляем текст
        TextSource text_clean; // чистый текст параграфа без тэгов
        QRegularExpressionMatchIterator reg_it_t = t_exp.globalMatch(source, 0, QRegularExpression::NormalMatch);
        while (reg_it_t.hasNext()) {
            QRegularExpressionMatch match = reg_it_t.next();
            QStringRef r = match.capturedRef();
            QStringRef r1;
            // надо скорректировать варианты <tag ....>текст<\tag>
            if (r.at(xml_t.length() + 1) == ' ') {
                // ищем закрывающую >
                int n = r.indexOf('>');
                if (n < 0)
                    continue;
                r1 = r.mid(n + 1, r.length() - n - 4 - xml_t.length());
            } else
                r1 = r.mid(xml_t.length() + 2, r.length() - xml_t.length() * 2 - 5);

            r1 = result_data.midRef(r1.position(), r1.length()); // иначе ссылка станет неопределенной
            text_clean.addFragment(r1);
        }

        // В рамках текста выявляем переменные и признаки начала/окончания секций
        // объединяем разделенные и переносим первый элемент <w:t>
        QRegularExpressionMatchIterator reg_it_var = var_exp.globalMatch(text_clean.toString(), 0, QRegularExpression::NormalMatch);
        while (reg_it_var.hasNext()) {
            QRegularExpressionMatch match = reg_it_var.next();
            QStringRef r_var = match.capturedRef();
            TextSource s_var = text_clean.mid(r_var.position(), r_var.length()); // содержит части одной переменной
            int pos = 0;
            for (int i = 1; i < s_var.count(); i++) {
                QStringRef r = s_var.fragments().at(i);
                QString moved = r.toString();
                result_data.remove(r.position(), r.length());
                result_data.insert(s_var.fragments().at(0).position() + s_var.fragments().at(0).length() + pos, moved);
                pos += r.length();
            }
        }
    }

    //*** разбиение переменных на отдельные текстовые блоки
    // выявляем все параграфы
    reg_it_p = p_exp.globalMatch(result_data, 0, QRegularExpression::NormalMatch);
    while (reg_it_p.hasNext()) {
        // reg_it_p.next().capturedRef() делать нельзя, т.к. оптимизатор убъет reg_it_p.next() и capturedRef
        // будет указывать на удаленную область памяти
        QRegularExpressionMatch match = reg_it_p.next();
        QStringRef r = match.capturedRef();
        auto source = result_data.midRef(r.position(), r.length()); // иначе ссылка станет неопределенной

        // В рамках каждого параграфа выявляем текст
        QRegularExpressionMatchIterator reg_it_t = t_exp.globalMatch(source, 0, QRegularExpression::NormalMatch);
        while (reg_it_t.hasNext()) {
            QRegularExpressionMatch match = reg_it_t.next();
            QStringRef r = match.capturedRef();
            QStringRef r1;
            // надо скорректировать варианты <tag ....>текст<\tag>
            if (r.at(xml_t.length() + 1) == ' ') {
                // ищем закрывающую >
                int n = r.indexOf('>');
                if (n < 0)
                    continue;
                r1 = r.mid(n + 1, r.length() - n - 4 - xml_t.length());
            } else
                r1 = r.mid(xml_t.length() + 2, r.length() - xml_t.length() * 2 - 5);

            r1 = result_data.midRef(r1.position(), r1.length()); // иначе ссылка станет неопределенной

            // В рамках текста выявляем переменные и признаки начала/окончания секций
            QRegularExpressionMatchIterator reg_it_var = var_exp.globalMatch(r1, 0, QRegularExpression::NormalMatch);
            QList<QPair<int, int>> vars;
            while (reg_it_var.hasNext()) {
                QRegularExpressionMatch match = reg_it_var.next();
                QStringRef r_var = match.capturedRef();
                vars << QPair<int, int> {r_var.position(), r_var.length()}; // нельзя хранить QStringRef - оно сдохнет
            }

            if (vars.count() > 1) {
                static const QString preserve_str = R"(xml:space="preserve")";
                static const QString text_block = R"(<w:t xml:space="preserve">%1</w:t>)";
                bool has_preserve = r1.contains(preserve_str);

                // больше чем одна переменная в текстовом блоке. Разбиваем на несколько последовательных блоков
                for (int i = vars.count() - 1; i >= 0; i--) {
                    QPair<int, int> v = vars.at(i);

                    QStringList new_blocks;
                    if (i < vars.count() - 1) {
                        QStringRef postfix = result_data.midRef(v.first + v.second, vars.at(i + 1).first - v.first - v.second);
                        if (!postfix.isEmpty()) {
                            new_blocks << text_block.arg(postfix);
                        }
                    }
                    new_blocks << text_block.arg(result_data.midRef(v.first, v.second));

                    for (int b = 0; b < new_blocks.count(); b++) {
                        result_data.insert(r.position() + r.length(), new_blocks.at(b));
                    }
                }

                result_data.remove(vars.at(0).first, r1.position() + r1.length() - vars.at(0).first);

                if (!has_preserve)
                    result_data.insert(r1.position() - 1, " " + preserve_str);
            }
        }
    }

    return Error();
}

Error DocxReportGenerator::setImage(const XMLNodePtr& set_to, const QString& text_before, const QString& text_after, const QImage& value,
                                    const QString& image_id)
{
    XMLNodePtr to_remove = nullptr;

    QString text_id_prepared = "image_" + image_id; // по правилам XSD ID должно начинаться с текста
    quint32 int_id = std::numeric_limits<uint16_t>::max() - _image_info.count();

    // set_to - это текстовый узел, но нам надо вставлять не в него, а в узел выше
    auto parent = set_to;
    while (parent == nullptr || parent->type() == XML_NodeType::XML_TEXT_NODE
           || (parent->type() == XML_NodeType::XML_ELEMENT_NODE && parent->name() == "t")) {
        to_remove = parent;
        parent = parent->parent();
    }

    Z_CHECK_NULL(to_remove);
    XMLDocument::removeNodes({to_remove});

    auto text_before_node = document()->newText(text_before);
    auto text_after_node = document()->newText(text_after);

    // верхний уровень
    parent->addChild(text_before_node);
    auto n_drawing = createNode(parent, "w:drawing");
    parent->addChild(text_after_node);

    // картинка
    auto n_inline = createNode(n_drawing, "wp:inline");
    n_drawing->addChild(n_inline);

    auto n_extent
        = createNode(n_inline, "wp:extent", {{"cx", pixelsToEMU(value.size().width())}, {"cy", pixelsToEMU(value.size().height())}});
    auto n_docPr = createNode(n_inline, "wp:docPr", {{"id", int_id}, {"name", text_id_prepared}});
    auto n_cNvGraphicFramePr = createNode(n_inline, "wp:cNvGraphicFramePr");
    auto n_graphicFrameLocks = createNode(n_cNvGraphicFramePr, "a:graphicFrameLocks",
                                          {{"xmlns:a", "http://schemas.openxmlformats.org/drawingml/2006/main"}, {"noChangeAspect", "1"}});
    auto n_graphic = createNode(n_inline, "a:graphic", {{"xmlns:a", "http://schemas.openxmlformats.org/drawingml/2006/main"}});
    auto n_graphicData = createNode(n_graphic, "a:graphicData", {{"uri", "http://schemas.openxmlformats.org/drawingml/2006/picture"}});
    auto n_pic = createNode(n_graphicData, "pic:pic", {{"xmlns:pic", "http://schemas.openxmlformats.org/drawingml/2006/picture"}});
    auto n_nvPicPr = createNode(n_pic, "pic:nvPicPr");
    createNode(n_nvPicPr, "pic:cNvPr", {{"id", int_id}, {"name", ""}});
    createNode(n_nvPicPr, "pic:cNvPicPr");
    auto n_blipFill = createNode(n_pic, "pic:blipFill");
    createNode(n_blipFill, "a:blip", {{"r:embed", text_id_prepared}});
    auto n_stretch = createNode(n_blipFill, "a:stretch");
    createNode(n_stretch, "a:fillRect");
    auto n_spPr = createNode(n_pic, "pic:spPr");
    auto n_xfrm = createNode(n_spPr, "a:xfrm");
    createNode(n_xfrm, "a:off", {{"x", 0}, {"y", 0}});
    createNode(n_xfrm, "a:ext", {{"cx", pixelsToEMU(value.size().width())}, {"cy", pixelsToEMU(value.size().height())}});
    auto n_prstGeomm = createNode(n_spPr, "a:prstGeom", {{"prst", "rect"}});
    createNode(n_prstGeomm, "a:avLst");

    _image_info[text_id_prepared] = value;

    return {};
}

} // namespace zf
