#include "zf_plain_text_edit.h"
#include "zf_utils.h"
#include "zf_core.h"
#include "zf_framework.h"
#include "zf_ui_size.h"
#include "zf_colors_def.h"

#include <QPaintEvent>
#include <QPainter>
#include <QScrollBar>
#include <QMimeData>

namespace zf
{
PlainTextEdit::PlainTextEdit(QWidget* parent)
    : QPlainTextEdit(parent)
    , _object_extension(new ObjectExtension(this))
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    connect(this, &PlainTextEdit::textChanged, this, &PlainTextEdit::sl_textChanged);
    setContentsMargins(0, 0, 0, 0);
    setViewportMargins(0, -2, 0, 0); // надо сдвинуть текст вверх, чтобы не было отличия с QLineEdit
    //    updateScrollbar();
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    connect(horizontalScrollBar(), &QScrollBar::valueChanged, this, &PlainTextEdit::sl_scroll);
    connect(verticalScrollBar(), &QScrollBar::valueChanged, this, &PlainTextEdit::sl_scroll);

    Framework::internalCallbackManager()->registerObject(this, "sl_callback");
}

PlainTextEdit::~PlainTextEdit()
{
    delete _object_extension;
}

bool PlainTextEdit::objectExtensionDestroyed() const
{
    return _object_extension->objectExtensionDestroyed();
}

void PlainTextEdit::objectExtensionDestroy()
{
    _object_extension->objectExtensionDestroy();
}

QVariant PlainTextEdit::objectExtensionGetData(const QString& data_key) const
{
    return _object_extension->objectExtensionGetData(data_key);
}

bool PlainTextEdit::objectExtensionSetData(const QString& data_key, const QVariant& value, bool replace) const
{
    return _object_extension->objectExtensionSetData(data_key, value, replace);
}

void PlainTextEdit::objectExtensionRegisterUseExternal(I_ObjectExtension* i)
{
    _object_extension->objectExtensionRegisterUseExternal(i);
}

void PlainTextEdit::objectExtensionUnregisterUseExternal(I_ObjectExtension* i)
{
    _object_extension->objectExtensionUnregisterUseExternal(i);
}

void PlainTextEdit::objectExtensionDeleteInfoExternal(I_ObjectExtension* i)
{
    _object_extension->objectExtensionDeleteInfoExternal(i);
}

void PlainTextEdit::objectExtensionRegisterUseInternal(I_ObjectExtension* i)
{
    _object_extension->objectExtensionRegisterUseInternal(i);
}

void PlainTextEdit::objectExtensionUnregisterUseInternal(I_ObjectExtension* i)
{
    _object_extension->objectExtensionUnregisterUseInternal(i);
}

bool PlainTextEdit::readOnlyBackground() const
{
    return _ro_background;
}

void PlainTextEdit::setReadOnlyBackground(bool b)
{
    if (b == _ro_background)
        return;

    _ro_background = b;
    Framework::internalCallbackManager()->addRequest(this, Framework::PLAIN_TEXT_EDIT_UPDATE_STYLE);
}

int PlainTextEdit::maximumLines() const
{
    return _maximum_lines;
}

void PlainTextEdit::setMaximumLines(int n)
{
    if (_maximum_lines == n)
        return;

    Z_CHECK(n > 0);

    _maximum_lines = n;
    updateGeometry();
}

int PlainTextEdit::minimumLines() const
{
    return _minimum_lines;
}

void PlainTextEdit::setMinimumLines(int n)
{
    if (_minimum_lines == n)
        return;

    Z_CHECK(n > 0);

    _minimum_lines = n;
    updateGeometry();
}

QSize PlainTextEdit::sizeHint() const
{
    QSize size = QPlainTextEdit::sizeHint();

    auto doc = document()->clone();
    doc->setTextWidth(contentsRect().width()); //299

    int min_height = _minimum_lines == 1 ? 0
                                         : qMax((int)doc->size().height() + fontMetrics().descent(),
                                                fontMetrics().lineSpacing() * _minimum_lines + fontMetrics().ascent());
    int calc_height = qMax(min_height, qMin((int)doc->size().height() + fontMetrics().descent(),
                                            fontMetrics().lineSpacing() * _maximum_lines + fontMetrics().ascent()));

    if (isSingleLine(calc_height)) {
        // одна строка
        QSize d_size = UiSizes::defaultLineEditSize(font());
        d_size.setWidth(size.width());
        size = d_size;

    } else {
        size.setHeight(calc_height);
    }

    delete doc;
    return size;
}

QSize PlainTextEdit::minimumSizeHint() const
{
    return sizeHint();
}

void PlainTextEdit::setStyleSheet(const QString& style_sheet)
{
    _external_style_sheet = !style_sheet.isEmpty();
    if (_external_style_sheet)
        QPlainTextEdit::setStyleSheet(style_sheet);
    else
        Framework::internalCallbackManager()->addRequest(this, Framework::PLAIN_TEXT_EDIT_UPDATE_STYLE);
}

bool PlainTextEdit::event(QEvent* e)
{
    if (e->type() == QEvent::ShowToParent || e->type() == QEvent::Resize) {
        // без этого не подгоняется высота при первом показе
        updateGeometry();
        updateScrollbar();
    }

    return QPlainTextEdit::event(e);
}

void PlainTextEdit::changeEvent(QEvent* event)
{
    QPlainTextEdit::changeEvent(event);

    if (objectExtensionDestroyed())
        return;

    if (event->type() == QEvent::ReadOnlyChange || event->type() == QEvent::EnabledChange || event->type() == QEvent::PaletteChange)
        Framework::internalCallbackManager()->addRequest(this, Framework::PLAIN_TEXT_EDIT_UPDATE_STYLE);
}

void PlainTextEdit::paintEvent(QPaintEvent* event)
{
    QPainter p(viewport());
    p.save();
    QPlainTextEdit::paintEvent(event);
    p.restore();
    if (styleSheet().isEmpty()) {
        QRect rect = viewport()->rect();
        // исправления последствий сдвига viewport

        int shift = 1;
        if (Utils::screenScaleFactor() > 1.5 || fuzzyCompare(Utils::screenScaleFactor(), 1.5))
            shift = 0;

        rect.setHeight(rect.height() + 1 - shift);
        rect.moveTop(rect.top() + shift);
        rect.moveLeft(rect.left() + 1);
        rect.setLeft(-1);
        Utils::paintBorder(viewport(), rect);
    }
}

void PlainTextEdit::scrollContentsBy(int dx, int dy)
{
    QPlainTextEdit::scrollContentsBy(dx, dy);
}

void PlainTextEdit::insertFromMimeData(const QMimeData* source)
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
            // В этом месте должно быть обрезание всего, что есть справа от курсора для того чтобы влезло
            // Делать это напрямую нельзя, так как сбивается undoStack и невозможно вернуться к моменту до вставки.
            // По этой причине сейчас только обрезается вставляемый текст.
            // Необходимо после исправления этого косяка исправить аналогичную функцию в TextEdit
            //curText = curText.left(textCursor().position());
            //document()->setPlainText(curText);
            //totalLength = curText.length() + textToPaste.length();
            int numToDelete = totalLength - _max_length;
            textToPaste = textToPaste.left(textToPaste.length() - numToDelete);
            scopy.setText(textToPaste);
        } else
            scopy.setText(source->text());
        QPlainTextEdit::insertFromMimeData(&scopy);
    } else {
        QPlainTextEdit::insertFromMimeData(source);
    }
}

void PlainTextEdit::keyPressEvent(QKeyEvent* e)
{
    auto curs = textCursor();
    QString selected_text = curs.selectedText();
    int selected_count = selected_text.length();
    if ((_max_length <= 0) || ((toPlainText().length() - selected_count) < _max_length)) {
        QPlainTextEdit::keyPressEvent(e);
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
            QPlainTextEdit::keyPressEvent(e);
        }
    }
    //QPlainTextEdit::keyPressEvent(e);
    // борьба с загадочным глюком: при сдвиге скроллбара через клавиатуру не вызывается QScrollBar::valueChanged
    // поэтому обновляем тут
    viewport()->update();
}

int PlainTextEdit::maximumLength() const
{
    return _max_length;
}

void PlainTextEdit::setMaximumLength(int length)
{
    if (_max_length == length)
        return;
    Z_CHECK(length > 0);
    _max_length = length;
}

void PlainTextEdit::focusInEvent(QFocusEvent* e)
{
    QPlainTextEdit::focusInEvent(e);
    // из-за сдвига viewport без обновления будут артефакты
    viewport()->update();
}

void PlainTextEdit::focusOutEvent(QFocusEvent* e)
{
    QPlainTextEdit::focusOutEvent(e);
    // из-за сдвига viewport без обновления будут артефакты
    viewport()->update();
}

void PlainTextEdit::sl_textChanged()
{
    updateGeometry();
    updateScrollbar();
}

void PlainTextEdit::sl_scroll(int value)
{
    Q_UNUSED(value)
    // из-за сдвига viewport без обновления будут артефакты
    viewport()->update();
}

void PlainTextEdit::sl_callback(int key, const QVariant& data)
{
    Q_UNUSED(data);
    if (objectExtensionDestroyed())
        return;

    if (key == Framework::PLAIN_TEXT_EDIT_UPDATE_STYLE) {
        updateStyle();
    }
}

bool PlainTextEdit::isSingleLine(int height) const
{
    return height < fontMetrics().lineSpacing() * 2;
}

void PlainTextEdit::updateScrollbar()
{
    if (isSingleLine(sizeHint().height())) {
        setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    } else {
        setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    }
}

void PlainTextEdit::updateStyle()
{
    if (_external_style_sheet)
        return;

    bool ro = _ro_background && (!isEnabled() || isReadOnly());
    QColor background = ro ? Colors::readOnlyBackground() : Colors::background();
    QString sh = QStringLiteral("QPlainTextEdit {background-color: %1}").arg(background.name());
    if (styleSheet() != sh)
        QPlainTextEdit::setStyleSheet(sh);
}

} // namespace zf
