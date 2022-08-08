#pragma once

#include "zf_script_player.h"

namespace zf
{
//! JS обертка вокруг ScriptPlayerFunction
class ScriptPlayerFunctionJSWrapper : public QObject
{
    Q_OBJECT
public:
    ScriptPlayerFunctionJSWrapper(QObject* parent = nullptr);
    ScriptPlayerFunctionJSWrapper(const QString& function_name, ScriptPlayerFunction function, QObject* parent = nullptr);

    //! Объект инициализирован
    bool isValid() const;

    //! Вызов функции из JS скрипта. Если была ошибка, то всегда возвращает пустую строку. Но обратное не верно,
    //! т.е. при возврате пустой строки ошибки может не быть. Надо проверять lastError
    Q_INVOKABLE QString call(QVariantMap args) const;

    //! Ошибка, которая была при вызове call если она есть. Если ошибки нет - пустая строка
    Q_INVOKABLE QString lastError() const;

private:
    ScriptPlayerFunction _function = nullptr;
    mutable Error _last_error;
};

//! JS обертка для логгера ошибок внутри js
class ScriptPlayerLoggerJSWrapper : public QObject
{
    Q_OBJECT
public:
    //! Значение для неинициализированного параметра QVariant
    static const QVariant js_undefinedValue;

    ScriptPlayerLoggerJSWrapper(QObject* parent);

    //! Добавить значение в журнал
    Q_INVOKABLE void add(const QVariant& v1, const QVariant& v2 = js_undefinedValue, const QVariant& v3 = js_undefinedValue,
                         const QVariant& v4 = js_undefinedValue, const QVariant& v5 = js_undefinedValue,
                         const QVariant& v6 = js_undefinedValue, const QVariant& v7 = js_undefinedValue,
                         const QVariant& v8 = js_undefinedValue, const QVariant& v9 = js_undefinedValue,
                         const QVariant& v10 = js_undefinedValue);

signals:
    //! Добавлено значение в журнал
    void sg_recordAdded(const QVariantList& value);
};

} // namespace zf
