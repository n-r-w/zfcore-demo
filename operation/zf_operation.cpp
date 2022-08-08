#include "zf_operation.h"
#include "zf_core.h"
//#include "yaml.h"
#include <QIcon>
#include <QMenu>

namespace zf
{
//! Данные для Operation
class Operation_SharedData : public QSharedData
{
public:
    Operation_SharedData() {}
    Operation_SharedData(const Operation_SharedData& d);
    ~Operation_SharedData();

    void copyFrom(const Operation_SharedData* d)
    {
        entity_code = d->entity_code;
        id = d->id;
        scope = d->scope;
        direction = d->direction;
        type = d->type;
        options = d->options;
        translation_id = d->translation_id;
        short_translation_id = d->short_translation_id;
        icon = d->icon;
    }

    EntityCode entity_code;
    OperationID id;
    OperationScope scope = OperationScope::Undefined;
    OperationDirection direction = OperationDirection::Undefined;
    OperationType type = OperationType::Undefined;
    OperationOptions options;
    QString translation_id;
    QString short_translation_id;
    QIcon icon;
};

Operation_SharedData::Operation_SharedData(const Operation_SharedData& d)
    : QSharedData(d)
{
    copyFrom(&d);
}

Operation_SharedData::~Operation_SharedData()
{
}

Operation::Operation()
    : _d(new Operation_SharedData())
{
}

Operation::Operation(const Operation& operation)
    : _d(operation._d)
{
}

Operation Operation::getOperation(const EntityCode& entity_code, const OperationID& operation_id)
{
    I_Plugin* plugin = Core::getPlugin(entity_code);
    Operation op = plugin->operation(operation_id);
    Z_CHECK_X(op.isValid(), QString("operation %1 not found in module %2").arg(operation_id.value()).arg(entity_code.value()));
    return op;
}

Operation::Operation(const EntityCode& entity_code, const OperationID& id, OperationScope scope, OperationDirection direction,
                     OperationType type, const QString& translation_id, const QIcon& icon, const QString& short_translation_id, const OperationOptions& options)
    : _d(new Operation_SharedData())
{
    Z_CHECK(entity_code.isValid() && scope != OperationScope::Undefined && direction != OperationDirection::Undefined
            && type != OperationType::Undefined && !translation_id.isEmpty());

    _d->entity_code = entity_code;
    _d->id = id;
    _d->scope = scope;
    _d->direction = direction;
    _d->type = type;
    _d->options = options;
    _d->translation_id = translation_id;
    _d->short_translation_id = short_translation_id;
    _d->icon = icon;    
}
Operation::Operation(const EntityCode& entity_code, const OperationID& id, OperationScope scope, const QString& translation_id,
                     const QIcon& icon, const QString& short_translation_id, const OperationOptions& options)
    : Operation(entity_code, id, scope, OperationDirection::General, OperationType::General, translation_id, icon,short_translation_id, options)
{
}

Operation::~Operation()
{
}

Operation& Operation::operator=(const Operation& operation)
{
    if (this != &operation)
        _d = operation._d;
    return *this;
}

bool Operation::operator==(const Operation& operation) const
{
    return entityCode() == operation.entityCode() && id() == operation.id();
}

bool Operation::operator!=(const Operation& operation) const
{
    return !operator==(operation);
}

bool Operation::operator<(const Operation& operation) const
{
    if (entityCode() == operation.entityCode())
        return id() < operation.id();
    else
        return entityCode() < operation.entityCode();
}

OperationList Operation::operator<<(const Operation& operation) const
{
    return OperationList {*this} << operation;
}

OperationList Operation::operator+(const Operation& operation) const
{
    return operator<<(operation);
}

OperationList Operation::operator+(const OperationList& operations) const
{
    return OperationList({*this}) + operations;
}

bool Operation::isValid() const
{
    return _d->scope != OperationScope::Undefined;
}

EntityCode Operation::entityCode() const
{
    return _d->entity_code;
}

OperationID Operation::id() const
{
    return _d->id;
}

OperationScope Operation::scope() const
{
    return _d->scope;
}

OperationDirection Operation::direction() const
{
    return _d->direction;
}

OperationType Operation::type() const
{
    return _d->type;
}

OperationOptions Operation::options() const
{
    return _d->options;
}

QString Operation::name() const
{
    return Translator::translate(translationID());
}

QString Operation::shortName() const
{
    return Translator::translate(shortTranslationID());
}

QString Operation::translationID() const
{
    return _d->translation_id;
}

QString Operation::shortTranslationID() const
{
    return _d->short_translation_id.isEmpty()? _d->translation_id : _d->short_translation_id;
}

QIcon Operation::icon() const
{
    return _d->icon;
}

OperationMenuNode::OperationMenuNode()
{
}

OperationMenuNode::OperationMenuNode(const OperationMenuNode& node)
    : _options(node._options)
    , _options_force(node._options_force)
    , _type(node._type)
    , _accesible_id(node._accesible_id)
    , _translation_id(node._translation_id)
    , _short_translation_id(node._short_translation_id)
    , _icon(node._icon)
    , _operation(node._operation)
    , _children(node._children)
{
}

OperationMenuNode::OperationMenuNode(const Operation& operation)
    : OperationMenuNode({}, Type::Operation, QString(), QString(), QString(), QIcon(), operation, {})
{
}

OperationMenuNode::OperationMenuNode(const Operation& operation, const OperationOptions& options)
    : OperationMenuNode(options, Type::Operation, QString(), QString(), QString(), QIcon(), operation, {})
{
    _options_force = true;
}

OperationMenuNode::OperationMenuNode(const QString& accessible_id, const QString& translation_id, const QIcon& icon, const QString& short_translation_id, const OperationOptions& options)
    : OperationMenuNode(options, Type::Text, accessible_id, translation_id, short_translation_id.isEmpty() ? translation_id : short_translation_id, icon, Operation(), {})
{
}


OperationMenuNode::OperationMenuNode(const QString& accessible_id, const QString& translation_id,
                                     const QList<OperationMenuNode>& children, const QIcon& icon, const QString& short_translation_id, const OperationOptions& options)
    : OperationMenuNode(options, Type::Text, accessible_id, translation_id, short_translation_id.isEmpty() ? translation_id : short_translation_id, icon, Operation(), children)
{
}

OperationMenuNode OperationMenuNode::separator()
{
    return OperationMenuNode({}, Type::Separator, QString(), QString(), QString(), QIcon(), Operation(), {});
}

OperationMenuNode::OperationMenuNode(const OperationOptions& options, Type type, const QString& accessible_id,
                                     const QString& translation_id, const QString& short_translation_id, const QIcon& icon,
                                     const Operation& operation, const QList<OperationMenuNode>& children)
    : _options(options)
    , _type(type)
    , _accesible_id(accessible_id)
    , _translation_id(translation_id)
    , _short_translation_id(short_translation_id)
    , _icon(icon)
    , _operation(operation)
    , _children(children)
{
    Z_CHECK(isValid());
    if (type == Type::Operation) {
        Z_CHECK(operation.isValid());
        Z_CHECK(children.isEmpty());
        Z_CHECK(accessible_id.isEmpty());
        Z_CHECK(translation_id.isEmpty());
        Z_CHECK(short_translation_id.isEmpty());

    } else if (type == Type::Text) {
        Z_CHECK(!operation.isValid());
        Z_CHECK(!accessible_id.isEmpty());
        Z_CHECK(!translation_id.isEmpty());
        Z_CHECK(!short_translation_id.isEmpty());
    }

    for (auto& o : children) {
        if (operation.isValid() && o.operation().isValid())
            Z_CHECK(o.operation().id() != operation.id());
    }
}

OperationMenuNode& OperationMenuNode::operator=(const OperationMenuNode& node)
{
    _type = node._type;
    _accesible_id = node._accesible_id;
    _translation_id = node._translation_id;
    _short_translation_id = node._short_translation_id;
    _icon = node._icon;
    _operation = node._operation;
    _children = node._children;
    _options = node._options;
    _options_force = node._options_force;
    return *this;
}

OperationMenuNode OperationMenuNode::operator<<(const OperationMenuNode& node) const
{
    return OperationMenuNode(_options, _type, _accesible_id, _translation_id, _short_translation_id, _icon, _operation,
                             QList<OperationMenuNode> {_children} + QList<OperationMenuNode> {node});
}

OperationMenuNode OperationMenuNode::operator<<(const Operation& operation) const
{
    return operator<<(OperationMenuNode(operation));
}

bool OperationMenuNode::isValid() const
{
    return _type != Type::Undefined;
}

bool OperationMenuNode::isSeparator() const
{
    return _type == Type::Separator;
}

QString OperationMenuNode::text() const
{
    if (_type == Type::Operation)
        return _operation.name();

    if (_type == Type::Text)
        return Translator::translate(_translation_id);

    return QString();
}

QString OperationMenuNode::shortText() const
{
    if (_type == Type::Operation)
        return _operation.shortName();

    if (_type == Type::Text)
        return Translator::translate(_short_translation_id);

    return QString();
}

QIcon OperationMenuNode::icon() const
{
    return _icon.isNull() ? _operation.icon() : _icon;
}

Operation OperationMenuNode::operation() const
{
    return _operation;
}

OperationOptions OperationMenuNode::options() const
{
    if (_options_force)
        return _options;

    if (_type == Type::Operation)
        return _operation.options();

    if (_type == Type::Text)
        return _options;

    return {};
}

QList<OperationMenuNode> OperationMenuNode::children() const
{
    return _children;
}

QString OperationMenuNode::accesibleId() const
{
    if (_type == Type::Operation)
        return QStringLiteral("%1_%2").arg(operation().entityCode().value()).arg(_operation.id().value());

    if (_type == Type::Text)
        return _accesible_id;

    return QString();
}

OperationList OperationMenuNode::allOperations() const
{
    OperationList children;

    if (operation().isValid())
        children << operation();

    for (const auto& c : qAsConst(_children)) {
        children << c.allOperations();
    }

    return children;
}

const OperationMenuNode* OperationMenuNode::find(const QString& accesible_id) const
{
    Z_CHECK(!accesible_id.isEmpty());

    if (_accesible_id == accesible_id)
        return this;

    for (const auto& c : qAsConst(_children)) {
        auto res = c.find(accesible_id);
        if (res != nullptr)
            return res;
    }

    return nullptr;
}

OperationMenu::OperationMenu()
{
}

OperationMenu::OperationMenu(const QList<OperationMenuNode>& root)
    : _root(root)
{
}

OperationMenu::OperationMenu(const OperationMenu& menu)
    : _root(menu._root)
{
}

OperationMenu::OperationMenu(const OperationMenuNode& node)
    : _root(QList<OperationMenuNode> {node})
{
}

OperationMenu::OperationMenu(const Operation& operation)
    : _root(QList<OperationMenuNode> {OperationMenuNode(operation)})
{
}

OperationMenu::OperationMenu(const OperationList& operations)
{
    QList<OperationMenuNode> list;
    for (auto& o : operations) {
        list << OperationMenuNode(o);
    }

    *this = OperationMenu(list);
}

OperationMenu& OperationMenu::operator=(const OperationMenu& menu)
{
    _root = menu._root;
    return *this;
}

OperationMenu& OperationMenu::operator=(const OperationMenuNode& node)
{
    _root = QList<OperationMenuNode> {node};
    return *this;
}

OperationMenu& OperationMenu::operator=(const Operation& operation)
{
    _root = QList<OperationMenuNode> {OperationMenuNode(operation)};
    return *this;
}

QList<OperationMenuNode> OperationMenu::root() const
{
    return _root;
}

OperationList OperationMenu::allOperations() const
{
    OperationList children;
    for (const auto& c : qAsConst(_root)) {
        children << c.allOperations();
    }

    return children;
}

OperationMenu& OperationMenu::operator<<(const OperationMenu& menu)
{
    *this = *this + menu;
    return *this;
}

OperationMenu& OperationMenu::operator<<(const OperationMenuNode& node)
{
    *this = *this + node;
    return *this;
}

OperationMenu& OperationMenu::operator<<(const Operation& operation)
{
    *this = *this + operation;
    return *this;
}

OperationMenu& OperationMenu::operator+=(const OperationMenu& menu)
{
    *this = *this + menu;
    return *this;
}

OperationMenu& OperationMenu::operator+=(const OperationMenuNode& node)
{
    *this = *this + node;
    return *this;
}

OperationMenu& OperationMenu::operator+=(const Operation& operation)
{
    *this = *this + operation;
    return *this;
}

QMenu* OperationMenu::createQtMenu(QWidget* parent, QMap<Operation, QAction*>& actions) const
{
    Z_CHECK(actions.isEmpty());

    QMenu* menu = new QMenu(parent);
    for (auto& n : _root) {
        createQtMenuHelper(n, menu, actions);
    }
    return menu;
}

OperationMenuNode OperationMenu::separator()
{
    return OperationMenuNode::separator();
}

const OperationMenuNode* OperationMenu::findNode(const QString& accesible_id) const
{
    for (auto& n : _root) {
        auto res = n.find(accesible_id);
        if (res != nullptr)
            return res;
    }
    return nullptr;
}

void OperationMenu::createQtMenuHelper(const OperationMenuNode& node, QMenu* parent, QMap<Operation, QAction*>& actions)
{
    Z_CHECK_NULL(parent);

    if (node.children().isEmpty()) {
        QAction* action;
        if (node.operation().isValid()) {
            action = parent->addAction(node.operation().icon(), node.text());
            action->setData(QVariant::fromValue(node.operation()));
            actions[node.operation()] = action;
        } else {
            action = parent->addAction(node.text());
            action->setIcon(node.icon());
        }

    } else {
        QMenu* sub_menu = parent->addMenu(node.text());
        sub_menu->setIcon(node.icon());
        for (auto& child : node.children()) {
            createQtMenuHelper(child, sub_menu, actions);
        }
    }
}

} // namespace zf

QDebug operator<<(QDebug debug, const zf::Operation& c)
{
    using namespace zf;

    QDebugStateSaver saver(debug);
    debug.noquote().nospace();

    if (!c.isValid()) {
        debug << "invalid";

    } else {
        debug << QStringLiteral("%1(%2)").arg(c.name()).arg(c.id().value());
        debug << '\n';

        Core::beginDebugOutput();
        debug << Core::debugIndent() << "entityCode: " << c.entityCode() << '\n';
        debug << Core::debugIndent() << "id:" << c.id() << '\n';
        debug << Core::debugIndent() << "scope: " << c.scope() << '\n';
        debug << Core::debugIndent() << "direction: " << c.direction() << '\n';
        debug << Core::debugIndent() << "type: " << c.type() << '\n';
        debug << Core::debugIndent() << "options: " << c.options() << '\n';
        debug << Core::debugIndent() << "name: " << c.name() << '\n';
        debug << Core::debugIndent() << "translationID: " << c.translationID() << '\n';
        debug << Core::debugIndent() << "shortTranslationID: " << c.shortTranslationID() << '\n';
        debug << Core::debugIndent() << "icon: " << !c.icon().isNull();
        zf::Core::endDebugOutput();
    }

    return debug;
}
