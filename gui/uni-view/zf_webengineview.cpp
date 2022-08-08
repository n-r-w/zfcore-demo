#include "zf_webengineview.h"
#include <QMenu>
#include <QContextMenuEvent>
#include <QWebChannel>

#include "zf_core.h"
#include "zf_translation.h"

namespace zf
{
std::unique_ptr<QString> WebEngineView::_web_channel_js;

WebEngineView::WebEngineView(WebEngineViewInjection* injection, QWidget* parent)
    : QWebEngineView(parent)
    , _injection(injection)
{
    if (_injection == nullptr) {
        _injection_private = new WebEngineViewInjection;
        _injection = _injection_private;
    }

    setContextMenuPolicy(Qt::NoContextMenu);

    if (_web_channel_js == nullptr) {
        // грузим qwebchannel.js
        QFile js(":/qtwebchannel/qwebchannel.js");
        Z_CHECK(js.open(QFile::ReadOnly));
        _web_channel_js = std::make_unique<QString>(QString::fromLatin1(js.readAll()));
        js.close();

        // грузим скрипт с injection
        js.setFileName(":/core/gui/uni-view/web_channel_injection.js");
        Z_CHECK(js.open(QFile::ReadOnly));
        _web_channel_js->append(QString::fromLatin1(js.readAll()));
        js.close();
    }

    _channel = new QWebChannel(page());
    _injection_object = new WebEngineViewInjectionObject(_injection, page());
    _channel->registerObject(QStringLiteral("injection_object"), _injection_object);
    page()->setWebChannel(_channel);

    connect(_injection, &WebEngineViewInjection::sg_optionsChanged, this, [&]() {
        if (!_injection_object.isNull())
            emit _injection_object->optionsChanged();
    });
    connect(_injection, &WebEngineViewInjection::sg_webKeyPressed, this, &WebEngineView::sg_webKeyPressed);
    connect(_injection, &WebEngineViewInjection::sg_webMouseClicked, this, &WebEngineView::sg_webMouseClicked);
    connect(_injection, &WebEngineViewInjection::sg_currentPageChanged, this, &WebEngineView::sg_currentPageChanged);

    connect(this, &QWebEngineView::loadStarted, this, &WebEngineView::sl_load_started);
    connect(this, &QWebEngineView::loadFinished, this, &WebEngineView::sl_load_finished);
}

WebEngineView::~WebEngineView()
{
    if (_injection_private != nullptr)
        delete _injection_private;
}

Error WebEngineView::setFittingType(FittingType type)
{
    Z_CHECK_NULL(_injection_object);
    if (_injection_object->isPdfView()) {
        emit _injection_object->setFittingType(static_cast<int>(type));
        return {};
    }
    return Error::unsupportedError();
}

Error WebEngineView::zoomIn()
{
    Z_CHECK_NULL(_injection_object);
    if (_injection_object->isPdfView()) {
        emit _injection_object->zoomIn();
        return {};
    }
    return Error::unsupportedError();
}

Error WebEngineView::zoomOut()
{
    Z_CHECK_NULL(_injection_object);
    if (_injection_object->isPdfView()) {
        emit _injection_object->zoomOut();
        return {};
    }
    return Error::unsupportedError();
}

Error WebEngineView::rotateRight()
{
    Z_CHECK_NULL(_injection_object);
    if (_injection_object->isPdfView()) {
        emit _injection_object->rotateRight();
        return {};
    }
    return Error::unsupportedError();
}

Error WebEngineView::rotateLeft()
{
    Z_CHECK_NULL(_injection_object);
    if (_injection_object->isPdfView()) {
        emit _injection_object->rotateLeft();
        return {};
    }
    return Error::unsupportedError();
}

Error WebEngineView::setCurrentPage(int page)
{
    Z_CHECK_NULL(_injection_object);
    if (_injection_object->isPdfView()) {
        emit _injection_object->setCurrentPage(page);
        return {};
    }
    return Error::unsupportedError();
}

WebEngineViewInjection* WebEngineView::injection() const
{
    return _injection;
}

void WebEngineView::contextMenuEvent(QContextMenuEvent* event)
{
    QMenu* menu = page()->createStandardContextMenu();

    // добавляем операции копирования, если они не существуют
    const QList<QAction*> actions = menu->actions();

    auto has_action = std::find(actions.cbegin(), actions.cend(), page()->action(QWebEnginePage::Copy));
    if (has_action == actions.cend()) {
        QAction* action = menu->addAction(ZF_TR(ZFT_COPY));
        action->setEnabled(hasSelection());
        connect(action, &QAction::triggered, this, [this]() { triggerPageAction(QWebEnginePage::Copy); });
    }
    has_action = std::find(actions.cbegin(), actions.cend(), page()->action(QWebEnginePage::SelectAll));
    if (has_action == actions.cend()) {
        QAction* action = menu->addAction(ZF_TR(ZFT_SELECT_ALL));
        connect(action, &QAction::triggered, this, [this]() {
            // Выполнение QWebEnginePage::SelectAll - не работает
            // Передача комбинации клавиш в QWebEnginePage тоже не работает, поэтому такой грязный хак
            QObjectList objects;
            Utils::findObjectsByClass(
                objects, page()->view(), "QtWebEngineCore::RenderWidgetHostViewQtDelegateWidget", false, false);
            if (objects.isEmpty()) {
                Core::systemError("RenderWidgetHostViewQtDelegateWidget not found");
            } else {
                QKeyEvent event(QEvent::KeyPress, Qt::Key_A, Qt::ControlModifier);
                QCoreApplication::sendEvent(objects.first(), &event);
            };
        });
    }

    menu->popup(event->globalPos());
}

void WebEngineView::sl_load_started()
{
    emit sg_load_started();
}

void WebEngineView::sl_load_finished(bool ok)
{
    if (_injection != nullptr && ok) {
        Z_CHECK_NULL(_web_channel_js);
        page()->runJavaScript(*_web_channel_js, [this](const QVariant& result) {
            Q_UNUSED(result)
            _injection->setHidePdfToolbar(true);
            _injection->setHidePdfZoom(true);
        });
    }

    emit sg_load_finished(ok);
}

WebEngineViewInjectionObject::WebEngineViewInjectionObject(WebEngineViewInjection* injection, QObject* parent)
    : QObject(parent)
    , _injection(injection)
{
    Z_CHECK_NULL(_injection);
}

bool WebEngineViewInjectionObject::isPdfView() const
{
    return _injection->isPdfView();
}

bool WebEngineViewInjectionObject::isHidePdfToolbar() const
{
    return _injection->isHidePdfToolbar();
}

bool WebEngineViewInjectionObject::isHidePdfZoom() const
{
    return _injection->isHidePdfZoom();
}

void WebEngineViewInjectionObject::onKeyPressed(const QString& target_id, const QString& key)
{
    _injection->onWebKeyPressed_helper(target_id, key);
}

void WebEngineViewInjectionObject::onMouseClicked(const QString& target_id, bool altKey, bool ctrlKey, bool shiftKey, int button)
{
    Qt::KeyboardModifiers modifiers = Qt::NoModifier;
    Qt::MouseButton m_button = Qt::NoButton;

    if (altKey)
        modifiers |= Qt::AltModifier;
    if (ctrlKey)
        modifiers |= Qt::ControlModifier;
    if (shiftKey)
        modifiers |= Qt::ShiftModifier;

    if (button == 0)
        m_button = Qt::LeftButton;
    else if (button == 1)
        m_button = Qt::MiddleButton;
    else if (button == 2)
        m_button = Qt::RightButton;
    else if (button == 3)
        m_button = Qt::BackButton;
    else if (button == 4)
        m_button = Qt::ForwardButton;
    else
        Core::error(QStringLiteral("WebEngineViewInjection::onMouseClicked - unknown button code: %1").arg(button));

    _injection->onWebMouseClicked_helper(target_id, modifiers, m_button);
}

void WebEngineViewInjectionObject::onCurrentPageChanged(int current, int maximum)
{
    _injection->onCurrentPageChanged_helper(current, maximum);
}

} // namespace zf
