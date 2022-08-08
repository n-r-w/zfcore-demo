#include "zf_logging.h"
#include "zf_core.h"
#include "zf_framework.h"
#include "zf_dbg_break.h"
#include <QDebug>
#include <QMessageBox>
#include <QDesktopServices>

#ifdef Q_OS_WIN
#include <qt_windows.h>
#endif

namespace zf
{
int Logging::_capture_counter = 0;
QString Logging::_captured_text;
QMap<QString, int> Logging::_output_counter;
int Logging::_indent = 0;

void Logging::beginCaptureErrorOutput()
{
    _capture_counter++;
    if (_capture_counter == 1)
        _captured_text.clear();
}

void Logging::endCaptureErrorOutput()
{
    if (_capture_counter <= 0) {
        Core::logError("Logging::endCaptureErrorOutput counter error");
        return;
    }
    _capture_counter--;
}

bool Logging::isCapturing()
{
    return _capture_counter > 0;
}

QString Logging::capturedText()
{
    return _captured_text;
}

static QString _cutQVariant(const QString& s)
{
    QString text = s;
    QString start = QStringLiteral("QVariant(");
    if (text.startsWith(start)) {
        text.remove(0, start.length());
        Z_CHECK(!text.isEmpty());
        text.remove(text.length() - 1, 1);
    }
    return text;
};

static void _variantToDebug(const QVariant& value)
{
    if (value.type() == QVariant::ByteArray || value.type() == QVariant::Icon || value.type() == QVariant::Pixmap)
        qDebug() << "binary";
    else
        qDebug() << value;
}

QMap<QString, QString> Logging::debugProperties(const QObject* object)
{
    Z_CHECK_NULL(object);
    QMap<QString, QString> res;

    QString object_info;
    if (object->objectName().isEmpty())
        object_info = object->metaObject()->className();
    else
        object_info = object->objectName();

    QString class_name = object->metaObject()->className();
    int type_id = QMetaType::type(QString(class_name + "*").toLatin1().constData());
    if (QMetaType::hasRegisteredDebugStreamOperator(type_id)) {
        // для объекта зарегистрирован собственный обработчик registerDebugStreamOperator
        res[object_info] = debugVariant(QVariant(type_id, object));
        return res;
    }

    for (int i = 0; i < object->metaObject()->propertyCount(); i++) {
        QMetaProperty property = object->metaObject()->property(i);
        if (!property.isReadable())
            continue;

        res[property.name()] = debugVariant(object->property(property.name()));
    }

    return res;
}

QString Logging::debugVariant(const QVariant& value)
{
    beginCaptureErrorOutput();
    if (value.type() == QVariant::UserType && !QMetaType::hasRegisteredDebugStreamOperator(value.userType())) {
        qDebug() << QStringLiteral("%1 - registerDebugStreamOperator not executed").arg(value.typeName());
    } else {
        _variantToDebug(value);
    }
    endCaptureErrorOutput();

    return _cutQVariant(capturedText());
}

Error Logging::debugFile(const QObject* object, const QDir& folder, bool open)
{
    Z_CHECK_NULL(object);
    return debugFile(object->metaObject()->className(), object->objectName(), debugProperties(object), folder, open);
}

Error Logging::debugFile(const QObject* object, QStandardPaths::StandardLocation location, bool open)
{
    return debugFile(object, QStandardPaths::writableLocation(location), open);
}

Error Logging::debugFile(const QVariant& value, const QDir& folder, bool open)
{
    return debugFile(value.typeName(), QString(), QMap<QString, QString> {{QString(), debugVariant(value)}}, folder, open);
}

Error Logging::debugFile(const QVariant& value, QStandardPaths::StandardLocation location, bool open)
{
    return debugFile(value, QStandardPaths::writableLocation(location), open);
}

Error Logging::debugFile(const QString& class_name, const QString& object_name, const QMap<QString, QString>& properties,
                         const QDir& folder, bool open)
{
    Z_CHECK(!class_name.isEmpty());

    if (!folder.exists())
        return Error(QString("folder not found: %1").arg(QDir::toNativeSeparators(folder.absolutePath())));

    int counter = _output_counter.value(class_name, 0) + 1;
    _output_counter[class_name] = counter;

    QFile f(folder.absoluteFilePath(QStringLiteral("%1_%2.txt").arg(Utils::validFileName(class_name)).arg(counter)));
    if (!f.open(QFile::WriteOnly | QFile::Truncate))
        return Error::fileIOError(f);
    QTextStream st(&f);
    st.setCodec("UTF-8");

    QString object_info;
    if (object_name.isEmpty())
        object_info = class_name;
    else
        object_info = QStringLiteral("%1 (%2)").arg(class_name).arg(object_name);

    st << object_info << QStringLiteral("\n\n");

    for (auto it = properties.constBegin(); it != properties.constEnd(); ++it) {
        if (it.key().isEmpty())
            st << it.value() << QStringLiteral("\n");
        else
            st << it.key() << QStringLiteral(": ") << it.value() << QStringLiteral("\n");
    }

    f.close();

    if (open)
        QDesktopServices::openUrl(QUrl::fromLocalFile(f.fileName()));

    return Error();
}

Error Logging::debugFile(const QString& class_name, const QString& object_name, const QMap<QString, QString>& properties,
                         QStandardPaths::StandardLocation location, bool open)
{
    return debugFile(class_name, object_name, properties, QStandardPaths::writableLocation(location), open);
}

Error Logging::debugFile(const QString& class_name, const QString& object_name, const QString& info, const QDir& folder, bool open)
{
    return debugFile(class_name, object_name, QMap<QString, QString> {{"", info}}, folder, open);
}

Error Logging::debugFile(const QString& class_name, const QString& object_name, const QString& info,
                         QStandardPaths::StandardLocation location, bool open)
{
    return debugFile(class_name, object_name, info, QStandardPaths::writableLocation(location), open);
}

void Logging::beginDebugOutput()
{
    _indent++;
}

void Logging::endDebugOutput()
{
    if (_indent == 0) {
        Core::logError("Logging::endDebPrint counter error");
        return;
    }
    _indent--;
}

int Logging::debugLevel()
{
    return _indent;
}

QString Logging::debugIndent()
{
    return _indent > 0 ? QString(_indent * 4, ' ') : QString();
}

void errorHandler(QtMsgType type, const QMessageLogContext& context, const QString& msg)
{        
    if (Logging::isCapturing()) {
        Logging::_captured_text += msg;
        return;
    }

    if (type == QtFatalMsg || type == QtWarningMsg || type == QtCriticalMsg) {
        static const QStringList warning_ignored_msg = {
            "Unable to free statement",
            "Unable to set geometry",
            "QLineEdit::setSelection",
            "Cannot create accessible child interface",
            "QWaitCondition",
            // https://bugreports.qt.io/browse/QTBUG-53979
            "Retrying to obtain clipboard",
            "Qt has caught an exception thrown from an event handler",
            "QItemSelectionModel: Selecting when no model has been set will result in a no-op.",
        };
        static const QStringList error_ignored_msg = {
            "QThread: Destroyed while thread is still running",
        };

        if (type == QtWarningMsg) {
            for (auto& m : warning_ignored_msg) {
                if (msg.contains(m))
                    return;
            }
        }
        if (type == QtFatalMsg || type == QtCriticalMsg) {
            for (auto& m : error_ignored_msg) {
                if (msg.contains(m))
                    return;
            }
        }
        Core::writeToLogStorage(msg, zf::InformationType::Error);
    }

    static QMutex _default_handler_mutex;

    if (type != QtFatalMsg) {
        _default_handler_mutex.lock();
        Framework::defaultMessageHandler()(type, context, msg);
        _default_handler_mutex.unlock();
        return;
    }

    _default_handler_mutex.lock();
    Framework::defaultMessageHandler()(type, context, msg);
    _default_handler_mutex.unlock();

    if (Core::mode().testFlag(CoreMode::Application) && !Core::mode().testFlag(CoreMode::Test) && Utils::isMainThread()) {
        QMessageBox::critical(nullptr, "Critical error",
                              Core::applicationVersionFull() + ", Host: " + Core::connectionInformation()->host()
                                  + ", User: " + Core::connectionInformation()->userInformation().login() + "\n" + msg);
    }

#ifdef QT_DEBUG
    debug_break();
#else
#ifdef Q_OS_LINUX
    std::exit(1);
#else
    *((int*)0) = 0; // вызываем запись креш-дампа
#endif
#endif
}

#ifdef Q_CC_MSVC
void invalidParameterHandler(
        const wchar_t* expression, const wchar_t* function, const wchar_t* file, unsigned int line, uintptr_t pReserved)
{
    Q_UNUSED(pReserved)

    qDebug() << QString("Invalid parameter detected in function %1. File: %2 Line: %3")
                        .arg(QString::fromWCharArray(function), QString::fromWCharArray(file))
                        .arg(line);
    qDebug() << QString("Expression: %1").arg(QString::fromWCharArray(expression));
}
#endif

} // namespace zf

