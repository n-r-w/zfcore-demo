#pragma once

#include <QObject>

#include "zf.h"
#include "zf_data_structure.h"
#include "zf_model.h"

namespace zf
{
class ModuleDataObject;
class View;

//! Условие, накладываемое на свойства сущности
class ZCORESHARED_EXPORT Condition
{
public:
    ~Condition();

    //! Не содержит условий
    bool isEmpty() const;

    //! Уникальный идентификатор
    QString id() const;

    //! Вид условия
    ConditionType type() const;
    //! Текстовое представление вида условия
    QString typeToText() const;

    //! Корневое условие
    bool isRoot() const;
    //! Логическое условия (Or/And)
    bool isLogical() const;
    //! Сравнение двух свойств
    bool isPropertyCompare() const;
    //! Сравнение поля со значением вне зависимости от того, имеет оно lookup или нет
    bool isRawCompare() const;

    //! Поле для проверки
    DataProperty source() const;
    //! Преобразование поля проверки
    ConversionType sourceConversion() const;

    //! Условие проверки
    CompareOperator compareOperator() const;
    //! Текстовое представление оператора сравнения
    QString compareOperatorText(
        //! Для использования в JavaScript
        bool for_java_script = false,
        //! Полное название (если for_java_script==true, то игнорируется)
        bool full_name = true) const;

    //! Значение сравнения
    QVariant targetValue() const;

    //! Поле, с которым идет сравнение
    DataProperty targetProperty() const;
    //! Преобразование поля, с которым идет сравнение
    ConversionType targetConversion() const;

    //! Родитель
    std::shared_ptr<Condition> parent() const;
    //! Вложенные условия
    const QList<std::shared_ptr<Condition>>& children() const;

    //! Поиск вложенного условия по его id
    std::shared_ptr<Condition> condition(const QString& id, bool recursive = true) const;

    //! Вычислить условие для указанных данных
    bool calculateOnData(
        //! Откуда брать данные
        const ModuleDataObject* data,
        //! Список моделей, данные которых нужны, но не загружены
        QList<ModelPtr>& data_not_ready,
        //! Ошибка
        Error& error,
        //! Список строк для условий, накладываемых на колонки. Тип свойства должен быть PropertyType::RowFull
        const DataPropertyList& rows = {}) const;
    //! Вычислить условие для указанного представления. Учитывает фактическое отображение данных в представлении
    bool calculateOnView(
        //! Откуда брать данные
        const View* view,
        //! Список моделей, данные которых нужны, но не загружены
        QList<ModelPtr>& data_not_ready,
        //! Ошибка
        Error& error,
        //! Список строк для условий, накладываемых на колонки. Тип свойства должен быть PropertyType::RowFull
        const DataPropertyList& rows = {}) const;

    //! Проверить на ошибки с учетом подчиненных
    bool isValid() const;
    //! Вид ошибки
    enum class ErrorType
    {
        Type,
        Source,
        SourceConversion,
        Operator,
        Target,
        TargetConversion
    };
    //! Виды ошибки этого узла
    QList<ErrorType> errors() const;

private:
    Condition();
    Condition(const std::shared_ptr<Condition>& parent, ConditionType type, ConversionType source_conversion, const DataProperty& source,
        CompareOperator compare_operator, ConversionType target_conversion, const DataProperty& target_property,
        const QVariant& target_value, bool _is_property_compare, bool validation);

    void clear();

    //! Вычислить условие для указанного представления
    bool calculate(
        //! Откуда брать данные
        const ModuleDataObject* data,
        //! Откуда брать данные (Учитывает фактическое отображение данных в представлении)
        const View* view,
        //! Список строк для условий, накладываемых на колонки. Тип свойства должен быть PropertyType::RowFull
        const DataPropertyList& rows,
        //! Список моделей, данные которых нужны, но не загружены
        QList<ModelPtr>& data_not_ready,
        //! Ошибка
        Error& error) const;

    //! Вычислить это условие для указанных данных (без вложенных)
    bool calculateSelf(
        //! Откуда брать данные
        const ModuleDataObject* data,
        //! Откуда брать данные (Учитывает фактическое отображение данных в представлении)
        const View* view,
        //! Список строк для условий, накладываемых на колонки. Тип свойства должен быть PropertyType::RowFull
        const DataPropertyList& rows,
        //! Список моделей, данные которых нужны, но не загружены
        QList<ModelPtr>& data_not_ready,
        //! Ошибка
        Error& error) const;
    //! Проверить на ошибки (без вложенных)
    bool isValidSelf() const;

    //! Вычислить значение
    static QVariant calculateValue(const ModuleDataObject* data, const View* view, const DataPropertyList& rows,
                                   const DataProperty& property, ConversionType conversion, QList<ModelPtr>& data_not_ready, Error& error);
    //! Преобразовать значение
    static QVariant convertValue(const ModuleDataObject* data, const View* view, const DataProperty& property,
        ConversionType conversion, const QVariant& value);

    //! Очистить вложенные и собрать их идентификаторы начиная с самого нижнего
    void clearChildren(QStringList& ids);

    //! Сбросить на значения по умолчнию для текущего типа условия
    void resetToDefault();

    //! Уникальный идентификатор
    QString _id;
    ConditionType _type = ConditionType::Undefined;
    //! Сравнение двух свойств
    bool _is_property_compare = false;

    ConversionType _source_conversion = ConversionType::Undefined;
    DataProperty _source;

    CompareOperator _compare_operator = CompareOperator::Undefined;

    ConversionType _target_conversion = ConversionType::Undefined;
    DataProperty _target_property;
    QVariant _target_value;

    //! Родитель
    std::weak_ptr<Condition> _parent;
    //! Вложенные условия
    QList<std::shared_ptr<Condition>> _children;

    friend class ComplexCondition;
    friend QDataStream& operator<<(QDataStream& out, const Condition& obj);
    friend QDataStream& operator>>(QDataStream& in, const std::shared_ptr<Condition>& obj);
};

typedef std::shared_ptr<Condition> ConditionPtr;

QDataStream
#ifndef _MSC_VER
    ZCORESHARED_EXPORT
#endif
        &
    operator<<(QDataStream& out, const Condition& obj);

QDataStream
#ifndef _MSC_VER
    ZCORESHARED_EXPORT
#endif
        &
    operator>>(QDataStream& in, const ConditionPtr& obj);

//! Составное условие
class ZCORESHARED_EXPORT ComplexCondition : public QObject
{
    Q_OBJECT
public:
    ComplexCondition();

    //! Корневое условие. По умолчанию ConditionType::And
    ConditionPtr root() const;

    bool isEmpty() const;

    //! Добавить логическое условие (ConditionType::And или ConditionType::Or)
    ConditionPtr addLogical(ConditionType type, const ConditionPtr& parent = nullptr);

    //! Добавить условие на обязательное наличие данных в поле
    ConditionPtr addRequired(
        const DataProperty& property, ConversionType conversion = ConversionType::Direct, const ConditionPtr& parent = nullptr);

    //! Добавить условие на сравнение поля со значением
    ConditionPtr addCompare(const DataProperty& property, CompareOperator op, const QVariant& value,
        ConversionType conversion = ConversionType::Direct, const ConditionPtr& parent = nullptr);

    //! Добавить условие на сравнение двух полей
    ConditionPtr addCompare(const DataProperty& source_property, CompareOperator op, const DataProperty& target_property,
        ConversionType source_conversion = ConversionType::Direct, ConversionType target_conversion = ConversionType::Direct,
        const ConditionPtr& parent = nullptr);

    //! Изменить условие на логическое условие (ConditionType::And или ConditionType::Or)
    ConditionPtr updateLogical(
        //! Изменяемое условие
        const ConditionPtr& condition, ConditionType type);

    //! Изменить условие на обязательное наличие данных в поле
    ConditionPtr updateRequired(
        //! Изменяемое условие
        const ConditionPtr& condition, const DataProperty& property, ConversionType conversion = ConversionType::Direct);

    //! Изменить условие на сравнение поля со значением
    ConditionPtr updateCompare(
        //! Изменяемое условие
        const ConditionPtr& condition, const DataProperty& property, CompareOperator op, const QVariant& value,
        ConversionType conversion = ConversionType::Direct);

    //! Изменить условие на сравнение двух полей
    ConditionPtr updateCompare(
        //! Изменяемое условие
        const ConditionPtr& condition, const DataProperty& source_property, CompareOperator op, const DataProperty& target_property,
        ConversionType source_conversion = ConversionType::Direct, ConversionType target_conversion = ConversionType::Direct);

    //! Удалить условие (только для дочерних условий)
    void remove(const ConditionPtr& c);
    //! Удалить вложенные условия
    void removeChildren(const ConditionPtr& c);

    //! Добавить пустое условие (использовать только при validation == false)
    ConditionPtr addEmpty(const ConditionPtr& parent = nullptr);

    //! Поиск условия по id
    ConditionPtr condition(const QString& id) const;

    //! Вычислить условие для указанных данных
    bool calculateOnData(
        //! Откуда брать данные
        const ModuleDataObject* data,
        //! Список моделей, данные которых нужны, но не загружены
        QList<ModelPtr>& data_not_ready,
        //! Ошибка
        Error& error,
        //! Список строк для условий, накладываемых на колонки. Тип свойства должен быть PropertyType::RowFull
        const DataPropertyList& rows = {}) const;
    //! Вычислить условие для указанного представления. Учитывает фактическое отображение данных в представлении
    bool calculateOnView(
        //! Откуда брать данные
        const View* view,
        //! Список моделей, данные которых нужны, но не загружены
        QList<ModelPtr>& data_not_ready,
        //! Ошибка
        Error& error,
        //! Список строк для условий, накладываемых на колонки. Тип свойства должен быть PropertyType::RowFull
        const DataPropertyList& rows = {}) const;

    //! Очистка всех условий
    void clear();

    //! Скопировать содержимое из указанного объекта
    void copyFrom(const ComplexCondition* c,
        //! Усли истина, то будут генерироваться сигналы для каждой операции копирования, иначе только один в конце
        bool interactive = false);

    //! Создать условие для указанного свойства на основании его ограничений
    static std::shared_ptr<ComplexCondition> createForProperty(const DataProperty& property);

    //! Проводить ли проверку на ошибки при добавлении
    bool validation() const;
    //! Проводить ли проверку на ошибки при добавлении
    void setValidation(bool b);

    //! Проверить на ошибки
    bool isValid() const;
    //! Найти все некорректные условия
    QList<ConditionPtr> findInvalidConditions() const;

signals:
    //! Изменилось условие
    void sg_changed(const QString& id);
    //! Добавлено условие
    void sg_inserted(const QString& id);
    //! Удалено условие
    void sg_removed(const QString& id);

private:
    ConditionPtr addHelper(Condition* p);
    void copyConditionHelper(ConditionPtr parent, ConditionPtr source);
    void removeChildrenHelper(Condition* condition);
    //! Найти все некорректные условия
    static void findInvalidConditionsHelper(QList<ConditionPtr>& errors, const ConditionPtr& parent);

    ConditionPtr _root;
    //! Проверка на ошибки при добавлении
    bool _validation = true;

    friend QDataStream& operator<<(QDataStream& out, const ComplexCondition& obj);
    friend QDataStream& operator>>(QDataStream& in, ComplexCondition& obj);
};

typedef std::shared_ptr<ComplexCondition> ComplexConditionPtr;

QDataStream
#ifndef _MSC_VER
    ZCORESHARED_EXPORT
#endif
        &
    operator<<(QDataStream& out, const ComplexCondition& obj);

QDataStream
#ifndef _MSC_VER
    ZCORESHARED_EXPORT
#endif
        &
    operator>>(QDataStream& in, ComplexCondition& obj);

//! Интерфейс фильтрации на основании условий для указанного набора данных
class I_ConditionFilter
{
public:
    virtual ~I_ConditionFilter() {}

    //! Фильтрация на основании условий для указанного набора данных
    virtual ComplexCondition* getConditionFilter(const DataProperty& dataset) const = 0;
    //! Вычислить условие
    virtual bool calculateConditionFilter(const DataProperty& dataset, int row, const QModelIndex& parent) const = 0;

    //! Сигнал о том, что поменялся фильтр для набора данных
    virtual void sg_conditionFilterChanged(const zf::DataProperty& dataset) = 0;
};

} // namespace zf
