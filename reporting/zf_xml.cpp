#include "zf_xml.h"

#include "zf_core.h"
#include "zf_utils.h"

#include <QDebug>
#include <QTextCodec>

#include <libxml/HTMLparser.h>
#include <libxml/uri.h>
#include <libxml/xmlsave.h>

#ifdef Q_OS_LINUX
#include <malloc.h>
#endif

namespace zf
{
static QString char_cast(xmlChar* text)
{
    return QString::fromUtf8(static_cast<const char*>(reinterpret_cast<void*>(text)));
}
static QString char_cast(const xmlChar* text)
{
    return QString::fromUtf8(static_cast<const char*>(reinterpret_cast<void*>(const_cast<xmlChar*>(text))));
}
static QString char_cast(char* text)
{
    return QString::fromUtf8(static_cast<const char*>(reinterpret_cast<void*>(text)));
}

#define STRING_CAST_XML(_s_) reinterpret_cast<const xmlChar*>((_s_).toUtf8().constData())

XMLDocument::XMLDocument()
{
    Z_DEBUG_NEW("XMLDocument");
}

XMLDocument::~XMLDocument()
{
    xmlFreeDoc(_doc);
    Z_DEBUG_DELETE("XMLDocument");
}

void XMLDocument::init()
{
    LIBXML_TEST_VERSION

    xmlInitParser();
}

void XMLDocument::cleanup()
{
    xmlCleanupParser();
}

XMLDocumentPtr XMLDocument::newDoc(const QString& version)
{
    auto doc = xmlNewDoc(STRING_CAST_XML(version));
    if (doc == nullptr)
        return nullptr;

    return Z_MAKE_SHARED(XMLDocument, doc);
}

XMLDocumentPtr XMLDocument::readHtmlFile(const QString& filename, const XML_ParserOptions& options, const QString& encoding)
{
    auto doc = htmlReadFile(filename.toLocal8Bit(), encoding.toLocal8Bit(), options);
    if (doc == nullptr)
        return nullptr;

    return Z_MAKE_SHARED(XMLDocument, doc);
}

XMLDocumentPtr XMLDocument::readXmlFile(const QString& filename, const XML_ParserOptions& options, const QString& encoding)
{
    auto doc = xmlReadFile(filename.toLocal8Bit(), encoding.toLocal8Bit(), options);
    if (doc == nullptr)
        return nullptr;

    return Z_MAKE_SHARED(XMLDocument, doc);
}

XMLDocumentPtr XMLDocument::readXmlMemory(const QString& content, const XML_ParserOptions& options)
{
    QByteArray data = content.toUtf8();
    auto doc = xmlReadMemory(data.constData(), data.size(), nullptr, "UTF-8", options);
    if (doc == nullptr)
        return nullptr;

    return Z_MAKE_SHARED(XMLDocument, doc);
}

XML_ElementType XMLDocument::type() const
{
    return static_cast<XML_ElementType>(_doc->type);
}

QString XMLDocument::name() const
{
    return char_cast(_doc->name);
}

QList<XMLNodePtr> XMLDocument::children() const
{
    QList<XMLNodePtr> res;
    auto p = _doc->children;
    while (p != nullptr) {
        res << Z_MAKE_SHARED(XMLNode, p);
        p = p->next;
    }
    return res;
}

XMLNodePtr XMLDocument::firstChild() const
{
    return _doc->children == nullptr ? nullptr : Z_MAKE_SHARED(XMLNode, _doc->children);
}

XMLNodePtr XMLDocument::last() const
{
    return _doc->last == nullptr ? nullptr : Z_MAKE_SHARED(XMLNode, _doc->last);
}

XMLNodePtr XMLDocument::parent() const
{
    return _doc->parent == nullptr ? nullptr : Z_MAKE_SHARED(XMLNode, _doc->parent);
}

XMLNodePtr XMLDocument::next() const
{
    return _doc->next == nullptr ? nullptr : Z_MAKE_SHARED(XMLNode, _doc->next);
}

XMLNodePtr XMLDocument::prev() const
{
    return _doc->prev == nullptr ? nullptr : Z_MAKE_SHARED(XMLNode, _doc->prev);
}

int XMLDocument::compression() const
{
    return _doc->compression;
}

QString XMLDocument::version() const
{
    return char_cast(_doc->version);
}

QString XMLDocument::encoding() const
{
    return char_cast(_doc->encoding);
}

QString XMLDocument::URL() const
{
    return char_cast(_doc->URL);
}

XML_ParserOptions XMLDocument::parseFlags() const
{
    return static_cast<XML_ParserOptions>(_doc->parseFlags);
}

XML_DocProperties XMLDocument::properties() const
{
    return static_cast<XML_DocProperties>(_doc->properties);
}

XMLNodePtr XMLDocument::getRootElement() const
{
    auto node = xmlDocGetRootElement(_doc);
    if (node == nullptr)
        return nullptr;

    return Z_MAKE_SHARED(XMLNode, node);
}

void XMLDocument::setRootElement(const XMLNodePtr& root)
{
    xmlDocSetRootElement(_doc, root->_node);
}

XMLNodePtr XMLDocument::newText(const QString& content)
{
    auto node = xmlNewDocText(_doc, STRING_CAST_XML(content));
    if (node == nullptr)
        return nullptr;

    return Z_MAKE_SHARED(XMLNode, node);
}

XMLNodePtr XMLDocument::newNode(const QString& name, const QString& content, const QString& ns)
{
    QString name_p = name;

    // ищем namespace
    xmlNsPtr ns_ptr = nullptr;
    if (auto root = getRootElement(); !ns.isEmpty() && root != nullptr) {
        ns_ptr = xmlSearchNs(_doc, root->_node, STRING_CAST_XML(ns));
        if (ns_ptr == nullptr)
            name_p = ns + ":" + name;
    }

    auto node = xmlNewDocNode(_doc, ns_ptr, STRING_CAST_XML(name_p), STRING_CAST_XML(content));
    if (node == nullptr)
        return nullptr;

    return Z_MAKE_SHARED(XMLNode, node);
}

XMLNodePtr XMLDocument::newRawNode(const QString& name, const QString& content, const QString& ns)
{
    QString name_p = name;

    // ищем namespace
    xmlNsPtr ns_ptr = nullptr;
    if (auto root = getRootElement(); !ns.isEmpty() && root != nullptr) {
        ns_ptr = xmlSearchNs(_doc, root->_node, STRING_CAST_XML(ns));
        if (ns_ptr == nullptr)
            name_p = ns + ":" + name;
    }

    auto node = xmlNewDocRawNode(_doc, ns_ptr, STRING_CAST_XML(name_p), STRING_CAST_XML(content));
    if (node == nullptr)
        return nullptr;

    return Z_MAKE_SHARED(XMLNode, node);
}

void XMLDocument::removeNodes(const QSet<XMLNodePtr>& nodes)
{
    QSet<XMLNodePtr> to_remove;
    // исключаем удаление вложенных, т.к. они удалятся сами при удалении родителя
    for (auto& n1 : nodes) {
        bool has_parent = false;
        for (auto& n2 : nodes) {
            if (n2->isParentOf(n1)) {
                has_parent = true;
                break;
            }
        }
        if (!has_parent)
            to_remove << n1;
    }
    for (auto& n : qAsConst(to_remove)) {
        n->remove();
    }
}

int XMLDocument::saveToFile(const QString& filename, const XML_SaveOptions& options, const QString& encoding) const
{
    Utils::removeFile(filename);

    // Q_UNUSED(options);
    //    int keep = xmlKeepBlanksDefault(1);
    //    xmlResetLastError();
    //    int result = xmlSaveFormatFileEnc(filename.toLocal8Bit(), _doc, encoding.toLocal8Bit(), 0);
    //    xmlKeepBlanksDefault(keep);
    //    return result;

    //    int keep = xmlKeepBlanksDefault(1);
    xmlSaveCtxtPtr context = xmlSaveToFilename(filename.toLocal8Bit(), encoding.toLocal8Bit(), options);
    int result;
    if (context == nullptr) {
        result = -1;

    } else {
        xmlSaveDoc(context, _doc);
        result = xmlSaveClose(context);
    }
    //    xmlKeepBlanksDefault(keep);
    return result;
}

XMLDocument::XMLDocument(_xmlDoc* doc)
    : _doc(doc)
{
    Z_CHECK_NULL(_doc);
}

_xmlDoc* XMLDocument::internalData() const
{
    return _doc;
}

XML_ElementType XMLAttr::type() const
{
    return static_cast<XML_ElementType>(_attr->type);
}

QString XMLAttr::name() const
{
    return char_cast(_attr->name);
}

QList<XMLNodePtr> XMLAttr::children() const
{
    QList<XMLNodePtr> res;
    auto p = _attr->children;
    while (p != nullptr) {
        res << Z_MAKE_SHARED(XMLNode, p);
        p = p->next;
    }
    return res;
}

XMLNodePtr XMLAttr::firstChild() const
{
    return _attr->children == nullptr ? nullptr : Z_MAKE_SHARED(XMLNode, _attr->children);
}

XMLNodePtr XMLAttr::parent() const
{
    return _attr->parent == nullptr ? nullptr : Z_MAKE_SHARED(XMLNode, _attr->parent);
}

XMLAttrPtr XMLAttr::next() const
{
    return _attr->next == nullptr ? nullptr : Z_MAKE_SHARED(XMLAttr, _attr->next);
}

XMLAttrPtr XMLAttr::prev() const
{
    return _attr->prev == nullptr ? nullptr : Z_MAKE_SHARED(XMLAttr, _attr->prev);
}

XMLDocumentPtr XMLAttr::doc() const
{
    return _attr->doc == nullptr ? nullptr : Z_MAKE_SHARED(XMLDocument, _attr->doc);
}

XML_AttributeType XMLAttr::atype() const
{
    return static_cast<XML_AttributeType>(_attr->atype);
}

XMLAttr::XMLAttr(_xmlAttr* attr)
    : _attr(attr)
{
    Z_CHECK_NULL(_attr);
}

_xmlAttr* XMLAttr::internalData() const
{
    return _attr;
}

bool XMLNode::isValid() const
{
    return _node != nullptr;
}

XML_NodeType XMLNode::type() const
{
    return static_cast<XML_NodeType>(_node->type);
}

QString XMLNode::name() const
{
    return char_cast(_node->name);
}

QString XMLNode::ns() const
{
    return _node->ns != nullptr && type() != XML_NodeType::XML_DOCUMENT_NODE ? char_cast(_node->ns->prefix) : QString();
}

QString XMLNode::debug() const
{
    QStringList res;
    QString s = content().trimmed();
    if (!s.isEmpty())
        res << s;

    auto list = children();
    for (auto& n : list) {
        s = n->debug();
        if (!s.isEmpty())
            res << s;
    }

    return res.join(" | ");
}

bool XMLNode::hasChildren() const
{
    return _node->children != nullptr;
}

QList<XMLNodePtr> XMLNode::children() const
{
    QList<XMLNodePtr> res;
    auto p = _node->children;
    while (p != nullptr) {
        res << Z_MAKE_SHARED(XMLNode, p);
        p = p->next;
    }
    return res;
}

XMLNodePtr XMLNode::firstChild() const
{
    return _node->children == nullptr ? nullptr : Z_MAKE_SHARED(XMLNode, _node->children);
}

QList<XMLNodePtr> XMLNode::allChildren() const
{
    QList<XMLNodePtr> res;
    auto list = children();
    for (auto& n : list) {
        res << n->allChildren();
    }
    return res;
}

QMap<int, XMLNodePtr> XMLNode::allChildrenMap() const
{
    QMap<int, XMLNodePtr> res;
    int id = 0;
    allChildrenMapHelper(res, id);
    return res;
}

QMap<QString, XMLNodePtr> XMLNode::createNodesMap(const QList<XMLNodePtr>& nodes)
{
    QMap<QString, XMLNodePtr> res;

    for (int i = 0; i < nodes.count(); i++) {
        QString key = QString::number(i) + QStringLiteral("N");
        Z_CHECK(!res.contains(key));
        res[key] = nodes.at(i);

        QMap<int, XMLNodePtr> map = nodes.at(i)->allChildrenMap();
        for (auto mi = map.constBegin(); mi != map.constEnd(); ++mi) {
            key = QString::number(i) + Consts::KEY_SEPARATOR + QString::number(mi.key());
            Z_CHECK(!res.contains(key));
            res[key] = mi.value();
        }
    }

    return res;
}

XMLNodePtr XMLNode::findNextContent(const QString& text, CompareOperator op, bool stop_at_first_incorrent) const
{
    // ищем в детях
    bool stop;
    _xmlNode* node = findContentHelper(_node, text, op, true, stop_at_first_incorrent, stop);
    if (node == nullptr) {
        if (stop)
            return nullptr;

        // ищем в следующих узлах
        _xmlNode* next = _node->next;
        while (next != nullptr) {
            node = findContentHelper(next, text, op, false, stop_at_first_incorrent, stop);
            if (node != nullptr)
                break;

            if (stop)
                return nullptr;

            next = next->next;
        }
    }

    return node == nullptr ? nullptr : Z_MAKE_SHARED(XMLNode, node);
}

_xmlNode* XMLNode::findContentHelper(_xmlNode* node, const QString& text, CompareOperator op, bool only_children,
                                     bool stop_at_first_incorrent, bool& stop)
{
    stop = false;
    if (!only_children && node->type == XML_TEXT_NODE) {
        QString content = char_cast(node->content);
        if (Utils::compareVariant(content, text, op))
            return node;

        if (stop_at_first_incorrent && !content.isEmpty()) {
            stop = true;
            return nullptr;
        }
    }

    _xmlNode* child = node->children;
    while (child != nullptr) {
        _xmlNode* node = findContentHelper(child, text, op, false, stop_at_first_incorrent, stop);
        if (node != nullptr)
            return node;

        if (stop)
            return nullptr;

        child = child->next;
    }

    return nullptr;
}

void XMLNode::allChildrenMapHelper(QMap<int, XMLNodePtr>& map, int& id) const
{
    auto list = children();
    for (auto& n : list) {
        map[id] = n;
        id++;

        n->allChildrenMapHelper(map, id);
    }
}

XMLNodePtr XMLNode::last() const
{
    return _node->last == nullptr ? nullptr : Z_MAKE_SHARED(XMLNode, _node->last);
}

XMLNodePtr XMLNode::parent() const
{
    return _node->parent == nullptr ? nullptr : Z_MAKE_SHARED(XMLNode, _node->parent);
}

XMLNodePtr XMLNode::next(XML_NodeType type) const
{
    _xmlNode* next = _node->next;
    while (next != nullptr) {
        if (type == XML_NodeType::Undefined || next->type == static_cast<xmlElementType>(type))
            return Z_MAKE_SHARED(XMLNode, next);
        next = next->next;
    }

    return nullptr;
}

XMLNodePtr XMLNode::prev(XML_NodeType type) const
{
    _xmlNode* prev = _node->prev;
    while (prev != nullptr) {
        if (type == XML_NodeType::Undefined || prev->type == static_cast<xmlElementType>(type))
            return Z_MAKE_SHARED(XMLNode, prev);
        prev = prev->prev;
    }

    return nullptr;
}

XMLDocumentPtr XMLNode::doc() const
{
    return _node->doc == nullptr ? nullptr : Z_MAKE_SHARED(XMLDocument, _node->doc);
}

QString XMLNode::content() const
{
    return char_cast(_node->content);
}

void XMLNode::setContent(const QString& c)
{
    xmlNodeSetContent(_node, STRING_CAST_XML(c));
#if !defined(RELEASE_MODE) && defined(QT_DEBUG)
    _content = content();
#endif
}

QList<XMLAttrPtr> XMLNode::properties() const
{
    QList<XMLAttrPtr> res;
    auto p = _node->properties;
    while (p != nullptr) {
        res << Z_MAKE_SHARED(XMLAttr, p);
        p = p->next;
    }
    return res;    
}

XMLAttrPtr XMLNode::firstProperty() const
{
    return _node->properties == nullptr ? nullptr : Z_MAKE_SHARED(XMLAttr, _node->properties);
}

int XMLNode::line() const
{
    return _node->line;
}

XMLNodePtr XMLNode::clone(CloneMode mode) const
{
    return Z_MAKE_SHARED(XMLNode, xmlCopyNode(_node, mode));
}

XMLNodePtr XMLNode::newNode(const QString& name)
{
    auto node = xmlNewNode(nullptr, STRING_CAST_XML(name));
    if (node == nullptr)
        return nullptr;

    return Z_MAKE_SHARED(XMLNode, node);
}

XMLNodePtr XMLNode::newText(const QString& content)
{
    auto node = xmlNewText(STRING_CAST_XML(content));
    if (node == nullptr)
        return nullptr;

    return Z_MAKE_SHARED(XMLNode, node);
}

bool XMLNode::compare(const QString& content, const QString& ns) const
{
    return comp(content.trimmed(), this->content().trimmed())
        && (ns.trimmed().isEmpty() || comp(ns.trimmed(), this->ns().trimmed()));
}

bool XMLNode::isChildOf(const XMLNodePtr& node) const
{
    Z_CHECK_NULL(node);
    _xmlNode* parent = _node->parent;
    while (parent != nullptr) {
        if (node->_node == parent)
            return true;
        parent = parent->parent;
    }
    return false;
}

bool XMLNode::isParentOf(const XMLNodePtr& node) const
{
    Z_CHECK_NULL(node);
    _xmlNode* parent = node->_node->parent;
    while (parent != nullptr) {
        if (parent == _node)
            return true;
        parent = parent->parent;
    }

    return false;
}

XMLNodePtr XMLNode::parentByName(
    const QString& name, const QString& ns, const QList<QPair<QString, QString>>& stop_if_found) const
{
    Z_CHECK(!name.isEmpty());
    XMLNodePtr parent = this->parent();
    while (parent != nullptr) {
        // проверяем на остановку
        for (auto& stop : stop_if_found) {
            bool ns_stop_ok = stop.first.isEmpty() || comp(stop.first, parent->ns());
            if (ns_stop_ok && comp(stop.second, parent->name()))
                return nullptr;
        }

        bool ns_ok = ns.isEmpty() || comp(ns, parent->ns());
        if (ns_ok && comp(name, parent->name()))
            return parent;
        parent = parent->parent();
    }
    return nullptr;
}

XMLNodePtr XMLNode::parentBySameParent(const XMLNodePtr& node) const
{
    Z_CHECK_NULL(node);

    XMLNodePtr parent = this->parent();
    while (parent != nullptr) {
        if (parent->parent() == node)
            return parent;

        parent = parent->parent();
    }
    return nullptr;
}

XMLNodePtr XMLNode::newChild(const QString& name, const QString& content)
{
    auto node = xmlNewChild(_node, nullptr, STRING_CAST_XML(name), STRING_CAST_XML(content));
    if (node == nullptr)
        return nullptr;

    return Z_MAKE_SHARED(XMLNode, node);
}

XMLNodePtr XMLNode::newTextChild(const QString& name, const QString& content)
{
    auto node = xmlNewTextChild(_node, nullptr, STRING_CAST_XML(name), STRING_CAST_XML(content));
    if (node == nullptr)
        return nullptr;

    return Z_MAKE_SHARED(XMLNode, node);
}

XMLNodePtr XMLNode::addChild(const XMLNodePtr& node)
{
    auto n = xmlAddChild(_node, node->_node);
    if (n == nullptr)
        return nullptr;

    return Z_MAKE_SHARED(XMLNode, n);
}

XMLNodePtr XMLNode::addNextSibling(const XMLNodePtr& elem)
{
    auto n = xmlAddNextSibling(_node, elem->_node);
    if (n == nullptr)
        return nullptr;

    return Z_MAKE_SHARED(XMLNode, n);
}

XMLNodePtr XMLNode::addPrevSibling(const XMLNodePtr& elem)
{
    auto n = xmlAddPrevSibling(_node, elem->_node);
    if (n == nullptr)
        return nullptr;

    return Z_MAKE_SHARED(XMLNode, n);
}

XMLNodePtr XMLNode::addSibling(const XMLNodePtr& elem)
{
    auto n = xmlAddSibling(_node, elem->_node);
    if (n == nullptr)
        return nullptr;

    return Z_MAKE_SHARED(XMLNode, n);
}

XMLAttrPtr XMLNode::newProp(const QString& name, const QString& value)
{
    auto p = xmlNewProp(_node, STRING_CAST_XML(name), STRING_CAST_XML(value));
    if (p == nullptr)
        return nullptr;

    return Z_MAKE_SHARED(XMLAttr, p);
}

bool XMLNode::containsProp(const QString& name) const
{
    if (name.isEmpty())
        return false;

    QString ns;
    QString nm;

    auto split = name.split(":");
    if (split.count() > 1) {
        ns = split.at(0);
        nm = split.at(1);
    } else {
        nm = name;
    }

    _xmlAttr* prop = _node->properties;
    while (prop != nullptr) {
        if (!ns.isEmpty()) {
            if (prop->ns == nullptr || char_cast(prop->ns->prefix) != ns) {
                prop = prop->next;
                continue;
            }
        }

        if (char_cast(prop->name) != nm) {
            prop = prop->next;
            continue;
        }

        return true;
    }

    return false;
}

void XMLNode::remove()
{
    unlink();
    xmlFreeNode(_node);
    _node = nullptr;
}

void XMLNode::unlink()
{
    xmlUnlinkNode(_node);
}

XMLNode::XMLNode(_xmlNode* node)
    : _node(node)
{
    Z_CHECK_NULL(_node);

#if !defined(RELEASE_MODE) && defined(QT_DEBUG)
    _ns = ns();
    _name = name();
    _content = content();
#endif
}

_xmlNode* XMLNode::internalData() const
{
    return _node;
}

} // namespace zf
