#ifndef ZF_TRANSLATOR_H
#define ZF_TRANSLATOR_H

#include <QString>
#include <QLocale>
#include "zf_global.h"

class QWidget;
class QTranslator;
class QAction;

namespace zf
{
//! Перевод пользовательского интерфейса
class ZCORESHARED_EXPORT Translator
{
public:
    //! Префикс виджетов для автоматического перевода
    static const QString PREFIX; // "_UI_"

    //! Перевод указанного виджета
    static void translate(QWidget* widget);
    //! Перевод списка экшенов
    static void translate(const QList<QAction*>& actions);
    //! Перевод произвольной переменной, заданной с помощью QT_TRID_NOOP
    static QString translate(const char* id, bool show_no_translate = true, const QString& default_value = QString());
    //! Перевод произвольной переменной, заданной с помощью QT_TRID_NOOP
    static QString translate(const QString& id, bool show_no_translate = true, const QString& default_value = QString());

    //! Инициализация языка
    static void installTranslation(
            //! Список файлов с переводами для разных языков. Путь к файлу - относительно директории приложения
            //! Если текущего языка в списке нет, то выбирается первый
            const QMap<QLocale::Language, QString>& translation,
            //! Желаемый язык интерфейса приложения. Если QLocale::AnyLanguage, то используется язык ОС
            QLocale::Language application_language);

private:
    static bool isNeedTranslate(const QString& text);
    //! Установить текст перевода для виджета
    static void setWidgetText(QWidget* widget, const QString& text);
    //! Перевод свойств виджета
    static void translateWidgetProperties(QWidget* widget);

    static QTranslator* _translator;
};

//! Перевод указанного виджета
inline void translate(QWidget* widget)
{
    Translator::translate(widget);
}
//! Перевод произвольной переменной, заданной с помощью QT_TRID_NOOP
inline QString translate(const char* id, bool show_no_translate = true, const QString& default_value = QString())
{
    return Translator::translate(id, show_no_translate, default_value);
}
//! Перевод произвольной переменной, заданной с помощью QT_TRID_NOOP
inline QString translate(const QString& id, bool show_no_translate = true, const QString& default_value = QString())
{
    return Translator::translate(id, show_no_translate, default_value);
}

} // namespace zf

#endif // TRANSLATOR_H
