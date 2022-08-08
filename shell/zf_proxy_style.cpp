#include "zf_proxy_style.h"
#include "zf_core.h"
#include "zf_ui_size.h"

#include <QStyleOptionTitleBar>
#include <QPushButton>
#include <QStyleFactory>
#include <QPainter>
#include <qdrawutil.h>
#include <QSplitter>
#include <QScrollBar>
#include <QScrollArea>
#include <QTextDocument>
#include <QDockWidget>

#ifdef Q_OS_WIN
#include <Windows.h>
#include <QMainWindow>
#endif

namespace zf
{
ProxyStyle::ProxyStyle(QStyle* style)
    : QProxyStyle(style)
{
}

ProxyStyle::ProxyStyle(const QString& key)
    : QProxyStyle(key)
{
}

QSize ProxyStyle::sizeFromContents(QStyle::ContentsType ct, const QStyleOption* opt, const QSize& csz, const QWidget* widget) const
{
    QSize sz = QProxyStyle::sizeFromContents(ct, opt, csz, widget);
    if (ct == CT_PushButton) {
        // исправление слишком узких боковых полей при наличии рисунка на кнопке
        const QStyleOptionButton* btn = qstyleoption_cast<const QStyleOptionButton*>(opt);
        if (btn != nullptr) {
            auto button = qobject_cast<const QPushButton*>(widget);
            Z_CHECK_NULL(button);
            if (!button->icon().isNull())
                sz.rwidth() += proxy()->pixelMetric(PM_DefaultFrameWidth, btn, widget) * 2 + button->fontMetrics().horizontalAdvance(' ');
        }

    } else if (ct == CT_ItemViewItem) {
        sz.setHeight(qMax(sz.height(), UiSizes::defaultTableRowHeight()));
    }

    return sz;
}

int ProxyStyle::pixelMetric(QStyle::PixelMetric metric, const QStyleOption* option, const QWidget* widget) const
{
    switch (metric) {
        case PM_SliderControlThickness:
        case PM_CustomBase:
            return QProxyStyle::pixelMetric(metric, option, widget);

        case PM_ButtonMargin:
            return Utils::scaleUI(8);
        case PM_ButtonDefaultIndicator:
        case PM_ButtonShiftHorizontal:
        case PM_ButtonShiftVertical:
            return 0;
        case PM_MenuButtonIndicator:
            return Utils::scaleUI(12);
        case PM_DefaultFrameWidth:
            return 1;
        case PM_SpinBoxFrameWidth:
            return 3;
        case PM_ComboBoxFrameWidth:
            return pixelMetric(PM_DefaultFrameWidth, option, widget);

        case PM_MaximumDragDistance:
            return -1;

        case PM_ScrollBarExtent:
            return Utils::scaleUI(18);

        case PM_DialogButtonsSeparator:
        case PM_ScrollBarSliderMin:
            return Utils::scaleUI(26);

        case PM_SliderThickness:
        case PM_SliderLength:
            return Utils::scaleUI(15);
            //        case PM_SliderControlThickness:

        case PM_SliderTickmarkOffset:
            return Utils::scaleUI(4);
        case PM_SliderSpaceAvailable:
            return QProxyStyle::pixelMetric(metric, option, widget);

        case PM_DockWidgetSeparatorExtent:
            return Utils::scaleUI(6);
        case PM_DockWidgetHandleExtent:
        case PM_SplitterWidth:
            return Utils::scaleUI(8);
        case PM_DockWidgetFrameWidth:
            return 1;

        case PM_TabBarTabOverlap:
            return Utils::scaleUI(1);
        case PM_TabBarTabHSpace:
            return Utils::scaleUI(24);
        case PM_TabBarTabVSpace:
            return Utils::scaleUI(12);
        case PM_TabBarBaseHeight:
            return QProxyStyle::pixelMetric(metric, option, widget);
        case PM_TabBarBaseOverlap:
            return Utils::scaleUI(2);
        case PM_TabBarTabShiftHorizontal:
            return 0;
        case PM_TabBarTabShiftVertical:
            return 2;
        case PM_TabBarScrollButtonWidth:
            return Utils::scaleUI(16);

        case PM_ProgressBarChunkWidth:
            return 9;

        case PM_TitleBarHeight: {
#ifdef Q_OS_WIN
            auto main_win = Utils::getMainWindow();
            if (main_win != nullptr) {
                TITLEBARINFOEX* ptinfo = (TITLEBARINFOEX*)malloc(sizeof(TITLEBARINFOEX));
                ptinfo->cbSize = sizeof(TITLEBARINFOEX);
                SendMessage((HWND)main_win->winId(), WM_GETTITLEBARINFOEX, 0, (LPARAM)ptinfo);
                int height = ptinfo->rcTitleBar.bottom - ptinfo->rcTitleBar.top;
                free(ptinfo);
                return height;
            } else
#endif
                return Utils::scaleUI(30);
        }

        case PM_MenuScrollerHeight:
            return Utils::scaleUI(10);

        case PM_MenuVMargin:
        case PM_MenuHMargin:
        case PM_MenuPanelWidth:
            return 0;

        case PM_MenuTearoffHeight:
            return Utils::scaleUI(10);
        case PM_MenuDesktopFrameWidth:
            return 0;

        case PM_MenuBarVMargin:
        case PM_MenuBarHMargin:
        case PM_MenuBarPanelWidth:
            return 0;

        case PM_MenuBarItemSpacing:
            return Utils::scaleUI(6);

        case PM_IndicatorHeight:
        case PM_IndicatorWidth:
        case PM_ExclusiveIndicatorHeight:
        case PM_ExclusiveIndicatorWidth:
            return Utils::scaleUI(14);

        case PM_DialogButtonsButtonWidth:
            return Utils::scaleUI(70);
        case PM_DialogButtonsButtonHeight:
            return Utils::scaleUI(30);

        case PM_MdiSubWindowFrameWidth:
            return Utils::scaleUI(4);
        case PM_MdiSubWindowMinimizedWidth:
            return Utils::scaleUI(196);

        case PM_HeaderMargin:
            return Utils::scaleUI(0);
        case PM_ToolTipLabelFrameWidth:
            return Utils::scaleUI(2);

        case PM_HeaderMarkSize:
            return Utils::scaleUI(16);
        case PM_HeaderGripMargin:
            return Utils::scaleUI(4);

        case PM_ToolBarFrameWidth:
        case PM_ToolBarItemMargin:
            return Utils::scaleUI(2);

        case PM_ToolBarHandleExtent:
            return Utils::scaleUI(9);
        case PM_ToolBarItemSpacing:
            return Utils::scaleUI(1);

        case PM_ToolBarSeparatorExtent:
            return Utils::scaleUI(6);
        case PM_ToolBarExtensionExtent:
            return Utils::scaleUI(12);

        case PM_SpinBoxSliderHeight:
            return 2;

        case PM_ToolBarIconSize:
            return Utils::scaleUI(24);

        case PM_ListViewIconSize:
            return Utils::scaleUI(24);

        case PM_IconViewIconSize:
            return pixelMetric(PM_LargeIconSize, option, widget);

        case PM_SmallIconSize:
            return Utils::scaleUI(16);
        case PM_ButtonIconSize:
            return Utils::scaleUI(16);

        case PM_LargeIconSize:
            return Utils::scaleUI(32);

        case PM_FocusFrameVMargin:
        case PM_FocusFrameHMargin:
            return 2;

        case PM_CheckBoxLabelSpacing:
            return Utils::scaleUI(6);
        case PM_TabBarIconSize:
            return pixelMetric(PM_SmallIconSize, option, widget);
        case PM_SizeGripSize:
            return Utils::scaleUI(13);
        case PM_DockWidgetTitleMargin:
            return Utils::scaleUI(1);
        case PM_MessageBoxIconSize:
            return Utils::scaleUI(48);

        case PM_DockWidgetTitleBarButtonMargin:
            return Utils::scaleUI(2);

        case PM_RadioButtonLabelSpacing:
            return Utils::scaleUI(6);

        case PM_LayoutLeftMargin:
        case PM_LayoutTopMargin:
        case PM_LayoutRightMargin:
        case PM_LayoutBottomMargin:
            return QProxyStyle::pixelMetric(metric, option, widget);

        case PM_LayoutHorizontalSpacing:
        case PM_LayoutVerticalSpacing:
            return pixelMetric(PM_DefaultLayoutSpacing, option);

        case PM_DefaultTopLevelMargin:
            return Utils::scaleUI(11);
        case PM_DefaultChildMargin:
            return Utils::scaleUI(9);
        case PM_DefaultLayoutSpacing:
            return Utils::scaleUI(6);

        case PM_TabBar_ScrollButtonOverlap:
            return 1;

        case PM_TextCursorWidth:
            return QProxyStyle::pixelMetric(metric, option, widget);

        case PM_TabCloseIndicatorWidth:
        case PM_TabCloseIndicatorHeight:
            return Utils::scaleUI(20);

        case PM_ScrollView_ScrollBarSpacing:
            return 0;
        case PM_ScrollView_ScrollBarOverlap:
            return QProxyStyle::pixelMetric(metric, option, widget);
        case PM_SubMenuOverlap:
            return Utils::scaleUI(-1);
        case PM_TreeViewIndentation:
            return Utils::scaleUI(20);

        case PM_HeaderDefaultSectionSizeHorizontal:
            return Utils::scaleUI(100);
        case PM_HeaderDefaultSectionSizeVertical:
            return Utils::scaleUI(22);

        case PM_TitleBarButtonIconSize:
            return Utils::scaleUI(16);
        case PM_TitleBarButtonSize:
            return Utils::scaleUI(19);
    }

    return QProxyStyle::pixelMetric(metric, option, widget);
}

QRect ProxyStyle::subControlRect(QStyle::ComplexControl cc, const QStyleOptionComplex* opt, QStyle::SubControl sc, const QWidget* widget) const
{
    if (cc == CC_SpinBox && (sc == SC_SpinBoxUp || sc == SC_SpinBoxDown)) {
        return QProxyStyle::subControlRect(cc, opt, sc, widget).adjusted(-3, 0, 0, 0);
    }

    if (cc == CC_ComboBox) {
        QRect rect;
        int arrow_width_base = 16;
        int arrow_width = 18;
        if (const QStyleOptionComboBox* cb = qstyleoption_cast<const QStyleOptionComboBox*>(opt)) {
            const int x = cb->rect.x(), y = cb->rect.y(), wi = cb->rect.width(), he = cb->rect.height();
            const int margin = cb->frame ? Utils::scaleUI(3) : 0;
            const int bmarg = cb->frame ? Utils::scaleUI(2) : 0;
            const int xpos = x + wi - bmarg - Utils::scaleUI(arrow_width_base);
            switch (sc) {
                case SC_ComboBoxArrow:
                    rect.setRect(xpos, y + bmarg, Utils::scaleUI(arrow_width_base), he - 2 * bmarg);
                    break;
                case SC_ComboBoxEditField:
                    rect.setRect(x + margin, y + margin, wi - 2 * margin - Utils::scaleUI(arrow_width_base), he - 2 * margin);
                    break;
                default:
                    return QProxyStyle::subControlRect(cc, opt, sc, widget);
            }
            rect = visualRect(cb->direction, cb->rect, rect);
        }

        switch (sc) {
            case SC_ComboBoxArrow: {
                rect = visualRect(opt->direction, opt->rect, rect);
                rect.setRect(rect.right() - Utils::scaleUI(arrow_width), rect.top() - 2, Utils::scaleUI(arrow_width + 1), rect.height() + 4);
                rect = visualRect(opt->direction, opt->rect, rect);
                return rect;
            }
            case SC_ComboBoxEditField: {
                int frameWidth = 2;
                rect = visualRect(opt->direction, opt->rect, rect);
                rect.setRect(opt->rect.left() + frameWidth, opt->rect.top() + frameWidth, opt->rect.width() - Utils::scaleUI(arrow_width + 1) - 2 * frameWidth,
                    opt->rect.height() - 2 * frameWidth);
                if (const QStyleOptionComboBox* box = qstyleoption_cast<const QStyleOptionComboBox*>(opt)) {
                    if (!box->editable) {
                        rect.adjust(2, 0, 0, 0);
                        if (box->state & (State_Sunken | State_On))
                            rect.translate(1, 1);
                    }
                }
                rect = visualRect(opt->direction, opt->rect, rect);
                return rect;
            }
            default:
                break;
        }
    }

    return QProxyStyle::subControlRect(cc, opt, sc, widget);
}

void ProxyStyle::drawPrimitive(QStyle::PrimitiveElement element, const QStyleOption* option, QPainter* painter, const QWidget* widget) const
{
    if (element == PE_FrameLineEdit || element == PE_Frame) {
        // запрет на отрисовку голубого фрейма вокруг виджетов, т.к. он работает очень странно
        Utils::paintBorder(painter, option->rect);
        return;
    }

    if (element == PE_IndicatorItemViewItemCheck) {
        bool need_paint = false;
        if (auto tv = qobject_cast<const TableViewBase*>(widget))
            need_paint = tv->isModernCheckBox();
        else if (auto tv = qobject_cast<const TreeView*>(widget))
            need_paint = tv->isModernCheckBox();

        if (need_paint) {
            const QStyleOptionViewItem* opt = qstyleoption_cast<const QStyleOptionViewItem*>(option);
            Z_CHECK_NULL(opt);
            QIcon image;
            static QIcon checked(Utils::resizeSvg(QStringLiteral(":/share_icons/ok_border.svg"), Z_PM(PM_SmallIconSize)));
            switch (opt->checkState) {
                case Qt::Unchecked:
                    break;
                case Qt::PartiallyChecked: // пока просто считаем что такого у нас нет (не представляю зачем оно вообще надо)
                case Qt::Checked:
                    image = checked;
                    break;
            }

            QIcon::Mode mode = QIcon::Normal;
            if (!(opt->state & QStyle::State_Enabled))
                mode = QIcon::Disabled;
            else if (opt->state & QStyle::State_Selected)
                mode = QIcon::Selected;
            QIcon::State state = opt->state & QStyle::State_Open ? QIcon::On : QIcon::Off;

            image.paint(painter, opt->rect, Qt::AlignCenter, mode, state);

            return;
        }
    }

    QProxyStyle::drawPrimitive(element, option, painter, widget);
}

static QStyle* _plastiqueStyle = nullptr;
static QStyle* _getPlastiqueStyle()
{
    if (!_plastiqueStyle) {
        if (QStyleFactory::keys().contains(QStringLiteral("plastique"))) {
            _plastiqueStyle = QStyleFactory::create(QStringLiteral("plastique"));
        }
    }

    return _plastiqueStyle;
}

void ProxyStyle::drawControl(QStyle::ControlElement element, const QStyleOption* option, QPainter* painter, const QWidget* widget) const
{
    if (element == QStyle::CE_Splitter) {
        QStyle* plasique = _getPlastiqueStyle();
        if (plasique != nullptr) {
            plasique->drawControl(element, option, painter, widget);

            // рамка вокруг сплиттера
            QRect rect = option->rect;
            if (qobject_cast<const QSplitter*>(widget)->orientation() == Qt::Horizontal)
                rect.adjust(0, 0, 0, 1);
            else
                rect.adjust(0, 0, 1, 0);
            qDrawShadeRect(painter, rect, widget->palette(), true, 1, 0);

            return;
        }

    } else if (element == QStyle::CE_PushButton) {
        // боремся с кривыми отступами между иконкой и текстом на кнопках
        QStyleOptionButton op = *qstyleoption_cast<const QStyleOptionButton*>(option);
        if (!op.icon.isNull()) {
            op.text = Utils::alignMultilineString(op.text);
            QProxyStyle::drawControl(element, &op, painter, widget);
            return;
        }
    }

    QProxyStyle::drawControl(element, option, painter, widget);
}

void ProxyStyle::drawComplexControl(QStyle::ComplexControl control, const QStyleOptionComplex* option, QPainter* painter, const QWidget* widget) const
{
    // запрет на отрисовку голубого фрейма вокруг виджетов, т.к. он работает очень странно
    if (control == CC_ComboBox) {
        QStyleOptionComboBox opt = *qstyleoption_cast<const QStyleOptionComboBox*>(option);
        opt.frame = false;
        opt.subControls.setFlag(SC_ComboBoxFrame, false);
        opt.activeSubControls.setFlag(SC_ComboBoxFrame, false);

        QProxyStyle::drawComplexControl(control, &opt, painter, widget);

        Utils::paintBorder(painter, opt.rect);

    } else if (control == CC_SpinBox) {
        QStyleOptionSpinBox opt = *qstyleoption_cast<const QStyleOptionSpinBox*>(option);
        opt.frame = false;
        opt.subControls.setFlag(SC_SpinBoxFrame, false);
        opt.subControls.setFlag(SC_GroupBoxFrame, false);

        opt.activeSubControls.setFlag(SC_SpinBoxFrame, false);
        opt.activeSubControls.setFlag(SC_GroupBoxFrame, false);

        opt.state.setFlag(State_HasFocus, false);
        opt.state.setFlag(State_FocusAtBorder, false);

        painter->save();
        QProxyStyle::drawComplexControl(control, &opt, painter, widget);
        painter->restore();

        painter->save();
        painter->setPen(widget->palette().color(QPalette::Base));
        painter->setBrush(Qt::NoBrush);
        painter->drawRoundedRect(opt.rect.adjusted(0, 1, -1, -2), 2, 2);
        painter->restore();

        Utils::paintBorder(painter, option->rect);

    } else if (control == CC_ScrollBar) {
        QProxyStyle::drawComplexControl(control, option, painter, widget);

        if (widget->parent() != nullptr && widget->parent()->objectName() == QStringLiteral("qt_scrollarea_hcontainer")) {
            if (auto area = qobject_cast<const QScrollArea*>(widget->parent()->parent()); area != nullptr && area->frameShape() == QFrame::NoFrame) {
                Utils::paintBorder(painter, option->rect);
            }
        }
    } else if (control == CC_TitleBar) {
        QProxyStyle::drawComplexControl(control, option, painter, widget);

        if (const QStyleOptionTitleBar* titleBar = qstyleoption_cast<const QStyleOptionTitleBar*>(option)) {
            // авторы стиля fusion были не совсем трезвы, когда рисовали кнопку минимизации в заголовке MDI окна - рисуем сами
            bool active = (titleBar->titleBarState & State_Active);
            QPalette palette = option->palette;
            QColor highlight = option->palette.highlight().color();
            QColor titlebarColor = QColor(active ? highlight : palette.window().color());
            QColor textColor = (active ? 0xffffff : 0xff000000);
            QColor textAlphaColor(active ? 0xffffff : 0xff000000);
            const int buttonMargin = 5;

            if ((titleBar->subControls & SC_TitleBarMinButton) && (titleBar->titleBarFlags & Qt::WindowMinimizeButtonHint)
                && !(titleBar->titleBarState & Qt::WindowMinimized)) {
                QRect minButtonRect = proxy()->subControlRect(CC_TitleBar, titleBar, SC_TitleBarMinButton, widget);
                if (minButtonRect.isValid()) {
                    painter->save();

                    QRect minButtonIconRect = minButtonRect.adjusted(buttonMargin, buttonMargin, -buttonMargin, -buttonMargin);

                    // стираем убожество, которое нарисовали в FusionStyle
                    QRect clear_rect = QRect(minButtonIconRect.center().x() - 3, minButtonIconRect.center().y() + 3, 8, 2);
                    painter->fillRect(clear_rect, QBrush(titlebarColor));

                    // рисуем свое убожество
                    int left_pos = minButtonIconRect.left() + 1;
                    int right_pos = minButtonIconRect.right() - 1;
                    int top_pos = minButtonIconRect.bottom() - buttonMargin;

                    painter->setPen(textColor);
                    painter->drawLine(left_pos, top_pos + 3, right_pos + 1, top_pos + 3);
                    painter->drawLine(left_pos, top_pos + 4, right_pos + 1, top_pos + 4);
                    painter->setPen(textAlphaColor);
                    painter->drawLine(left_pos - 1, top_pos + 3, left_pos - 1, top_pos + 4);
                    painter->drawLine(right_pos + 2, top_pos + 3, right_pos + 2, top_pos + 4);

                    painter->restore();
                    return;
                }
            }
        }

    } else {
        QProxyStyle::drawComplexControl(control, option, painter, widget);
    }
}

QRect ProxyStyle::subElementRect(SubElement element, const QStyleOption* option, const QWidget* widget) const
{
    if (element == SE_ItemViewItemCheckIndicator) {
        // сдвигаем отображение чекбокса
        const QStyleOptionViewItem* vopt = qstyleoption_cast<const QStyleOptionViewItem*>(option);
        if (vopt->text.isEmpty()) {
            QRect rect = QProxyStyle::subElementRect(SE_ItemViewItemCheckIndicator, option, widget);
            rect.moveCenter(vopt->rect.center());
            return rect;
        }

    } else if (element == SE_ItemViewItemDecoration) {
        // сдвигаем отображение иконки
        const QStyleOptionViewItem* vopt = qstyleoption_cast<const QStyleOptionViewItem*>(option);
        if (vopt->text.isEmpty()) {
            QRect rect = QProxyStyle::subElementRect(SE_ItemViewItemDecoration, option, widget);
            rect.moveCenter(vopt->rect.center());
            return rect;
        }

    } else if (element == SE_ItemViewItemText) {
        //        const QStyleOptionViewItem* vopt = qstyleoption_cast<const QStyleOptionViewItem*>(option);
    }

    return QProxyStyle::subElementRect(element, option, widget);
}

QIcon ProxyStyle::standardIcon(StandardPixmap standardIcon, const QStyleOption* opt, const QWidget* widget) const
{
    int size = pixelMetric(PM_ButtonIconSize);

    switch (standardIcon) {
        case SP_DialogOkButton:
            return Utils::resizeSvg(QStringLiteral(":/share_icons/blue/check.svg"), size);
        case SP_DialogCancelButton:
            return Utils::resizeSvg(QStringLiteral(":/share_icons/red/close.svg"), size);
        case SP_DialogHelpButton:
            return Utils::resizeSvg(QStringLiteral(":/share_icons/help.svg"), size);
        case SP_DialogOpenButton:
            return Utils::resizeSvg(QStringLiteral(":/share_icons/blue/document.svg"), size);
        case SP_DialogSaveButton:
            return Utils::resizeSvg(QStringLiteral(":/share_icons/blue/save.svg"), size);
        case SP_DialogCloseButton:
            return Utils::resizeSvg(QStringLiteral(":/share_icons/red/close.svg"), size);
        case SP_DialogApplyButton:
            return Utils::resizeSvg(QStringLiteral(":/share_icons/blue/check.svg"), size);
        case SP_DialogResetButton:
            return Utils::resizeSvg(QStringLiteral(":/share_icons/blue/reload.svg"), size);
        case SP_DialogDiscardButton:
            return Utils::resizeSvg(QStringLiteral(":/share_icons/red/close.svg"), size);
        case SP_DialogYesButton:
            return Utils::resizeSvg(QStringLiteral(":/share_icons/blue/check.svg"), size);
        case SP_DialogNoButton:
            return Utils::resizeSvg(QStringLiteral(":/share_icons/red/close.svg"), size);
        default:
            return QProxyStyle::standardIcon(standardIcon, opt, widget);
    }
}

QPixmap ProxyStyle::standardPixmap(StandardPixmap sp, const QStyleOption* opt, const QWidget* widget) const
{
    return QProxyStyle::standardPixmap(sp, opt, widget);
}

int ProxyStyle::styleHint(StyleHint hint, const QStyleOption* option, const QWidget* widget, QStyleHintReturn* returnData) const
{
    if (hint == SH_DialogButtonBox_ButtonsHaveIcons)
        return true;

#ifdef Q_OS_LINUX
    if (hint == SH_LineEdit_PasswordCharacter) {
        // стандартный символ слишком жирный
        return 0x2055;
    }
#endif

    return QProxyStyle::styleHint(hint, option, widget, returnData);
}

} // namespace zf
