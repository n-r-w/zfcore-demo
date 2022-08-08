#include "zf_line_scrollarea.h"
#include <QScrollBar>
#include <QTimer>
#include <QEvent>

namespace zf
{
LineScrollArea::LineScrollArea(QWidget* parent, Qt::Orientation orientation, int line_size)
    : QScrollArea(parent)
    , _line_size(line_size)
    , _orientation(orientation)
    , _body(new QWidget)
{
    _spacer_timer = new QTimer(this);
    _spacer_timer->setSingleShot(true);
    _spacer_timer->setInterval(0);
    connect(_spacer_timer, &QTimer::timeout, this, [&]() {
        // разделитель всегда в конец
        int index = widgetLayout()->indexOf(_spacer);
        if (index >= 0 && index != widgetLayout()->count() - 1) {
            auto item = widgetLayout()->takeAt(index);
            widgetLayout()->addItem(item);            
        }
        widgetLayout()->setStretch(widgetLayout()->count() - 1, 1);
    });

    _body->setObjectName("zlscb");
    _body->setStyleSheet(QStringLiteral("QWidget#zlscb {background-color: white}"));
    setWidget(_body);
    setWidgetResizable(true);
    setAlignment(Qt::AlignCenter);

    if (_orientation == Qt::Vertical) {
        setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        _body->setLayout(new QVBoxLayout);
        _spacer = new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding);
    } else {
        setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        _body->setLayout(new QHBoxLayout);
        _spacer = new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum);
    }

    _body->layout()->setMargin(0);
    _body->layout()->setSpacing(0);
    _body->installEventFilter(this);
    _body->layout()->addItem(_spacer);
}

Qt::Orientation LineScrollArea::orientation() const
{
    return _orientation;
}

QBoxLayout* LineScrollArea::widgetLayout() const
{
    return qobject_cast<QBoxLayout*>(widget()->layout());
}

int LineScrollArea::lineSize() const
{
    return _line_size;
}

void LineScrollArea::setLineSize(int size)
{
    if (size == _line_size)
        return;

    _line_size = size;
    updateGeometry();
}

QSize LineScrollArea::sizeHint() const
{
    QSize base_size = QScrollArea::sizeHint();

    QSize size;
    if (_orientation == Qt::Vertical) {
        if (_line_size > 0) {
            size.setWidth(_line_size + widgetLayout()->contentsMargins().left() + widgetLayout()->contentsMargins().right());
            if (verticalScrollBarPolicy() != Qt::ScrollBarAlwaysOff)
                size.setWidth(size.width() + verticalScrollBar()->sizeHint().width() + 2);

        } else {
            size.setWidth(base_size.width() > 0 ? base_size.width() : MINIMUM_OPPOZITE);
        }

    } else {
        if (_line_size > 0) {
            size.setHeight(_line_size + widgetLayout()->contentsMargins().top() + widgetLayout()->contentsMargins().bottom());

            if (horizontalScrollBarPolicy() != Qt::ScrollBarAlwaysOff)
                size.setHeight(size.height() + horizontalScrollBar()->sizeHint().height() + 2);

        } else {
            size.setHeight(base_size.height() > 0 ? base_size.height() : MINIMUM_OPPOZITE);
        }
    }

    if (size.width() <= 0)
        size.setWidth(MINIMUM_OPPOZITE);
    if (size.height() <= 0)
        size.setHeight(MINIMUM_OPPOZITE);

    return size;
}

QSize LineScrollArea::minimumSizeHint() const
{
    return sizeHint();
}

bool LineScrollArea::eventFilter(QObject* obj, QEvent* event)
{
    if (event->type() == QEvent::ChildAdded && obj == _body) {
        _spacer_timer->start();
    }

    return QScrollArea::eventFilter(obj, event);
}

} // namespace zf
