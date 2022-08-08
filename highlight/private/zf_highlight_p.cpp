#include "zf_highlight_p.h"
#include "zf_core.h"
#include "zf_html_tools.h"
#include <QPainter>
#include <qdrawutil.h>
#include <QTextDocument>
#include <QTextLayout>
#include <QAbstractTextDocumentLayout>

namespace zf
{
HighlightDelegate::HighlightDelegate(HighlightView* view)
    : QStyledItemDelegate(view)
    , _view(view)
{
    Z_CHECK_NULL(_view);
}

void HighlightDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    painter->save();

    QStyleOptionViewItem opt = option;
    opt.decorationSize = {Z_PM(PM_SmallIconSize), Z_PM(PM_SmallIconSize)};

    initStyleOption(&opt, index);
    QStyle* style = opt.widget ? opt.widget->style() : QApplication::style();

    QRect textRect = style->subElementRect(QStyle::SE_ItemViewItemText, &opt).adjusted(2, 0, -2, -2);

    QTextDocument doc;
    initTextDocument(option, index, doc);

    // Рисуем фон
    painter->save();
    painter->fillRect(opt.rect, Qt::white);
    painter->restore();

    // Рисуем выделение текущего
    if ((opt.state & QStyle::State_Selected) > 0) {
        painter->save();
        painter->fillRect(opt.rect.adjusted(1, 1, -2, -4), QColor("#D6EAFF"));
        painter->setPen(QPen(QColor("#8EC6FF"), 1, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        painter->drawRect(opt.rect.adjusted(1, 1, -2, -4));
        painter->restore();
    }

    // Рисуем иконку
    if (!opt.icon.isNull()) {
        QStyleOptionViewItem optIcon = opt;
        optIcon.text = " "; // чтобы исключить сдвиг иконки в zf::ProxyStyle
        optIcon.state = opt.state & ~(QStyle::State_HasFocus | QStyle::State_Active | QStyle::State_Selected);        
        style->drawControl(QStyle::CE_ItemViewItem, &optIcon, painter, opt.widget);
    }

    painter->save();
    painter->translate(textRect.topLeft());
    QRect clip = textRect.translated(-textRect.topLeft());
    painter->setClipRect(clip);

    QAbstractTextDocumentLayout::PaintContext ctx;
    ctx.clip = clip;
    if (opt.state & QStyle::State_Selected) {
        // Рисуем обводную линию вокруг текста
        QTextCharFormat format;
        format.setTextOutline(QPen(QColor("#fafafa"), 3, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));

        QTextCursor cursor(&doc);
        cursor.select(QTextCursor::Document);
        cursor.mergeCharFormat(format);
        doc.documentLayout()->draw(painter, ctx);

        format.setTextOutline(QPen(Qt::NoPen));
        cursor.mergeCharFormat(format);
    }
    // Рисуем текст без обводной линии
    doc.documentLayout()->draw(painter, ctx);
    painter->restore();

    // Рисуем разделительную линию
    QPalette linePalette;
    linePalette.setColor(QPalette::Light, Qt::lightGray);
    linePalette.setColor(QPalette::Dark, Qt::darkGray);
    linePalette.setColor(QPalette::Mid, Qt::gray);
    painter->save();
    qDrawShadeLine(painter, opt.rect.left(), opt.rect.bottom(), opt.rect.right() + 5, opt.rect.bottom(), linePalette, false);
    painter->restore();

    painter->restore();
}

QSize HighlightDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    QTextDocument doc;
    initTextDocument(option, index, doc);
    return QSize(doc.idealWidth(), doc.size().height() + 2);
}

void HighlightDelegate::initTextDocument(const QStyleOptionViewItem& option, const QModelIndex& index, QTextDocument& doc) const
{
    QStyleOptionViewItem optionV4 = option;
    initStyleOption(&optionV4, index);

    QColor color;
    auto property = DataProperty::fromVariant(index.data(ZF_HIGHLIGHT_PROPERTY_ROLE));
    if (!property.isValid()) {
        optionV4.text.clear();

    } else {
        auto items = _view->highlightProcessor()->highlight()->items(property, false);
        if (!items.isEmpty()) {
            QStringList texts;
            QList<InformationType> types;
            for (auto& i : qAsConst(items)) {
                types << i.type();
            }
            color = highlightColorText(Utils::getTopErrorLevel(types));

        } else {
            optionV4.text.clear();
        }
    }

    QStyle* style = optionV4.widget ? optionV4.widget->style() : QApplication::style();
    QRect textRect = style->subElementRect(QStyle::SE_ItemViewItemText, &optionV4).adjusted(2, 0, -2, -2);

    //    doc.setDefaultFont(Utils::fontSize(doc.defaultFont(), Core::settings()->font().pointSize()));
    doc.setTextWidth(textRect.width());

    //    doc.setHtml(QStringLiteral("<font color=\"%2\">%1</font>").arg(optionV4.text, color.name()));
    if (HtmlTools::isHtml(optionV4.text))
        doc.setHtml(optionV4.text);
    else
        doc.setPlainText(optionV4.text);
}

QColor HighlightDelegate::highlightColorBorder(InformationType type)
{
    switch (type) {
        case InformationType::Information:
        case InformationType::Success:
            return QColor("#00aeff");

        case InformationType::Warning:
            return QColor("#FFAA00");

        case InformationType::Error:
        case InformationType::Critical:
        case InformationType::Fatal:
        case InformationType::Required:
            return QColor("#FF4900");

        case InformationType::Invalid:
            break;
    }
    Z_HALT_INT;
    return QColor();
}

QColor HighlightDelegate::highlightColorText(InformationType type)
{
    switch (type) {
        case InformationType::Information:
        case InformationType::Success:
            return QColor("#0350AC");

        case InformationType::Warning:
            return QColor("#AD4E00");

        case InformationType::Error:
        case InformationType::Critical:
        case InformationType::Fatal:
        case InformationType::Required:
            return QColor("#BF0505");

        case InformationType::Invalid:
            break;
    }
    Z_HALT_INT;
    return QColor();
}

HighlightTableView::HighlightTableView(HighlightView* view)
    : _view(view)
{
}

void HighlightTableView::keyPressEvent(QKeyEvent* event)
{
    _view->onStartKeyPressEvent(event);

    if (event->isAccepted())
        QTableView::keyPressEvent(event);

    _view->onEndKeyPressEvent(event);
}

void HighlightTableView::mousePressEvent(QMouseEvent* event)
{
    _view->onStartMousePressEvent(event);

    if (event->isAccepted())
        QTableView::mousePressEvent(event);

    _view->onEndMousePressEvent(event);
}

} // namespace zf
