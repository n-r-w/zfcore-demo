#pragma once

#include "zf_global.h"
#include "zf_error.h"
#include <QStandardPaths>

class QDir;

namespace zf
{
class ZCORESHARED_EXPORT Logging
{
public:
    //! Получить все свойства объекта
    static QMap<QString, QString> debugProperties(const QObject* object);
    //! Перехватить вывод QVariant в QDebug
    static QString debugVariant(const QVariant& value);

    //! Вывести информацию об объекте в файл
    static Error debugFile(const QObject* object, const QDir& folder, bool open = true);
    //! Вывести информацию об объекте в файл
    static Error debugFile(const QObject* object, QStandardPaths::StandardLocation location = QStandardPaths::DownloadLocation,
                           bool open = true);

    //! Вывести информацию о QVariant в файл
    static Error debugFile(const QVariant& value, const QDir& folder, bool open = true);
    //! Вывести информацию об объекте в файл
    static Error debugFile(const QVariant& value, QStandardPaths::StandardLocation location = QStandardPaths::DownloadLocation,
                           bool open = true);

    //! Вывести информацию об объекте в файл
    static Error debugFile(const QString& class_name, const QString& object_name, const QMap<QString, QString>& properties,
                           const QDir& folder, bool open);
    //! Вывести информацию об объекте в файл
    static Error debugFile(const QString& class_name, const QString& object_name, const QMap<QString, QString>& properties,
                           QStandardPaths::StandardLocation location = QStandardPaths::DownloadLocation, bool open = true);
    //! Вывести информацию об объекте в файл
    static Error debugFile(const QString& class_name, const QString& object_name, const QString& info, const QDir& folder, bool open);
    //! Вывести информацию об объекте в файл
    static Error debugFile(const QString& class_name, const QString& object_name, const QString& info,
                           QStandardPaths::StandardLocation location = QStandardPaths::DownloadLocation, bool open = true);

    //! Начало вывода объекта в debug
    static void beginDebugOutput();
    //! Окончание вывода объекта в debug
    static void endDebugOutput();
    //! Текущий уровень вложенности вывода объектов в debug
    static int debugLevel();
    //! Текущий уровень вложенности вывода объектов в debug в виде строки сдвига
    static QString debugIndent();

private:
    //! Начать перехват вывода в qDebug
    static void beginCaptureErrorOutput();
    //! Завершить перехват вывода в qDebug
    static void endCaptureErrorOutput();
    //! Находимся ли в процессе перехваты вывода в qDebug
    static bool isCapturing();
    //! Захваченный текст
    static QString capturedText();

    static QString _captured_text;
    static int _capture_counter;
    static QMap<QString, int> _output_counter;

    //! Текущий сдвиг (уровень вложенности)
    static int _indent;

    friend void errorHandler(QtMsgType type, const QMessageLogContext& context, const QString& msg);
};

//! Обработка вывода в qDebug/консоль
void errorHandler(QtMsgType type, const QMessageLogContext& context, const QString& msg);

#ifdef Q_CC_MSVC
//! Обработка ошибок "Invalid parameter passed to C runtime function"
void invalidParameterHandler(const wchar_t* expression, const wchar_t* function, const wchar_t* file, unsigned int line,
        uintptr_t pReserved);
#endif

} // namespace zf
