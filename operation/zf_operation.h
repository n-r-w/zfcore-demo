#ifndef ZF_OPERATION_H
#define ZF_OPERATION_H

#include "zf_error.h"
#include "zf_core_consts.h"
#include <QSharedDataPointer>
#include <QIcon>

class QMenu;
class QAction;

namespace zf
{
class Operation_SharedData;
class View;

class Operation;

typedef QList<Operation> OperationList;

//! Описание операции
class ZCORESHARED_EXPORT Operation
{
public:
    Operation();
    Operation(const Operation& operation);
    //! Использовать в конструкторе плагина
    Operation(
        //! Код сущности (модуля)
        const EntityCode& entity_code,
        //! Идентификатор операции
        const OperationID& id,
        //! Область действия операции
        OperationScope scope,
        //! Константа перевода
        const QString& translation_id,
        //! Иконка
        const QIcon& icon = QIcon(),
        //! Константа перевода для короткого имени, используется при отображении на кнопках тулбара
        const QString& short_translation_id = QString(),
        //! Различные параметры операции
        const OperationOptions& options = OperationOption::ShowProgress);
    //! Использовать в конструкторе плагина
    Operation(
        //! Код сущности (модуля)
        const EntityCode& entity_code,
        //! Идентификатор операции
        const OperationID& id,
        //! Область действия операции
        OperationScope scope,
        //! Направление операции
        OperationDirection direction,
        //! Тип операции
        OperationType type,
        //! Константа перевода
        const QString& translation_id,
        //! Иконка
        const QIcon& icon = QIcon(),
        //! Константа перевода для короткого имени, используется при отображении на кнопках тулбара
        const QString& short_translation_id = QString(),
        //! Различные параметры операции
        const OperationOptions& options = OperationOption::ShowProgress);
    ~Operation();

    Operation& operator=(const Operation& operation);
    bool operator==(const Operation& operation) const;
    bool operator!=(const Operation& operation) const;
    bool operator<(const Operation& operation) const;
    OperationList operator<<(const Operation& operation) const;
    OperationList operator+(const Operation& operation) const;
    OperationList operator+(const OperationList& operations) const;

    //! Операция по коду модуля и идентификатору. Нельзя использовать при начальном формировании списка
    //! операций в конструкторе плагина
    static Operation getOperation(const EntityCode& entity_code, const OperationID& operation_id);

    bool isValid() const;

    //! Код модуля
    EntityCode entityCode() const;
    //! Идентификатор операции
    OperationID id() const;
    //! Область действия операции
    OperationScope scope() const;
    //! Направление операции
    OperationDirection direction() const;
    //! Тип операции
    OperationType type() const;
    //! Различные параметры операции
    OperationOptions options() const;
    //! Название
    QString name() const;
    //! Название для отображения на кнопках тулбара
    QString shortName() const;
    //! Код перевода
    QString translationID() const;
    //! Код перевода для отображения на кнопках тулбара
    QString shortTranslationID() const;
    //! Иконка
    QIcon icon() const;

private:
    //! Разделяемые данные
    QSharedDataPointer<Operation_SharedData> _d;
};

inline bool operator==(const Operation& op, const OperationID& id)
{
    return op.isValid() && op.id() == id;
}
inline bool operator!=(const Operation& op, const OperationID& id)
{
    return !operator==(op, id);
}
inline bool operator==(const OperationID& id, const Operation& op)
{
    return op.isValid() && op.id() == id;
}
inline bool operator!=(const OperationID& id, const Operation& op)
{
    return !operator==(op, id);
}

//! Узел меню операций
class ZCORESHARED_EXPORT OperationMenuNode
{
public:
    OperationMenuNode();
    OperationMenuNode(const OperationMenuNode& node);
    OperationMenuNode(
        //! Для узла с операцией не может быть дочерних узлов
        const Operation& operation);
    OperationMenuNode(
        //! Для узла с операцией не может быть дочерних узлов
        const Operation& operation,
        //! Различные параметры операции
        const OperationOptions& options);
    OperationMenuNode(
        //! Идентификатор узла. Нужен для UI Automation
        const QString& accessible_id,
        //! ID перевода текста узла
        const QString& translation_id,
        //! Иконка узла
        const QIcon& icon = QIcon(),
        //! ID сокращенного перевода текста узла
        const QString& short_translation_id = QString(),
        //! Различные параметры операции
        const OperationOptions& options = {});
    OperationMenuNode(
        //! Идентификатор узла. Нужен для UI Automation
        const QString& accessible_id,
        //! ID перевода текста узла
        const QString& translation_id,
        //! Дочерние узлы
        const QList<OperationMenuNode>& children,
        //! Иконка узла
        const QIcon& icon = QIcon(),
        //! ID сокращенного перевода текста узла
        const QString& short_translation_id = QString(),
        //! Различные параметры операции
        const OperationOptions& options = {});

    static OperationMenuNode separator();

    OperationMenuNode& operator=(const OperationMenuNode& node);

    OperationMenuNode operator<<(const OperationMenuNode& node) const;
    OperationMenuNode operator<<(const Operation& operation) const;

    bool isValid() const;
    bool isSeparator() const;

    //! Текст узла (если задано, то название операции игнорируется). Обязательно, если не задана операция
    QString text() const;
    //! Сокращенный текст узла (если задано, то название операции игнорируется). Обязательно, если не задана операция
    QString shortText() const;
    //! Иконка узла (если задано, то иконка операции игнорируется)
    QIcon icon() const;
    //! Операция
    Operation operation() const;
    //! Различные параметры операции
    OperationOptions options() const;
    //! Дочерние узлы
    QList<OperationMenuNode> children() const;
    //! Идентификатор узла. Нужен для UI Automation
    QString accesibleId() const;

    //! Список всех дочерних операций, включая вложенные
    OperationList allOperations() const;

    //! Поиск узла по его accesible_id проверяет этот узел и его дочерние узлы
    const OperationMenuNode* find(const QString& accesible_id) const;

private:
    enum class Type
    {
        Undefined,
        Operation,
        Text,
        Separator
    };

    OperationMenuNode(
        //! Различные параметры операции. Сейчас используется только ShowButtonText
        const OperationOptions& options,
        //! Тип узла
        Type type,
        // Идентификатор узла.Нужен для UI Automation
        const QString& accessible_id,
        //! ID перевода текста узла
        const QString& translation_id,
        //! ID сокращенного перевода текста узла
        const QString& short_translation_id,
        //! Иконка узла (если задано, то иконка операции игнорируется)
        const QIcon& icon,
        //! Операция
        const Operation& operation,
        //! Дочерние узлы
        const QList<OperationMenuNode>& children);

    OperationOptions _options;
    //! Принудительно использовать _options и игнорировать options операции
    bool _options_force = false;

    Type _type = Type::Undefined;
    //! Идентификатор узла. Нужен для UI Automation
    QString _accesible_id;
    //! ID перевода текста узла (если задано, то название операции игнорируется). Обязательно, если не задана операция
    QString _translation_id;
    //! ID сокращенного перевода текста узла (если задано, то название операции игнорируется). Обязательно, если не задана операция
    QString _short_translation_id;
    //! Иконка узла (если задано, то иконка операции игнорируется)
    QIcon _icon;
    //! Операция
    Operation _operation;
    //! Дочерние узлы
    QList<OperationMenuNode> _children;
};

//! Меню операций
class ZCORESHARED_EXPORT OperationMenu
{
public:
    OperationMenu();
    explicit OperationMenu(const QList<OperationMenuNode>& root);
    OperationMenu(const OperationMenu& menu);
    OperationMenu(const OperationMenuNode& node);
    OperationMenu(const Operation& operation);
    OperationMenu(const OperationList& operations);
    OperationMenu& operator=(const OperationMenu& menu);
    OperationMenu& operator=(const OperationMenuNode& node);
    OperationMenu& operator=(const Operation& operation);

    //! Верхний уровень меню
    QList<OperationMenuNode> root() const;

    //! Список всех дочерних операций, включая вложенные
    OperationList allOperations() const;

    OperationMenu& operator<<(const OperationMenu& menu);
    OperationMenu& operator<<(const OperationMenuNode& node);
    OperationMenu& operator<<(const Operation& operation);

    OperationMenu& operator+=(const OperationMenu& menu);
    OperationMenu& operator+=(const OperationMenuNode& node);
    OperationMenu& operator+=(const Operation& operation);

    //! Создать QMenu на основе OperationMenu. Только для меню с операциями OperationScope::Module
    QMenu* createQtMenu(QWidget* parent,
        //! Экшены для каждой операции
        QMap<Operation, QAction*>& actions) const;

    //! Создать разделитель
    static OperationMenuNode separator();

    //! Поиск узла по его accesible_id
    const OperationMenuNode* findNode(const QString& accesible_id) const;

private:
    static void createQtMenuHelper(const OperationMenuNode& node, QMenu* parent, QMap<Operation, QAction*>& actions);

    QList<OperationMenuNode> _root;
};

inline OperationMenu operator+(const Operation& operation, const OperationMenuNode& node)
{
    return OperationMenu(QList<OperationMenuNode>({OperationMenuNode(operation)}) + QList<OperationMenuNode>({node}));
}
inline OperationList operator+(const OperationList& operation1, const Operation& operation2)
{
    return OperationList(operation1) + OperationList({operation2});
}

inline OperationMenu operator+(const OperationMenuNode& node1, const OperationMenuNode& node2)
{
    return OperationMenu(QList<OperationMenuNode>({node1}) + QList<OperationMenuNode>({node2}));
}
inline OperationMenu operator+(const OperationMenuNode& node, const Operation& operation)
{
    return OperationMenu(QList<OperationMenuNode>({node}) + QList<OperationMenuNode>({OperationMenuNode(operation)}));
}

inline OperationMenu operator+(const OperationMenu& menu, const OperationMenuNode& node)
{
    return OperationMenu(menu.root() + QList<OperationMenuNode>({node}));
}
inline OperationMenu operator+(const OperationMenu& menu, const Operation& operation)
{
    return OperationMenu(menu.root() + QList<OperationMenuNode>({OperationMenuNode(operation)}));
}

inline OperationMenu operator+(const OperationMenu& menu1, const OperationMenu& menu2)
{
    return OperationMenu(menu1.root() + menu2.root());
}

inline uint qHash(const Operation& operation)
{
    return ::qHash(QString::number(operation.entityCode().value()) + Consts::KEY_SEPARATOR + QString::number(operation.id().value()));
}

} // namespace zf

Q_DECLARE_METATYPE(zf::Operation)
Q_DECLARE_METATYPE(zf::OperationMenuNode)
Q_DECLARE_METATYPE(zf::OperationMenu)

ZCORESHARED_EXPORT QDebug operator<<(QDebug debug, const zf::Operation& c);

#endif // ZF_OPERATION_H
