#include "zf_collapsible_group_box.h"
#include "zf_core.h"

#include <QDebug>
#include <QEvent>
#include <QHBoxLayout>
#include <QStyleOptionFrame>
#include <QStylePainter>
#include <QVBoxLayout>
#include <QKeyEvent>

#define LABEL_SHIFT 3

#define MARGIN_TITLE 4
#define MARGIN_BODY 2

namespace zf
{
class CollapsibleGroupBoxHeader : public QWidget
{
public:
    CollapsibleGroupBoxHeader(CollapsibleGroupBox* parent)
        : gb(parent)
    {
        setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    }

    void mousePressEvent(QMouseEvent* event) override
    {
        gb->toggle();
        QWidget::mousePressEvent(event);
    }

    void paintEvent(QPaintEvent* event) override
    {
        Q_UNUSED(event)

        QRect header_rect = this->rect();

        QPainter p(this);

        QStyleOptionButton baseOption;
        baseOption.initFrom(this);
        baseOption.rect = header_rect;
        baseOption.text = gb->title();

        //        if (_header_contains_mouse)
        //            baseOption.state |= QStyle::State_MouseOver;

        QStyle::PrimitiveElement element;
        if (gb->isExpanded())
            element = QStyle::PE_IndicatorArrowDown;
        else
            element = QStyle::PE_IndicatorArrowLeft;

        QStyleOptionButton indicatorOption = baseOption;
        indicatorOption.rect = style()->subElementRect(QStyle::SE_CheckBoxIndicator, &indicatorOption, this);
        indicatorOption.rect.moveRight(rect().right() - MARGIN_BODY);

        QStyleOptionButton labelOption = baseOption;
        labelOption.icon = gb->icon();

        labelOption.iconSize = gb->iconSize();
        int shift = 6;
        double zoom = (double)(labelOption.rect.height() - shift) / (double)labelOption.iconSize.height();
        labelOption.iconSize.setHeight(labelOption.iconSize.height() * zoom);
        labelOption.iconSize.setWidth(labelOption.iconSize.width() * zoom);

        labelOption.rect = style()->subElementRect(QStyle::SE_CheckBoxContents, &labelOption, this);
        labelOption.text = " " + labelOption.text;
        if (!labelOption.icon.isNull())
            labelOption.rect.setHeight(qMax(labelOption.rect.height(), labelOption.iconSize.height() + LABEL_SHIFT * 2));
        labelOption.rect.moveLeft(rect().left() + MARGIN_BODY + MARGIN_TITLE);

        if (gb->isBoldTitle())
            p.setFont(zf::Utils::fontBold(p.font()));

        if (gb->titleBackgroundColor().isValid())
            p.fillRect(header_rect, gb->titleBackgroundColor());

        style()->drawPrimitive(element, &indicatorOption, &p, this);
        style()->drawControl(QStyle::CE_CheckBoxLabel, &labelOption, &p, this);
    }

    QSize sizeHint() const override
    {
        QStyleOption option;
        option.initFrom(this);

        QSize textSize
            = style()
                  ->itemTextRect(option.fontMetrics, QRect(), Qt::TextShowMnemonic, false, gb->title().isEmpty() ? "X" : gb->title())
                  .size();

        QSize size = style()->sizeFromContents(QStyle::CT_CheckBox, &option, textSize, this);

        if (!gb->icon().isNull())
            size.setHeight(qMax(size.height(), gb->iconSize().height() + LABEL_SHIFT * 2));

        size.setWidth(size.width() + style()->pixelMetric(QStyle::PM_IndicatorWidth));

        size.setWidth(size.width() + MARGIN_TITLE);
        size.setHeight(size.height() + MARGIN_TITLE);

        return size;
    }
    QSize minimumSizeHint() const override { return sizeHint(); }

    CollapsibleGroupBox* gb;
};

CollapsibleGroupBox::CollapsibleGroupBox(QWidget* parent)
    : QWidget(parent)    
{
    _icon_size = {Z_PM(PM_SmallIconSize), Z_PM(PM_SmallIconSize)};

    _layout = new QVBoxLayout(this);
    _layout->setMargin(0);
    _layout->setSpacing(0);

    _header = new CollapsibleGroupBoxHeader(this);
    _layout->addWidget(_header);

    _layout->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Preferred));

    QWidget* body = new QWidget;
    body->setObjectName("zf_CollapsibleGroupBox_body");
    replaceBody(body);

    setAttribute(Qt::WA_Hover, true);
    setExpanded(true);
    setTitleBackgroundColor(qApp->palette().color(QPalette::Normal, QPalette::Midlight));
    setBoldTitle(false);
    _layout->installEventFilter(this);

    // в дизайнере почему то не схлопывается body если изначально expanded = false
    QTimer::singleShot(0, this, [this]() {
        if (!_expanded && !_body->isHidden()) {
            _body->setHidden(true);
            updateGeometry();
        }
    });
}

CollapsibleGroupBox::CollapsibleGroupBox(const QString& title, QWidget* parent)
    : CollapsibleGroupBox(parent)
{
    setTitle(title);
}

QLayout* CollapsibleGroupBox::layout() const
{
    return _body->layout();
}

void CollapsibleGroupBox::setLayout(QLayout* layout)
{
    _body->setLayout(layout);
}

void CollapsibleGroupBox::appendWidget(QWidget* widget, Qt::Orientation orientation)
{
    auto la = createLayout(orientation);
    insertWidget(la->count(), widget, orientation);
}

void CollapsibleGroupBox::insertWidget(int pos, QWidget* widget, Qt::Orientation orientation)
{
    auto la = createLayout(orientation);
    Z_CHECK(pos >= 0 && pos <= la->count());
    createLayout(orientation)->insertWidget(pos, widget);
}

void CollapsibleGroupBox::setTitle(const QString& title)
{
    _title = title;
    _header->updateGeometry();
    _header->update();
    emit sg_titleChanged();
}

QString CollapsibleGroupBox::title() const
{
    return _title;
}

void CollapsibleGroupBox::setExpanded(bool expanded)
{
    if (expanded == _expanded)
        return;

    _expanded = expanded;

    _body->setHidden(!_expanded);
    _header->update();
    updateGeometry();

    emit sg_expandedChanged();
}

bool CollapsibleGroupBox::isExpanded() const
{
    return _expanded;
}

QIcon CollapsibleGroupBox::icon() const
{
    return _icon;
}

void CollapsibleGroupBox::setIcon(const QIcon& icon)
{
    _icon = icon;
    _header->updateGeometry();
    _header->update();
    emit sg_iconChanged();
}

QSize CollapsibleGroupBox::iconSize() const
{
    return _icon_size;
}

void CollapsibleGroupBox::setIconSize(const QSize& size)
{
    if (_icon_size == size)
        return;

    _icon_size = size;
    _header->updateGeometry();
    _header->update();
    emit sg_iconChanged();
}

QColor CollapsibleGroupBox::backgroundColor() const
{
    return _background_color;
}

void CollapsibleGroupBox::setBackgroundColor(const QColor& c)
{
    if (_background_color == c)
        return;

    _background_color = c;
    update();
}

QColor CollapsibleGroupBox::titleBackgroundColor() const
{
    return _title_background_color;
}

void CollapsibleGroupBox::setTitleBackgroundColor(const QColor& c)
{
    if (_title_background_color == c)
        return;

    _title_background_color = c;
    _header->update();
}

bool CollapsibleGroupBox::isBoldTitle() const
{
    return _title_bold;
}

void CollapsibleGroupBox::setBoldTitle(bool b)
{
    if (_title_bold == b)
        return;

    _title_bold = b;
    _header->updateGeometry();
    _header->update();
}

QSize CollapsibleGroupBox::sizeHint() const
{
    if (_expanded)
        return widgetSizeHint();
    else
        return _header->sizeHint();
}

QSize CollapsibleGroupBox::minimumSizeHint() const
{
    return sizeHint();
}

QWidget* CollapsibleGroupBox::body() const
{
    return _body;
}

void CollapsibleGroupBox::replaceBody(QWidget* w)
{
    Z_CHECK_NULL(w);
    if (w == _body)
        return;

    if (_body != nullptr)
        delete _body;

    _body = w;
    _body->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    _layout->insertWidget(1, _body);
    _body->installEventFilter(this);
}

void CollapsibleGroupBox::toggle()
{
    setExpanded(!isExpanded());
}

void CollapsibleGroupBox::expand()
{
    setExpanded(true);
}

void CollapsibleGroupBox::collapse()
{
    setExpanded(false);
}

void CollapsibleGroupBox::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event)
    QPainter p(this);

    if (_background_color.isValid())
        p.fillRect(rect(), _background_color);
}

bool CollapsibleGroupBox::event(QEvent* event)
{    
    return QWidget::event(event);
}

bool CollapsibleGroupBox::eventFilter(QObject* watched, QEvent* event)
{
    return QWidget::eventFilter(watched, event);
}

void CollapsibleGroupBox::mousePressEvent(QMouseEvent* event)
{    
    QWidget::mousePressEvent(event);
}

void CollapsibleGroupBox::mouseMoveEvent(QMouseEvent* event)
{    
    QWidget::mouseMoveEvent(event);
}

void CollapsibleGroupBox::leaveEvent(QEvent* event)
{
    QWidget::leaveEvent(event);
}

void CollapsibleGroupBox::keyPressEvent(QKeyEvent* event)
{
    if (hasFocus()) {
        const int key = event->key();
        if (key == Qt::Key_Space || key == Qt::Key_Enter || key == Qt::Key_Return)
            toggle();
    }

    QWidget::keyPressEvent(event);
}

void CollapsibleGroupBox::changeEvent(QEvent* event)
{
    QWidget::changeEvent(event);
}

void CollapsibleGroupBox::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
}

QSize CollapsibleGroupBox::widgetSizeHint() const
{
    QSize size = bodySizeHint();
    QSize header = _header->sizeHint();

    size.setHeight(size.height() + header.height());
    size.setWidth(qMax(size.width(), header.width()));

    return size;
}


QSize CollapsibleGroupBox::bodySizeHint() const
{
    QSize size = _body->sizeHint();
    size.setWidth(size.width() + MARGIN_BODY);
    size.setHeight(size.height() + MARGIN_BODY);
    return size;
}

QBoxLayout* CollapsibleGroupBox::createLayout(Qt::Orientation orientation)
{
    QBoxLayout* la = nullptr;
    if (layout() == nullptr) {
        if (orientation == Qt::Vertical)
            la = new QVBoxLayout;
        else
            la = new QHBoxLayout;

        setLayout(la);
    } else {
        la = qobject_cast<QBoxLayout*>(layout());
        Z_CHECK(la != nullptr);

        if (orientation == Qt::Vertical)
            la = qobject_cast<QVBoxLayout*>(layout());
        else
            la = qobject_cast<QHBoxLayout*>(layout());
        Z_CHECK(la != nullptr);
    }

    la->setObjectName(QStringLiteral("zfla"));
    return la;
}

} // namespace zf
