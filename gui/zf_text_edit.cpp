#include "zf_text_edit.h"
#include "zf_utils.h"

#include <QPaintEvent>
#include <QMimeData>

namespace zf
{
TextEdit::TextEdit(QWidget* parent)
    : QTextEdit(parent)
{
}

bool TextEdit::readOnlyBackground() const
{
    return _ro_background;
}

void TextEdit::setReadOnlyBackground(bool b)
{
    if (b == _ro_background)
        return;

    _ro_background = b;
    Utils::updatePalette(_ro_background && (!isEnabled() || isReadOnly()), false, this);
}

int TextEdit::maximumLength() const
{
    return _max_length;
}

void TextEdit::setMaximumLength(int length)
{
    if (_max_length == length)
        return;
    Z_CHECK(length > 0);
    _max_length = length;
}

void TextEdit::changeEvent(QEvent* event)
{
    QTextEdit::changeEvent(event);

    if (event->type() == QEvent::ReadOnlyChange || event->type() == QEvent::EnabledChange || event->type() == QEvent::PaletteChange)
        Utils::updatePalette(_ro_background && (!isEnabled() || isReadOnly()), false, this);
}

void TextEdit::paintEvent(QPaintEvent* event)
{
    QTextEdit::paintEvent(event);
}

void TextEdit::insertFromMimeData(const QMimeData* source)
{
    if (_max_length > 0 && source->hasText()) {
        QMimeData scopy;
        QString textToPaste = source->text();
        QString curText = toPlainText();
        auto curs = textCursor();
        QString selected_text = curs.selectedText();
        int selected_count = selected_text.length();
        int totalLength = curText.length() + textToPaste.length() - selected_count;
        if ((curText.length() - selected_count) >= _max_length) {
            scopy.setText(QString());
        } else if (totalLength > _max_length) {
            // Обязательно посмотри на PlainTextEdit. Там аналогичная функция с примечанием.
            int numToDelete = totalLength - _max_length;
            textToPaste = textToPaste.left(textToPaste.length() - numToDelete);
            scopy.setText(textToPaste);
        } else
            scopy.setText(source->text());
        QTextEdit::insertFromMimeData(&scopy);
    } else {
        QTextEdit::insertFromMimeData(source);
    }
}

void TextEdit::keyPressEvent(QKeyEvent* e)
{
    auto curs = textCursor();
    QString selected_text = curs.selectedText();
    int selected_count = selected_text.length();
    if ((_max_length <= 0) || ((toPlainText().length() - selected_count) < _max_length)) {
        QTextEdit::keyPressEvent(e);
    } else {
        QString txt = e->text();
        int key = e->key();
        bool delCondition = (key == Qt::Key_Delete) || (key == Qt::Key_Backspace) || (key == Qt::Key_Cancel);
        bool asciiCtrl = false;
        if (txt.length() == 1) {
            int sym = txt.toLatin1().at(0);
            if ((sym >= 0 && sym <= 31) || sym == 127)
                asciiCtrl = true;
        }
        if (txt.isEmpty() || delCondition || asciiCtrl) {
            QTextEdit::keyPressEvent(e);
        }
    }
    //QTextEdit::keyPressEvent(e);
}
} // namespace zf
