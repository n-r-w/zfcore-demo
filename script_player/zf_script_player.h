#pragma once

#include "zf.h"
#include "zf_data_container.h"

#include <QJSValue>

namespace zf
{
class ScriptPlayer;
class ScriptStep;
class ScriptChoise;
class ScriptPlayerFunctionJSWrapper;

typedef std::shared_ptr<ScriptChoise> ScriptChoisePtr;
typedef std::shared_ptr<ScriptStep> ScriptStepPtr;
typedef std::shared_ptr<ScriptPlayer> ScriptPlayerPtr;

//! Идентификатор категории изображения
class ZCORESHARED_EXPORT ScriptChoiseID : public ID_Wrapper
{
public:
    ScriptChoiseID();
    explicit ScriptChoiseID(int id);

    //! Преобразовать в QVariant
    QVariant variant() const;
    //! Восстановить из QVariant
    static ScriptChoiseID fromVariant(const QVariant& v);
};

inline uint qHash(const ScriptChoiseID& d)
{
    return ::qHash(d.value());
}

//! Идентификатор изображения
class ZCORESHARED_EXPORT ScriptStepID : public ID_Wrapper
{
public:
    ScriptStepID();
    explicit ScriptStepID(int id);

    //! Преобразовать в QVariant
    QVariant variant() const;
    //! Восстановить из QVariant
    static ScriptStepID fromVariant(const QVariant& v);
};

inline uint qHash(const ScriptStepID& d)
{
    return ::qHash(d.value());
}

//! Выбор на определенном шаге
class ZCORESHARED_EXPORT ScriptChoise
{
public:
    //! Код выбора (уникален в рамках шага)
    ScriptChoiseID id() const;

    //! Является ли текущим
    bool isCurrent() const;
    //! Является ли выбранным для собственного шага
    bool isSelected() const;

    //!  Шаг
    ScriptStep* step() const;

    //! Какие значения свойств должны автоматически заполниться при выборе данного шага. Ключ - id свойства, значение -
    //! значение свойства
    QMap<PropertyID, QVariant> values() const;

    //! Коды свойств, которые необходимо заполнить при данном выборе
    PropertyIDList targetProperties() const;

    //! Отображаемый текст вместе в метками подстановки значений
    QString textTemplate() const;
    //! Отображаемый текст
    QString text(
        //! Подставлять значение в html формате
        bool html_format, Error& error) const;    

private:
    ScriptChoise(ScriptPlayer* player, ScriptStep* step, const ScriptChoiseID& id_choise, const QString& text,
                 const QMap<PropertyID, QVariant>& values, const PropertyIDList& target_properties);

    //! Инициализировать данные значениями values данного выбора
    Error setPlayerValues();

    //! Плеер
    ScriptPlayer* _player = nullptr;
    //! Шаг
    ScriptStep* _step = nullptr;
    //! Код выбора
    ScriptChoiseID _id_choise;
    //! Отображаемый текст вместе в метками подстановки значений
    QString _text;
    //! Какие значения свойств должны автоматически заполниться при выборе данного шага. Ключ - id свойства, значение -
    //! значение свойства
    const QMap<PropertyID, QVariant> _values;
    //! Коды свойств, которые необходимо заполнить при данном выборе
    PropertyIDList _target_properties;

    friend class ScriptStep;
    friend class ScriptPlayer;
};

//! Шаг скрипта
class ZCORESHARED_EXPORT ScriptStep
{
public:
    //! Вид шага
    enum Type
    {
        //! Вывести запрос пользователю
        Say,
        //! Вызвать функцию
        Function
    };

    //! Добавить вариант выбора на данном шаге
    Error addChoise(
        //! Код выбора (уникальный в рамках шага)
        const ScriptChoiseID& id_choise,
        //! Отображаемый текст вместе в метками подстановки значений
        const QString& text,
        //! Какие значения свойств должны автоматически заполниться при выборе данного шага. Ключ - id свойства,
        //! значение - значение свойства
        const QMap<PropertyID, QVariant>& values,
        //! Коды свойств, которые необходимо заполнить вручную при этом выборе
        const PropertyIDList& target_properties = {});
    //! Добавить вариант выбора на данном шаге
    Error addChoise(
        //! Код выбора (уникальный в рамках шага)
        const ScriptChoiseID& id_choise,
        //! Отображаемый текст вместе в метками подстановки значений
        const QString& text,
        //! Какие значения свойств должны автоматически заполниться при выборе данного шага. Ключ - id свойства,
        //! значение - значение свойства
        const QPair<PropertyID, QVariant>& values,
        //! Коды свойств, которые необходимо заполнить вручную при этом выборе
        const PropertyIDList& target_properties = {});

    //! Вид шага
    Type type() const;

    //! Код шага
    ScriptStepID id() const;
    //! Какие значения свойств должны автоматически заполниться на данном шаге. Ключ - id свойства,
    //! значение - значение свойства
    QMap<PropertyID, QVariant> defaultValues() const;
    //! Коды свойств, которые необходимо заполнить вручную на данном шаге
    PropertyIDList targetProperties() const;

    //! Является ли текущим
    bool isCurrent() const;
    //! Шаг с которого пришли на данный шаг
    ScriptStep* previousStep() const;
    //! Активный выбор для данного шага
    ScriptChoise* selectedChoise() const;

    //! Отображаемый текст вместе в метками подстановки значений
    QString textTemplate() const;
    //! Отображаемый текст
    QString text(
        //! Подставлять значение в html формате
        bool html_format, Error& error) const;

    //! Куда записать результат функции
    PropertyID functionResult() const { return _function_result; }
    //! Вызвать функцию. Только для шагов, в которых задана функция вместо текста
    Error callFunction(QVariant& result) const;

    //! Какой выбор предоставить пользователю
    QList<ScriptChoise*> choises() const;
    //! Выбор по его коду
    ScriptChoise* choise(const ScriptChoiseID& choise_id) const;
    //! Есть ли такой выбор
    bool isChoiseExists(const ScriptChoiseID& choise_id) const;

    //! Возможные следующие шаги
    QList<ScriptStepID> possibleNextSteps() const;

    //! На какой шаг перейти при выборе. Возвращает несколько если в промежутке были шаги-функции
    QList<ScriptStepID> nextStepId(
        //! Вычислять ли функции при поиске шага
        bool calc_functions, Error& error) const;
    //! На какой шаг перейти при выборе. Возвращает несколько если в промежутке были шаги-функции
    QList<ScriptStep*> nextStep(
        //! Вычислять ли функции при поиске шага
        bool calc_functions, Error& error) const;

    //! Начальный шаг
    bool isFirst() const;
    //! Конечный шаг
    bool isFinal() const;
    //! Готов ли к переходу на следующий шаг. Если нет, то возвращает ошибку с причиной
    Error tryNext() const;

private:
    struct Condition;
    ScriptStep(ScriptPlayer* player, const ScriptStepID& id_step, bool is_first, const QString& text, const QString& function_name,
               const PropertyIDMap& arg_props, const QMap<QString, QVariant>& arg_values, const PropertyID& function_result,
               const QList<std::shared_ptr<Condition>>& conditions, const QMap<PropertyID, QVariant>& default_values,
               const PropertyIDList& target_properties);

    //! На какой шаг перейти при выборе
    void nextStepHelper(bool calc_functions, QList<ScriptStepID>& steps, Error& error) const;
    //! Возможные следующие шаги
    void possibleNextStepsHelper(QList<ScriptStepID>& steps) const;

    //! Вид шага
    Type _type = Say;

    //! Плеер
    ScriptPlayer* _player = nullptr;
    //! Начальный шаг
    bool _first = false;
    //! Код шага
    ScriptStepID _id_step;

    //! Отображаемый текст вместе в метками подстановки значений
    QString _text;

    //! Имя функции
    QString _function_name;
    //! Аргументы функции. Ключ - имя параметра, значение - свойство откуда брать значение
    PropertyIDMap _arg_props;
    //! Аргументы функции. Ключ - имя параметра, значение
    QMap<QString, QVariant> _arg_values;
    //! Куда записать результат функции
    PropertyID _function_result;

    //! Активный выбор для данного шага
    ScriptChoise* _selected_choise = nullptr;
    //! Шаг с которого пришли на данный шаг
    ScriptStep* _previous_step = nullptr;

    //! Какой выбор предоставить пользователю
    QList<ScriptChoisePtr> _choises;
    //! Ключ -  код выбора
    QMap<ScriptChoiseID, ScriptChoisePtr> _choises_map;

    //! Условие перехода
    struct Condition
    {
        QString js_func_text;
        //! Функция для принятия решения о переходе на следующий шаг. Должна возвращать true или false
        QJSValue js_func;
        //! Список кодов свойств данных для инициализации функции
        PropertyIDList parameters;
        //! Шаг перехода
        ScriptStepID step_id;
    };

    /*! На какой шаг перейти при выборе. Список пар: формула - шаг. Если формула nullptr, то безусловный переход */
    QList<std::shared_ptr<Condition>> _conditions;

    //! Значения, которые должны быть внесены в свойства при переходе на данных шаг
    QMap<PropertyID, QVariant> _default_values;
    //! Какие свойства должны быть заполнены пользователем
    PropertyIDList _target_properties;

    friend class ScriptPlayer;
};

//! Функция для расширения функциональности скриптов
typedef Error (*ScriptPlayerFunction)(
    //! Пары [имя параметра][значение]
    const QVariantMap& args,
    //! Результат работы функции
    QVariant& result);

//! Состояние плеера скриптов
class ZCORESHARED_EXPORT ScriptPlayerState
{
public:
    ScriptPlayerState();
    ScriptPlayerState(const ScriptStepID& current_step, const QMap<PropertyID, QVariant>& values,
                      //! Последовательность шагов
                      const QList<ScriptStepID>& history,
                      //! Текущий выбор
                      const QMap<ScriptStepID, ScriptChoiseID>& choises);
    ScriptPlayerState(const ScriptPlayerState& state);
    ScriptPlayerState& operator=(const ScriptPlayerState& state);

    bool isValid() const;

    ScriptStepID currentStep() const;
    QMap<PropertyID, QVariant> values() const;
    QList<ScriptStepID> history() const;
    QMap<ScriptStepID, ScriptChoiseID> choises() const;

    //! Сериалиазация в JSON
    Error toJSON(QByteArray& data,
                 //! Соответствие id шагов и текстовых меток. Если задано, в JSON выводятся текстовые метки
                 const QMap<ScriptStepID, QString>& step_mapping = {},
                 //! Соответствие id свойств и текстовых меток. Если задано, в JSON выводятся текстовые метки
                 const QMap<PropertyID, QString>& property_mapping = {}) const;
    //! Десериалиазация из JSON
    static Error fromJSON(const QByteArray& json, ScriptPlayerState& state,
                          //! Соответствие id шагов и текстовых меток. Если задано, в JSON выводятся текстовые метки
                          const QMap<ScriptStepID, QString>& step_mapping = {},
                          //! Соответствие id и текстовых меток. Если задано, в JSON ищутся текстовые метки
                          const QMap<PropertyID, QString>& property_mapping = {});

private:
    static QString propertyCode(const PropertyID& property_id, const QMap<PropertyID, QString>& property_mapping, Error& error);
    static PropertyID propertyID(const QString& property_id, const QMap<PropertyID, QString>& property_mapping, Error& error);
    static QString stepCode(const ScriptStepID& step_id, const QMap<ScriptStepID, QString>& step_mapping, Error& error);
    static ScriptStepID stepID(const QString& step_id, const QMap<ScriptStepID, QString>& step_mapping, Error& error);
    static ScriptChoiseID choiseID(const QString& choise_id, Error& error);

    bool _valid = false;
    ScriptStepID _current_step;
    QMap<PropertyID, QVariant> _values;
    QList<ScriptStepID> _history;
    QMap<ScriptStepID, ScriptChoiseID> _choises;
};

/*! Проигрыватель скриптов */
class ZCORESHARED_EXPORT ScriptPlayer : public QObject
{
    Q_OBJECT
public:
    //! Количество property_ids, property_types, property_defaults должно совпадать
    explicit ScriptPlayer(const DataPropertyList& properties);
    ~ScriptPlayer();

    //! Получить текущее состояние
    ScriptPlayerState getState() const;
    //! Задать текущее состояние
    Error setState(const ScriptPlayerState& state);

    //! Восстановление состояния из JSON
    Error setStateFromJSON(const QByteArray& json);
    //! Сохранение состояния в JSON
    QByteArray stateToJSON() const;

    /*! Регистрация функции для расширения функциональности скриптов
     * Функция может быть использована внутри javascript из addStep. Принимает  Пример:
     * Была зарегистрирована функция compareValues, которая принимает два парамера arg1, arg2 и возвращает true/false
     * (a=1, b=4) { 
     *      var res = compareValues.call(new Map([["arg1", a], ["arg2", b]]));
     *      if (compareValues.lastError() != "")
     *          log.add(compareValues.lastError());
     *      
     *      if (res)
     *          return true;
     * 
     *      if (a*b + b*3 == 16) 
     *          return true; 
     *      else 
     *          return false; 
     * }
     */
    void registerFunction(const QString& function_name, ScriptPlayerFunction function);
    //! Зарегистрирована ли функция расширения функциональности скриптов
    bool isFunctionRegistered(const QString& function_name) const;

    //! Добавить шаг. Финальный шаг не должен иметь next_step
    Error addStep(
        //! Код шага
        const ScriptStepID& id_step,
        //! Текст (допустима подстановка значений свойств через {{id свойства}}
        const QString& text,
        //! Первый шаг (точка входа)
        bool is_first = false,
        //! Какие значения свойств должны автоматически заполниться на данном шаге. Ключ - id свойства,
        //! значение - значение свойства
        const QMap<PropertyID, QVariant>& default_values = {},
        //! Коды свойств, которые необходимо заполнить вручную на данном шаге
        const PropertyIDList& target_properties = {},
        /*! На какой шаг перейти при выборе. Список пар: формула - шаг. Если формула пустая,
         * то безусловный переход
         * Формула на языке JavaScript вида:
         * (a=1, b=4) { if (a*b + b*3 == 16) return true; else return false; }
         * где: a,b - переменные 1,4 - коды свойств данных для инициализации */
        const QList<QPair<QString, ScriptStepID>>& next_step = {});
    //! Добавить шаг. Финальный шаг не должен иметь next_step
    Error addStep(
        //! Код шага
        const ScriptStepID& id_step,
        //! Имя функции
        const QString& function_name,
        //! Аргументы функции. Ключ - имя параметра, значение - свойство откуда брать значение
        const PropertyIDMap& arg_props,
        //! Аргументы функции. Ключ - имя параметра, значение
        const QMap<QString, QVariant>& arg_values,
        //! Куда записать результат функции
        const PropertyID& function_result,
        //! Первый шаг (точка входа)
        bool is_first = false,
        //! Какие значения свойств должны автоматически заполниться на данном шаге. Ключ - id свойства,
        //! значение - значение свойства
        const QMap<PropertyID, QVariant>& default_values = {},
        //! Коды свойств, которые необходимо заполнить вручную на данном шаге
        const PropertyIDList& target_properties = {},
        /*! На какой шаг перейти при выборе. Список пар: формула - шаг. Если формула пустая,
         * то безусловный переход
         * Формула на языке JavaScript вида:
         * (a=1, b=4) { if (a*b + b*3 == 16) return true; else return false; }
         * где: a,b - переменные 1,4 - коды свойств данных для инициализации */
        const QList<QPair<QString, ScriptStepID>>& next_step = {});

    //! Начать выполнение скрипта. Может быть выполнено только один раз
    Error start();
    //! Перейти на следующий шаг
    Error toNextStep();
    //! Перейти на предыдущий шаг
    Error toPreviousStep();

    //! Принудительно перейти на указанный шаг. Может привести к странным результатам с точки зрения логики работы скрипта
    Error toStep(const ScriptStepID& step_id,
                 //! История
                 const QList<ScriptStepID>& history, const QMap<ScriptStepID, ScriptChoiseID>& choises);

    //! Выбрать указанное свойство текущего шага
    Error activateChoise(const ScriptChoiseID& choise_id);

    //! Задать значение свойства
    Error setValue(const PropertyID& property_id, const QVariant& v);
    //! Задать значение свойства по текстовой метке (propertyMapping)
    Error setValue(const QString& property_text_id, const QVariant& v);

    //! Значение свойства
    QVariant value(const PropertyID& property_id) const;
    //! Значение свойства по текстовой метке (propertyMapping)
    QVariant value(const QString& property_text_id) const;

    //! Первый шаг
    ScriptStep* firstStep() const;
    //! Текущий шаг
    ScriptStep* currentStep() const;
    //! Текущий выбор
    ScriptChoise* currentChoise() const;

    //! Следующий шаг. Может быть несколько, если есть промежуточные шаги-функции
    QList<ScriptStep*> nextStep(
        //! Вычислять ли шаги-функции
        bool calc_functions, Error& error) const;

    //! Предыдущий шаг
    ScriptStep* previousStep() const;

    //! Запущен ли проигрыватель
    bool isStarted() const;
    //! Находимся на конечном шаге
    bool isFinalStep() const;
    //! Находимся на первом шаге
    bool isFirstStep() const;

    //! Шаг по его коду
    ScriptStep* step(const ScriptStepID& id_step) const;
    //! Выбор по его коду
    ScriptChoise* choise(const ScriptStepID& step_id, const ScriptChoiseID& choise_id) const;

    //! Есть ли такой шаг
    bool isStepExists(const ScriptStepID& id_step) const;
    //! Есть ли такой выбор для указанного шага
    bool isChoiseExists(const ScriptStepID& step_id, const ScriptChoiseID& choise_id) const;

    //! Данные
    DataContainerPtr data() const;

    //! Задать соответствие между кодами шагов и текстовыми метками (не обязательно)
    void setStepMapping(const QMap<ScriptStepID, QString>& step_id_map);
    //! Код шага по текстовой метке
    ScriptStepID stepByTextId(const QString& text_id) const;
    //! Текстовая метка по коду шага (пусто если не найдено)
    QString stepTextById(const ScriptStepID& step_id) const;

    //! Задать соответствие между свойствами и текстовыми метками (не обязательно)
    void setPropertyMapping(const QMap<PropertyID, QString>& property_id_map);
    //! Задать соответствие между свойствами и текстовыми метками (не обязательно)
    void setPropertyMapping(const QMap<DataProperty, QString>& property_map);
    //! ID свойства по текстовой метке (-1 если не найдено)
    PropertyID propertyMapping(const QString& text_id) const;
    //! Текстовая метка по id свойства (пусто если не найдено)
    QString propertyMapping(const PropertyID& property_id) const;

    //! Открывающий тег обрамления кодов свойств внутри текста По умолчанию: {{
    QString openMark() const;
    //! Закрывающий тег обрамления кодов свойств внутри текста По умолчанию: {{
    QString closeMark() const;
    //! Задать теги обрамления кодов свойств внутри текста
    void setOpenCloseMarks(const QString& open_mark, const QString& close_mark);

    //! Поиск меток в строке
    //! QPair::first - метка с маркерами, QPair::second - метка без маркеров
    static QList<QPair<QStringRef, QStringRef>> findTags(const QString& text, const QString& open_mark, const QString& close_mark);
    //! Парсинг строки с метками и заполнение меток значениями. Метки вида: <open_mark>код свойства<close_mark>
    static Error prepareText(const QString& text, const QString& open_mark, const QString& close_mark,
                             const QMap<PropertyID, DataProperty>& properties, const QMap<PropertyID, QVariant>& values,
                             //! Подставлять значение в html формате
                             bool html_format, QString& parsed);

    virtual void debPrint(void) const;

signals:
    //! Начато выполнение скрипта
    void sg_started();
    //! Выполнение скрипта закончено
    void sg_finished();

    //! Ушли с шага
    void sg_stepLeft(const zf::ScriptStepID& step_id);
    //! Перешли на шаг
    void sg_stepEntered(
        //! id предыдущего шага (isValid()==false, если перешли на первый шаг)
        const zf::ScriptStepID& prev_id,
        //! id нового шага
        const zf::ScriptStepID& current_id);

    //! Плеер готов к переходу на следующий шаг (заполнены все необходимые данные)
    void sg_nextStepReady(const zf::ScriptStepID& step_id);

    //! Активирован выбор
    void sg_choiseActivated(const zf::ScriptStepID& step_id, const zf::ScriptChoiseID& choise_id);

private slots:
    //! Добавлено значение в журнал
    void sl_logRecordAdded(const QVariantList& value);

private:
    //! Добавить шаг. Финальный шаг не должен иметь next_step
    Error addStepHelper(
        //! Код шага
        const ScriptStepID& id_step,
        //! Текст (допустима подстановка значений свойств через {{id свойства}}
        const QString& text,
        //! Имя функции
        const QString& function_name,
        //! Аргументы функции. Ключ - имя параметра, значение - свойство откуда брать значение
        const PropertyIDMap& arg_props,
        //! Аргументы функции. Ключ - имя параметра, значение
        const QMap<QString, QVariant>& arg_values,
        //! Куда записать результат функции
        const PropertyID& function_result,
        //! Первый шаг (точка входа)
        bool is_first,
        //! Какие значения свойств должны автоматически заполниться на данном шаге. Ключ - id свойства,
        //! значение - значение свойства
        const QMap<PropertyID, QVariant>& default_values,
        //! Коды свойств, которые необходимо заполнить вручную на данном шаге
        const PropertyIDList& target_properties,
        /*! На какой шаг перейти при выборе. Список пар: формула - шаг. Если формула пустая,
         * то безусловный переход
         * Формула на языке JavaScript вида:
         * (a=1, b=4) { if (a*b + b*3 == 16) return true; else return false; }
         * где: a,b - переменные 1,4 - коды свойств данных для инициализации */
        const QList<QPair<QString, ScriptStepID>>& next_step);

    //! Парсинг строки с метками и заполнение меток значениями. Метки вида: <open_mark>код свойства<close_mark>
    Error prepareDataText(const QString& text,
                          //! Подставлять значение в html формате
                          bool html_format, QString& parsed) const;

    /*! Создать QJSValue функцию из текста
     * Формула на языке JavaScript вида:
     * (a=1, b=4) { if (a*b + b*3 == 16) return true; else return false; }
     * где: a,b - переменные 1,4 - коды свойств данных для инициализации */
    Error prepareFunc(const QString& text, QJSValue& func, PropertyIDList& property_ids) const;
    //! Вызвать функцию расширения, добавленную через registerFunction
    Error callFunction(const QString& function_name, const QVariantMap& args, QVariant& result);

    //! Вызывается при переходе на новый шаг
    Error onNewStepHelper(
        //! предыдущий шаг
        ScriptStep* prev_step,
        //! новый шаг
        ScriptStep* new_step);

    //! Получить все шаги, которые могут быть после данного шага
    void getTree(ScriptStep* step, QList<ScriptStep*>& tree) const;

    //! метки начало тега, конец тега
    QString _open_mark, _close_mark;

    //! Ключ - код шага, значение - шаг
    QHash<ScriptStepID, ScriptStepPtr> _steps;
    //! Данные
    DataContainerPtr _data;

    //! Соответствие между кодами шагов и текстовыми метками
    QMap<ScriptStepID, QString> _step_id_map;
    //! Соответствие между id свойств и текстовыми метками
    QMap<PropertyID, QString> _property_id_map;

    //! Идентификатор первого шага
    ScriptStepID _first_step_id;

    //! Текущий шаг
    ScriptStep* _current_step = nullptr;

    //! Информация о функции
    struct FunctionInfo
    {
        ~FunctionInfo();

        QString name;
        ScriptPlayerFunction ptr = nullptr;
        ScriptPlayerFunctionJSWrapper* wrapper = nullptr;
    };
    //! Функции
    QMap<QString, std::shared_ptr<FunctionInfo>> _functions;

    //! Движок JS
    std::unique_ptr<QJSEngine> _js_engine;

    friend class ScriptStep;
    friend class ScriptChoise;
};

} // namespace zf

Q_DECLARE_METATYPE(zf::ScriptChoiseID)
Q_DECLARE_METATYPE(zf::ScriptStepID)
