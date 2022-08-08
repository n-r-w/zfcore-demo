#include "zf_script_player.h"
#include "private/zf_script_player_p.h"
#include "zf_core.h"
#include "zf_html_tools.h"
#include "zf_translation.h"

#include <QJSEngine>
#include <QJsonDocument>
#include <QJsonValue>
#include <QJsonObject>
#include <QJsonArray>

namespace zf
{
const QVariant ScriptPlayerLoggerJSWrapper::js_undefinedValue = QVariant("___zf_undefined___");

ScriptPlayer::ScriptPlayer(const DataPropertyList& properties)
    : QObject()
    , _open_mark(QStringLiteral("{{"))
    , _close_mark(QStringLiteral("}}"))
{
    Z_CHECK(!properties.isEmpty());

    // Инициализируем движок JS
    _js_engine = std::make_unique<QJSEngine>();
    _js_engine->setObjectName("zf_script_player_js_engine");
    _js_engine->installExtensions(QJSEngine::ConsoleExtension);

    // добавляем туда функцию log
    auto log_wrapper = new ScriptPlayerLoggerJSWrapper(_js_engine.get());
    QJSValue log_object = _js_engine->newQObject(log_wrapper);
    _js_engine->globalObject().setProperty("log", log_object);
    connect(log_wrapper, &ScriptPlayerLoggerJSWrapper::sg_recordAdded, this, &ScriptPlayer::sl_logRecordAdded);

    // создаем контекст данных
    auto structure = DataStructure::independentStructure(PropertyID::def(), 1);

    for (auto& p : qAsConst(properties)) {
        Z_CHECK(p.isEntityIndependent());
        structure->addProperty(p);
    }

    _data = std::make_unique<DataContainer>(structure);
}

ScriptPlayer::~ScriptPlayer()
{
    static int b = 0;
    if (b)
        debPrint();
}

ScriptPlayerState ScriptPlayer::getState() const
{
    QMap<PropertyID, QVariant> values;
    for (auto& p : _data->initializedProperties()) {
        QVariant value = _data->value(p);
        if (!value.isValid())
            continue;
        if (value.type() == QVariant::String && value.toString().trimmed().isEmpty())
            continue;

        values[p.id()] = _data->value(p);
    }

    QMap<ScriptStepID, ScriptChoiseID> choises;
    for (auto it = _steps.constBegin(); it != _steps.constEnd(); ++it) {
        if (it.value()->selectedChoise() != nullptr)
            choises[it.key()] = it.value()->selectedChoise()->id();
    }

    ScriptStep* step = _current_step;
    QList<ScriptStepID> history;
    while (step != nullptr) {
        history.prepend(step->id());
        step = step->previousStep();
    }

    return ScriptPlayerState(_current_step == nullptr ? ScriptStepID() : _current_step->id(), values, history, choises);
}

Error ScriptPlayer::setState(const ScriptPlayerState& state)
{
    _data->resetValues();
    auto values = state.values();
    for (auto it = values.constBegin(); it != values.constEnd(); ++it) {
        if (!_data->structure()->contains(it.key()))
            continue;

        setValue(it.key(), it.value());
    }

    return toStep(state.currentStep(), state.history(), state.choises());
}

Error ScriptPlayer::setStateFromJSON(const QByteArray& json)
{
    ScriptPlayerState state;
    auto error = ScriptPlayerState::fromJSON(json, state, _step_id_map, _property_id_map);
    if (error.isError())
        return error;

    return setState(state);
}

QByteArray ScriptPlayer::stateToJSON() const
{
    auto state = getState();
    QByteArray data;
    auto error = state.toJSON(data, _step_id_map, _property_id_map);
    Z_CHECK_ERROR(error); // тут ошибки быть не должно
    return data;
}

void ScriptPlayer::registerFunction(const QString& function_name, ScriptPlayerFunction function)
{
    Z_CHECK_NULL(function);
    Z_CHECK(!isFunctionRegistered(function_name));

    auto info = std::make_shared<FunctionInfo>();
    info->name = function_name.trimmed();
    info->ptr = function;

    // регистрируем функцию в движке JS
    info->wrapper = new ScriptPlayerFunctionJSWrapper(function_name.trimmed(), function, this);
    QJSValue function_js_object = _js_engine->newQObject(info->wrapper);
    _js_engine->globalObject().setProperty(function_name.trimmed(), function_js_object);

    _functions[function_name.trimmed().toLower()] = info;
}

bool ScriptPlayer::isFunctionRegistered(const QString& function_name) const
{
    QString fn = function_name.trimmed().toLower();
    Z_CHECK(!fn.isEmpty());
    return _functions.contains(fn);
}

Error ScriptPlayer::start()
{
    Z_CHECK(_current_step == nullptr);
    if (!_first_step_id.isValid())
        return Error("First step not defined");

    _current_step = _steps.value(_first_step_id).get();
    Z_CHECK_NULL(_current_step);

    emit sg_started();
    emit sg_stepEntered(ScriptStepID(), _current_step->id());

    return Error();
}

Error ScriptPlayer::toNextStep()
{
    if (!isStarted())
        return start();

    Z_CHECK_NULL(_current_step);

    Error error;
    auto steps = _current_step->nextStepId(true, error);
    if (error.isError())
        return error;

    Z_CHECK(!steps.isEmpty());

    ScriptStep* c_step = _current_step;
    for (auto& id : qAsConst(steps)) {
        if (!_steps.contains(id))
            return Error(QStringLiteral("Step not found: %1").arg(id.value()));

        error = onNewStepHelper(c_step, _steps.value(id).get());
        if (error.isError())
            return error;

        c_step = _steps.value(id).get();
    }

    auto prev_step = _current_step;
    _current_step = _steps.value(steps.last()).get();
    _current_step->_previous_step = prev_step;

    emit sg_stepLeft(prev_step->id());
    emit sg_stepEntered(prev_step->id(), _current_step->id());
    return Error();
}

Error ScriptPlayer::toPreviousStep()
{
    ScriptStep* prev_step = previousStep();
    if (prev_step == nullptr)
        return Error(QStringLiteral("No previous step"));

    auto error = onNewStepHelper(_current_step, prev_step);
    if (error.isError())
        return error;

    _current_step->_previous_step = nullptr;
    ScriptStep* current = _current_step;
    _current_step = prev_step;

    emit sg_stepLeft(current->id());
    emit sg_stepEntered(current->id(), prev_step->id());
    return Error();
}

Error ScriptPlayer::toStep(const ScriptStepID& step_id, const QList<ScriptStepID>& history,
                           const QMap<ScriptStepID, ScriptChoiseID>& choises)
{
    ScriptStepID id;
    if (step_id.isValid()) {
        id = step_id;
    } else {
        if (firstStep() == nullptr)
            return {};

        id = firstStep()->id();
    }

    auto step = this->step(id);

    if (currentStep() == nullptr || currentStep()->id() != id) {
        auto error = onNewStepHelper(currentStep(), step);
        if (error.isError())
            return error;
    }

    // очищаем информацию
    for (auto& s : _steps.values()) {
        s->_previous_step = nullptr;
        s->_selected_choise = nullptr;
    }

    // восстанавливаем информацию о предыдущих шагах
    for (int i = 0; i < history.count(); i++) {
        auto h_step = this->step(history.at(i));
        if (h_step == nullptr)
            return Error(QString("Шаг не найден %1").arg(history.at(i).value()));

        if (i < history.count() - 1) {
            auto h_next_step = this->step(history.at(i + 1));
            if (h_next_step == nullptr)
                return Error(QString("Шаг не найден %1").arg(history.at(i + 1).value()));

            bool next_found = false;
            for (auto c : h_step->possibleNextSteps()) {
                if (c == h_next_step->id()) {
                    next_found = true;
                    break;
                }
            }
            if (!next_found)
                return Error("Структура скрипта не совпадает");

            h_next_step->_previous_step = h_step;
        }
    }

    // восстанавливаем информацию о выборе
    for (auto it = choises.constBegin(); it != choises.constEnd(); ++it) {
        auto choise = this->choise(it.key(), it.value());
        if (choise == nullptr)
            return Error(QString("Выбор %1 для шага %2 не найден").arg(it.key().value()).arg(it.value().value()));

        this->step(it.key())->_selected_choise = choise;
        emit sg_choiseActivated(it.key(), choise->id());
    }

    if (currentStep() == nullptr || currentStep()->id() != id) {
        ScriptStep* current = _current_step;
        _current_step = step;

        if (current != nullptr)
            emit sg_stepLeft(current->id());
        emit sg_stepEntered(current ? current->id() : ScriptStepID(), step->id());
    }
    return Error();
}

Error ScriptPlayer::activateChoise(const ScriptChoiseID& choise_id)
{
    if (_current_step == nullptr)
        return Error(QStringLiteral("Player not started"));

    auto c = choise(_current_step->id(), choise_id);
    if (c == nullptr)
        return Error(QStringLiteral("Choise not found: step %1, choise %2").arg(_current_step->id().value()).arg(choise_id.value()));

    if (c == _current_step->selectedChoise())
        return Error();

    Error error = c->setPlayerValues();

    _current_step->_selected_choise = c;
    emit sg_choiseActivated(_current_step->id(), choise_id);

    return error;
}

Error ScriptPlayer::setValue(const PropertyID& property_id, const QVariant& v)
{
    if (!_data->contains(property_id))
        return Error(QStringLiteral("Property not found: %1").arg(property_id.value()));

    Error error;
    auto next_step_before = nextStep(true, error);

    error = _data->setValue(property_id, v);

    if (error.isOk()) {
        auto next_step_after = nextStep(true, error);
        if (next_step_before != next_step_after && !next_step_after.isEmpty())
            emit sg_nextStepReady(next_step_after.last()->id());
    }

    return error;
}

Error ScriptPlayer::setValue(const QString& property_text_id, const QVariant& v)
{
    PropertyID id = propertyMapping(property_text_id);
    if (!id.isValid())
        return Error(QStringLiteral("Property not found: %1").arg(property_text_id));

    return setValue(id, v);
}

QVariant ScriptPlayer::value(const PropertyID& property_id) const
{
    return _data->contains(property_id) && _data->isInitialized(property_id) ? _data->value(property_id) : QVariant();
}

QVariant ScriptPlayer::value(const QString& property_text_id) const
{
    PropertyID id = propertyMapping(property_text_id);
    if (!id.isValid())
        return QVariant();

    return value(id);
}

ScriptStep* ScriptPlayer::firstStep() const
{
    return !_first_step_id.isValid() ? nullptr : step(_first_step_id);
}

ScriptStep* ScriptPlayer::currentStep() const
{
    return _current_step;
}

ScriptChoise* ScriptPlayer::currentChoise() const
{
    return _current_step != nullptr ? _current_step->selectedChoise() : nullptr;
}

QList<ScriptStep*> ScriptPlayer::nextStep(bool calc_functions, Error& error) const
{
    error.clear();
    return _current_step == nullptr ? QList<ScriptStep*> {firstStep()} : _current_step->nextStep(calc_functions, error);
}

ScriptStep* ScriptPlayer::previousStep() const
{
    return _current_step != nullptr ? _current_step->previousStep() : nullptr;
}

bool ScriptPlayer::isStarted() const
{
    return _current_step != nullptr;
}

bool ScriptPlayer::isFinalStep() const
{
    return _current_step == nullptr ? true : _current_step->isFinal();
}

bool ScriptPlayer::isFirstStep() const
{
    return _current_step == nullptr ? false : _current_step->previousStep() == nullptr;
}

ScriptStep* ScriptPlayer::step(const ScriptStepID& id_step) const
{
    ScriptStepPtr p = _steps.value(id_step);
    return p == nullptr ? nullptr : p.get();
}

ScriptChoise* ScriptPlayer::choise(const ScriptStepID& step_id, const ScriptChoiseID& choise_id) const
{
    auto step = _steps.value(step_id);
    if (step == nullptr)
        return nullptr;

    return step->choise(choise_id);
}

bool ScriptPlayer::isStepExists(const ScriptStepID& id_step) const
{
    return _steps.contains(id_step);
}

bool ScriptPlayer::isChoiseExists(const ScriptStepID& step_id, const ScriptChoiseID& choise_id) const
{
    auto step = _steps.value(step_id);
    if (step == nullptr)
        return false;

    return step->isChoiseExists(choise_id);
}

DataContainerPtr ScriptPlayer::data() const
{
    return _data;
}

void ScriptPlayer::setStepMapping(const QMap<ScriptStepID, QString>& step_id_map)
{
    _step_id_map = step_id_map;
}

ScriptStepID ScriptPlayer::stepByTextId(const QString& text_id) const
{
    return _step_id_map.key(text_id.toLower());
}

QString ScriptPlayer::stepTextById(const ScriptStepID& step_id) const
{
    return _step_id_map.value(step_id);
}

void ScriptPlayer::setPropertyMapping(const QMap<PropertyID, QString>& property_id_map)
{
    for (auto it = property_id_map.constBegin(); it != property_id_map.constEnd(); ++it) {
        Z_CHECK(it.key().isValid());
        Z_CHECK(!it.value().trimmed().isEmpty());
    }

    _property_id_map = property_id_map;
}

void ScriptPlayer::setPropertyMapping(const QMap<DataProperty, QString>& property_map)
{
    QMap<PropertyID, QString> property_id_map;
    for (auto it = property_map.constBegin(); it != property_map.constEnd(); ++it) {
        property_id_map[it.key().id()] = it.value();
    }
    setPropertyMapping(property_id_map);
}

PropertyID ScriptPlayer::propertyMapping(const QString& text_id) const
{
    return _property_id_map.key(text_id.toLower());
}

QString ScriptPlayer::propertyMapping(const PropertyID& property_id) const
{
    return _property_id_map.value(property_id);
}

QString ScriptPlayer::openMark() const
{
    return _open_mark;
}

QString ScriptPlayer::closeMark() const
{
    return _close_mark;
}

QList<QPair<QStringRef, QStringRef>> ScriptPlayer::findTags(const QString& text, const QString& open_mark, const QString& close_mark)
{
    // ищем все маркеры
    QList<QPair<QStringRef, QStringRef>> found_tags;

    QString reg_text = QStringLiteral("%1(.*?)%2").arg(QRegularExpression::escape(open_mark), QRegularExpression::escape(close_mark));
    QRegularExpression reg(reg_text, QRegularExpression::CaseInsensitiveOption
                                         | QRegularExpression::DotMatchesEverythingOption
                                         | QRegularExpression::MultilineOption);
    QRegularExpressionMatchIterator reg_it = reg.globalMatch(text, 0, QRegularExpression::NormalMatch);
    while (reg_it.hasNext()) {
        // reg_it.next().capturedRef() делать нельзя, т.к. оптимизатор убъет reg_it.next() и capturedRef
        // будет указывать на удаленную область памяти
        QRegularExpressionMatch match = reg_it.next();
        QStringRef id_str_found = match.capturedRef();
        found_tags.append({
            text.midRef(id_str_found.position(), id_str_found.length()),
            text.midRef(id_str_found.position() + open_mark.length(), id_str_found.length() - open_mark.length() - close_mark.length()),
        });
    }

    return found_tags;
}

void ScriptPlayer::setOpenCloseMarks(const QString& open_mark, const QString& close_mark)
{
    Z_CHECK(!open_mark.isEmpty() && !close_mark.isEmpty());

    _open_mark = open_mark;
    _close_mark = close_mark;
}

void ScriptPlayer::debPrint(void) const
{
    zf::Core::debPrint(this);
}

void ScriptPlayer::sl_logRecordAdded(const QVariantList& value)
{
    for (auto& v : value) {
        Core::logError(Utils::variantToString(v));
    }
}

Error ScriptPlayer::addStep(const ScriptStepID& id_step, const QString& text, bool is_first, const QMap<PropertyID, QVariant>& values,
                            const PropertyIDList& target_properties, const QList<QPair<QString, ScriptStepID>>& next_step)
{
    return addStepHelper(id_step, text, QString(), {}, {}, {}, is_first, values, target_properties, next_step);
}

Error ScriptPlayer::addStep(const ScriptStepID& id_step, const QString& function_name, const PropertyIDMap& arg_props,
                            const QMap<QString, QVariant>& arg_values, const PropertyID& function_result, bool is_first,
                            const QMap<PropertyID, QVariant>& default_values, const PropertyIDList& target_properties,
                            const QList<QPair<QString, ScriptStepID>>& next_step)
{
    return addStepHelper(id_step, QString(), function_name, arg_props, arg_values, function_result, is_first, default_values,
                         target_properties, next_step);
}

Error ScriptPlayer::addStepHelper(const ScriptStepID& id_step, const QString& text, const QString& function_name,
                                  const PropertyIDMap& arg_props, const QMap<QString, QVariant>& arg_values,
                                  const PropertyID& function_result, bool is_first, const QMap<PropertyID, QVariant>& default_values,
                                  const PropertyIDList& target_properties, const QList<QPair<QString, ScriptStepID>>& next_step)
{
    if (!id_step.isValid())
        return Error(QStringLiteral("Incorrect step id: %1").arg(id_step.value()));

    if (_steps.contains(id_step))
        return Error(QStringLiteral("Step duplicated: %1").arg(id_step.value()));

    if (is_first && _first_step_id == id_step)
        return Error(QStringLiteral("First step duplicated: %1").arg(id_step.value()));

    if (text.trimmed().isEmpty() && function_name.trimmed().isEmpty())
        return Error(QStringLiteral("Empty step text and function: %1").arg(id_step.value()));

    if (!text.trimmed().isEmpty() && !function_name.trimmed().isEmpty())
        return Error(QStringLiteral("Set text or function, but not both: %1").arg(id_step.value()));

    if (!function_name.trimmed().isEmpty() && !function_result.isValid())
        return Error(QStringLiteral("Function result property not defined: %1").arg(id_step.value()));

    for (auto& p : target_properties) {
        Z_CHECK(p.isValid());
    }

    QList<std::shared_ptr<ScriptStep::Condition>> conditions;
    for (auto& ns : qAsConst(next_step)) {
        QJSValue func;
        PropertyIDList property_ids;
        if (!ns.first.trimmed().isEmpty()) {
            Error error = prepareFunc(ns.first, func, property_ids);
            if (error.isError())
                return error;
        }

        std::shared_ptr<ScriptStep::Condition> condition = std::shared_ptr<ScriptStep::Condition>(new ScriptStep::Condition);
        condition->js_func_text = ns.first;
        condition->js_func = func;
        condition->parameters = property_ids;
        condition->step_id = ns.second;

        conditions << condition;
    }

    _steps[id_step] = std::shared_ptr<ScriptStep>(new ScriptStep(this, id_step, is_first, text, function_name, arg_props, arg_values,
                                                                 function_result, conditions, default_values, target_properties));
    if (is_first)
        _first_step_id = id_step;

    return Error();
}

Error ScriptPlayer::prepareText(const QString& text, const QString& open_mark, const QString& close_mark,
                                const QMap<PropertyID, DataProperty>& properties, const QMap<PropertyID, QVariant>& values,
                                bool html_format, QString& parsed)
{
    parsed.clear();

    QList<QPair<QStringRef, QStringRef>> found_tags = findTags(text, open_mark, close_mark);

    PropertyIDList found_properties;
    for (int i = 0; i < found_tags.count(); ++i) {
        QStringRef id_str = found_tags.at(i).second;
        bool ok;
        int id = id_str.toInt(&ok);
        if (!ok || id < Consts::MINUMUM_PROPERTY_ID)
            return Error(QStringLiteral("Text parsing error. Bad property id: %1\n(%2)").arg(id_str).arg(text));

        if (!properties.contains(PropertyID(id)))
            return Error(QStringLiteral("Text parsing error. Property id not found: %1\n(%2)").arg(id_str).arg(text));

        found_properties << PropertyID(id);
    }

    // заменяем
    parsed = text;
    for (int i = found_tags.count() - 1; i >= 0; i--) {
        QString value = Utils::variantToString(values.value(found_properties.at(i))).trimmed();

        if (!value.isEmpty() && html_format)
            value = HtmlTools::bold(value);

        QStringRef ref = found_tags.at(i).first;
        parsed.replace(ref.position(), ref.length(), value);
    }

    if (html_format)
        parsed.replace(QChar('\n'), "<br/>");

    return Error();
}

Error ScriptPlayer::prepareDataText(const QString& text, bool html_format, QString& parsed) const
{
    QMap<PropertyID, DataProperty> properties;
    QMap<PropertyID, QVariant> values;

    for (auto& p : _data->structure()->propertiesByType(PropertyType::Field)) {
        properties[p.id()] = p;
        QVariant value;
        if (_data->isInitialized(p))
            value = _data->value(p);
        values[p.id()] = value;
    }

    return prepareText(text, _open_mark, _close_mark, properties, values, html_format, parsed);
}

Error ScriptPlayer::prepareFunc(const QString& text, QJSValue& func, PropertyIDList& property_ids) const
{
    // ищем первую и вторую скобки
    int bracket_pos1 = text.indexOf('(');
    if (bracket_pos1 < 0)
        return Error(QStringLiteral("Formula error: first bracket not found (%1)").arg(text));

    int bracket_pos2 = text.indexOf(')', bracket_pos1 + 1);
    if (bracket_pos2 < 0)
        return Error(QStringLiteral("Formula error: second bracket not found (%1)").arg(text));

    // строка с параметрами
    QString parameters_str = text.mid(bracket_pos1 + 1, bracket_pos2 - bracket_pos1 - 1).simplified();
    parameters_str.replace(QStringLiteral(" "), QLatin1String("")); //-V567
    // отдельные параметры (текст вида xxx=12)
    QStringList p_splitted = parameters_str.split(",", QString::KeepEmptyParts);

    QStringList params_prepared;
    PropertyIDList properties;
    for (auto& p : qAsConst(p_splitted)) {
        QStringList p_s = p.split("=", QString::KeepEmptyParts);
        if (p_s.count() != 2)
            return Error(QStringLiteral("Formula error: bad parameters (%1)").arg(text));

        bool ok;
        int property_id = p_s.at(1).toInt(&ok);
        if (!ok || property_id < Consts::MINUMUM_PROPERTY_ID)
            return Error(QStringLiteral("Formula error: bad parameter code [%1] (%2)").arg(p_s.at(1), text));

        if (!_data->structure()->contains(PropertyID(property_id)))
            return Error(QStringLiteral("Formula error: unknown property code [%1] (%2)").arg(property_id).arg(text));

        properties << PropertyID(property_id);
        params_prepared << p_s.at(0);
    }

    QString func_prepared
        = QStringLiteral("(function (%1) %2)").arg(params_prepared.join(",")).arg(text.rightRef(text.count() - bracket_pos2 - 1));

    QJSValue func_created = Core::jsEngine()->evaluate(func_prepared);
    if (func_created.isError())
        return Error(QStringLiteral("Formula error: %1 (%2)").arg(func_created.toString()).arg(text));

    func = func_created;
    property_ids = properties;

    return Error();
}

Error ScriptPlayer::callFunction(const QString& function_name, const QVariantMap& args, QVariant& result)
{
    auto info = _functions.value(function_name.trimmed().toLower());
    if (info == nullptr)
        return Error(QString("Function %1 not found"));

    return info->ptr(args, result);
}

Error ScriptPlayer::onNewStepHelper(ScriptStep* prev_step, ScriptStep* new_step)
{
    Error error;
    if (prev_step != nullptr) {
        if (prev_step->type() == ScriptStep::Function) {
            // шаг - функция, надо вычислить результат
            QVariant result;
            error = prev_step->callFunction(result);
            if (error.isError())
                return error;

            setValue(prev_step->functionResult(), result);
        }
    }

    if (new_step != nullptr) {
        // заполнение данных значениями по умолчанию
        auto def_vals = new_step->defaultValues();
        for (auto it = def_vals.constBegin(); it != def_vals.constEnd(); ++it) {
            error += setValue(it.key(), it.value());
        }
    }

    return error;
}

void ScriptPlayer::getTree(ScriptStep* step, QList<ScriptStep*>& tree) const
{
    if (tree.contains(step))
        return; // зацикливание

    tree << step;
    for (auto c : step->possibleNextSteps()) {
        getTree(this->step(c), tree);
    }
}

ScriptChoiseID ScriptChoise::id() const
{
    return _id_choise;
}

bool ScriptChoise::isCurrent() const
{
    return _player->currentChoise() == this;
}

bool ScriptChoise::isSelected() const
{
    return _step->selectedChoise() == this;
}

ScriptStep* ScriptChoise::step() const
{
    return _step;
}

QMap<PropertyID, QVariant> ScriptChoise::values() const
{
    return _values;
}

QString ScriptChoise::textTemplate() const
{
    return _text;
}

PropertyIDList ScriptChoise::targetProperties() const
{
    return _target_properties;
}

QString ScriptChoise::text(bool html_format, Error& error) const
{
    QString s;
    error = _player->prepareDataText(_text, html_format, s);
    return s;
}

ScriptChoise::ScriptChoise(zf::ScriptPlayer* player, ScriptStep* step, const ScriptChoiseID& id_choise, const QString& text,
                           const QMap<PropertyID, QVariant>& values, const PropertyIDList& target_properties)
    : _player(player)
    , _step(step)
    , _id_choise(id_choise)
    , _text(text)
    , _values(values)
    , _target_properties(target_properties)
{
}

Error ScriptChoise::setPlayerValues()
{
    Error error;
    for (auto it = _values.constBegin(); it != _values.constEnd(); ++it) {
        error << _player->setValue(it.key(), it.value());
    }

    return error;
}

Error ScriptStep::addChoise(const ScriptChoiseID& id_choise, const QString& text, const QMap<PropertyID, QVariant>& values,
                            const PropertyIDList& target_properties)
{
    if (values.isEmpty())
        return Error(QStringLiteral("Empty values: step %1, choise %2").arg(_id_step.value()).arg(id_choise.value()));

    for (auto it = values.constBegin(); it != values.constEnd(); ++it) {
        if (!_player->_data->contains(it.key()))
            return Error(QStringLiteral("Property not found: step %1, choise %2, property %3")
                             .arg(_id_step.value())
                             .arg(id_choise.value())
                             .arg(it.key().value()));
    }

    for (auto& c : qAsConst(_choises)) {
        if (c->id() == id_choise)
            return Error(QStringLiteral("Choise id duplicated: step %1, choise %2").arg(_id_step.value()).arg(id_choise.value()));
    }
    if (text.trimmed().isEmpty() && _player->step(_id_step)->type() == ScriptStep::Say)
        return Error(QStringLiteral("Choise text is empty: step %1, choise %2").arg(_id_step.value()).arg(id_choise.value()));

    _choises << std::shared_ptr<ScriptChoise>(new ScriptChoise(_player, this, id_choise, text, values, target_properties));
    _choises_map[id_choise] = _choises.last();
    return Error();
}

Error ScriptStep::addChoise(const ScriptChoiseID& id_choise, const QString& text, const QPair<PropertyID, QVariant>& value,
                            const PropertyIDList& target_properties)
{
    return addChoise(id_choise, text, QMap<PropertyID, QVariant> {{value.first, value.second}}, target_properties);
}

ScriptStep::Type ScriptStep::type() const
{
    return _type;
}

ScriptStepID ScriptStep::id() const
{
    return _id_step;
}

QMap<PropertyID, QVariant> ScriptStep::defaultValues() const
{
    return _default_values;
}

PropertyIDList ScriptStep::targetProperties() const
{
    return _target_properties;
}

bool ScriptStep::isCurrent() const
{
    return _player->currentStep() == this;
}

ScriptStep* ScriptStep::previousStep() const
{
    ScriptStep* prev = _previous_step;
    while (prev != nullptr) {
        if (prev->type() != ScriptStep::Function)
            return prev;
        prev = prev->previousStep();
    }

    return nullptr;
}

ScriptChoise* ScriptStep::selectedChoise() const
{
    return _selected_choise;
}

QString ScriptStep::textTemplate() const
{
    return _text;
}

QString ScriptStep::text(bool html_format, Error& error) const
{
    Z_CHECK(_type == Say);
    QString s;
    error = _player->prepareDataText(_text, html_format, s);
    return s;
}

Error ScriptStep::callFunction(QVariant& result) const
{
    Z_CHECK(_type == Function);

    QVariantMap args;
    for (auto it = _arg_values.constBegin(); it != _arg_values.constEnd(); ++it) {
        args[it.key()] = it.value();
    }
    for (auto it = _arg_props.constBegin(); it != _arg_props.constEnd(); ++it) {
        args[it.key()] = _player->data()->value(it.value());
    }

    return _player->callFunction(_function_name, args, result);
}

QList<ScriptChoise*> ScriptStep::choises() const
{
    QList<ScriptChoise*> res;

    for (auto& c : qAsConst(_choises)) {
        res << c.get();
    }

    return res;
}

ScriptChoise* ScriptStep::choise(const ScriptChoiseID& choise_id) const
{
    return _choises_map.value(choise_id).get();
}

bool ScriptStep::isChoiseExists(const ScriptChoiseID& choise_id) const
{
    return _choises_map.contains(choise_id);
}

QList<ScriptStepID> ScriptStep::possibleNextSteps() const
{
    QList<ScriptStepID> steps;
    possibleNextStepsHelper(steps);
    return steps;
}

void ScriptStep::nextStepHelper(bool calc_functions, QList<ScriptStepID>& steps, Error& error) const
{
    ScriptStepID next_step_id;
    QMap<PropertyID, QVariant> prepared_values;

    if (_type == ScriptStep::Function) {
        // шаг - функция, надо вычислить результат
        QVariant result;
        error = callFunction(result);
        if (error.isError())
            return;

        prepared_values[_function_result] = result;
    }

    for (auto& c : qAsConst(_conditions)) {
        if (!c->js_func.isCallable()) {
            next_step_id = c->step_id;
            break;
        }

#if !defined(RELEASE_MODE) && defined(QT_DEBUG)
        QStringList param_values;
#endif
        QJSValueList args;
        for (auto& prop_id : c->parameters) {
            QVariant value;
            if (prepared_values.contains(prop_id))
                value = prepared_values.value(prop_id);
            else
                value = _player->value(prop_id);
            args << Core::jsEngine()->toScriptValue(value);
#if !defined(RELEASE_MODE) && defined(QT_DEBUG)
            param_values << _player->_data->toString(prop_id);
#endif
        }

        QJSValue res = c->js_func.call(args);
        if (res.isError()) {
            error = Error(QStringLiteral("Formula error: %1 (%2)").arg(res.toString(), c->js_func_text));
            return;
        }

        if (!res.isBool()) {
            error = Error(QStringLiteral("Formula error: result must be boolean type (%2)").arg(res.toString(), c->js_func_text));
            return;
        }

#if !defined(RELEASE_MODE) && defined(QT_DEBUG)
//        qDebug() << QString("[%1] %2").arg(param_values.join(","), c->js_func_text);
#endif

        if (res.toBool()) {
            next_step_id = c->step_id;

            // проверяем на наличие ошибок в заполняемых полях
            PropertyIDList checked_properties;
            if (_player->currentChoise() != nullptr)
                checked_properties << _player->currentChoise()->targetProperties();
            for (auto& p : _player->currentStep()->targetProperties()) {
                if (!checked_properties.contains(p))
                    checked_properties << p;
            }

            for (auto& p : qAsConst(checked_properties)) {
                auto property = _player->data()->property(p);
                for (auto& c : property.constraints()) {
                    Z_CHECK(c->type() == ConditionType::Required);
                    QVariant v = _player->value(p);
                    if ((v.type() == QVariant::String && v.toString().trimmed().isEmpty()) || !v.isValid())
                        error += Error(HtmlTools::bold(property.name(false)));
                }
            }

            if (error.isError()) {
                error += Error(ZF_TR(ZFT_EMPTY_REQUIRED_FIELDS)).setMain(true);
                return;
            }

            break;
        }
    }

    if (!next_step_id.isValid())
        return;

    steps << next_step_id;

    // если это функция, то надо сразу перейти на следующий шаг
    if (calc_functions && _player->step(next_step_id)->type() == ScriptStep::Function)
        _player->step(next_step_id)->nextStepHelper(calc_functions, steps, error);
}

void ScriptStep::possibleNextStepsHelper(QList<ScriptStepID>& steps) const
{
    for (auto c : qAsConst(_conditions)) {
        auto step = _player->step(c->step_id);
        if (step->type() == ScriptStep::Function)
            step->possibleNextStepsHelper(steps);
        else
            steps << step->id();
    }
}

QList<ScriptStepID> ScriptStep::nextStepId(bool calc_functions, Error& error) const
{
    QList<ScriptStepID> res;
    nextStepHelper(calc_functions, res, error);
    if (error.isError())
        res.clear();

    return res;
}

QList<ScriptStep*> ScriptStep::nextStep(bool calc_functions, Error& error) const
{
    auto id = nextStepId(calc_functions, error);
    if (id.isEmpty())
        return {};

    QList<ScriptStep*> res;
    for (auto& s : qAsConst(id)) {
        ScriptStep* step = _player->step(s);
        if (step == nullptr) {
            error = Error(QStringLiteral("Step not found: %1").arg(s.value()));
            return {};
        }
        res << step;
    }

    return res;
}

bool ScriptStep::isFirst() const
{
    return _first;
}

bool ScriptStep::isFinal() const
{
    return _conditions.isEmpty();
}

Error ScriptStep::tryNext() const
{
    Error error;
    if (!nextStepId(false, error).isEmpty())
        return Error();

    if (error.isError())
        return error;

    // TODO перевод ошибок

    if (isFinal())
        return Error("Final step reached");

    return Error("Required data not entered");
}

ScriptStep::ScriptStep(ScriptPlayer* player, const ScriptStepID& id_step, bool is_first, const QString& text, const QString& function_name,
                       const PropertyIDMap& arg_props, const QMap<QString, QVariant>& arg_values,
                       const PropertyID& function_result, const QList<std::shared_ptr<Condition>>& conditions,
                       const QMap<PropertyID, QVariant>& values, const PropertyIDList& target_properties)
    : _player(player)
    , _first(is_first)
    , _id_step(id_step)
    , _text(text)
    , _function_name(function_name)
    , _arg_props(arg_props)
    , _arg_values(arg_values)
    , _function_result(function_result)
    , _conditions(conditions)
    , _default_values(values)
    , _target_properties(target_properties)
{
    if (!text.isEmpty())
        _type = Say;
    else
        _type = Function;
}

ScriptChoiseID::ScriptChoiseID()
{
}

ScriptChoiseID::ScriptChoiseID(int id)
    : ID_Wrapper(id)
{
    Z_CHECK(id > 0);
}

QVariant ScriptChoiseID::variant() const
{
    return QVariant::fromValue(*this);
}

ScriptChoiseID ScriptChoiseID::fromVariant(const QVariant& v)
{
    return v.value<ScriptChoiseID>();
}

ScriptStepID::ScriptStepID()
{
}

ScriptStepID::ScriptStepID(int id)
    : ID_Wrapper(id)
{
    Z_CHECK(id > 0);
}

QVariant ScriptStepID::variant() const
{
    return QVariant::fromValue(*this);
}

ScriptStepID ScriptStepID::fromVariant(const QVariant& v)
{
    return v.value<ScriptStepID>();
}

ScriptPlayer::FunctionInfo::~FunctionInfo()
{
    if (wrapper != nullptr)
        delete wrapper;
}

ScriptPlayerState::ScriptPlayerState()
{
}

ScriptPlayerState::ScriptPlayerState(const ScriptStepID& current_step, const QMap<PropertyID, QVariant>& values,
                                     const QList<ScriptStepID>& history, const QMap<ScriptStepID, ScriptChoiseID>& choises)
    : _valid(true)
    , _current_step(current_step)
    , _values(values)
    , _history(history)
    , _choises(choises)
{
}

ScriptPlayerState::ScriptPlayerState(const ScriptPlayerState& state)
    : _valid(state._valid)
    , _current_step(state._current_step)
    , _values(state._values)
    , _history(state._history)
    , _choises(state._choises)
{
}

ScriptPlayerState& ScriptPlayerState::operator=(const ScriptPlayerState& state)
{
    _valid = state._valid;
    _current_step = state._current_step;
    _values = state._values;
    _history = state._history;
    _choises = state._choises;
    return *this;
}

bool ScriptPlayerState::isValid() const
{
    return _valid;
}

ScriptStepID ScriptPlayerState::currentStep() const
{
    return _current_step;
}

QMap<PropertyID, QVariant> ScriptPlayerState::values() const
{
    return _values;
}

QList<ScriptStepID> ScriptPlayerState::history() const
{
    return _history;
}

QMap<ScriptStepID, ScriptChoiseID> ScriptPlayerState::choises() const
{
    return _choises;
}

Error ScriptPlayerState::toJSON(QByteArray& data, const QMap<ScriptStepID, QString>& step_mapping,
                                const QMap<PropertyID, QString>& property_mapping) const
{
    Error full_error;
    Error error;

    QJsonObject json;
    json["step_id"] = stepCode(_current_step, step_mapping, error);
    full_error << error;

    QJsonArray variables;
    for (auto it = _values.constBegin(); it != _values.constEnd(); ++it) {
        QJsonObject variable;
        variable["name"] = propertyCode(it.key(), property_mapping, error);
        full_error << error;
        variable["value"] = Utils::variantToJsonValue(it.value());

        variables.append(variable);
    }
    json["variables"] = variables;

    QJsonArray history;
    for (int i = 0; i < _history.count(); i++) {
        QJsonObject h;
        h["step_id"] = stepCode(_history.at(i), step_mapping, error);
        full_error << error;
        h["prev_step_id"] = (i == 0 ? "" : stepCode(_history.at(i - 1), step_mapping, error));
        full_error << error;

        history.append(h);
    }
    json["history"] = history;

    QJsonArray choises;
    for (auto it = _choises.constBegin(); it != _choises.constEnd(); ++it) {
        QJsonObject h;
        h["step_id"] = stepCode(it.key(), step_mapping, error);
        full_error << error;
        h["choise"] = it.value().value();
        choises.append(h);
    }
    json["choises"] = choises;

    if (full_error.isError())
        return full_error;

    QJsonDocument doc(json);
    data = doc.toJson(QJsonDocument::JsonFormat::Indented);

    return Error();
}

Error ScriptPlayerState::fromJSON(const QByteArray& json, ScriptPlayerState& state, const QMap<ScriptStepID, QString>& step_mapping,
                                  const QMap<PropertyID, QString>& property_mapping)
{
    state = ScriptPlayerState();

    Error error;
    QJsonDocument doc = Utils::openJson(json, error);
    if (error.isError())
        return error;

    ScriptStepID current_step;
    QMap<PropertyID, QVariant> props;

    QString step_id = doc.object().value("step_id").toString();
    current_step = stepID(step_id, step_mapping, error);
    if (error.isError())
        return error;

    auto variables = doc.object().value("variables").toArray();
    for (const auto& var : variables) {
        auto var_obj = var.toObject();

        if (!var_obj.contains("name"))
            return Error("Не найдено: variables->name");
        if (!var_obj.contains("value"))
            return Error("Не найдено: variables->value");

        QString name = var_obj.value("name").toString();
        QVariant value = Utils::variantFromJsonValue(var_obj.value("value"));

        PropertyID property = propertyID(name, property_mapping, error);
        if (error.isError())
            return error;

        if (!property.isValid())
            continue; // игнорируем неизвестные свойства

        if (props.contains(property))
            return Error(QString("Дублирование переменной: %1").arg(name));

        props[property] = value;
    }

    QMap<ScriptStepID, ScriptStepID> history_map;
    QMap<ScriptStepID, ScriptChoiseID> choises_map;
    auto history = doc.object().value("history").toArray();
    for (const auto& h : history) {
        auto h_obj = h.toObject();

        if (!h_obj.contains("step_id"))
            return Error("Не найдено: history->step_id");
        if (!h_obj.contains("prev_step_id"))
            return Error("Не найдено: history->prev_step_id");

        ScriptStepID step_id = stepID(h_obj.value("step_id").toString(), step_mapping, error);
        if (error.isError())
            return error;

        QString prev_step_id_value = h_obj.value("prev_step_id").toString();
        ScriptStepID prev_step_id
            = (prev_step_id_value.isEmpty() ? ScriptStepID() : stepID(h_obj.value("prev_step_id").toString(), step_mapping, error));
        if (error.isError())
            return error;

        if (history_map.contains(step_id))
            return Error(QString("Дублирование шага %1 в history").arg(h_obj.value("step_id").toString()));

        history_map[step_id] = prev_step_id;
    }

    auto choises = doc.object().value("choises").toArray();
    for (const auto& h : choises) {
        auto h_obj = h.toObject();

        if (!h_obj.contains("step_id"))
            return Error("Не найдено: choises->step_id");
        if (!h_obj.contains("choise"))
            return Error("Не найдено: choises->choise");

        ScriptStepID step_id = stepID(h_obj.value("step_id").toString(), step_mapping, error);
        if (error.isError())
            return error;

        ScriptChoiseID choise_id = choiseID(h_obj.value("choise").toVariant().toString(), error);
        if (error.isError())
            return error;

        choises_map[step_id] = choise_id;
    }

    QList<ScriptStepID> history_list;
    while (!history_map.isEmpty()) {
        // ищем у кого нет предка
        int count = history_map.count();
        for (auto it = history_map.constBegin(); it != history_map.constEnd(); ++it) {
            ScriptStepID main_step = it.key();
            ScriptStepID prev_step = it.value();

            if (!prev_step.isValid() || !history_map.contains(prev_step)) {
                history_list << main_step;
                history_map.remove(main_step);
                break;
            }
        }
        if (count == history_map.count())
            return Error("Цикл в history");
    }

    state = ScriptPlayerState(current_step, props, history_list, choises_map);
    return {};
}

QString ScriptPlayerState::propertyCode(const PropertyID& property_id, const QMap<PropertyID, QString>& property_mapping, Error& error)
{
    Z_CHECK(property_id.isValid());
    error.clear();

    if (property_mapping.isEmpty())
        return QString::number(property_id.value());

    QString tag = property_mapping.value(property_id).trimmed();
    if (tag.isEmpty())
        error = Error(QString("Свойство %1 не найдено").arg(property_id.value()));

    return tag;
}

PropertyID ScriptPlayerState::propertyID(const QString& property_id, const QMap<PropertyID, QString>& property_mapping, Error& error)
{
    error.clear();

    QString p_id = property_id.trimmed();
    if (p_id.isEmpty()) {
        error = Error("Не задано имя переменной");
        return PropertyID();
    }

    if (!property_mapping.isEmpty())
        return property_mapping.key(p_id);

    bool ok;
    int id = property_id.toInt(&ok);
    if (!ok || id <= 0) {
        error = Error(QString("Неверное имя переменной: %1").arg(property_id));
        return PropertyID();
    }

    return PropertyID(id);
}

QString ScriptPlayerState::stepCode(const ScriptStepID& step_id, const QMap<ScriptStepID, QString>& step_mapping, Error& error)
{
    if (!step_id.isValid())
        return QString();

    if (step_mapping.isEmpty())
        return QString::number(step_id.value());

    QString tag = step_mapping.value(step_id).trimmed();
    if (tag.isEmpty())
        error = Error(QString("Шаг %1 не найден").arg(step_id.value()));

    return tag;
}

ScriptStepID ScriptPlayerState::stepID(const QString& step_id, const QMap<ScriptStepID, QString>& step_mapping, Error& error)
{
    error.clear();

    QString s_id = step_id.trimmed();
    if (s_id.isEmpty()) {
        error = Error("Не задан код шага");
        return ScriptStepID();
    }

    if (!step_mapping.isEmpty())
        return step_mapping.key(s_id);

    bool ok;
    int id = step_id.toInt(&ok);
    if (!ok || id <= 0) {
        error = Error(QString("Неверный код шага: %1").arg(step_id));
        return ScriptStepID();
    }

    return ScriptStepID(id);
}

ScriptChoiseID ScriptPlayerState::choiseID(const QString& choise_id, Error& error)
{
    bool ok;
    int id = choise_id.toInt(&ok);
    if (!ok || id <= 0) {
        error = Error(QString("Неверный код выбора: %1").arg(choise_id));
        return ScriptChoiseID();
    }

    return ScriptChoiseID(id);
}

} // namespace zf
