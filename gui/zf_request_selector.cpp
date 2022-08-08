#include "zf_request_selector.h"
#include "private/zf_request_selector_p.h"
#include "zf_core.h"
#include "zf_core_uids.h"
#include "zf_colors_def.h"
#include "zf_ui_size.h"
#include "zf_translation.h"

#include <QMovie>
#include <QStylePainter>
#include <QBoxLayout>
#include <QtMath>
#include <QtConcurrent>

#include "zf_child_objects_container.h"

namespace zf
{

RequestSelector::RequestSelector(QWidget* parent)
    : QWidget(parent)
    , _data(new RequestSelectorItemModel(this))
{
    init();
}

RequestSelector::RequestSelector(I_ExternalRequest* delegate, QWidget* parent)
    : RequestSelector(parent)
{
    setDelegate(delegate);
}

RequestSelector::RequestSelector(const Uid& request_service_uid, QWidget* parent)
    : RequestSelector(Utils::getInterface<I_ExternalRequest>(request_service_uid), parent)
{
}

RequestSelector::RequestSelector(const EntityCode& request_service_entity_code, QWidget* parent)
    : RequestSelector(Utils::getInterface<I_ExternalRequest>(request_service_entity_code), parent)
{
}

RequestSelector::~RequestSelector()
{
    _destroing = true;
    if (_container != nullptr)
        delete _container;

    delete _object_extension;
}

RequestSelector::Status RequestSelector::status() const
{
    return _status;
}

void RequestSelector::setDelegate(I_ExternalRequest* delegate)
{
    Z_CHECK_NULL(delegate);
    _delegate = delegate;

    QObject* object = dynamic_cast<QObject*>(_delegate);
    Z_CHECK_X(object != nullptr, "Delegate not inherited from QObject");
    connect(object, &QObject::destroyed, this, &RequestSelector::sl_delegateObjectDestroyed);

    _text_edited_timer->setInterval(_delegate->requestDelay(_request_options));
    _hide_wait_movie_timer->setInterval(_delegate->requestDelay(_request_options));

    if (_id.isValid())
        setStatus(NotFound);

    if (_id.isValid())
        updateLookupText();
}

int RequestSelector::popupHeightCount() const
{
    return _view_height_count;
}

void RequestSelector::setPopupHeightCount(int count)
{
    _view_height_count = qBound(3, count, 20);
}

bool RequestSelector::isReadOnly() const
{
    return _read_only;
}

void RequestSelector::setReadOnly(bool b)
{
    if (_read_only == b)
        return;

    _read_only = b;

    updateEnabled();
    updateGeometry();
    updateControlsGeometry();
}

QString RequestSelector::placeholderText() const
{
    return _editor->placeholderText();
}

void RequestSelector::setPlaceholderText(const QString& s)
{
    _editor->setPlaceholderText(s);
}

void RequestSelector::setOverrideText(const QString& text)
{
    if (_override_text == text)
        return;

    _override_text = text;

    if (text.isEmpty()) {
        _override_editor->hide();

    } else {
        _override_editor->setText(text);
        _override_editor->setGeometry(_editor->geometry());
        _override_editor->show();
        _override_editor->raise();
    }
}

bool RequestSelector::isEditable() const
{
    if (_text_mode == TextMode::Free && !isReadOnly() && isEnabled())
        return true;

    return _delegate != nullptr && !isReadOnly() && isEnabled()
           && (_request_type.isEmpty() || (_text_mode == TextMode::Combined || _delegate->canEdit(_parent_keys, _request_type, _request_options)));
}

bool RequestSelector::isPopup() const
{
    return _popup;
}

void RequestSelector::setCurrentID(const Uid& key)
{
    _user_search = false;
    setCurrentID_helper(key);
}

bool RequestSelector::setCurrentID_helper(const Uid& key)
{
    if (_delegate == nullptr)
        return false;

    bool changes = (key != _id);
    if (changes) {
        cancelSearch();

        _id = key;
        updateCurrentAttributes(_user_search);

        int row = _data->findUid(_id);
        if (row >= 0) {
            Z_CHECK(updateLookupText());

        } else {
            if (_id.isValid() && _delegate->isCorrectKey(_id, _request_options))
                keySearch();
        }

        emit sg_changed(_id);
    }

    updateViewCurrent();

    return changes;
}

void RequestSelector::updateCurrentAttributes(bool by_user)
{
    auto old_attributes = _current_attributes;

    if (!_id.isValid() || _attributes.isEmpty()) {
        _current_attributes.clear();

    } else {
        int row = _data->findUid(_id);
        if (row >= 0) {
            Z_CHECK(row < _attributes.count());
            _current_attributes = _attributes.at(row);

        } else {
            _current_attributes.clear();
        }
    }

    // атрибутов может быть много, поэтому проверяем только на пустоту
    if (!old_attributes.isEmpty() || !_current_attributes.isEmpty())
        emit sg_attributesChanged(by_user, old_attributes, _current_attributes);
}

void RequestSelector::setCurrentID(const QString& key)
{
    setCurrentID(Uid::general(QVariantList {key}));
}

void RequestSelector::setCurrentID(int key)
{
    setCurrentID(Uid::general(QVariantList {key}));
}

Uid RequestSelector::currentID() const
{
    return _id;
}

QString RequestSelector::currentID_string() const
{
    return currentID().isValid() ? currentID().asString(0) : QString();
}

int RequestSelector::currentID_int() const
{
    return currentID().isValid() ? currentID().asInt(0) : -1;
}

const QMap<QString, QVariant>& RequestSelector::attributes() const
{
    return _current_attributes;
}

QString RequestSelector::currentText() const
{
    return _editor->text();
}

bool RequestSelector::search(const QString& text)
{
    _user_search = false;
    if (setEditorTextHelper(Utils::prepareSearchString(text, false)))
        return textSearch(_editor->text(), true);

    return false;
}

bool RequestSelector::setRequestType(const QString& request_type)
{
    if (_request_type == request_type)
        return false;

    _request_type = request_type;

    updateEnabled();
    updateGeometry();
    updateControlsGeometry();

    return true;
}

QString RequestSelector::requestType() const
{
    return _request_type;
}

void RequestSelector::setRequestOptions(const QVariant& o)
{
    _request_options = o;
}

QVariant RequestSelector::requestOptions() const
{
    return _request_options;
}

bool RequestSelector::setParentKeys(const UidList& parent_keys)
{
    if (_parent_keys == parent_keys)
        return false;

    _parent_keys = parent_keys;
    return true;
}

bool RequestSelector::setParentKeys(const QVariantList& parent_keys)
{
    UidList keys;
    for (auto& k : parent_keys) {
        keys << Uid::general(QVariantList {k});
    }

    return setParentKeys(keys);
}

bool RequestSelector::setParentKeys(const QStringList& parent_keys)
{
    UidList keys;
    for (auto& k : parent_keys) {
        keys << Uid::general(QVariantList {k});
    }

    return setParentKeys(keys);
}

bool RequestSelector::setParentKeys(const QList<int>& parent_keys)
{
    UidList keys;
    for (auto& k : parent_keys) {
        keys << Uid::general(QVariantList {k});
    }

    return setParentKeys(keys);
}

UidList RequestSelector::parentKeys() const
{
    return _parent_keys;
}

void RequestSelector::setFilterText(const QString& s)
{
    _filter_text = s.trimmed();
}

QString RequestSelector::filterText() const
{
    return _filter_text;
}

void RequestSelector::clear(const QString& text)
{
    clearHelper(text, false);
}

RequestSelector::TextMode RequestSelector::textMode() const
{
    return _text_mode;
}

void RequestSelector::setTextMode(TextMode mode)
{
    if (_text_mode == mode)
        return;

    _text_mode = mode;
    if (_text_mode == TextMode::Free)
        cancelSearch();
    else
        search(currentText());
}

bool RequestSelector::isClearOnExit() const
{
    return _clear_on_exit;
}

void RequestSelector::setClearOnExit(bool b)
{
    _clear_on_exit = b;
}

void RequestSelector::setComboboxMode(bool b)
{
    if (b == _combobox_mode)
        return;

    _combobox_mode = b;

    updateEnabledHelper();
    updateControlsGeometry();

    update();
}

bool RequestSelector::isComboboxMode() const
{
    return _combobox_mode;
}

QLineEdit* RequestSelector::editor() const
{
    return _editor;
}

int RequestSelector::maximumControlsSize() const
{
    return _maximum_controls_size;
}

void RequestSelector::setMaximumControlsSize(int n)
{
    if (_maximum_controls_size == n)
        return;

    _maximum_controls_size = n;
    updateControlsGeometry();
}

QSize RequestSelector::sizeHint() const
{
    return UiSizes::defaultLineEditSize(font());
}

QSize RequestSelector::minimumSizeHint() const
{
    return sizeHint();
}

void RequestSelector::popupHide(bool apply)
{
    if (!_popup || _popup_visible_chinging)
        return;

    _popup = false;
    _popup_visible_chinging = true;
    bool edited = false;

    // поиск активен только при активном выпадающем списке
    cancelSearch();

    if (apply) {
        int current_row = view()->currentIndex().row();
        if (current_row >= 0) {
            auto new_id = _data->rowUid(current_row);
            edited = currentID() != new_id;
            if (edited)
                setCurrentID_helper(new_id);
        }
    }

    container()->hide();

    updateArrow(QStyle::State_None);
    setFocus();

    _popup_visible_chinging = false;

    emit sg_popupClosed(apply);

    if (edited)
        emit sg_edited(_id);
}

void RequestSelector::popupShow()
{
    if (_popup || _popup_visible_chinging || isReadOnly() || !hasFocus())
        return;

    _popup = true;
    _popup_visible_chinging = true;

    if (!updatePopupGeometry()) {
        _popup_visible_chinging = false;
        return;
    }

    updateArrow(QStyle::State_Sunken);

    _editor->deselect();
    _editor->setFocus();
    container()->show();

    _popup_visible_chinging = false;

    emit sg_popupOpened();
}

bool RequestSelector::updatePopupGeometry()
{
    QRect screen_geometry = Utils::screenRect(this, Qt::AlignLeft | Qt::AlignBottom);
    if (screen_geometry.isNull())
        return false;

    QRect list_rect(rect());

    QPoint below = mapToGlobal(list_rect.bottomLeft());
    int below_height = screen_geometry.bottom() - below.y();
    QPoint above = mapToGlobal(list_rect.topLeft());
    int above_height = above.y() - screen_geometry.y();

    int height_count = 1;
    if (view()->model() != nullptr) {
        if (view()->model()->rowCount() > 0)
            list_rect.setHeight(view()->visualRect(view()->model()->index(0, 0)).height());
        height_count = qMax(1, qMin(view()->model()->rowCount(), _view_height_count));
    }

    list_rect.setHeight(list_rect.height() * height_count);
    if (height_count > 1) {
        int spacing = 0;
        spacing = container()->_list_view->spacing();
        list_rect.setHeight(list_rect.height() + (height_count - 1) * spacing);
    }
    list_rect.setHeight(list_rect.height() + 2);

    if (list_rect.height() <= below_height) {
        list_rect.moveTopLeft(below);
    } else if (list_rect.height() <= above_height) {
        list_rect.moveBottomLeft(above);
    } else if (below_height >= above_height) {
        list_rect.setHeight(below_height);
        list_rect.moveTopLeft(below);
    } else {
        list_rect.setHeight(above_height);
        list_rect.moveBottomLeft(above);
    }

    if (!fuzzyCompare(_view_width_count, 1)) {
        int old_width = list_rect.width();
        list_rect.setWidth(static_cast<int>(list_rect.width() * _view_width_count));
        list_rect.moveLeft(list_rect.left() - (list_rect.width() - old_width));
    }

    container()->setGeometry(list_rect);

    return true;
}

bool RequestSelector::event(QEvent* event)
{
    switch (event->type()) {
        case QEvent::LayoutDirectionChange:
        case QEvent::ApplicationLayoutDirectionChange:
            updateControlsGeometry();
            break;

        case QEvent::HoverEnter:
        case QEvent::HoverLeave:
        case QEvent::HoverMove:
            if (const QHoverEvent* he = static_cast<const QHoverEvent*>(event))
                updateHoverControl(he->pos());
            break;

        default:
            break;
    }

    return QWidget::event(event);
}

void RequestSelector::changeEvent(QEvent* event)
{
    switch (event->type()) {
        case QEvent::StyleChange:
        case QEvent::FontChange:
            updateControlsGeometry();
            break;
        default:
            break;
    }
    QWidget::changeEvent(event);
}

void RequestSelector::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    updateControlsGeometry();
    updateGeometry();
}

bool RequestSelector::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == _editor) {
        switch (event->type()) {
            case QEvent::HoverEnter:
            case QEvent::HoverLeave:
            case QEvent::HoverMove: {
                if (const QHoverEvent* he = static_cast<const QHoverEvent*>(event))
                    updateHoverControl(he->pos());
                break;
            }
            case QEvent::MouseButtonDblClick: {
                // если можно запрашивать пустое значение, то по даблклику делаем запрос
                if (!_id.isValid() && _editor->text().trimmed().isEmpty() && !isPopup() && _delegate != nullptr
                    && _delegate->checkText("", _request_type, _request_options)) {
                    textSearch("", false);
                    popupShow();
                    return true;
                }
            }
            default:
                break;
        }

        if (isEditable() && _eat_focus_out && event->type() == QEvent::FocusOut) {
            if (view()->isVisible())
                return true;
        }

        if (event->type() == QEvent::EnabledChange) {
            if (_editor->testAttribute(Qt::WA_Disabled))
                _editor->setAttribute(Qt::WA_Disabled, false);

            updateEnabled();
            return true;
        }

        // пока идет запрос по ключу не даем вводить текст
        if (_status == RequestingKey && (event->type() == QEvent::KeyPress || event->type() == QEvent::KeyRelease || event->type() == QEvent::Clipboard))
            return true;

        // нажатие на enter или клик для не выбранного идентификатора должно запускать поиск (если это допустимо)
        if (_delegate != nullptr && !_id.isValid() && _delegate->checkText(_editor->text().trimmed(), _request_type, _request_options)
            && _delegate->canEdit(_parent_keys, _request_type, _request_options) && (_status != RequestingValue && _status != RequestingKey)) {
            if (event->type() == QEvent::KeyPress) {
                QKeyEvent* ke = static_cast<QKeyEvent*>(event);
                if (ke->modifiers() == Qt::NoModifier && (ke->key() == Qt::Key_Enter || ke->key() == Qt::Key_Return || ke->key() == Qt::Key_Down)) {
                    startRequestTimer();
                }

            } else if (event->type() == QEvent::MouseButtonRelease && !_editor->hasSelectedText() && !_combobox_mode) {
                QMouseEvent* me = static_cast<QMouseEvent*>(event);
                if (me->button() == Qt::MouseButton::LeftButton) {
                    startRequestTimer();
                }

            } else if (event->type() == QEvent::MouseButtonPress) {
                if (_text_edited_timer->isActive() && _status != RequestingValue && _status != RequestingKey)
                    cancelSearch();
            }
        }
    }

    if (watched == container()) {
        switch (event->type()) {
            case QEvent::MouseButtonPress: {
                QMouseEvent* me = static_cast<QMouseEvent*>(event);
                if (isPopup() && !container()->underMouse()) {
                    // нажатие за пределами popup при открытом popup
                    auto pos = mapFromGlobal(container()->mapToGlobal(me->pos()));

                    if (isArrowPos(pos)) {
                        // Нажата кнопка комбобокса при открытом view. Надо проигнорировать нажатие, чтобы не было
                        // открытия view сразу после закрытия
                        _ignore_popup_click_timer->start();
                        popupHide(view()->currentIndex().isValid());

                    } else {
                        if (_erase_button->geometry().contains(pos) || (_clear_on_exit && !_id.isValid() && !rect().contains(pos)))
                            clearHelper("", true);

                        popupHide(false);
                    }
                    return true;
                }

                break;
            }

            case QEvent::KeyPress: {
                QKeyEvent* ke = static_cast<QKeyEvent*>(event);
                const int key = ke->key();
                const int row_count = _data->rowCount();
                const bool no_modifier = ke->modifiers() == Qt::NoModifier;

                switch (key) {
                    case Qt::Key_Tab:
                        if (no_modifier)
                            popupHide(true);
                        break;

                    case Qt::Key_Enter:
                    case Qt::Key_Return:
                        if (no_modifier) {
                            popupHide(true);
                            return true;
                        }
                        return false;

                    case Qt::Key_End:
                    case Qt::Key_Home:
                        if (ke->modifiers() & Qt::ControlModifier)
                            return false;
                        break;

                    case Qt::Key_Up:
                        if (no_modifier) {
                            itemUp();
                            return true;
                        }
                        return false;

                    case Qt::Key_Down:
                        if (no_modifier) {
                            itemDown();
                            return true;
                        }

                        return false;

                    case Qt::Key_PageUp:
                        if (ke->modifiers() == Qt::ControlModifier) {
                            scrollUp();
                            return false;
                        }

                        if (row_count > 0 && no_modifier) {
                            scrollPageUp();
                            return true;
                        }
                        return false;

                    case Qt::Key_PageDown:
                        if (ke->modifiers() == Qt::ControlModifier) {
                            scrollDown();
                            return false;
                        }

                        if (row_count > 0 && no_modifier) {
                            scrollPageDown();
                            return true;
                        }
                        return false;
                }

                _eat_focus_out = false;
                (static_cast<QObject*>(_editor))->event(ke);
                _eat_focus_out = true;
                if (event->isAccepted() || !view()->isVisible()) {
                    // widget lost focus, hide the popup
                    if (_editor && (!_editor->hasFocus()))
                        popupHide(false);
                    if (event->isAccepted())
                        return true;
                }

                if (ke->matches(QKeySequence::Cancel)) {
                    popupHide(false);
                    return true;
                }

                break;
            }

            default:
                break;
        }
    }

    return QWidget::eventFilter(watched, event);
}

void RequestSelector::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event)

    QStylePainter painter(this);

    QStyleOptionFrame panel;
    _editor->initStyleOption(&panel);
    panel.rect = rect();
    if (_has_frame) {
        panel.frameShape = QFrame::StyledPanel;
        panel.lineWidth = 1;
    }

    panel.styleObject = this;
    style()->drawPrimitive(QStyle::PE_PanelLineEdit, &panel, &painter, this);

    if (_combobox_mode && !isReadOnly()) {
        painter.save();
        //        painter.setPen(palette().color(QPalette::Text));
        QStyleOptionComboBox opt_combo;
        initStyleOption(&opt_combo);
        painter.drawComplexControl(QStyle::CC_ComboBox, opt_combo);
        painter.restore();
    }
}

void RequestSelector::showEvent(QShowEvent* event)
{
    QWidget::showEvent(event);
}

void RequestSelector::mousePressEvent(QMouseEvent* event)
{
    if (_internal_data_update_counter > 0) {
        event->ignore();
        return;
    }

    if (isEnabled()) {
        if (isAllowPressArrow() && isArrowPos(event->pos())) {
            if (!isPopup() && _ignore_popup_click_timer->isActive()) {
                _ignore_popup_click_timer->stop();
                return;
            }

            if (isPopup()) {
                popupHide(true);

            } else {
                if (_id.isValid() && _delegate != nullptr && _delegate->checkText("", _request_type, _request_options)) {
                    textSearch("", false);
                    popupShow();

                } else {
                    textSearch(_editor->text(), true);
                    popupShow();
                }
            }
        }
    }

    QWidget::mousePressEvent(event);
}

void RequestSelector::sl_message_dispatcher_inbound(const Uid& sender, const Message& message, SubscribeHandle subscribe_handle)
{
    Q_UNUSED(subscribe_handle)
    Q_UNUSED(sender)

    Core::messageDispatcher()->confirmMessageDelivery(message, this);

    if (_feedback_message_id.isValid() && message.feedbackMessageId() == _feedback_message_id) {
        _feedback_message_id.clear();
        _current_search_text.reset();
        onSearchFeedback(message);
    }
}

void RequestSelector::sl_delegateObjectDestroyed(QObject* obj)
{
    Q_UNUSED(obj)
    Z_HALT("I_ExternalRequest object destroyed");
}

void RequestSelector::sl_editTextChanged(const QString& s)
{
    if (_delegate == nullptr)
        return;

    emit sg_textChaged(s);
    _update_enabled_timer->start();

    if (_combobox_mode)
        update(arrowRect());
}

void RequestSelector::sl_editTextEdited(const QString& s)
{
    if (_delegate == nullptr)
        return;

    cancelSearch();
    // раз ищем по тексту, то идентификтор стирается
    bool changed = setCurrentID_helper(Uid());

    QString prepared = Utils::prepareSearchString(s, true);
    if (!prepared.isEmpty() || !_combobox_mode || isPopup()) {
        if (!_delegate->checkText(prepared, _request_type, _request_options) || !_delegate->canEdit(_parent_keys, _request_type, _request_options)) {
            if (isPopup())
                popupHide(false);

        } else {
            startRequestTimer();
        }
    }

    emit sg_textEdited(s);
    if (changed)
        emit sg_edited(currentID());
}

void RequestSelector::sl_textEditTimeout()
{
    _user_search = true;
    textSearch(_editor->text(), true);
}

void RequestSelector::sl_onBeginInternalUpdateData()
{
    _internal_data_update_counter++;
    updateEnabled();
}

void RequestSelector::sl_onEndUpdateInternalData(int key_row)
{
    _internal_data_update_counter--;
    Z_CHECK(_internal_data_update_counter >= 0);
    if (_internal_data_update_counter > 0)
        return;

    updateCurrentAttributes(_user_search);

    if (_status == RequestingKey) {
        // поиск по ключам
        Z_CHECK(_id.isValid());
        if (key_row >= 0) {
            // найдено
            setStatus(Found);

        } else {
            setStatus(NotFound);
        }
        // т.к. мы искали по конкретным ключам, то надо принудительно прописать результат поиска в редактор
        updateLookupText();

    } else {
        // поиск по тексту
        if (_data->rowCount() > 0 && Utils::prepareSearchString(_data->rowName(0), true) == Utils::prepareSearchString(_editor->text(), true)) {
            // текст поиска совпадает с найденным результатом, значит надо установить текущее значение ключа
            setCurrentID_helper(_data->rowUid(0));
            if (_data->rowCount() > 1)
                popupShow();

        } else {
            if (_status == RequestingValue)
                popupShow();
        }
    }

    updateInternalDataFinish();
}

void RequestSelector::init()
{
    connect(_data, &RequestSelectorItemModel::sg_onBeginUpdateData, this, &RequestSelector::sl_onBeginInternalUpdateData);
    connect(_data, &RequestSelectorItemModel::sg_onEndUpdateData, this, &RequestSelector::sl_onEndUpdateInternalData);

    _object_extension = new ObjectExtension(this);

    Z_CHECK(Core::messageDispatcher()->registerObject(CoreUids::REQUEST_SELECTOR, this, QStringLiteral("sl_message_dispatcher_inbound")));

    _editor = new RequestSelectorEditor(this);
    _editor->setObjectName(QStringLiteral("zfed"));
    _editor->installEventFilter(this);
    connect(_editor, &QLineEdit::textChanged, this, &RequestSelector::sl_editTextChanged);
    connect(_editor, &QLineEdit::textEdited, this, &RequestSelector::sl_editTextEdited);
    connect(_editor, &QLineEdit::textChanged, this, &RequestSelector::updateEnabled);

    setFocusProxy(_editor);

    _override_editor = new RequestSelectorEditor(this);
    _override_editor->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    _override_editor->setHidden(true);

    setAttribute(Qt::WA_Hover, true);
    setFocusPolicy(Qt::StrongFocus);
    setAttribute(Qt::WA_InputMethodEnabled);
    setAttribute(Qt::WA_KeyCompression);
    setMouseTracking(true);
    setAcceptDrops(true);
    setAttribute(Qt::WA_MacShowFocusRect);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    _text_edited_timer = new QTimer(this);
    _text_edited_timer->setObjectName("zft1");
    _text_edited_timer->setSingleShot(true);
    connect(_text_edited_timer, &QTimer::timeout, this, &RequestSelector::sl_textEditTimeout);

    _update_enabled_timer = new QTimer(this);
    _update_enabled_timer->setObjectName("zft2");
    _update_enabled_timer->setSingleShot(true);
    _update_enabled_timer->setInterval(0);
    connect(_update_enabled_timer, &QTimer::timeout, this, [&]() { updateEnabledHelper(); });
    _update_enabled_timer->start();

    _wait_movie_label = Utils::createWaitingMovie(this);
    _wait_movie_label->setHidden(true);

    _hide_wait_movie_timer = new QTimer(this);
    _hide_wait_movie_timer->setSingleShot(true);
    connect(_hide_wait_movie_timer, &QTimer::timeout, this, [&]() {
        _wait_movie_label->hide();
        _wait_movie_label->movie()->stop();
    });

    _erase_button = new QToolButton(this);
    _erase_button->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    _erase_button->setToolTip(ZF_TR(ZFT_CLEAR));
    _erase_button->setObjectName(QStringLiteral("zfer"));
    _erase_button->installEventFilter(this);
    _erase_button->show();
    _erase_button->setAutoRaise(true);
    _erase_button->setFocusPolicy(Qt::NoFocus);
    _erase_button->setIcon(QIcon(":/share_icons/cancel-bw.svg"));
    connect(_erase_button, &QToolButton::clicked, this, [&]() { clearHelper("", true); });

    _ignore_popup_click_timer = new QTimer(this);
    _ignore_popup_click_timer->setSingleShot(true);
    _ignore_popup_click_timer->setInterval(qApp->doubleClickInterval());

    updateEnabled();
    updateControlsGeometry();

    connect(qApp, &QApplication::focusChanged, this, [&](QWidget* old, QWidget* now) {
        if (!isVisible() || !_clear_on_exit)
            return;

        if (!_id.isValid() && (old == _editor || old == container()) && now != _editor && now != container())
            clearHelper("", true);
    });
}

void RequestSelector::initStyleOption(QStyleOptionComboBox* option) const
{
    if (option == nullptr)
        return;

    option->initFrom(this);
    option->editable = isEditable();
    option->frame = _has_frame;
    if (hasFocus() && !isEditable())
        option->state |= QStyle::State_Selected;

    option->subControls = QStyle::SC_All;
    option->subControls &= ~(QStyle::SC_ComboBoxListBoxPopup);

    if (!_has_frame) {
        option->subControls &= ~(QStyle::SC_ComboBoxFrame | QStyle::SC_ComboBoxEditField);
        option->state &= ~(QStyle::State_HasFocus | QStyle::State_FocusAtBorder);
    }

    if (isAllowPressArrow()) {
        if (_arrow_state == QStyle::State_Sunken) {
            option->activeSubControls = QStyle::SC_ComboBoxArrow;
            option->state |= _arrow_state;

        } else {
            option->activeSubControls = _hover_control;
        }

    } else {
        option->state.setFlag(QStyle::State_Enabled, false);
        option->state.setFlag(QStyle::State_MouseOver, false);
        option->state.setFlag(QStyle::State_HasFocus, false);
        option->state.setFlag(QStyle::State_ReadOnly, true);
        option->palette.setBrush(QPalette::ButtonText, option->palette.color(QPalette::ColorGroup::Disabled, QPalette::ColorRole::ButtonText));
    }

    if (isPopup())
        option->state |= QStyle::State_On;
}

RequestSelectorContainer* RequestSelector::container() const
{
    if (_destroing)
        return nullptr;

    if (_container.isNull()) {
        _container = new RequestSelectorContainer(const_cast<RequestSelector*>(this), _data);
        _container->setObjectName(QStringLiteral("zfc"));
        _container->hide();
        _container->installEventFilter(const_cast<RequestSelector*>(this));
    }
    return _container;
}

QListView* RequestSelector::view() const
{
    return container()->_list_view;
}

void RequestSelector::updateEnabled()
{
    _update_enabled_timer->start();
}

bool RequestSelector::objectExtensionDestroyed() const
{
    return _object_extension->objectExtensionDestroyed();
}

void RequestSelector::updateEnabledHelper()
{
    _editor->setVisible(_delegate != nullptr);
    bool is_editable = isEditable();
    // через палитру не работает. почему - загадка
    QColor background = is_editable ? Colors::background() : Colors::readOnlyBackground();

    _editor->setStyleSheet(QStringLiteral("border: none; background-color: %1").arg(background.name()));
    _editor->setReadOnly(!is_editable);

    _override_editor->setStyleSheet(_editor->styleSheet());
    _override_editor->setReadOnly(_editor->isReadOnly());

    _erase_button->setHidden(isReadOnly());
    if (!_erase_button->isHidden())
        _erase_button->setStyleSheet(QStringLiteral("background-color: %1").arg(background.name()));

    _erase_button->setEnabled(_internal_data_update_counter == 0 && !_editor->text().trimmed().isEmpty());
}

void RequestSelector::updateControlsGeometry()
{
    _editor->setGeometry(editorRect());

    auto wait_rect = waitMovieRect();
    auto clear_rect = clearButtonRect();

    _wait_movie_label->setGeometry(wait_rect);
    _wait_movie_label->movie()->setScaledSize(wait_rect.size());

    int right_shift = 0;

    _erase_button->setGeometry(clear_rect);
    UiSizes::prepareEditorToolButton(font(), _erase_button, false);

    if (_erase_button->isVisible())
        right_shift += _erase_button->width() + 1;

    if (right_shift > 0)
        right_shift += 1;

    auto margins = _editor->textMargins();
    margins.setRight(right_shift);
    _editor->setTextMargins(margins);
}

bool RequestSelector::updateLookupText()
{
    QString text;
    bool found = true;

    if (_id.isValid()) {
        int row = _data->findUid(_id);
        if (row >= 0)
            text = _data->rowName(row); // найдено в загруженных данных
        else
            found = false;
    }

    setEditorTextHelper(text);

    return found;
}

void RequestSelector::keySearch()
{
    Z_CHECK_NULL(_delegate);
    Z_CHECK(_id.isValid());

    cancelSearch();

    _feedback_message_id = _delegate->requestLookup(this, _id, _request_type, _request_options);
    setStatus(RequestingKey);

    showSearchStatus();
    emit sg_searchStarted(_user_search);
}

bool RequestSelector::textSearch(const QString& text, bool clear_id)
{
    if (_delegate == nullptr)
        return false;

    if (_text_mode == TextMode::Free)
        return false;

    cancelSearch();

    // раз ищем по тексту, то идентификтор стирается
    if (clear_id)
        setCurrentID_helper(Uid());

    QString prepared = Utils::prepareSearchString(text, true);
    if (_current_search_text != nullptr && prepared == *_current_search_text)
        return false;

    if (!_delegate->canEdit(_parent_keys, _request_type, _request_options) || !_delegate->checkText(prepared, _request_type, _request_options)) {
        if (_user_search)
            emit sg_edited(currentID());
        return false;
    }

    if (!_filter_text.isEmpty()) {
        // дополняем строку поиска фильтром
        if (!prepared.contains(_filter_text, Qt::CaseInsensitive)) {
            prepared = QStringLiteral("%1 %2").arg(_filter_text, prepared);
        }
    }

    _feedback_message_id = _delegate->requestLookup(this, prepared, _parent_keys, _request_type, _request_options);
    _current_search_text = std::make_unique<QString>(prepared);
    setStatus(RequestingValue);

    _data->setInternalData({});

    showSearchStatus();
    emit sg_searchStarted(_user_search);

    return true;
}

void RequestSelector::cancelSearch()
{
    auto status = _status;

    _text_edited_timer->stop();
    hideSearchStatus();

    if (!_feedback_message_id.isValid()) {
        if (_id.isValid())
            setStatus(Undefined);
        else
            setStatus(Found);

        return;
    }

    _feedback_message_id.clear();

    if (_delegate == nullptr || !_id.isValid())
        setStatus(Invalid);
    else
        setStatus(NotFound);

    if (status == RequestingKey || status == RequestingValue)
        emit sg_searchFinished(_user_search, true);
}

bool RequestSelector::setEditorTextHelper(const QString& text)
{
    if (_editor->text().trimmed() == text.trimmed())
        return false;

    _editor->setText(text);
    _editor->setCursorPosition(_editor->text().length());

    if (_delegate && !_delegate->checkText(text, _request_type, _request_options) && (_status == RequestingKey || _status == RequestingValue))
        cancelSearch();

    return true;
}

void RequestSelector::onSearchFeedback(const Message& message)
{
    Z_CHECK_NULL(_delegate);
    Z_CHECK(_status == RequestingKey || _status == RequestingValue);

    _user_search_internal_data_update = _user_search;
    bool thread_started = false;

    if (message.messageType() != MessageType::Error) {
        auto msg = ExternalRequestMessage(message);
        // заполняем контейнер полученными данными
        _attributes = msg.attributes();
        thread_started = _data->setInternalData(msg.result());

    } else {
        thread_started = _data->setInternalData({});
        _attributes.clear();
        Z_CHECK(!thread_started);
        setStatus(NotFound);
        Core::postError(ErrorMessage(message).error().fullText());
    }

    if (!thread_started) {
        updateCurrentAttributes(_user_search_internal_data_update);
        updateInternalDataFinish();
    }
}

void RequestSelector::setStatus(RequestSelector::Status status)
{
    if (_status == status)
        return;

    _status = status;

    updateEnabled();
    updateControlsGeometry();

    emit sg_statusChanged(_status);
}

int RequestSelector::pageStep() const
{
    int row_count = view()->model()->rowCount();
    QModelIndex top_index = view()->indexAt(QPoint(0, 0));
    QModelIndex bottom_index = view()->indexAt(QPoint(0, view()->height()));
    return qMax(0, (bottom_index.isValid() ? bottom_index.row() : row_count - 1) - top_index.row() - 1);
}

void RequestSelector::scrollPageUp()
{
    if (isReadOnly())
        return;

    int step = pageStep();
    if (step <= 0)
        return;

    QModelIndex cur_index = view()->currentIndex();
    int next_row = qBound(0, cur_index.row() - step, view()->model()->rowCount(cur_index.parent()) - 1);
    view()->setCurrentIndex(view()->model()->index(next_row, 0, cur_index.parent()));
}

void RequestSelector::scrollPageDown()
{
    if (isReadOnly())
        return;

    int step = pageStep();
    if (step <= 0)
        return;

    QModelIndex cur_index = view()->currentIndex();
    int next_row = qBound(0, cur_index.row() + step, view()->model()->rowCount(cur_index.parent()) - 1);
    view()->setCurrentIndex(view()->model()->index(next_row, 0, cur_index.parent()));
}

void RequestSelector::scrollUp()
{
    if (isReadOnly())
        return;

    int row_count = view()->model()->rowCount();
    if (row_count > 0)
        view()->setCurrentIndex(view()->model()->index(0, 0));
}

void RequestSelector::scrollDown()
{
    if (isReadOnly())
        return;

    int row_count = view()->model()->rowCount();
    if (row_count > 0)
        view()->setCurrentIndex(view()->model()->index(row_count - 1, 0));
}

void RequestSelector::itemUp()
{
    if (isReadOnly())
        return;

    QModelIndex cur_index = view()->currentIndex();
    const int row_count = view()->model()->rowCount();

    if (cur_index.isValid()) {
        QModelIndex next_index = Utils::getNextItemModelIndex(cur_index, false);
        if (next_index.isValid())
            view()->setCurrentIndex(view()->model()->index(next_index.row(), 0, next_index.parent()));

    } else if (row_count > 0) {
        view()->setCurrentIndex(view()->model()->index(0, 0));
    }
}

void RequestSelector::itemDown()
{
    if (isReadOnly())
        return;

    QModelIndex cur_index = view()->currentIndex();
    const int row_count = view()->model()->rowCount();

    if (cur_index.isValid()) {
        QModelIndex next_index = Utils::getNextItemModelIndex(cur_index, true);
        if (next_index.isValid())
            view()->setCurrentIndex(view()->model()->index(next_index.row(), 0, next_index.parent()));

    } else if (row_count > 0) {
        view()->setCurrentIndex(view()->model()->index(0, 0));
    }
}

QRect RequestSelector::editorRect() const
{
    return rect().adjusted(1, 1, -1 - arrowButtonShift(), -1);
}

QRect RequestSelector::waitMovieRect() const
{
    QRect rect = this->rect();
    int size = qMin(Utils::scaleUI(24), rect.height());
    if (_maximum_controls_size > 0)
        size = qMin(_maximum_controls_size + 1, size);

    rect.setLeft(rect.right() - size);
    rect.setWidth(size);
    rect.setHeight(size);
    rect.moveTop(this->rect().center().y() - size / 2);

    rect.adjust(2, 2, -2, -2);

    rect.moveLeft(rect.left() - clearButtonRect().width() - 1 - arrowButtonShift());

    return rect;
}

QRect RequestSelector::clearButtonRect() const
{
    QRect rect = this->rect();
    int size = qMin(Utils::scaleUI(24), rect.height() - 4);
    if (_maximum_controls_size > 0)
        size = qMin(_maximum_controls_size + 1, size);

    rect.setLeft(rect.right() - size - 1);
    rect.setWidth(size);
    rect.setHeight(size);
    rect.moveCenter({rect.center().x(), this->rect().center().y()});

    rect.moveLeft(rect.left() - arrowButtonShift());

    return rect;
}

QRect RequestSelector::arrowRect() const
{
    QStyleOptionComboBox opt;
    initStyleOption(&opt);
    style()->subControlRect(QStyle::CC_ComboBox, &opt, QStyle::SC_ComboBoxArrow, this).width();

    QSize size = QSize(style()->subControlRect(QStyle::CC_ComboBox, &opt, QStyle::SC_ComboBoxArrow, this).width(), height() - 2);
    QRect res = QRect(0, 0, size.width(), size.height());
    res.moveRight(rect().right() - 1);
    return res;
}

int RequestSelector::arrowButtonShift() const
{
    if (!_combobox_mode)
        return 0;

    return arrowRect().width() + 1;
}

bool RequestSelector::isArrowPos(const QPoint& pos) const
{
    if (isReadOnly() || !_combobox_mode)
        return false;

    return arrowRect().contains(pos);
}

bool RequestSelector::isAllowPressArrow() const
{
    if (!_combobox_mode || _delegate == nullptr)
        return false;

    if (_id.isValid())
        return _delegate->checkText("", _request_type, _request_options);

    return _delegate->checkText(_editor->text().trimmed(), _request_type, _request_options);
}

void RequestSelector::showSearchStatus()
{
    _hide_wait_movie_timer->stop();

    updateControlsGeometry();
    _wait_movie_label->show();
    if (_wait_movie_label->movie()->state() != QMovie::MovieState::Running)
        _wait_movie_label->movie()->start();
}

void RequestSelector::hideSearchStatus()
{
    _hide_wait_movie_timer->start();
}

void RequestSelector::startRequestTimer()
{
    _text_edited_timer->start();
    showSearchStatus();
}

void RequestSelector::updateArrow(QStyle::StateFlag state)
{
    if (_arrow_state == state)
        return;
    _arrow_state = state;
    update(arrowRect());
}

bool RequestSelector::updateHoverControl(const QPoint& pos)
{
    // взято из QComboBoxPrivate
    QRect lastHoverRect = _hover_rect;
    QStyle::SubControl lastHoverControl = _hover_control;
    bool doesHover = testAttribute(Qt::WA_Hover);
    if (lastHoverControl != newHoverControl(pos) && doesHover) {
        update(lastHoverRect);
        update(_hover_rect);
        return true;
    }
    return !doesHover;
}

QStyle::SubControl RequestSelector::newHoverControl(const QPoint& pos)
{
    // взято из QComboBoxPrivate
    QStyleOptionComboBox opt;
    initStyleOption(&opt);
    opt.subControls = QStyle::SC_All;
    _hover_control = style()->hitTestComplexControl(QStyle::CC_ComboBox, &opt, pos);
    _hover_rect = (_hover_control != QStyle::SC_None) ? style()->subControlRect(QStyle::CC_ComboBox, &opt, _hover_control) : QRect();
    return _hover_control;
}

void RequestSelector::updateViewCurrent()
{
    if (isPopup() && _id.isValid() && !view()->currentIndex().isValid()) {
        int row = _data->findUid(_id);
        if (row >= 0) {
            view()->scrollTo(_data->index(row, 0), QAbstractItemView::ScrollHint::PositionAtCenter);
            view()->setCurrentIndex(_data->index(row, 0));
        }
    }
}

void RequestSelector::clearHelper(const QString& text, bool by_user)
{
    _data->setInternalData({});
    setCurrentID(Uid());
    setEditorTextHelper(text);

    if (by_user) {
        emit sg_textEdited("");
        emit sg_edited(_id);
    }
}

void RequestSelector::updateInternalDataFinish()
{
    if (isPopup())
        updatePopupGeometry();

    hideSearchStatus();

    emit sg_searchFinished(_user_search_internal_data_update, false);
    emit sg_dataReceived(_user_search_internal_data_update);

    updateViewCurrent();
}

void RequestSelector::objectExtensionDestroy()
{
    _object_extension->objectExtensionDestroy();
}

QVariant RequestSelector::objectExtensionGetData(const QString& data_key) const
{
    return _object_extension->objectExtensionGetData(data_key);
}

bool RequestSelector::objectExtensionSetData(const QString& data_key, const QVariant& value, bool replace) const
{
    return _object_extension->objectExtensionSetData(data_key, value, replace);
}

void RequestSelector::objectExtensionRegisterUseExternal(I_ObjectExtension* i)
{
    _object_extension->objectExtensionRegisterUseExternal(i);
}

void RequestSelector::objectExtensionUnregisterUseExternal(I_ObjectExtension* i)
{
    _object_extension->objectExtensionUnregisterUseExternal(i);
}

void RequestSelector::objectExtensionDeleteInfoExternal(I_ObjectExtension* i)
{
    _object_extension->objectExtensionDeleteInfoExternal(i);
}

void RequestSelector::objectExtensionRegisterUseInternal(I_ObjectExtension* i)
{
    _object_extension->objectExtensionRegisterUseInternal(i);
}

void RequestSelector::objectExtensionUnregisterUseInternal(I_ObjectExtension* i)
{
    _object_extension->objectExtensionUnregisterUseInternal(i);
}

RequestSelectorContainer::RequestSelectorContainer(RequestSelector* selector, RequestSelectorItemModel* model)
    : QFrame(selector, Qt::Popup)
    , _selector(selector)
{
    setFrameShape(StyledPanel);
    setAttribute(Qt::WA_WindowPropagation);

    QBoxLayout* layout = new QBoxLayout(QBoxLayout::TopToBottom, this);
    layout->setObjectName("zfla");
    layout->setSpacing(0);
    layout->setMargin(0);

    _list_view = new RequestSelectorListView(selector, this);
    _list_view->setModel(model);
    _list_view->setObjectName(QStringLiteral("zf_list"));
    _list_view->setItemDelegate(new RequestSelectorDelegate(_list_view, selector));
    layout->addWidget(_list_view);

    _list_view->installEventFilter(selector);
}

RequestSelectorContainer::~RequestSelectorContainer()
{
}

void RequestSelectorContainer::paintEvent(QPaintEvent* e)
{
    QFrame::paintEvent(e);
}

void RequestSelectorContainer::hideEvent(QHideEvent* e)
{
    QFrame::hideEvent(e);
}

RequestSelectorListView::RequestSelectorListView(RequestSelector* selector, QWidget* parent)
    : QListView(parent)
    , _selector(selector)
{
    setFrameShape(QFrame::NoFrame);
    setEditTriggers(NoEditTriggers);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setSelectionBehavior(QAbstractItemView::SelectRows);
    setSelectionMode(QAbstractItemView::SingleSelection);
    setFocusPolicy(Qt::NoFocus);
}

void RequestSelectorListView::keyPressEvent(QKeyEvent* event)
{
    QListView::keyPressEvent(event);
}

void RequestSelectorListView::mousePressEvent(QMouseEvent* event)
{
    QListView::mousePressEvent(event);

    if (indexAt(event->pos()).isValid())
        _selector->popupHide(true);
}

void RequestSelectorListView::resizeEvent(QResizeEvent* event)
{
    resizeContents(viewport()->width(), contentsSize().height());
    QListView::resizeEvent(event);
}

void RequestSelectorListView::hideEvent(QHideEvent* event)
{
    QListView::hideEvent(event);
    _selector->popupHide(false);
}

QStyleOptionViewItem RequestSelectorListView::viewOptions() const
{
    QStyleOptionViewItem option = QListView::viewOptions();
    option.showDecorationSelected = true;
    option.font = parentWidget()->font();
    return option;
}

QSize RequestSelectorListView::sizeHint() const
{
    return {};
}

QSize RequestSelectorListView::minimumSizeHint() const
{
    return sizeHint();
}

RequestSelectorDelegate::RequestSelectorDelegate(QAbstractItemView* view, RequestSelector* selector)
    : QStyledItemDelegate(view)
    , _view(view)
    , _selector(selector)
{
}

void RequestSelectorDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);
    QStyle* style = opt.widget ? opt.widget->style() : QApplication::style();

    opt.state = opt.state & ~QStyle::State_MouseOver;

    if (_view->currentIndex() == index) {
        opt.state = QStyle::State_Enabled | QStyle::State_Selected | QStyle::State_Active | QStyle::State_Window;

        opt.backgroundBrush = opt.palette.brush(QPalette::Active, QPalette::Highlight);
        opt.palette.setBrush(QPalette::Text, opt.palette.brush(QPalette::Active, QPalette::HighlightedText));
        opt.palette.setBrush(QPalette::Window, opt.palette.brush(QPalette::Active, QPalette::Highlight));
    }

    painter->save();
    style->drawPrimitive(QStyle::PE_PanelItemViewItem, &opt, painter, opt.widget);
    painter->restore();

    painter->save();
    style->drawControl(QStyle::CE_ItemViewItem, &opt, painter, opt.widget);
    painter->restore();
}

RequestSelectorEditor::RequestSelectorEditor(RequestSelector* selector)
    : QLineEdit(selector)
    , _selector(selector)
    , _focus_timer(new QTimer(this))
{
    setFrame(false);
    _focus_timer->setSingleShot(true);
    _focus_timer->setInterval(0);
    connect(_focus_timer, &QTimer::timeout, this, [&]() {
        if (hasFocus())
            Utils::selectLineEditAll(this);
    });
}

bool RequestSelectorEditor::event(QEvent* event)
{
    if (_selector->_delegate == nullptr) {
        switch (event->type()) {
            case QEvent::KeyPress:
            case QEvent::Clipboard:
            case QEvent::MouseButtonPress:
            case QEvent::InputMethodQuery:
            case QEvent::ContextMenu:
                return true;

            default:
                break;
        }
    }

    return QLineEdit::event(event);
}
void RequestSelectorEditor::focusInEvent(QFocusEvent* event)
{
    QLineEdit::focusInEvent(event);

    _selector->update();
    // был установле focusProxy. При этом главный виджет почему не получает сообщения о смене фокуса
    QApplication::sendEvent(_selector, event);

    if (_selector->_select_all_on_focus && !isReadOnly())
        _focus_timer->start();
}

void RequestSelectorEditor::focusOutEvent(QFocusEvent* event)
{
    QLineEdit::focusOutEvent(event);
    _selector->update();
    // был установле focusProxy. При этом главный виджет почему не получает сообщения о смене фокуса
    QApplication::sendEvent(_selector, event);

    if (!isReadOnly()) {
        deselect();
        setCursorPosition(0);
    }
}

void RequestSelectorEditor::initStyleOption(QStyleOptionFrame* option) const
{
    QLineEdit::initStyleOption(option);
}

void RequestSelectorEditor::highlight(bool b)
{
    _highlighted = b;
    Utils::updatePalette(_selector->isEditable(), b, this);
}

RequestSelectorItemModel::RequestSelectorItemModel(RequestSelector* s)
    : QAbstractItemModel(s)
    , _selector(s)
{
    _watcher_key_row.connect(&_watcher_key_row, &QFutureWatcher<int>::finished, &_watcher_key_row, [&]() {
        _in_update_data = false;
        endResetModel();
        emit sg_onEndUpdateData(_watcher_key_row.result());
    });
}

bool RequestSelectorItemModel::setInternalData(const QList<QPair<Uid, QVariant>>& msg_data)
{
    if (!_in_update_data && msg_data.isEmpty()) {
        emit sg_onBeginUpdateData();

        beginResetModel();
        _mutex.lock();
        _data_hash.clear();
        _string_hash.clear();
        _data.clear();
        _mutex.unlock();
        endResetModel();

        emit sg_onEndUpdateData(-1);
        return false;
    }

    if (_in_update_data) {
        endResetModel();
        emit sg_onEndUpdateData(-1);

    } else {
        _in_update_data = true;
    }

    emit sg_onBeginUpdateData();

    beginResetModel();

    QFuture<int> future = QtConcurrent::run([this, msg_data]() -> int {
        _mutex.lock();
        _data_hash.clear();
        _string_hash.clear();
        _data.clear();

        int keys_row = -1;
        for (int row = 0; row < msg_data.count(); row++) {
            auto row_data = Z_MAKE_SHARED(RequestSelectorItemModel::RowData);
            row_data->row = row;
            row_data->uid = msg_data.at(row).first;
            row_data->data = msg_data.at(row).second;
            row_data->name = _selector->_delegate->extractText(msg_data.at(row).second, _selector->_request_options);

            if (_selector->_id == row_data->uid)
                keys_row = row;

            _data << row_data;
        }
        _mutex.unlock();
        return keys_row;
    });
    _watcher_key_row.setFuture(future);

    return true;
}

int RequestSelectorItemModel::findUid(const Uid& uid) const
{
    if (!uid.isValid())
        return -1;

    QMutexLocker lock(&_mutex);

    if (_data.count() == 0)
        return -1;

    if (!_data_hash.isEmpty()) {
        auto res = _data_hash.constFind(uid);
        if (res == _data_hash.constEnd())
            return -1;
        return res.value()->row;
    }

    RowDataPtr found;
    for (int i = 0; i < _data.count(); i++) {
        auto row_data = _data.at(i);
        if (row_data == nullptr)
            continue;

        Z_CHECK(row_data->uid.isValid() && row_data->row >= 0 && row_data->row < _data.count());

        _data_hash[row_data->uid] = row_data;
        if (found == nullptr && row_data->uid == uid)
            found = row_data;
    }

    return found == nullptr ? -1 : found->row;
}

int RequestSelectorItemModel::findString(const QString s) const
{
    QMutexLocker lock(&_mutex);

    QString prepared = Utils::prepareSearchString(s, true);
    if (_data.count() == 0)
        return -1;

    if (!_string_hash.isEmpty()) {
        auto res = _string_hash.constFind(prepared);
        if (res == _string_hash.constEnd())
            return -1;
        return res.value()->row;
    }

    RowDataPtr found;
    for (int i = 0; i < _data.count(); i++) {
        auto row_data = _data.at(i);
        if (row_data == nullptr)
            continue;

        Z_CHECK(row_data->uid.isValid() && row_data->row >= 0 && row_data->row < _data.count());

        QString row_name = row_data->name.toLower();
        if (_string_hash.contains(row_name))
            continue; // повторы игнорируем

        _string_hash[row_name] = row_data;
        if (found == nullptr && row_name == prepared)
            found = row_data;
    }

    return found == nullptr ? -1 : found->row;
}

QString RequestSelectorItemModel::rowName(int row) const
{
    QMutexLocker lock(&_mutex);
    Z_CHECK(row >= 0 && row < _data.count());
    return _data.at(row) == nullptr ? QString() : _data.at(row)->name;
}

Uid RequestSelectorItemModel::rowUid(int row) const
{
    QMutexLocker lock(&_mutex);
    Z_CHECK(row >= 0 && row < _data.count());
    return _data.at(row) == nullptr ? Uid() : _data.at(row)->uid;
}

QModelIndex RequestSelectorItemModel::index(int row, int column, const QModelIndex& parent) const
{
    Z_CHECK(column == 0);

    if (!hasIndex(row, column, parent))
        return QModelIndex();

    return createIndex(row, column, reinterpret_cast<void*>(quintptr(row)));
}

QModelIndex RequestSelectorItemModel::parent(const QModelIndex&) const
{
    return QModelIndex();
}

int RequestSelectorItemModel::rowCount(const QModelIndex& parent) const
{
    QMutexLocker lock(&_mutex);
    return parent.isValid() ? 0 : _data.count();
}

int RequestSelectorItemModel::columnCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : 1;
}

QVariant RequestSelectorItemModel::data(const QModelIndex& index, int role) const
{
    QMutexLocker lock(&_mutex);

    if ((role != Qt::DisplayRole && role != Qt::EditRole) || !index.isValid() || index.parent().isValid() || index.row() >= _data.count())
        return QVariant();

    auto v = _data.at(index.row());
    return v == nullptr ? QVariant() : v->name;
}

bool RequestSelectorItemModel::setData(const QModelIndex&, const QVariant&, int)
{
    Z_HALT("not supported");
    return false;
}

} // namespace zf
