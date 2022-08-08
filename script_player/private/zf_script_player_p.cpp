#include "zf_script_player_p.h"
#include "zf_core.h"

namespace zf
{
ScriptPlayerFunctionJSWrapper::ScriptPlayerFunctionJSWrapper(QObject* parent)
    : QObject(parent)
{
}

ScriptPlayerFunctionJSWrapper::ScriptPlayerFunctionJSWrapper(const QString& function_name, ScriptPlayerFunction function, QObject* parent)
    : QObject(parent)
    , _function(function)
{
    Z_CHECK(!function_name.trimmed().isEmpty());
    setObjectName(function_name.trimmed());
}

bool ScriptPlayerFunctionJSWrapper::isValid() const
{
    return _function != nullptr;
}

QString ScriptPlayerFunctionJSWrapper::call(QVariantMap args) const
{
    _last_error.clear();

    if (!isValid()) {
        _last_error = Error(QStringLiteral("ScriptPlayerFunctionJSWrapper not initialized"));
        return QString();
    }

    for (auto it = args.constBegin(); it != args.constEnd(); ++it) {
        QString arg_name = it.key().trimmed().toLower();
        QVariant value = it.value();
        if (arg_name.isEmpty()) {
            _last_error = Error(QStringLiteral("Function %1: parameter name is empty").arg(objectName()));
            return QString();
        }
    }

    QVariant result;
    _last_error = _function(args, result);
    if (_last_error.isError())
        return QString();

    return Utils::variantToString(result, LocaleType::Universal);
}

QString ScriptPlayerFunctionJSWrapper::lastError() const
{
    return _last_error.fullText();
}

ScriptPlayerLoggerJSWrapper::ScriptPlayerLoggerJSWrapper(QObject* parent)
    : QObject(parent)
{
}

void ScriptPlayerLoggerJSWrapper::add(const QVariant& v1, const QVariant& v2, const QVariant& v3, const QVariant& v4, const QVariant& v5,
                                      const QVariant& v6, const QVariant& v7, const QVariant& v8, const QVariant& v9, const QVariant& v10)
{
    if (v1.toString().simplified().isEmpty())
        return;

    QVariantList l = {v1};

    if (v2 != js_undefinedValue)
        l << v2;
    if (v3 != js_undefinedValue)
        l << v3;
    if (v4 != js_undefinedValue)
        l << v4;
    if (v5 != js_undefinedValue)
        l << v5;
    if (v6 != js_undefinedValue)
        l << v6;
    if (v7 != js_undefinedValue)
        l << v7;
    if (v8 != js_undefinedValue)
        l << v8;
    if (v9 != js_undefinedValue)
        l << v9;
    if (v10 != js_undefinedValue)
        l << v10;

    emit sg_recordAdded(l);
}
} // namespace zf
