#include "zf_squeezed_label.h"
#include "zf_utils.h"

#include <QAction>
#include <QContextMenuEvent>
#include <QDesktopWidget>
#include <QMenu>
#include <QClipboard>
#include <QApplication>
#include <QTextDocument>

namespace zf
{
SqueezedTextLabel::SqueezedTextLabel(const QString& text, QWidget* parent)
    : QLabel(parent)

{
    setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed));
    _full_text = text;
    squeezeTextToLabel();
}

SqueezedTextLabel::SqueezedTextLabel(QWidget* parent)
    : QLabel(parent)
{
    setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed));
}

SqueezedTextLabel::~SqueezedTextLabel()
{
}

void SqueezedTextLabel::resizeEvent(QResizeEvent*)
{
    squeezeTextToLabel();
}

QSize SqueezedTextLabel::minimumSizeHint() const
{
    QSize sh = QLabel::minimumSizeHint();
    sh.setWidth(-1);
    return sh;
}

QSize SqueezedTextLabel::sizeHint() const
{
    int maxWidth = QApplication::desktop()->screenGeometry(this).width() * 3 / 4;
    QFontMetrics fm(fontMetrics());
    int textWidth = fm.boundingRect(_full_text).width();
    if (textWidth > maxWidth) {
        textWidth = maxWidth;
    }
    const int chromeWidth = width() - contentsRect().width();
    return QSize(textWidth + chromeWidth, QLabel::sizeHint().height());
}

void SqueezedTextLabel::setIndent(int indent)
{
    QLabel::setIndent(indent);
    squeezeTextToLabel();
}

void SqueezedTextLabel::setMargin(int margin)
{
    QLabel::setMargin(margin);
    squeezeTextToLabel();
}

void SqueezedTextLabel::setText(const QString& text)
{
    _full_text = text;
    squeezeTextToLabel();
}

void SqueezedTextLabel::clear()
{
    _full_text.clear();
    QLabel::clear();
}

void SqueezedTextLabel::squeezeTextToLabel()
{
    QFontMetrics fm(fontMetrics());
    const int labelWidth = contentsRect().width();

    QStringList squeezedLines;
    bool squeezed = false;
    const auto textLines = _full_text.split(QLatin1Char('\n'));
    squeezedLines.reserve(textLines.size());
    for (const QString& line : textLines) {
        int lineWidth = fm.boundingRect(line).width();
        if (lineWidth > labelWidth) {
            squeezed = true;
            squeezedLines << fm.elidedText(line, _elide_mode, labelWidth);
        } else {
            squeezedLines << line;
        }
    }

    if (squeezed) {
        QLabel::setText(squeezedLines.join(QLatin1Char('\n')));

        setToolTip(Utils::stringToMultiline(_full_text.simplified(), qMax(width(), Utils::scaleUI(400)), fontMetrics()));

    } else {
        QLabel::setText(_full_text);
        setToolTip(QString());
    }
}

QRect SqueezedTextLabel::contentsRect() const
{
    // calculation according to API docs for QLabel::indent
    const int margin = this->margin();
    int indent = this->indent();
    if (indent < 0) {
        if (frameWidth() == 0) {
            indent = 0;
        } else {
            indent = fontMetrics().horizontalAdvance(QLatin1Char('x')) / 2 - margin;
        }
    }

    QRect result = QLabel::contentsRect();
    if (indent > 0) {
        const int alignment = this->alignment();
        if (alignment & Qt::AlignLeft) {
            result.setLeft(result.left() + indent);
        }
        if (alignment & Qt::AlignTop) {
            result.setTop(result.top() + indent);
        }
        if (alignment & Qt::AlignRight) {
            result.setRight(result.right() - indent);
        }
        if (alignment & Qt::AlignBottom) {
            result.setBottom(result.bottom() - indent);
        }
    }

    result.adjust(margin, margin, -margin, -margin);
    return result;
}

void SqueezedTextLabel::setAlignment(Qt::Alignment alignment)
{
    // save fullText and restore it
    QString tmpFull(_full_text);
    QLabel::setAlignment(alignment);
    _full_text = tmpFull;
}

Qt::TextElideMode SqueezedTextLabel::textElideMode() const
{
    return _elide_mode;
}

void SqueezedTextLabel::setTextElideMode(Qt::TextElideMode mode)
{
    _elide_mode = mode;
    squeezeTextToLabel();
}

QString SqueezedTextLabel::fullText() const
{
    return _full_text;
}

bool SqueezedTextLabel::isSqueezed() const
{
    return _full_text != text();
}

void SqueezedTextLabel::contextMenuEvent(QContextMenuEvent* ev)
{
    // We want to reimplement "Copy" to include the elided text.
    // But this means reimplementing the full popup menu, so no more
    // copy-link-address or copy-selection support anymore, since we
    // have no access to the QTextDocument.
    // Maybe we should have a boolean flag in KSqueezedTextLabel itself for
    // whether to show the "Copy Full Text" custom popup?
    // For now I chose to show it when the text is squeezed; when it's not, the
    // standard popup menu can do the job (select all, copy).

    if (isSqueezed()) {
        QMenu menu(this);

        QAction* act = new QAction(tr("&Copy"), &menu);
        connect(act, &QAction::triggered, this, [this]() { QApplication::clipboard()->setText(selectedText()); });
        menu.addAction(act);
        act->setEnabled(!selectedText().isEmpty());

        menu.addSeparator();

        act = new QAction(tr("Select &All"), &menu);
        connect(act, &QAction::triggered, this, [this]() { setSelection(0, text().length()); });
        menu.addAction(act);

        ev->accept();
        menu.exec(ev->globalPos());

    } else {
        QLabel::contextMenuEvent(ev);
    }
}

void SqueezedTextLabel::mouseReleaseEvent(QMouseEvent* ev)
{
    QLabel::mouseReleaseEvent(ev);

    /* тут какая-то странная хрень от авторов оригинального SqueezedTextLabel. Нам она не нужна
      if (QApplication::clipboard()->supportsSelection() && textInteractionFlags() != Qt::NoTextInteraction && ev->button() == Qt::LeftButton
        && !_full_text.isEmpty() && hasSelectedText()) {
        // Expand "..." when selecting with the mouse
        QString txt = selectedText();
        const QChar ellipsisChar(0x2026); // from qtextengine.cpp
        const int dotsPos = txt.indexOf(ellipsisChar);
        if (dotsPos > -1) {
            // Ex: abcde...yz, selecting de...y  (selectionStart=3)
            // charsBeforeSelection = selectionStart = 2 (ab)
            // charsAfterSelection = 1 (z)
            // final selection length= 26 - 2 - 1 = 23
            const int start = selectionStart();
            int charsAfterSelection = text().length() - start - selectedText().length();
            txt = _full_text;
            // Strip markup tags
            if (textFormat() == Qt::RichText || (textFormat() == Qt::AutoText && Qt::mightBeRichText(txt))) {
                txt.remove(QRegExp(QStringLiteral("<[^>]*>")));
                // account for stripped characters
                charsAfterSelection -= _full_text.length() - txt.length();
            }
            txt = txt.mid(selectionStart(), txt.length() - start - charsAfterSelection);
        }
        QApplication::clipboard()->setText(txt, QClipboard::Selection);
    } else {
        QLabel::mouseReleaseEvent(ev);
    }*/
}

} // namespace zf
