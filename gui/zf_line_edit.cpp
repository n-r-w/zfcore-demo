#include "zf_line_edit.h"
#include "QDebug"
#include "zf_utils.h"

#include <QPaintEvent>
#include <QMenu>
#include <QClipboard>
#include <QStyleHints>

namespace zf
{
LineEdit::LineEdit(QWidget* parent)
    : QLineEdit(parent)
{
    updateStyle();
}

LineEdit::LineEdit(const QString& text, QWidget* parent)
    : LineEdit(parent)
{
    setText(text);
    updateStyle();
}

bool LineEdit::isStandardReadOnlyBehavior() const
{
    return _standard_ro_behavior;
}

void LineEdit::setStandardReadOnlyBehavior(bool b)
{
    if (b == _standard_ro_behavior)
        return;

    _standard_ro_behavior = b;

    if (b)
        QLineEdit::setReadOnly(_read_only);
    else
        QLineEdit::setReadOnly(false);

    updateStyle();
}

bool LineEdit::isReadOnly() const
{
    return _standard_ro_behavior ? QLineEdit::isReadOnly() : _read_only;
}

void LineEdit::setReadOnly(bool b)
{
    if (_read_only == b)
        return;

    _read_only = b;
    updateStyle();

    if (_standard_ro_behavior)
        QLineEdit::setReadOnly(b);

    if (_read_only)
        setCursorPosition(0);
}

void LineEdit::setText(const QString &s)
{
    QLineEdit::setText(s);
    if (isReadOnly())
        setCursorPosition(0);
}

void LineEdit::changeEvent(QEvent* event)
{
    QLineEdit::changeEvent(event);

    if (event->type() == QEvent::ReadOnlyChange || event->type() == QEvent::EnabledChange
        || event->type() == QEvent::PaletteChange)
        updateStyle();
}

void LineEdit::paintEvent(QPaintEvent* event)
{
    QLineEdit::paintEvent(event);
}

void LineEdit::focusInEvent(QFocusEvent* e)
{
    QLineEdit::focusInEvent(e);
    // необходимо для обновления рамки
    update();
}

void LineEdit::focusOutEvent(QFocusEvent* e)
{
    QLineEdit::focusOutEvent(e);
    // необходимо для обновления рамки
    update();
}

void LineEdit::contextMenuEvent(QContextMenuEvent *event)
{
    if (_standard_ro_behavior) {
        QLineEdit::contextMenuEvent(event);
        return;
    }

    QMenu* menu = createStandardContextMenu();
    menu->setAttribute(Qt::WA_DeleteOnClose);
    menu->popup(event->globalPos());

    /*
    QMenu* menu = new QMenu(this);
    menu->setAttribute(Qt::WA_DeleteOnClose);
    QAction* action = NULL;

    QLatin1Char tab = QLatin1Char('\t');

    if (!_read_only) {
        action = menu->addAction("Отменить" + tab + QKeySequence(QKeySequence::Undo).toString());
        action->setEnabled(isUndoAvailable());
        connect(action, &QAction::triggered, this, &LineEdit::undo);

        action = menu->addAction("Повторить" + tab + QKeySequence(QKeySequence::Redo).toString());
        action->setEnabled(isRedoAvailable());
        connect(action, &QAction::triggered, this, &LineEdit::redo);

        menu->addSeparator();

        action = menu->addAction("Вырезать" + tab + QKeySequence(QKeySequence::Cut).toString());
        action->setEnabled(hasSelectedText() && echoMode() == QLineEdit::Normal);
        connect(action, &QAction::triggered, this, &LineEdit::cut);
    }

    action = menu->addAction("Копировать" + tab + QKeySequence(QKeySequence::Copy).toString());
    action->setEnabled(hasSelectedText() && echoMode() == QLineEdit::Normal);
    connect(action, &QAction::triggered, this, &LineEdit::copy);

    if (!_read_only) {
        action = menu->addAction("Вставить" + tab + QKeySequence(QKeySequence::Paste).toString());
        action->setEnabled(!QApplication::clipboard()->text().isEmpty());
        connect(action, &QAction::triggered, this, &LineEdit::paste);

        action = menu->addAction("Удалить" + tab + QKeySequence(QKeySequence::Delete).toString());
        action->setEnabled(!text().isEmpty() && hasSelectedText());
        connect(action, &QAction::triggered, this, &LineEdit::sl_deleteSelected);
    }

    if (!menu->isEmpty())
        menu->addSeparator();

    action = menu->addAction("Выделить все" + tab + QKeySequence(QKeySequence::SelectAll).toString());
    action->setEnabled(!text().isEmpty() && !(selectedText() == text()));
    connect(action, &QAction::triggered, this, &LineEdit::selectAll);

    menu->popup(event->globalPos());*/
}

void LineEdit::keyPressEvent(QKeyEvent *kEvent)
{
    if (!_standard_ro_behavior && _read_only) {
        QKeySequence ks = Utils::toKeySequence(kEvent);
        if (ks == QKeySequence(QKeySequence::Paste) || ks == QKeySequence(QKeySequence::Cut) || ks == QKeySequence(QKeySequence::Undo)
            || ks == QKeySequence(QKeySequence::Redo)) {
            // Блокируем комбинации клавиш
            kEvent->setAccepted(false);
            return;
        }

        if (kEvent->key() == Qt::Key_Backspace || kEvent->key() == Qt::Key_Delete) {
            // Блокируем удаление
            kEvent->setAccepted(false);
            return;
        }

        // Копируем
        if (kEvent->modifiers() == Qt::ControlModifier && kEvent->key() == Qt::Key_C) {
            QLineEdit::keyPressEvent(kEvent);
            return;
        }

        if (!kEvent->text().isEmpty()) {
            // Блокируем ввод текста
            kEvent->setAccepted(false);
            return;
        }
    }

    QLineEdit::keyPressEvent(kEvent);
}

void LineEdit::onContextMenuCreated(QMenu* m)
{
    Q_UNUSED(m);
}

void LineEdit::sl_deleteSelected()
{
    if (_standard_ro_behavior)
        return;

    del();
}

void LineEdit::updateStyle()
{
    Utils::updatePalette(isReadOnly() || !isEnabled(), false, this);
}

#define ACCEL_KEY(k) QLatin1Char('\t') + QKeySequence(k).toString(QKeySequence::NativeText)

static void setActionIcon(QAction* action, const QString& name)
{
    const QIcon icon = QIcon::fromTheme(name);
    if (!icon.isNull())
        action->setIcon(icon);
}

QMenu* LineEdit::createStandardContextMenu()
{
    QMenu* popup = new QMenu(this);
    QAction* action = nullptr;

    if (!isReadOnly()) {
        action = popup->addAction(QLineEdit::tr("&Undo") + ACCEL_KEY(QKeySequence::Undo));
        action->setEnabled(isUndoAvailable());
        setActionIcon(action, QStringLiteral("edit-undo"));
        connect(action, SIGNAL(triggered()), this, SLOT(undo()));

        action = popup->addAction(QLineEdit::tr("&Redo") + ACCEL_KEY(QKeySequence::Redo));
        action->setEnabled(isRedoAvailable());
        setActionIcon(action, QStringLiteral("edit-redo"));
        connect(action, SIGNAL(triggered()), this, SLOT(redo()));

        popup->addSeparator();
    }

    if (!isReadOnly()) {
        action = popup->addAction(QLineEdit::tr("Cu&t") + ACCEL_KEY(QKeySequence::Cut));
        action->setEnabled(!isReadOnly() && hasSelectedText() && echoMode() == QLineEdit::Normal);
        setActionIcon(action, QStringLiteral("edit-cut"));
        connect(action, SIGNAL(triggered()), this, SLOT(cut()));
    }

    action = popup->addAction(QLineEdit::tr("&Copy") + ACCEL_KEY(QKeySequence::Copy));
    action->setEnabled(hasSelectedText() && echoMode() == QLineEdit::Normal);
    setActionIcon(action, QStringLiteral("edit-copy"));
    connect(action, SIGNAL(triggered()), this, SLOT(copy()));

    if (!isReadOnly()) {
        action = popup->addAction(QLineEdit::tr("&Paste") + ACCEL_KEY(QKeySequence::Paste));
        action->setEnabled(!isReadOnly() && !QGuiApplication::clipboard()->text().isEmpty());
        setActionIcon(action, QStringLiteral("edit-paste"));
        connect(action, SIGNAL(triggered()), this, SLOT(paste()));
    }

    if (!isReadOnly()) {
        action = popup->addAction(QLineEdit::tr("Delete"));
        action->setEnabled(!isReadOnly() && !text().isEmpty() && hasSelectedText());
        setActionIcon(action, QStringLiteral("edit-delete"));
        connect(action, SIGNAL(triggered()), this, SLOT(sl_deleteSelected()));
    }

    if (!popup->isEmpty())
        popup->addSeparator();

    action = popup->addAction(QLineEdit::tr("Select All") + ACCEL_KEY(QKeySequence::SelectAll));
    action->setEnabled(!text().isEmpty() && !(selectedText() == text()));
    setActionIcon(action, QStringLiteral("edit-select-all"));
    connect(action, SIGNAL(triggered()), SLOT(selectAll()));

    onContextMenuCreated(popup);

    return popup;
}

} // namespace zf
