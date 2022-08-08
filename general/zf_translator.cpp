#include "zf_translator.h"
#include "zf_core.h"
#include <QAbstractButton>
#include <QApplication>
#include <QDebug>
#include <QLabel>
#include <QLineEdit>
#include <QGroupBox>
#include <QTranslator>
#include <QPlainTextEdit>
#include <QComboBox>

#include "zf_collapsible_group_box.h"
#include "zf_checkbox.h"

namespace zf
{
const QString Translator::PREFIX = "_UI_";
QTranslator* Translator::_translator = nullptr;

void Translator::translate(QWidget* widget)
{
    Z_CHECK_NULL(widget);
    QList<QWidget*> widgets = widget->findChildren<QWidget*>();

    for (auto w : widgets) {
        translateWidgetProperties(w);

        if (isNeedTranslate(w->objectName()))
            setWidgetText(w, translate(w->objectName()));
    }
}

void Translator::translate(const QList<QAction*>& actions)
{
    for (QAction* a : actions) {
        Z_CHECK_NULL(a);
        QString object_name;
        if (a->menu() != nullptr)
            object_name = a->menu()->objectName();
        else
            object_name = a->objectName();

        if (isNeedTranslate(object_name)) {
            QString translated = translate(object_name);

            a->setText(translated);
            a->setIconText(translated);
        }
        if (a->menu() != nullptr)
            translate(a->menu()->actions());
    }
}

QString Translator::translate(const char* id, bool show_no_translate, const QString& default_value)
{
    Z_CHECK_NULL(id);
    if (QString(id).contains(' '))
        return QString();

    QString translation = qtTrId(id);
    if (translation == utf(id)) {
        if (show_no_translate) {
            translation = QString("%1: no translation").arg(id);
            qWarning() << "WARNING: translation not found:" << id;

        } else if (!default_value.isEmpty()) {
            translation = default_value;
        }
    }

    return translation;
}

QString Translator::translate(const QString& id, bool show_no_translate, const QString& default_value)
{
    if (id.isEmpty() || id.contains(' '))
        return QString();

    Z_CHECK_NULL(id);
    QString translation = qtTrId(id.toLocal8Bit().constData());
    if (comp(translation, utf(id.toLocal8Bit().constData()))) {
        if (show_no_translate) {
            translation = QString("%1: no translation").arg(id);
            qWarning() << "WARNING: translation not found:" << id;

        } else if (!default_value.isEmpty()) {
            translation = default_value;
        }
    }

    return translation;
}

void Translator::installTranslation(
        const QMap<QLocale::Language, QString>& translation, QLocale::Language application_language)
{
    if (translation.isEmpty())
        return;

    if (application_language == QLocale::AnyLanguage)
        application_language = QLocale::system().language();

    QString translation_file = translation.value(application_language);
    if (translation_file.isEmpty())
        translation_file = translation.first();

    QTranslator* tr = new QTranslator(qApp);

    if (!tr->load(translation_file)) {
        delete tr;
        Core::logError(QString("Invalid translation file: %1").arg(translation_file));
        return;
    }

    if (_translator != nullptr) {
        qApp->removeTranslator(_translator);
        delete _translator;
    }

    qApp->installTranslator(tr);
    _translator = tr;
}

bool Translator::isNeedTranslate(const QString& text)
{
    return text.startsWith(PREFIX, Qt::CaseInsensitive);
}

void Translator::setWidgetText(QWidget* widget, const QString& text)
{
    if (auto w = qobject_cast<QLabel*>(widget)) {
        w->setText(text);
        return;
    }

    if (auto w = qobject_cast<QLineEdit*>(widget)) {
        w->setText(text);
        return;
    }

    if (auto w = qobject_cast<QPlainTextEdit*>(widget)) {
        w->setPlainText(text);
        return;
    }

    if (auto w = qobject_cast<QTextEdit*>(widget)) {
        w->setPlainText(text);
        return;
    }

    if (auto w = qobject_cast<QAbstractButton*>(widget)) {
        w->setText(text);
        return;
    }

    if (auto w = qobject_cast<zf::CollapsibleGroupBox*>(widget)) {
        w->setTitle(text);
        return;
    }

    if (auto w = qobject_cast<zf::CollapsibleGroupBox*>(widget)) {
        w->setTitle(text);
        return;
    }

    if (auto parent_tab = Utils::findParentWidget<QTabWidget*>(widget)) {
        int index = parent_tab->indexOf(widget);
        if (index >= 0) {
            parent_tab->setTabText(index, text);
            return;
        }
    }
}

void Translator::translateWidgetProperties(QWidget* widget)
{
    if (isNeedTranslate(widget->toolTip()))
        widget->setToolTip(translate(widget->toolTip()));

    if (isNeedTranslate(widget->statusTip()))
        widget->setStatusTip(translate(widget->statusTip()));

    if (auto w = qobject_cast<QGroupBox*>(widget)) {
        if (isNeedTranslate(w->title()))
            w->setTitle(translate(w->title()));

        return;
    }

    if (auto w = qobject_cast<QLineEdit*>(widget)) {
        if (isNeedTranslate(w->placeholderText()))
            w->setPlaceholderText(translate(w->placeholderText()));
        if (isNeedTranslate(w->text()))
            w->setText(translate(w->text()));

        return;
    }

    if (auto w = qobject_cast<QPlainTextEdit*>(widget)) {
        if (isNeedTranslate(w->placeholderText()))
            w->setPlaceholderText(translate(w->placeholderText()));
        if (isNeedTranslate(w->toPlainText()))
            w->setPlainText(translate(w->toPlainText()));

        return;
    }

    if (auto w = qobject_cast<QTextEdit*>(widget)) {
        if (isNeedTranslate(w->placeholderText()))
            w->setPlaceholderText(translate(w->placeholderText()));
        if (isNeedTranslate(w->toPlainText()))
            w->setPlainText(translate(w->toPlainText()));

        return;
    }

#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
    if (auto w = qobject_cast<QComboBox*>(widget)) {
        if (isNeedTranslate(w->placeholderText()))
            w->setPlaceholderText(translate(w->placeholderText()));

        return;
    }
#endif

    if (auto w = qobject_cast<QAbstractButton*>(widget)) {
        if (isNeedTranslate(w->text()))
            w->setText(translate(w->text()));

        return;
    }

    if (auto w = qobject_cast<QLabel*>(widget)) {
        if (isNeedTranslate(w->text()))
            w->setText(translate(w->text()));

        return;
    }

    if (auto w = qobject_cast<zf::CollapsibleGroupBox*>(widget)) {
        if (isNeedTranslate(w->title()))
            w->setTitle(translate(w->title()));

        return;
    }

    if (auto w = qobject_cast<QTabWidget*>(widget)) {
        for (int i = 0; i < w->count(); i++) {
            if (isNeedTranslate(w->tabText(i)))
                w->setTabText(i, translate(w->tabText(i)));
        }
    }
}

} // namespace zf
