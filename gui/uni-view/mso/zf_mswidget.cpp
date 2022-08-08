#ifdef UNICODE
#undef UNICODE
#endif
#ifdef _UNICODE
#undef _UNICODE
#endif

#include "zf_mswidget.h"
#include "MSOControl.h"

#include <QApplication>
#include <QDebug>
#include <QDir>
#include <QEvent>
#include <QResizeEvent>
#include <QWindow>

namespace zf
{
static MSOControl* initControl(HWND hwnd, bool edit, const QString& file_name, int type)
{
    MSOControl* c = new MSOControl(hwnd, type);
    if (!c->IsValid()) {
        delete c;
        return nullptr;
    }

    if (edit) {
        c->setLightMSO(false);
        c->loadFromFile(QDir::toNativeSeparators(file_name), false);
    } else {
        c->setLightMSO(true);
        c->loadFromFile(QDir::toNativeSeparators(file_name), true);
        c->setReadOnly(true);
    }

    return c;
}

void MSWidget::initEngine()
{
    init_mso_utilities();
}

void MSWidget::clearEngine()
{
    clear_mso_utilities();
}

MSWidget::MSWidget(QWidget* parent)
    : QWidget(parent)
{
    initEngine();

    qApp->installEventFilter(this);
}

MSWidget::~MSWidget()
{
    clear();
}

Error MSWidget::loadWord(const QString& file_name, bool edit)
{
    if (!QFile::exists(file_name))
        return Error::fileNotFoundError(file_name);

    _read_only = !edit;
    clear();
    _container = initControl(reinterpret_cast<HWND>(winId()), edit, file_name, VIEWFORM_TYPE_MSWORD);
    return _container != nullptr ? Error() : Error::badFileError(file_name);
}

Error MSWidget::loadExcel(const QString& file_name, bool edit)
{
    if (!QFile::exists(file_name))
        return Error::fileNotFoundError(file_name);

    _read_only = !edit;
    clear();
    _container = initControl(reinterpret_cast<HWND>(winId()), edit, file_name, VIEWFORM_TYPE_MSEXCEL);
    return _container != nullptr ? Error() : Error::badFileError(file_name);
}

void MSWidget::clear()
{
    if (_container != nullptr) {
        _container->PrepareToDestroy();
        delete _container;
    }
    _container = nullptr;
}

bool MSWidget::isEdit() const
{
    return _container == nullptr ? _read_only : !_container->IsReadOnly();
}

MSWidget::ViewType MSWidget::viewType() const
{
    if (_container == nullptr)
        return ViewType::Undefined;

    int vt = _container->getViewType();
    if (vt == WORDOLE_PRINT_VIEW)
        return ViewType::PrintView;
    if (vt == WORDOLE_WEB_VIEW)
        return ViewType::WebView;
    if (vt == WORDOLE_PRINT_PREVIEW)
        return ViewType::PrintPreview;

    return ViewType::Undefined;
}

bool MSWidget::setViewType(MSWidget::ViewType t)
{
    if (_container == nullptr || t == ViewType::Undefined)
        return false;

    if (t == ViewType::PrintView)
        _container->setViewType(WORDOLE_PRINT_VIEW);
    else if (t == ViewType::WebView)
        _container->setViewType(WORDOLE_WEB_VIEW);
    else if (t == ViewType::PrintPreview)
        _container->setViewType(WORDOLE_PRINT_PREVIEW);
    else
        return false;

    return true;
}

bool MSWidget::print()
{
    return _container == nullptr ? false : _container->print();
}

bool MSWidget::isDirty() const
{
    return _container == nullptr ? false : _container->isDirty();
}

Error MSWidget::saveAs(const QString& file_name)
{
    if (_container == nullptr)
        return Error("MSWidget: not initialized");

    if (!QFile::exists(file_name))
        return Error::fileNotFoundError(file_name);

    if (_container->saveAs(QDir::toNativeSeparators(file_name)))
        return Error();

    return Error::fileIOError(file_name);
}

QSize MSWidget::sizeHint() const
{
    return QSize(400, 400);
}

QSize MSWidget::minimumSizeHint() const
{
    return QSize(400, 400);
}

bool MSWidget::nativeEvent(const QByteArray& eventType, void* message, long* result)
{
    bool res = QWidget::nativeEvent(eventType, message, result);

    MSG* msg = static_cast<MSG*>(message);
    if (msg->message == WM_MOUSEACTIVATE)
        setFocus();

    return res;
}

bool MSWidget::event(QEvent* event)
{
    return QWidget::event(event);
}

bool MSWidget::eventFilter(QObject* watched, QEvent* event)
{    
    if (_container != nullptr && event->type() == QEvent::WindowActivate && !QApplication::topLevelWidgets().isEmpty()
        && QApplication::topLevelWidgets().first() == watched) {
        _container->onActivateApp();
    }

    return QWidget::eventFilter(watched, event);
}

void MSWidget::hideEvent(QHideEvent* e)
{
    clear();
    QWidget::hideEvent(e);
}

void MSWidget::changeEvent(QEvent* e)
{
    if (_container == nullptr)
        return;

    switch (e->type()) {
        case QEvent::EnabledChange:
            if (!_read_only)
                _container->setReadOnly(!isEnabled());
            break;

        default:
            break;
    }
}

void MSWidget::resizeEvent(QResizeEvent* e)
{
    QWidget::resizeEvent(e);

    if (_container != nullptr)
        _container->resize(e->size());
}

void MSWidget::paintEvent(QPaintEvent* e)
{
    Q_UNUSED(e);
}

void MSWidget::showEvent(QShowEvent* e)
{
    QWidget::showEvent(e);

    if (_container != nullptr)
        _container->resize(size());
}

void MSWidget::focusInEvent(QFocusEvent* e)
{
    QWidget::focusInEvent(e);

    if (_container != nullptr)
        _container->onFocus(true);
}

void MSWidget::focusOutEvent(QFocusEvent* e)
{
    QWidget::focusOutEvent(e);

    if (_container != nullptr)
        _container->onFocus(false);
}

QPaintEngine* MSWidget::paintEngine() const
{
    return nullptr;
}

} // namespace zf
