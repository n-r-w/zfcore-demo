#include "zf_uniview_toolbar.h"
#include "ui_zf_uniview_toolbar.h"
#include "zf_colors_def.h"
#include "zf_core.h"
#include "zf_uniview_widget.h"

#include <QFrame>
#include <QLayoutItem>
#include <QSpacerItem>
#include <QToolButton>

namespace zf {
UniViewToolbar::UniViewToolbar(UniViewWidget* uniview, QWidget* parent)
    : QWidget(parent)
    , _uni_view(uniview)
    , _ui(new Ui::UniViewToolbarUI)
{
    Z_CHECK_NULL(_uni_view);

    setLayout(new QHBoxLayout);
    layout()->setMargin(0);

    _border = new QFrame;
    _ui->setupUi(_border);
    _border->setFrameShape(QFrame::NoFrame);
    _border->setObjectName("UniViewToolbarFrame");
    _border->setStyleSheet(QStringLiteral("QFrame#UniViewToolbarFrame {"
                                          "margin: 0px; padding: 0px; "
                                          "border: 1px %1;"
                                          "border-top-style: none; "
                                          "border-right-style: none; "
                                          "border-bottom-style: none; "
                                          "border-left-style: none; "
                                          "background-color: %2;}")
                               .arg(Colors::uiLineColor(true).name())
                               .arg(Colors::uiWindowColor().name()));

    layout()->addWidget(_border);

    QTimer* zoom_timer = new QTimer(this);
    zoom_timer->setInterval(EDIT_TIMEOUT_MS);
    zoom_timer->setSingleShot(true);
    connect(zoom_timer, &QTimer::timeout, this, [&]() {
        if (_by_user)
            return;

        if ((int)(_uni_view->scaleFactor() * 100) != _ui->scale->value())
            _uni_view->setScale((double)_ui->scale->value() / 100.0);
    });

    QTimer* page_timer = new QTimer(this);
    page_timer->setInterval(EDIT_TIMEOUT_MS);
    page_timer->setSingleShot(true);
    connect(page_timer, &QTimer::timeout, this, [&]() {
        if (_by_user)
            return;

        _uni_view->setCurrentPage(_ui->page_no_ed->value());
    });

    connect(_ui->rotate, &QToolButton::clicked, this, [&, zoom_timer]() {
        _by_user = true;
        zoom_timer->stop();
        _uni_view->rotateRight();
        _by_user = false;
    });

    connect(_ui->zoom_in, &QToolButton::clicked, this, [&, zoom_timer]() {
        _by_user = true;
        zoom_timer->stop();
        _uni_view->zoomIn();
        _fit_mode = FittingType::Undefined;
        _by_user = false;
    });

    connect(_ui->zoom_out, &QToolButton::clicked, this, [&, zoom_timer]() {
        _by_user = true;
        zoom_timer->stop();
        _uni_view->zoomOut();
        _fit_mode = FittingType::Undefined;
        _by_user = false;
    });

    connect(_ui->fit_mode, &QToolButton::clicked, this, [&, zoom_timer]() {
        _by_user = true;
        zoom_timer->stop();

        FittingType type;
        if (_fit_mode == FittingType::Undefined || _fit_mode == FittingType::Page)
            type = FittingType::Width;
        else
            type = FittingType::Page;

        setFitMode(type);
        _by_user = false;
    });

    connect(_ui->scale, qOverload<int>(&QSpinBox::valueChanged), this, [&, zoom_timer]() {
        if (_by_user)
            return;
        zoom_timer->start();
        _fit_mode = FittingType::Undefined;
    });

    connect(_uni_view, &UniViewWidget::sg_currentPageChanged, this, [&](int current, int maximum) { updatePageNum(current, maximum); });
    connect(_ui->page_no_ed, qOverload<int>(&QSpinBox::valueChanged), this, [&, page_timer]() {
        if (_by_user)
            return;
        page_timer->start();
    });

    connect(_uni_view, &UniViewWidget::sg_zoomScaleChanged, this, [&](double scale) {
        if (scale * 100.0 == _ui->scale->value())
            return;

        auto mode = _fit_mode;
        _ui->scale->setValue(scale * 100.0);
        _fit_mode = mode;
    });

    connect(uniview, &UniViewWidget::sg_fittingTypeChaged, this, [&](FittingType mode) { _fit_mode = mode; });

    connect(_uni_view, &UniViewWidget::sg_parametersChanged, this, [&]() { testEnabled(); });
    connect(_uni_view, &UniViewWidget::sg_loadStarted, this, [&]() { testEnabled(); });
    connect(_uni_view, &UniViewWidget::sg_loadFinished, this, [&]() { testEnabled(); });

    setFitMode(FittingType::Width);
    updatePageNum(_uni_view->currentPage(), _uni_view->maximumPage());

    testEnabled();
}

UniViewToolbar::~UniViewToolbar()
{
    delete _ui;
}

QFrame* UniViewToolbar::border() const
{
    return _border;
}

QToolButton* UniViewToolbar::addButton(const QString& translation_id, const QIcon& icon, Qt::ToolButtonStyle style)
{
    QToolButton* b = new QToolButton();
    b->setToolButtonStyle(style);
    b->setIcon(icon);
    if (!translation_id.isEmpty())
        b->setText(zf::translate(translation_id));
    b->setIconSize(_ui->rotate->iconSize());
    b->setAutoRaise(_ui->rotate->autoRaise());

    connect(b, &QToolButton::clicked, this, [&]() {
        auto b = static_cast<QToolButton*>(sender());
        Z_CHECK_NULL(b);
        emit sg_buttonClicked(b);
    });

    _buttons << b;

    addWidget(b);
    return b;
}

QToolButton* UniViewToolbar::addButton(const QIcon& icon)
{
    return addButton("", icon, Qt::ToolButtonIconOnly);
}

QFrame* UniViewToolbar::addLine()
{
    auto vert_line = new QFrame();
    vert_line->setFrameShadow(QFrame::Raised);
    vert_line->setFrameShape(QFrame::VLine);
    addWidget(vert_line);
    return vert_line;
}

QSpacerItem* UniViewToolbar::addSpacer(int width, QSizePolicy::Policy policy)
{
    Z_CHECK(width >= 0);
    auto s = new QSpacerItem(width, 0, policy);
    addItem(s);
    return s;
}

UniviewParameters UniViewToolbar::defaultParameters() const
{
    return _default_parameters;
}

void UniViewToolbar::setDefaultParameters(const UniviewParameters& params)
{
    if (_default_parameters == params)
        return;

    _default_parameters = params;
    testEnabled();
}

bool UniViewToolbar::hasVisibleControls() const
{
    for (auto b : _buttons) {
        if (!b->isHidden())
            return true;
    }

    if (!_ui->rotate->isHidden() || !_ui->fit_mode->isHidden() || !_ui->zoom_out->isHidden() || !_ui->zoom_in->isHidden() ||
        !_ui->scale->isHidden() || !_ui->page_no_ed->isHidden() || !_ui->page_max->isHidden() || !_ui->page_no_lb->isHidden())
        return true;

    return false;
}

void UniViewToolbar::testEnabled()
{
    UniviewParameters support_params;
    UniviewParameters avail_params;

    if (_uni_view->isValid()) {
        support_params = _uni_view->supportedParameters();
        avail_params = _uni_view->availableParameters();
    } else {
        support_params = _default_parameters;
    }

    bool s_zoomStep = support_params.testFlag(UniviewParameter::ZoomStep);
    bool s_zoomPercent = support_params.testFlag(UniviewParameter::ZoomPercent);
    bool s_rotateStep = support_params.testFlag(UniviewParameter::RotateStep);
    bool s_autoZoom = support_params.testFlag(UniviewParameter::AutoZoom);
    bool s_readPageNo = support_params.testFlag(UniviewParameter::ReadPageNo);
    bool s_setPageNo = support_params.testFlag(UniviewParameter::SetPageNo);

    bool a_zoomStep = avail_params.testFlag(UniviewParameter::ZoomStep);
    bool a_zoomPercent = avail_params.testFlag(UniviewParameter::ZoomPercent);
    bool a_rotateStep = avail_params.testFlag(UniviewParameter::RotateStep);
    bool a_autoZoom = avail_params.testFlag(UniviewParameter::AutoZoom);
    // bool  a_readPageNo = avail_params.testFlag(UniviewParameter::ReadPageNo);
    bool a_setPageNo = avail_params.testFlag(UniviewParameter::SetPageNo);

    _ui->fit_mode->setEnabled(a_autoZoom);
    _ui->fit_mode->setVisible(s_autoZoom);

    _ui->rotate->setEnabled(a_rotateStep);
    _ui->rotate->setVisible(s_rotateStep);

    _ui->zoom_in->setEnabled(a_zoomStep);
    _ui->zoom_in->setVisible(s_zoomStep);

    _ui->zoom_out->setEnabled(a_zoomStep);
    _ui->zoom_out->setVisible(s_zoomStep);

    _ui->scale->setEnabled(a_zoomPercent);
    _ui->scale->setVisible(s_zoomPercent);

    _ui->page_no_ed->setEnabled(!_uni_view->isLoading() && a_setPageNo);
    _ui->page_no_ed->setVisible(s_setPageNo);

    _ui->page_no_lb->setVisible(s_readPageNo && !s_setPageNo);

    _ui->page_lb->setVisible(s_readPageNo || s_setPageNo);
    _ui->page_div->setVisible(s_readPageNo || s_setPageNo);
    _ui->page_max->setVisible(s_readPageNo || s_setPageNo);

    _ui->page_spacer->changeSize(s_readPageNo || s_setPageNo ? SPACER_WIDTH : 0, 0, QSizePolicy::Fixed);
}

void UniViewToolbar::setFitMode(FittingType mode)
{
    _uni_view->setFittingType(mode);
    _fit_mode = mode;
}

void UniViewToolbar::addWidget(QWidget* w)
{
    _ui->main_la->insertWidget(_ui->main_la->count() - 1, w);
    testEnabled();
}

void UniViewToolbar::addItem(QLayoutItem* item)
{
    _ui->main_la->insertItem(_ui->main_la->count() - 1, item);
}

void UniViewToolbar::updatePageNum(int current, int maximum)
{
    if (current <= 0 || maximum <= 0)
        return;

    if (_uni_view->supportedParameters().testFlag(UniviewParameter::SetPageNo))
        _ui->page_no_ed->setValue(current);
    else
        _ui->page_no_lb->setText(QString::number(current));

    _ui->page_max->setText(QString::number(maximum));
    testEnabled();
}

} // namespace zf
