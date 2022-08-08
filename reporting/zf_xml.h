#pragma once

#include "zf.h"

struct _xmlNode;
struct _xmlDoc;
struct _xmlAttr;

namespace zf
{
class XMLDocument;
typedef std::shared_ptr<XMLDocument> XMLDocumentPtr;

class XMLNode;
typedef std::shared_ptr<XMLNode> XMLNodePtr;

class XMLAttr;
typedef std::shared_ptr<XMLAttr> XMLAttrPtr;

//! Документ XML
class ZCORESHARED_EXPORT XMLDocument
{
public:
    XMLDocument();
    ~XMLDocument();

    //! Инициализация парсера. Вызывать один раз при старте программы, если парсер может использоваться в разных потоках
    static void init();
    //! Очистка памяти парсера. Вызывать один раз при завершении работы программы или когда парсер больше не нужен
    static void cleanup();

    //! Creates a new XML document
    static XMLDocumentPtr newDoc(const QString& version);
    //! Открыть файл XML
    static XMLDocumentPtr readXmlFile(const QString& filename, const XML_ParserOptions& options = XML_ParserOptions(),
                                      const QString& encoding = "UTF-8");
    //! Открыть XML из строки UTF8
    static XMLDocumentPtr readXmlMemory(const QString& content, const XML_ParserOptions& options = XML_ParserOptions());
    //! Открыть файл HTML
    static XMLDocumentPtr readHtmlFile(const QString& filename, const XML_ParserOptions& options = XML_ParserOption::HTML_PARSE_DEFAULT,
                                       const QString& encoding = "UTF-8");

    /*! XML_DOCUMENT_NODE или XML_HTML_DOCUMENT_NODE */
    XML_ElementType type() const;
    /*! name/filename/URI of the document */
    QString name() const;
    /*! Список дочерних узлов. Формируется динамически, путем перебора через next */
    QList<XMLNodePtr> children() const;
    //! Первый дочерний узел. Использовать для перебора через next
    XMLNodePtr firstChild() const;
    /*! last child link */
    XMLNodePtr last() const;
    /*! child->parent link */
    XMLNodePtr parent() const;
    /*! next sibling link  */
    XMLNodePtr next() const;
    /*! previous sibling link  */
    XMLNodePtr prev() const;

    /*! level of zlib compression */
    int compression() const;

    /*! the XML version string */
    QString version() const;
    /*! external initial encoding, if any */
    QString encoding() const;
    /*! The URI for that document */
    QString URL() const;
    /*! set of xmlParserOption used to parse the  document */
    XML_ParserOptions parseFlags() const;
    /*! set of xmlDocProperties for this document set at the end of parsing */
    XML_DocProperties properties() const;

    //! Get the root element of the document (doc->children is a list containing possibly comments, etc ...).
    XMLNodePtr getRootElement() const;
    //! Set the root element of the document (doc->children is a list containing possibly comments, etc ...).
    void setRootElement(const XMLNodePtr& root);

    //! Creation of a new text node within a document
    XMLNodePtr newText(const QString& content);
    //! Creation of a new node element within a document. @content are optional.  NOTE: @content is
    //! supposed to be a piece of XML CDATA, so it allow entities references, but XML special chars need to be escaped
    //! first. Use newRawNode() if you don't need entities support.
    XMLNodePtr newNode(const QString& name, const QString& content, const QString& ns = QString());
    //! Creation of a new node element within a document. @content are optional.
    XMLNodePtr newRawNode(const QString& name, const QString& content, const QString& ns = QString());

    //! Удалить узлы из XML
    static void removeNodes(const QSet<XMLNodePtr>& nodes);

    //! Записать в файл XML
    int saveToFile(const QString& filename, const XML_SaveOptions& options, const QString& encoding = "UTF-8") const;

    //! Не использовать
    XMLDocument(_xmlDoc* doc);
    _xmlDoc* internalData() const;

private:
    _xmlDoc* _doc = nullptr;
};

//! Узел XML
class XMLNode
{
public:
    bool isValid() const;

    /*! type number */
    XML_NodeType type() const;
    /*! the name of the node, or the entity */
    QString name() const;
    //! Пространство имен
    QString ns() const;

    //! Выводит содержимое всех дочерних узлов одной строкой
    QString debug() const;

    //! Есть ли дочерние узлы
    bool hasChildren() const;
    /*! Список дочерних узлов. Формируется динамически, путем перебора через next */
    QList<XMLNodePtr> children() const;
    //! Первый дочерний узел. Использовать для перебора через next
    XMLNodePtr firstChild() const;
    //! Список всех дочерних узлов (не только прямые потомки)
    QList<XMLNodePtr> allChildren() const;
    //! Список всех дочерних узлов (не только прямые потомки). Каждому узлу присваивается уникальный идентификатор в
    //! рамках этого узла. Использовать для маппинга узлов при клонировании
    QMap<int, XMLNodePtr> allChildrenMap() const;
    //! Создать уникальный идентификатор для набора узлов. Включает список всех дочерних узлов для всего списка.
    //! Использовать для маппинга узлов при клонировании
    static QMap<QString, XMLNodePtr> createNodesMap(const QList<XMLNodePtr>& nodes);

    //! Ищет следующий текстовый узел, содержащий указанный текст
    XMLNodePtr findNextContent(const QString& text, zf::CompareOperator op = zf::CompareOperator::Equal,
                               bool stop_at_first_incorrent = false) const;

    /*! last child link */
    XMLNodePtr last() const;
    /*! child->parent link */
    XMLNodePtr parent() const;
    /*! next sibling link  */
    XMLNodePtr next(XML_NodeType type = XML_NodeType::Undefined) const;
    /*! previous sibling link  */
    XMLNodePtr prev(XML_NodeType type = XML_NodeType::Undefined) const;
    /*! the containing document */
    XMLDocumentPtr doc() const;
    /*! the content */
    QString content() const;
    void setContent(const QString& c);
    /*! Список всех свойств. Формируется динамически, путем перебора через next */
    QList<XMLAttrPtr> properties() const;
    //! Первое свойство. Использовать для перебора через next
    XMLAttrPtr firstProperty() const;
    /*! line number */
    int line() const;

    //! Параметры копирования узла
    enum CloneMode
    {
        CloneSimple = 0, // только сам узел без свойств
        CloneRecurcive = 1, // рекурсивно без свойств
        CloneProperties = 2, // рекурсивно, включая свойства
    };
    //! Создать копию узла
    XMLNodePtr clone(CloneMode mode = CloneProperties) const;

    //! Creation of a new node element
    static XMLNodePtr newNode(const QString& name);
    //! Creation of a new text node
    static XMLNodePtr newText(const QString& content);

    //! Сравнить по содержимому и пространству имен (если ns не задано, то оно игнорируемся)
    bool compare(const QString& content, const QString& ns = QString()) const;
    //! Является ли дочерним узлом для указанного узла
    bool isChildOf(const XMLNodePtr& node) const;
    //! Является ли родительским узлом для указанного узла
    bool isParentOf(const XMLNodePtr& node) const;
    //! Найти родительский узел по имени
    XMLNodePtr parentByName(const QString& name,
        //! Пространство имен
        const QString& ns = QString(),
        /*! Пары [пространство имен,имя узла] при нахождении которых поиск надо прекратить
         * Пространство имен можно задать пустым, тогда оно будет игнорироваться */
        const QList<QPair<QString, QString>>& stop_if_found = QList<QPair<QString, QString>>()) const;
    //! Найти родительский узел, у которого родитель совпадает с node
    XMLNodePtr parentBySameParent(const XMLNodePtr& node) const;

    /*! Creation of a new child element, added at the end of this node children list. If @content is non
     * NULL, a child list containing the TEXTs and ENTITY_REFs node will be created. NOTE: @content is supposed to be a
     * piece of XML CDATA, so it allows entity references. XML special chars must be escaped first, or newTextChild()
     * should be used. */
    XMLNodePtr newChild(const QString& name, const QString& content);
    /*! Creation of a new child element, added at the end of this node children list. If @content is non
     * NULL, a child TEXT node will be created containing the string @content. NOTE: Use newChild() if @content will
     * contain entities that need to be preserved. Use this function, if you need to ensure that
     * reserved XML chars that might appear in @content, such as the ampersand, greater-than or less-than signs, are
     * automatically replaced by their XML escaped entity representations.*/
    XMLNodePtr newTextChild(const QString& name, const QString& content);
    /*! Add a list of node at the end of the child list of the parent merging adjacent TEXT nodes
     * Returns:	the last child */
    XMLNodePtr addChild(const XMLNodePtr& node);
    /*! Add a new node @elem as the next sibling of this node. If the new node was already inserted in a document it is
     * first unlinked from its existing context. As a result of text merging @elem may be freed. If the new node is
     * ATTRIBUTE, it is added into properties instead of children. If there is an attribute with equal name, it is first
     * destroyed. */
    XMLNodePtr addNextSibling(const XMLNodePtr& elem);
    /*! Add a new node @elem as the previous sibling of this node, merging adjacent TEXT nodes (@elem may be freed) If
     * the new node was already inserted in a document it is first unlinked from its existing context. If the new node
     * is ATTRIBUTE, it is added into properties instead of children. If there is an attribute with equal name, it is
     * first destroyed. */
    XMLNodePtr addPrevSibling(const XMLNodePtr& elem);
    /*! Add a new element @elem to the list of siblings of this node, merging adjacent TEXT nodes (@elem may be freed)
     * If the new element was already inserted in a document it is first unlinked from its existing context. */
    XMLNodePtr addSibling(const XMLNodePtr& elem);
    //! Create a new property carried by a node
    XMLAttrPtr newProp(const QString& name, const QString& value);
    //! Содержит указанное свойство
    bool containsProp(const QString& name) const;

    //! Удалить узел из дерева и из памяти. После этого содержимое этого объекта станет невалидным
    void remove();
    //! Удалить узел из дерева, но оставить в памяти
    void unlink();

    //! Не использовать
    XMLNode(_xmlNode* node);
    _xmlNode* internalData() const;

private:
    void allChildrenMapHelper(QMap<int, XMLNodePtr>& map, int& id) const;
    //! Ищет следующий текстовый узел, содержащий указанный текст
    static _xmlNode* findContentHelper(_xmlNode* node, const QString& text, zf::CompareOperator op, bool only_children,
                                       bool stop_at_first_incorrent, bool& stop);

    _xmlNode* _node = nullptr;

#if !defined(RELEASE_MODE) && defined(QT_DEBUG)
    QString _ns;
    QString _name;
    QString _content;
#endif

    friend class XMLDocument;
};

//! Атрибут XML
class XMLAttr
{
public:
    XMLAttr() {}

    /*! XML_ATTRIBUTE_NODE */
    XML_ElementType type() const;
    /*! the name of the property */
    QString name() const;
    /*! Список дочерних узлов. Формируется динамически, путем перебора через next */
    QList<XMLNodePtr> children() const;
    //! Первый дочерний узел. Использовать для перебора через next
    XMLNodePtr firstChild() const;
    /*! child->parent link */
    XMLNodePtr parent() const;
    /*! next sibling link  */
    XMLAttrPtr next() const;
    /*! previous sibling link  */
    XMLAttrPtr prev() const;
    /*! the containing document */
    XMLDocumentPtr doc() const;
    /*! the attribute type if validating */
    XML_AttributeType atype() const;

    //! Не использовать
    XMLAttr(_xmlAttr* attr);
    _xmlAttr* internalData() const;

private:
    _xmlAttr* _attr = nullptr;
};

inline bool operator==(const XMLDocumentPtr& n1, const XMLDocumentPtr& n2)
{
    if (n1 == nullptr && n2 == nullptr)
        return true;
    if (n1 == nullptr || n2 == nullptr)
        return false;

    return n1->internalData() == n2->internalData();
}

inline bool operator!=(const XMLDocumentPtr& n1, const XMLDocumentPtr& n2)
{
    return !operator==(n1, n2);
}

inline bool operator==(const XMLNodePtr& n1, const XMLNodePtr& n2)
{
    if (n1 == nullptr && n2 == nullptr)
        return true;
    if (n1 == nullptr || n2 == nullptr)
        return false;

    return n1->internalData() == n2->internalData();
}

inline bool operator!=(const XMLNodePtr& n1, const XMLNodePtr& n2)
{
    return !operator==(n1, n2);
}

inline bool operator==(const XMLAttrPtr& n1, const XMLAttrPtr& n2)
{
    if (n1 == nullptr && n2 == nullptr)
        return true;
    if (n1 == nullptr || n2 == nullptr)
        return false;

    return n1->internalData() == n2->internalData();
}

inline bool operator!=(const XMLAttrPtr& n1, const XMLAttrPtr& n2)
{
    return !operator==(n1, n2);
}
} // namespace zf

// на основании https://github.com/KDE/clazy/blob/1.10/docs/checks/README-qhash-namespace.md
namespace std
{
inline uint qHash(const zf::XMLDocumentPtr& n)
{
    return n == nullptr ? ::qHash(0) : ::qHash(n->internalData());
}

inline uint qHash(const zf::XMLNodePtr& n)
{
    return n == nullptr ? ::qHash(0) : ::qHash(n->internalData());
}

inline uint qHash(const zf::XMLAttrPtr& n)
{
    return n == nullptr ? ::qHash(0) : ::qHash(n->internalData());
}
} // namespace std
