#include <QAbstractItemModel>
#include <QApplication>
#include <QDynamicPropertyChangeEvent>
#include <QCryptographicHash>
#include <QDate>
#include <QFormLayout>
#include <QIcon>
#include <QLayout>
#include <QMainWindow>
#include <QScrollArea>
#include <QStyleFactory>
#include <QSplitter>
#include <QStylePainter>
#include <QTextCodec>
#include <QToolBox>
#include <QUuid>
#include <QtConcurrent>
#include <QAbstractSocket>
#include <QUiLoader>
#include <QChart>
#include <QGraphicsScene>
#include <QPdfWriter>
#include <QSvgGenerator>
#include <QMovie>
#include <QDir>
#include <QEventLoop>
#include <QSet>
#include <QtMath>
#include <QCryptographicHash>
#include <QLocale>
#include <QMutex>
#include <QSplitter>
#include <QTabWidget>
#include <QApplication>
#include <QElapsedTimer>
#include <QMdiSubWindow>
#include <QProxyStyle>
#include <QGridLayout>
#include <QStackedWidget>
#include <QLayoutItem>
#include <QDesktopServices>
#include <QNetworkProxyFactory>
#include <QNetworkProxy>
#include <qpa/qplatformscreen.h>
#include <qpa/qplatformscreen_p.h>

#if QT_VERSION < QT_VERSION_CHECK(5, 10, 0)
#include <QDesktopWidget>
#else
#include <QScreen>
#endif

#include "zf_callback.h"
#include "zf_core.h"
#include "zf_exception.h"
#include "zf_framework.h"
#include "zf_header_config_dialog.h"
#include "zf_header_view.h"
#include "zf_html_tools.h"
#include "zf_itemview_header_item.h"
#include "zf_progress_object.h"
#include "zf_translation.h"
#include "zf_utils.h"
#include "zf_core_messages.h"
#include "zf_dbg_break.h"
#include "zf_picture_selector.h"
#include "zf_conditions_dialog.h"
#include "zf_console_utils.h"
#include "zf_numeric.h"
#include <texthyphenationformatter.h>
#include "private/zf_item_view_p.h"
#include "zf_image_list.h"
#include "zf_request_selector.h"
#include "zf_shared_ptr_deleter.h"
#include "zf_cumulative_error.h"
#include "zf_graphics_effects.h"
#include "zf_highlight_mapping.h"
#include "zf_colors_def.h"
#include "NcFramelessHelper.h"
#include "zf_svg_icons_cache.h"
#include "zf_ui_size.h"
#include "../../../client_version.h"

#include <cstdlib>

#ifdef Q_OS_LINUX
#include <unistd.h>
#include <sys/types.h>
#endif

#ifdef Q_OS_WIN
#include <Windows.h>
// сохранять порядок
#include <Psapi.h>
#include <QWinTaskbarProgress>
#include "mso/MSOControl.h"

#ifdef _MSC_VER
#define PATH_MAX MAX_PATH
#endif

#ifdef __MINGW32__
#include <limits>
#endif

#undef MessageBox

#else
#include <linux/limits.h>
#include <unistd.h>
#endif

#include <QPdfDocument>
#include <QPdfDocumentRenderOptions>

namespace zf
{
// Приложение было остановлено
bool Utils::_is_app_halted = false;
//! Включить/отключить остановку приложения при критической ошибке.
//! Если отключено, то будет генерироваться исключение
int Utils::_fatal_halt_block_counter = 0;
//! Базовый UUID для генерации V5
const QUuid Utils::_baseUuid = QUuid("{87e42293-018e-4753-ba13-698c3123f89a}");
//! "соль" для генерации GUID
const QString Utils::_guid_salt = "k8B3";

int Utils::_process_events_counter = 0;
//! Контроль частоты вызова QApplication::processEvents()
QElapsedTimer Utils::_time_process_events_counter;

QAtomicInt Utils::_disable_process_events_pause_counter { 0 };

//! Путь к журналам работы
QString Utils::_log_folder;

QMutex DebugMemoryLeak::_mutex;
std::unique_ptr<QHash<QString, qint64>> DebugMemoryLeak::_counter;

QMutex Utils::_location_mutex;
std::unique_ptr<QHash<QString, QString>> Utils::_location_created;

//! Список классов, которые являются контейнерами для других виджетов
const QStringList Utils::_container_widget_classes
    = {"QWidget", "QTabWidget", "QTabBar", "CollapsibleGroupBox", "QScrollArea", "QStackedWidget", "QToolBox", "QSplitter"};

//! Пользовательские варианты разделителей
const QList<QChar> Utils::USER_SEPARATORS = {',', ';', '|', '/', '\\'};

//! Кэш SVG
SvgIconCache Utils::_svg_cache(1000);

long double roundReal(long double v, RoundOption options)
{
    switch (options) {
        case RoundOption::Up:
            return std::ceil(v);

        case RoundOption::Down:
            return std::floor(v);

        case RoundOption::Percent: {
            if (v > 0 && v < 1)
                return 1;
            else if (v > 99 && v < 100)
                return 99;
            else
                return std::round(v);
        }
        default:
            return std::round(v);
    }
}

QString doubleToString(qreal v, int precision, const QLocale* locale, RoundOption options)
{
    if (qFuzzyIsNull(v))
        return QStringLiteral("0");

    if (locale == nullptr)
        locale = Core::locale(LocaleType::UserInterface);

    QString s = locale->toString(roundPrecision(v, precision, options), 'f', precision);
    int i = s.length() - 1;
    for (; i > 0; i--) {
        if (s.at(i) != '0')
            break;
    }
    return s.at(i) == '.' ? s.left(i) : s.left(i + 1);
}

void Utils::blockProcessEvents()
{
    _process_events_counter++;
}

void Utils::unBlockProcessEvents()
{
    if (_process_events_counter == 0) {
        Z_HALT("unBlockProcessEvents - counter error");
    }

    _process_events_counter--;
}

bool Utils::isProcessingEvent()
{
    return _process_events_counter > 0;
}

void Utils::processEvents(QEventLoop::ProcessEventsFlags flags)
{
    CumulativeError::block();
    blockProcessEvents();

    if (_process_events_counter == 1) {
        if (isProcessEventsInterval()) {
            MessagePauseHelper ph;
            ph.pause();

            QApplication::processEvents(flags);
        }
    } else {
        _time_process_events_counter.invalidate();
    }

    unBlockProcessEvents();
    CumulativeError::unblock();
}

void Utils::disableProcessEventsPause()
{
    _disable_process_events_pause_counter++;
}

void Utils::enableProcessEventsPause()
{
    Z_CHECK(--_disable_process_events_pause_counter >= 0);
}

bool Utils::isProcessEventsInterval()
{
    bool allow = !_time_process_events_counter.isValid() || _time_process_events_counter.elapsed() > Framework::PROCESS_EVENTS_TIMEOUT_MS;
    if (allow)
        _time_process_events_counter.start();

    return allow;
}

static void delay_helper(int ms)
{
    if (ms <= 0)
        return;

#ifdef Q_OS_WIN
    Sleep(uint(ms));
#else
    struct timespec ts = {ms / 1000, (ms % 1000) * 1000 * 1000};
    nanosleep(&ts, nullptr);
#endif
}

void Utils::delay(int ms, QEventLoop::ProcessEventsFlags flags, bool use_event_loop)
{
    Z_CHECK(ms >= 0);
    if (ms == 0)
        return;

    if (use_event_loop) {
        QEventLoop wait_loop;
        QTimer::singleShot(ms, &wait_loop, [&wait_loop]() { wait_loop.quit(); });
        wait_loop.exec(flags);
        return;
    }

#ifdef Q_PROCESSOR_X86_64
    if (ms < 500) {
        delay_helper(ms);

    } else {
        QFuture<void> future = QtConcurrent::run(delay_helper, ms);
        while (future.isRunning()) {
            QThread::currentThread()->msleep(10);
            qApp->processEvents(flags);
        }
    }
#else
    Q_UNUSED(flags)
    delay_helper(ms);
#endif
}

bool Utils::isMainThread(QObject* object)
{
    if (QApplication::instance() == nullptr)
        return true;

    return (object == nullptr ? QThread::currentThread() : object->thread()) == QApplication::instance()->thread();
}

QString Utils::systemUserName()
{
#ifdef Q_OS_WIN
    QString s = QString::fromLocal8Bit(qgetenv("USER"));
    if (s.isEmpty())
        s = QString::fromLocal8Bit(qgetenv("USERNAME"));
    return s;
#else
    return qgetenv("USER").constData();
#endif
}

double Utils::screenScaleFactor(const QScreen* screen)
{
    QPlatformScreen* ps = (screen != nullptr ? screen : QApplication::primaryScreen())->handle();
    return ps->logicalDpi().first / ps->logicalBaseDpi().first;
}

// черная магия
// https://stackoverflow.com/questions/16960166/issue-with-qt-thread-affinity-and-movetothread
/*template <typename Func> inline QTimer* runOnThread(QThread* thread, Func&& func)
{
    QTimer* timer = new QTimer();
    timer->moveToThread(thread);
    Z_CHECK(timer->thread() == thread);
    timer->setSingleShot(true);
    timer->setInterval(0);
    QObject::connect(timer, &QTimer::timeout, [=]() {
        func();
        timer->deleteLater();
    });
    QMetaObject::invokeMethod(timer, "start", Qt::QueuedConnection);
    return timer;
}

void Utils::moveToThread(QObject* object_to_move, QThread* target_thread, int wait_msecs)
{
    Z_CHECK_NULL(object_to_move);
    Z_CHECK_NULL(target_thread);
    Z_CHECK(!isMainThread());
    Z_CHECK(object_to_move->thread() == qApp->instance()->thread());

    QMutex mutex_wait;
    mutex_wait.lock();
    QWaitCondition condition_wait;
    // ждем срабатывания таймера чтобы вызов лямбда фукции был в главном потоке и сработало перемещение в другой поток
    runOnThread(object_to_move->thread(), [&] {
        object_to_move->setParent(nullptr);
        object_to_move->moveToThread(target_thread);
        condition_wait.wakeOne();
    });

    condition_wait.wait(&mutex_wait);
    mutex_wait.unlock();
}*/

bool Utils::disconnectSocket(QAbstractSocket* socket, int wait_msecs)
{
    Z_CHECK_NULL(socket);
    Z_CHECK(wait_msecs > 0);

    if (socket->state() == QAbstractSocket::UnconnectedState) {
        socket->abort();
        return true;
    }

    socket->waitForDisconnected(wait_msecs);

    if (socket->state() == QAbstractSocket::UnconnectedState) {
        socket->abort();
        return true;
    }

#ifdef Q_OS_WINDOWS
    QEventLoop* loop = new QEventLoop;
    QTimer* timer = new QTimer;
    bool* disconnected = new bool(false);
    timer->setSingleShot(true);
    timer->setInterval(wait_msecs);
    timer->start();

    QObject* tmp_object = new QObject;

    QObject::connect(
        socket, &QAbstractSocket::disconnected, tmp_object,
        [loop, disconnected]() {
            *disconnected = true;
            loop->quit();
        },
        Qt::DirectConnection);
    QObject::connect(
        timer, &QTimer::timeout, tmp_object, [loop]() { loop->quit(); }, Qt::DirectConnection);

    loop->exec();

    delete tmp_object;

    timer->stop();
    delete loop;
    delete timer;

    bool ok = *disconnected;
    delete disconnected;

    socket->abort();
    return ok;
#else
    return socket->waitForDisconnected(wait_msecs);
#endif
}

void Utils::deleteLater(QObject* obj)
{
    Z_CHECK_NULL(obj);

    if (I_ObjectExtension* o_ex = dynamic_cast<I_ObjectExtension*>(obj))
        o_ex->objectExtensionDestroy();
    else
        obj->deleteLater();

    if (QWidget* w = qobject_cast<QWidget*>(obj)) {
        w->setHidden(true);
        w->setParent(nullptr);
    }
}

void Utils::deleteLater(const std::shared_ptr<void>& obj)
{
    Z_CHECK_NULL(obj);
    Core::fr()->sharedPtrDeleter()->deleteLater(obj);
}

void Utils::safeDelete(QObject* obj)
{
    Z_CHECK_NULL(obj);

    QObjectList to_safe_destroy;
    safeDeleteHelper(obj, to_safe_destroy);

    for (auto& c : qAsConst(to_safe_destroy)) {
        I_ObjectExtension* o_ex = dynamic_cast<I_ObjectExtension*>(c);
        Z_CHECK_NULL(o_ex);

        o_ex->objectExtensionDestroy();

        // setParent(nullptr) важно задать для виджета не виджета отдельно (см. QObject::setParent)
        if (QWidget* w = qobject_cast<QWidget*>(obj)) {
            w->setHidden(true);
            w->setParent(nullptr);

        } else {
            obj->setParent(nullptr);
        }
    }

    if (!to_safe_destroy.contains(obj))
        delete obj;
}

void Utils::safeDeleteHelper(QObject* obj, QObjectList& to_safe_destroy)
{
    if (dynamic_cast<I_ObjectExtension*>(obj) != nullptr)
        to_safe_destroy << obj;

    for (auto& c : obj->children()) {
        safeDeleteHelper(c, to_safe_destroy);
    }
}

QString Utils::informationTypeText(InformationType type)
{
    if (type == InformationType::Invalid)
        return ZF_TR(ZFT_WRONG_DATA);
    else if (type == InformationType::Success)
        return ZF_TR(ZFT_SUCCESS);
    else if (type == InformationType::Error)
        return ZF_TR(ZFT_ERROR);
    else if (type == InformationType::Critical)
        return ZF_TR(ZFT_CRITICAL_ERROR);
    else if (type == InformationType::Fatal)
        return ZF_TR(ZFT_FATAL_ERROR);
    else if (type == InformationType::Warning)
        return ZF_TR(ZFT_WARNING);
    else if (type == InformationType::Information)
        return ZF_TR(ZFT_INFORMATION);
    else if (type == InformationType::Required)
        return ZF_TR(ZFT_REQUIRED);

    Z_HALT_INT;
    return QString();
}

QColor Utils::informationTypeColor(InformationType type)
{
    switch (type) {
        case InformationType::Information:
        case InformationType::Success:
            return QColor("#0B61A4");

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

InformationType Utils::getTopErrorLevel(const QList<InformationType>& types)
{
    Z_CHECK(!types.isEmpty());

    QList<InformationType> types_sorted = types;
    std::sort(
        types_sorted.begin(), types_sorted.end(), [](InformationType i1, InformationType i2) -> bool { return static_cast<int>(i1) < static_cast<int>(i2); });

    return types_sorted.constLast();
}

QIcon Utils::informationTypeIcon(InformationType type, int size)
{
    switch (type) {
        case InformationType::Information:
            return size <= 0 ? QIcon(QStringLiteral(":/share_icons/blue/info.svg")) : Utils::resizeSvg(QStringLiteral(":/share_icons/blue/info.svg"), size);
        case InformationType::Success:
            return size <= 0 ? QIcon(QStringLiteral(":/share_icons/green/check.svg")) : Utils::resizeSvg(QStringLiteral(":/share_icons/green/check.svg"), size);

        case InformationType::Warning:
            return size <= 0 ? QIcon(QStringLiteral(":/share_icons/warning.svg")) : Utils::resizeSvg(QStringLiteral(":/share_icons/warning.svg"), size);

        case InformationType::Error:
        case InformationType::Critical:
        case InformationType::Fatal:
        case InformationType::Required:
            return size <= 0 ? QIcon(QStringLiteral(":/share_icons/red/important.svg"))
                             : Utils::resizeSvg(QStringLiteral(":/share_icons/red/important.svg"), size);

        case InformationType::Invalid:
            break;
    }
    Z_HALT_INT;
    return QIcon();
}

QString Utils::messageTypeName(MessageType type)
{
    return qtEnumToString(type);
}

QLayout* Utils::findParentLayout(QWidget* w)
{
    return findParentLayoutHelper(w->parentWidget(), w);
}

QLayout* Utils::findTopMargins(QWidget* w)
{
    Z_CHECK_NULL(w);
    if (w->layout() == nullptr)
        return nullptr;

    return findTopMargins(w->layout());
}

QLayout* Utils::findTopMargins(QLayout* l)
{
    Z_CHECK_NULL(l);

    auto margins = l->contentsMargins();

    if (margins.left() > 0 || margins.right() > 0 || margins.top() > 0 || margins.bottom() > 0)
        return l;

    for (int i = 0; i < l->count(); i++) {
        auto item = l->itemAt(i);
        if (item == nullptr)
            continue;

        QLayout* res = nullptr;
        if (item->layout() != nullptr)
            res = findTopMargins(item->layout());
        else if (item->widget() != nullptr)
            res = findTopMargins(item->widget());

        if (res != nullptr)
            return res;
    }

    return nullptr;
}

QLayout* Utils::findParentLayoutHelper(QWidget* parent, QWidget* w)
{
    if (parent == nullptr)
        return nullptr;

    auto la_list = parent->findChildren<QLayout*>();
    for (auto la : la_list) {
        if (la->indexOf(w) >= 0)
            return la;
    }

    return findParentLayoutHelper(parent->parentWidget(), w);
}

void Utils::findObjectsByClass(QObjectList& list, QObject* parent, const QString& class_name, bool is_check_parent, bool has_names)
{
    if (is_check_parent && (has_names ? (!parent->objectName().isEmpty()) : true) && parent->inherits(class_name.toLatin1().constData()))
        list.append(parent);

    for (QObject* obj : parent->children()) {
        if ((has_names ? (!obj->objectName().isEmpty()) : true) && obj->inherits(class_name.toLatin1().constData()))
            list.append(obj);

        Utils::findObjectsByClass(list, obj, class_name, false, has_names);
    }
}

QString Utils::libraryName(const QString& libraryBaseName)
{
#if defined(Q_OS_WIN)
    QString prefix = "";
    QString extention = ".dll";
#elif defined(Q_OS_LINUX)
    QString prefix = "lib";
    QString extention = ".so";
#else
    Z_HALT("Операционная система не поддерживается");
#endif

    return prefix + libraryBaseName + extention;
}

QString Utils::executableName(const QString& executableBaseName)
{
#if defined(Q_OS_WIN)
    QString prefix = "";
    QString extention = ".exe";
#elif defined(Q_OS_LINUX)
    QString prefix = "";
    QString extention = "";
#else
    Z_HALT("Операционная система не поддерживается");
#endif

    return prefix + executableBaseName + extention;
}

QPair<bool, bool> Utils::compressActionList(const QList<QAction*>& al, bool allowBorderSeparators)
{
    bool has_visible_actions = false;
    bool has_enabled_actions = false;

    // Клонируем экшены, чтобы избежать изменений на экране
    QList<bool> visibility;
    for (int i = 0; i < al.count(); i++) {
        Z_CHECK_NULL(al.at(i));

        visibility << al.at(i)->isVisible();
        if (!al.at(i)->isSeparator() && al.at(i)->isVisible())
            has_visible_actions = true;
    }

    // Сначала делаем все сепараторы видимыми
    for (int i = 0; i < al.count(); i++) {
        QAction* a = al.at(i);
        if (!a->isSeparator())
            continue;
        visibility[i] = true;
    }

    // Проверяем нет ли повторяющихся разделителей и формируем список без дубликатов
    QList<QAction*> finalList;
    for (int i = 0; i < al.count(); i++) {
        QAction* a = al.at(i);

        if (!visibility.at(i))
            continue;

        finalList.append(a);
        if (!a->isSeparator())
            continue;

        if (i < al.count() - 1) {
            // Перебираем следующие элементы до тех пор пока они являются разделителями и
            // делаем их невидимыми
            for (int rep = i + 1; rep < al.count(); rep++) {
                QAction* nextAction = al.at(rep);
                if (!nextAction->isSeparator() && visibility.at(rep))
                    break;

                visibility[rep] = false;
            }
        }
    }

    // Устанавливаем видимость на основе сохраненной информации
    for (int i = 0; i < al.count(); i++) {
        if (finalList.count() > 0) {
            if (!allowBorderSeparators || !has_visible_actions) {
                // Проверяем нет ли разделителя на первом месте
                if (al.at(i) == finalList.at(0) && finalList.at(0)->isSeparator()) {
                    finalList.at(0)->setVisible(false);
                    finalList.removeAt(0);
                    continue;
                }
                // Проверяем нет ли разделителя на последнем месте
                if (al.at(i) == finalList.at(finalList.count() - 1) && finalList.at(finalList.count() - 1)->isSeparator()) {
                    finalList.at(finalList.count() - 1)->setVisible(false);
                    finalList.removeAt(finalList.count() - 1);
                    continue;
                }
            }
        }

        al.at(i)->setVisible(visibility.at(i));
    }

    for (int i = 0; i < al.count(); i++) {
        if (!al.at(i)->isVisible())
            continue;

        if (al.at(i)->menu() != nullptr) {
            auto child_info = compressActionList(al.at(i)->menu()->actions(), false);

            if (!al.at(i)->menu()->actions().isEmpty())
                al.at(i)->setEnabled(child_info.second);
        }

        has_enabled_actions |= al.at(i)->isEnabled();
    }

    return {finalList.count() > 0, has_enabled_actions};
}

QPair<bool, bool> Utils::compressToolbarActions(QToolBar* toolbar, bool allowBorderSeparators)
{
    Z_CHECK_NULL(toolbar);
    return Utils::compressActionList(toolbar->actions(), allowBorderSeparators);
}

bool Utils::hideEmptyMenuActions(const QList<QAction*>& al)
{
    if (al.isEmpty())
        return false;

    bool has_visible = false;

    for (int i = 0; i < al.count(); i++) {
        Z_CHECK_NULL(al.at(i));
        if (al.at(i)->menu() == nullptr) {
            if (al.at(i)->isVisible())
                has_visible = true;
            continue;
        }

        al.at(i)->setVisible(hideEmptyMenuActions(al.at(i)->menu()->actions()));
    }

    return has_visible;
}

QAction* Utils::createToolbarMenuAction(Qt::ToolButtonStyle button_style, const QString& text, const QIcon& icon, QToolBar* toolbar, QAction* before,
    QMenu* menu, const QString& action_object_name, const QString& button_object_name)
{
    QAction* action = new QAction(icon, text);
    if (!icon.isNull())
        action->setIconText(alignMultilineString(text));

    if (toolbar != nullptr) {
        if (before == nullptr)
            toolbar->addAction(action);
        else
            toolbar->insertAction(before, action);

    } else {
        Z_CHECK(before == nullptr);
    }

    if (menu != nullptr)
        action->setMenu(menu);

    action->setObjectName(action_object_name);

    if (toolbar != nullptr) {
        QToolButton* button = prepareToolbarButton(toolbar, action);
        button->setToolButtonStyle(button_style);
        button->setObjectName(button_object_name);
        button->setToolTip(button->text().simplified());
    }

    return action;
}

QToolBar* Utils::prepareToolbar(QToolBar* toolbar, ToolbarType type)
{
    Z_CHECK_NULL(toolbar);
    if (type == ToolbarType::Main)
        toolbar->setStyleSheet("QToolBar {border: none; "
                               "margin-left: 2px; margin-right: 2px; margin-top: 2px; margin-bottom: 1px; "
                               "padding: 0px; }");
    else
        toolbar->setStyleSheet("QToolBar {border: none; }");
    toolbar->setMovable(false);
    toolbar->setFloatable(false);
    toolbar->setToolButtonStyle(Core::toolbarButtonStyle(type));
    toolbar->setIconSize(QSize(Core::toolbarButtonSize(type), Core::toolbarButtonSize(type)));
    return toolbar;
}

QToolButton* Utils::prepareToolbarButton(QToolBar* toolbar, QAction* action)
{
    Z_CHECK_NULL(toolbar);
    Z_CHECK_NULL(action);

    QToolButton* button = qobject_cast<QToolButton*>(toolbar->widgetForAction(action));
    Z_CHECK_NULL(button);

    prepareToolbarButton(button);
    return button;
}

void Utils::prepareToolbarButton(QToolButton* button)
{
    Z_CHECK_NULL(button);

    bool has_menu = false;
    if (button->menu() == nullptr) {
        auto acts = button->actions();
        for (auto a : qAsConst(acts)) {
            if (a->menu() == nullptr)
                continue;
            has_menu = true;
            break;
        }

    } else {
        has_menu = true;
    }

    if (has_menu) {
        button->setPopupMode(QToolButton::InstantPopup);
        button->setStyleSheet(QStringLiteral("QToolButton::menu-indicator{width:0px;}"));

    } else {
        int min_size = UiSizes::defaultToolbarIconSize(ToolbarType::Internal);
        if (button->iconSize().width() < min_size)
            button->setIconSize({min_size, min_size});
    }
}

void Utils::setHighlightWidgetFrame(QWidget* w, InformationType type)
{
    Z_CHECK_NULL(w);
    w->setGraphicsEffect(new HighlightBorderEffect(w, type));
}

void Utils::removeHighlightWidgetFrame(QWidget* w)
{
    Z_CHECK_NULL(w);
    w->setGraphicsEffect(nullptr);
}

QString Utils::stringToMultiline(const QString& value, int width, const QFontMetrics& fm)
{
    QString v = value;
    v.replace(QChar('\n'), QStringLiteral(" "));

    using namespace Hyphenation;
    int w = width - fm.horizontalAdvance(QChar('O'));
    if (w <= 0)
        return v;

    QStringList sl = GlobalTextHyphenationFormatter::formatter()->split(v, w, fm, TextHyphenationFormatter::HyphenateWrap);
    return sl.join(QChar('\n'));
}

int Utils::editDistance(const QString& str1, const QString& str2)
{
    const int len1 = str1.size();
    const int len2 = str2.size();

    QVector<int> col(len2 + 1);
    QVector<int> prev_col(len2 + 1);

    for (int i = 0; i < prev_col.size(); i++)
        prev_col[i] = i;

    for (int i = 0; i < len1; i++) {
        col[0] = i + 1;
        for (int j = 0; j < len2; j++) {
            if (str1[i] == str2[j])
                col[j + 1] = qMin(qMin(1 + col[j], 1 + prev_col[1 + j]), prev_col[j]);
            else
                col[j + 1] = qMin(qMin(1 + col[j], 1 + prev_col[1 + j]), prev_col[j] + 1);
        }

        col.swap(prev_col);
    }
    return prev_col[len2];
}

QMainWindow* Utils::getMainWindow()
{
    auto tw = QApplication::topLevelWidgets();
    for (QWidget* widget : qAsConst(tw)) {
        if (widget->inherits("QMainWindow")) {
            return dynamic_cast<QMainWindow*>(widget);
        }
    }
    return nullptr;
}

bool Utils::isAlnum(const QChar& ch)
{
    uint u = uint(ch.unicode());
    // matches [a-zA-Z0-9_]
    return u - 'a' < 26 || u - 'A' < 26 || u - '0' < 10 || u == '_';
}

bool Utils::compareVariant(const QVariant& v1, const QVariant& v2, CompareOperator op, const QLocale* locale, CompareOptions options)
{
    if ((!v1.isValid() || v1.isNull()) && (!v2.isValid() || v2.isNull())) {
        if (op == CompareOperator::Equal || op == CompareOperator::LessOrEqual || op == CompareOperator::MoreOrEqual)
            return true;
        return false;
    }

    bool invalid_1 = InvalidValue::isInvalidValueVariant(v1);
    bool invalid_2 = InvalidValue::isInvalidValueVariant(v2);
    if (invalid_1 && invalid_2)
        return InvalidValue::fromVariant(v1) == InvalidValue::fromVariant(v2);
    if (invalid_1 || invalid_2)
        return false;

    if (v1.type() == QVariant::Bool && v2.type() == QVariant::Bool) {
        switch (op) {
            case CompareOperator::Equal:
                return v1.toBool() == v2.toBool();
            case CompareOperator::NotEqual:
                return v1.toBool() != v2.toBool();
            case CompareOperator::More:
                return v1.toBool() > v2.toBool();
            case CompareOperator::MoreOrEqual:
                return v1.toBool() >= v2.toBool();
            case CompareOperator::Less:
                return v1.toBool() < v2.toBool();
            case CompareOperator::LessOrEqual:
                return v1.toBool() <= v2.toBool();
            default:
                Z_HALT_INT;
                return false;
        }
    }
    if ((v1.type() == QVariant::Bool && v2.type() == QVariant::Invalid) || (v1.type() == QVariant::Invalid && v2.type() == QVariant::Bool)) {
        switch (op) {
            case CompareOperator::Equal:
                return false;
            case CompareOperator::NotEqual:
                return true;
            case CompareOperator::More:
                return v1.toBool() > v2.toBool();
            case CompareOperator::MoreOrEqual:
                return v1.toBool() >= v2.toBool();
            case CompareOperator::Less:
                return v1.toBool() < v2.toBool();
            case CompareOperator::LessOrEqual:
                return v1.toBool() <= v2.toBool();
            default:
                Z_HALT_INT;
                return false;
        }
    }

    auto qt_case = (options & CompareOption::CaseInsensitive ? Qt::CaseInsensitive : Qt::CaseSensitive);

    if (locale == nullptr)
        locale = Core::locale(LocaleType::UserInterface);

    if (op == CompareOperator::Contains)
        return variantToStringHelper(v1, locale, -1).contains(variantToStringHelper(v2, locale, -1), qt_case);
    else if (op == CompareOperator::StartsWith)
        return variantToStringHelper(v1, locale, -1).startsWith(variantToString(v2), qt_case);
    if (op == CompareOperator::EndsWith)
        return variantToStringHelper(v1, locale, -1).endsWith(variantToString(v2), qt_case);

    if (Numeric::isNumeric(v1) || Numeric::isNumeric(v2)) {
        Numeric n1 = Numeric::fromVariant(v1);
        Numeric n2 = Numeric::fromVariant(v2);
        switch (op) {
            case CompareOperator::Equal:
                return n1 == n2;
            case CompareOperator::NotEqual:
                return n1 != n2;
            case CompareOperator::More:
                return n1 > n2;
            case CompareOperator::MoreOrEqual:
                return n1 >= n2;
            case CompareOperator::Less:
                return n1 < n2;
            case CompareOperator::LessOrEqual:
                return n1 <= n2;
            default:
                Z_HALT_INT;
                return false;
        }
    }

    if (v1.type() != QVariant::String || v2.type() != QVariant::String) {
        QVariant v1_prep = variantToDigitalHelper(v1, true, locale);
        QVariant v2_prep = variantToDigitalHelper(v2, true, locale);
        bool ok1 = false;
        bool ok2 = false;
        double d1 = 0;
        double d2 = 0;
        if (v1_prep.isValid() && v2_prep.isValid()) {
            d1 = v1_prep.toDouble(&ok1);
            d2 = v2_prep.toDouble(&ok2);
        }
        if (ok1 && ok2) {
            switch (op) {
                case CompareOperator::Equal:
                    return fuzzyCompare(d1, d2);
                case CompareOperator::NotEqual:
                    return !fuzzyCompare(d1, d2);
                case CompareOperator::More:
                    return d1 > d2;
                case CompareOperator::MoreOrEqual:
                    return fuzzyCompare(d1, d2) || d1 > d2;
                case CompareOperator::Less:
                    return d1 < d2;
                case CompareOperator::LessOrEqual:
                    return fuzzyCompare(d1, d2) || d1 < d2;
                default:
                    Z_HALT_INT;
                    return false;
            }
        }
    }

    if ((v1.type() == QVariant::Date || v1.type() == QVariant::DateTime) && (v2.type() == QVariant::Date || v2.type() == QVariant::DateTime)) {
        QDateTime s1 = v1.toDateTime();
        QDateTime s2 = v2.toDateTime();

        switch (op) {
            case CompareOperator::Equal:
                return s1 == s2;
            case CompareOperator::NotEqual:
                return s1 != s2;
            case CompareOperator::More:
                return s1 > s2;
            case CompareOperator::MoreOrEqual:
                return s1 >= s2;
            case CompareOperator::Less:
                return s1 < s2;
            case CompareOperator::LessOrEqual:
                return s1 <= s2;
            default:
                Z_HALT_INT;
                return false;
        }
    } else {
        QString s1 = variantToString(v1);
        QString s2 = variantToString(v2);

        switch (op) {
            case CompareOperator::Equal:
                return comp(s1, s2, qt_case);
            case CompareOperator::NotEqual:
                return !comp(s1, s2, qt_case);
            case CompareOperator::Less:
                return Core::fr()->collator(locale->language(), qt_case, false, true)->compare(s1, s2) < 0;
            case CompareOperator::LessOrEqual:
                return Core::fr()->collator(locale->language(), qt_case, false, true)->compare(s1, s2) <= 0;
            case CompareOperator::More:
                return Core::fr()->collator(locale->language(), qt_case, false, true)->compare(s2, s1) < 0;
            case CompareOperator::MoreOrEqual:
                return Core::fr()->collator(locale->language(), qt_case, false, true)->compare(s2, s1) <= 0;
            default:
                Z_HALT_INT;
                return false;
        }
    }
}

bool Utils::compareVariant(const QVariant& v1, const QVariant& v2, CompareOperator op, LocaleType locale_type, CompareOptions options)
{
    return compareVariant(v1, v2, op, Core::locale(locale_type), options);
}

QString Utils::compareOperatorToText(CompareOperator op, bool for_java_script, bool full_name)
{
    switch (op) {
        case CompareOperator::Equal:
            return for_java_script ? QStringLiteral("==") : (full_name ? ZF_TR(ZFT_EQUAL) : QStringLiteral("="));
        case CompareOperator::NotEqual:
            return for_java_script ? QStringLiteral("!=") : (full_name ? ZF_TR(ZFT_NOT_EQUAL) : QStringLiteral("<>"));
        case CompareOperator::More:
            return full_name && !for_java_script ? ZF_TR(ZFT_MORE) : QStringLiteral(">");
        case CompareOperator::MoreOrEqual:
            return full_name && !for_java_script ? ZF_TR(ZFT_MORE_EQUAL) : QStringLiteral(">=");
        case CompareOperator::Less:
            return full_name && !for_java_script ? ZF_TR(ZFT_LESS) : QStringLiteral("<");
        case CompareOperator::LessOrEqual:
            return full_name && !for_java_script ? ZF_TR(ZFT_LESS_EQUAL) : QStringLiteral("<=");
        case CompareOperator::Contains:
            Z_CHECK(!for_java_script);
            return ZF_TR(ZFT_CONTAINS);
        case CompareOperator::StartsWith:
            Z_CHECK(!for_java_script);
            return ZF_TR(ZFT_STARTS_WITH);
        case CompareOperator::EndsWith:
            Z_CHECK(!for_java_script);
            return ZF_TR(ZFT_ENDS_WITH);
        default: {
            Z_HALT_INT;
            return QString();
        }
    }
}

QString Utils::conditionTypeToText(ConditionType c)
{
    switch (c) {
        case ConditionType::And:
            return ZF_TR(ZFT_CONDITION_AND);
        case ConditionType::Or:
            return ZF_TR(ZFT_CONDITION_OR);
        case ConditionType::Required:
            return ZF_TR(ZFT_CONDITION_REQUIRED);
        case ConditionType::Compare:
            return ZF_TR(ZFT_CONDITION_COMPARE);
        default:
            Z_HALT_INT;
            return QString();
    }
}

QVariantList Utils::toVariantList(const QStringList& list)
{
    QVariantList res;
    for (auto& v : list) {
        res << QVariant(v);
    }
    return res;
}

QVariantList Utils::toVariantList(const UidList& list)
{
    QVariantList res;
    for (auto& v : list) {
        res << v.variant();
    }
    return res;
}

QVariantList Utils::toVariantList(const QList<int>& list)
{
    QVariantList res;
    for (auto& v : list) {
        res << QVariant(v);
    }
    return res;
}

UidList Utils::toUidList(const QVariantList& list)
{
    UidList res;
    for (auto& v : list) {
        res << Uid::fromVariant(v);
    }
    return res;
}

QStringList Utils::toStringList(const QVariantList& list)
{
    QStringList res;
    for (auto& v : list) {
        res << v.toString();
    }
    return res;
}

QStringList Utils::toStringList(const QList<int>& list)
{
    QStringList res;
    for (auto& v : list) {
        res << QString::number(v);
    }
    return res;
}

QList<int> Utils::toIntList(const QVariantList& list)
{
    QList<int> res;
    for (auto& v : list) {
        res << v.toInt();
    }
    return res;
}

bool Utils::containsVariantList(const QVariantList& list, const QVariant& value, const QLocale* locale, CompareOptions options)
{
    return indexOfVariantList(list, value, locale, options) >= 0;
}

int Utils::indexOfVariantList(const QVariantList& list, const QVariant& value, const QLocale* locale, CompareOptions options)
{
    for (int i = 0; i < list.count(); i++) {
        if (compareVariant(list.at(i), value, CompareOperator::Equal, locale, options))
            return i;
    }
    return -1;
}

bool Utils::hasSlot(QObject* obj, const char* member, QGenericReturnArgument ret, QGenericArgument val0, QGenericArgument val1, QGenericArgument val2,
    QGenericArgument val3, QGenericArgument val4, QGenericArgument val5, QGenericArgument val6, QGenericArgument val7, QGenericArgument val8,
    QGenericArgument val9)
{
    // код выдран из qmetaobject.cpp QMetaObject::invokeMethod

    QVarLengthArray<char, 512> sig;
    int len = qstrlen(member);
    if (len <= 0)
        return false;
    sig.append(member, len);
    sig.append('(');

    const char* typeNames[]
        = {ret.name(), val0.name(), val1.name(), val2.name(), val3.name(), val4.name(), val5.name(), val6.name(), val7.name(), val8.name(), val9.name()};

    static const int MaximumParamCount = 11;
    int paramCount;
    for (paramCount = 1; paramCount < MaximumParamCount; ++paramCount) {
        len = qstrlen(typeNames[paramCount]);
        if (len <= 0)
            break;
        sig.append(typeNames[paramCount], len);
        sig.append(',');
    }
    if (paramCount == 1)
        sig.append(')'); // no parameters
    else
        sig[sig.size() - 1] = ')';
    sig.append('\0');

    const QMetaObject* meta = obj->metaObject();
    int idx = meta->indexOfMethod(sig.constData());
    if (idx < 0) {
        QByteArray norm = QMetaObject::normalizedSignature(sig.constData());
        idx = meta->indexOfMethod(norm.constData());
    }

    if (idx < 0 || idx >= meta->methodCount())
        return false;

    return true;
}

bool Utils::hasSlot(QObject* obj, const char* member, QGenericArgument val0, QGenericArgument val1, QGenericArgument val2, QGenericArgument val3,
    QGenericArgument val4, QGenericArgument val5, QGenericArgument val6, QGenericArgument val7, QGenericArgument val8, QGenericArgument val9)
{
    return hasSlot(obj, member, QGenericReturnArgument(), val0, val1, val2, val3, val4, val5, val6, val7, val8, val9);
}

QList<int> Utils::distributeEqual(int total, int count, int seed)
{
    Z_CHECK(count >= 0);

    QList<int> res = QVector<int>(count, 0).toList();

    if (count == 0)
        return res;

    int alloc_one = round(static_cast<double>(total) / count, RoundOption::Up);
    int alloc_sum = 0;

    for (int i = 0; i < count; i++) {
        // разница между суммой фактического распределения и планируемой
        int diff = alloc_sum + alloc_one //  сколько распределено на этом шаге
                   + alloc_one * (count - i - 1) // сколько будет еще распределено, если не поменять размер инкремента
                   - total;
        if (diff > 0) {
            // уменьшаем размер инкремента
            alloc_one -= round(static_cast<double>(diff) / (count - i), RoundOption::Down);
            if (alloc_one <= 0)
                alloc_one = 1;
        }

        res[i] = alloc_one;
        alloc_sum += alloc_one;

        if (alloc_sum >= total) {
            // корректируем остаток от деления
            res[i] += total - alloc_sum;
            break;
        }
    }

    // перемешиваем в случайном порядке
    shuffleList<int>(res, seed);
    return res;
}

QList<int> Utils::distribute(const QList<int>& base, const QList<int>& limit, int total)
{
    Z_CHECK(base.count() == limit.count() || limit.isEmpty());

    qreal base_total = 0;
    foreach (int v, base) {
        base_total += v;
        Z_CHECK(v >= 0);
    }
    Z_CHECK(base_total > 0);

    QList<qreal> base_norm;
    for (int i = 0; i < base.count(); i++) {
        base_norm << static_cast<qreal>(total) * static_cast<qreal>(base.at(i)) / base_total;
    }

    // готовим лимиты
    QList<int> limit_prepared;
    for (int i = 0; i < base.count(); i++) {
        if (limit.isEmpty()) {
            if (base_norm.at(i) > 0)
                limit_prepared << total;
            else
                limit_prepared << 0;
        } else {
            if (base_norm.at(i) > 0)
                limit_prepared[i] = limit.at(i);
            else
                limit_prepared[i] = 0;
        }
    }

    // распределяем без учета ошибки округления
    QList<int> res = QVector<int>(base.count(), 0).toList();
    if (total == 0)
        return res;

    int res_total = 0;
    //    int limit_total = 0;
    for (int i = 0; i < base_norm.count(); i++) {
        res[i] = round(static_cast<qreal>(total) * static_cast<qreal>(base_norm.at(i)) / total, RoundOption::Down);
        res[i] = qMin(res.at(i), limit_prepared.at(i));
        res_total += res.at(i);

        //        if (!limit_prepared.isEmpty())
        //            limit_total += limit_prepared.at(i);
    }

    if (total > res_total) {
        QList<int> pos_sorted;
        for (int i = 0; i < base_norm.count(); i++)
            pos_sorted << i;

        // остаток распределяем между элементами, где не превышен лимит
        while (total > res_total) {
            std::sort(pos_sorted.begin(), pos_sorted.end(), [=](int i1, int i2) -> bool {
                qreal base1 = static_cast<qreal>(res.at(i1)) / static_cast<qreal>(total);
                qreal base2 = static_cast<qreal>(res.at(i2)) / static_cast<qreal>(total);

                if (qFuzzyIsNull(base_norm.at(i1)) && qFuzzyIsNull(base_norm.at(i2)))
                    return false; // нельзя распределять вообще

                if (qFuzzyIsNull(base_norm.at(i1)))
                    return false; // нельзя распределять в первый, значит приоритет второму
                if (qFuzzyIsNull(base_norm.at(i2)))
                    return true; // нельзя распределять во второй, значит приоритет первому

                if (qFuzzyIsNull(base1) && qFuzzyIsNull(base2))
                    return base_norm.at(i1) > base_norm.at(i2);

                return base1 / base_norm.at(i1) < base2 / base_norm.at(i2); // приоритет относительно базы распределения
            });

            for (int i : qAsConst(pos_sorted)) {
                if (res.at(i) >= limit_prepared.at(i))
                    continue;

                res[i]++;
                res_total++;
                break;
            }
        }
    }

    Z_CHECK(res_total == total);

    return res;
}

bool Utils::contains(const QSet<QVariant>& v1, const QSet<QVariant>& v2)
{
    for (auto i2 = v2.constBegin(); i2 != v2.constEnd(); ++i2) {
        bool found = false;
        for (auto i1 = v1.constBegin(); i1 != v1.constEnd(); ++i1) {
            if (compareVariant(*i1, *i2)) {
                found = true;
                break;
            }
        }
        if (!found)
            return false;
    }

    return true;
}

bool Utils::contains(const QList<QVariant>& v1, const QList<QVariant>& v2)
{
    for (auto i2 = v2.constBegin(); i2 != v2.constEnd(); ++i2) {
        bool found = false;
        for (auto i1 = v1.constBegin(); i1 != v1.constEnd(); ++i1) {
            if (compareVariant(*i1, *i2)) {
                found = true;
                break;
            }
        }
        if (!found)
            return false;
    }

    return true;
}

bool Utils::equal(const QSet<QVariant>& v1, const QSet<QVariant>& v2)
{
    if (v1.count() != v2.count())
        return false;

    return contains(v1, v2);
}

bool Utils::equal(const QList<QVariant>& v1, const QList<QVariant>& v2)
{
    if (v1.count() != v2.count())
        return false;

    for (int i = 0; i < v1.count(); i++) {
        if (!compareVariant(v1.at(i), v2.at(i)))
            return false;
    }
    return true;
}

QSize Utils::grownBy(const QSize& size, const QMargins& m)
{
    return {size.width() + m.left() + m.right(), size.height() + m.top() + m.bottom()};
}

QString Utils::generateUniqueStringDefault(const QString& data)
{
    return generateUniqueString(QCryptographicHash::Sha1, data);
}

QString Utils::generateUniqueString(QCryptographicHash::Algorithm algorithm, const QString& data, bool remove_curly_braces)
{
    if (data.isEmpty()) {
        return generateGUIDString(QString(), remove_curly_braces);
    } else
        return QString(QCryptographicHash::hash(data.toUtf8(), algorithm).toHex());
}

QString Utils::generateGUIDString(const QString& data, bool remove_curly_braces)
{
    return generateGUID(data).toString(remove_curly_braces ? QUuid::Id128 : QUuid::WithBraces);
}

QUuid Utils::generateGUID(const QString& data)
{
    if (data.isEmpty())
        return QUuid::createUuidV5(QUuid::createUuid(), _guid_salt);
    else
        return QUuid::createUuidV5(_baseUuid, data);
}

QString Utils::generateUniqueString(const QVariantList& values, const QList<bool>& caseInsensitive)
{
    Z_CHECK(caseInsensitive.isEmpty() || values.count() == caseInsensitive.count());

    static QString data;
    if (data.capacity() < 1000)
        data.reserve(1000); // резервируем размер, чтобы минимизировать работу с памятью
    data.resize(0);

    int n = 0;
    for (auto i = values.constBegin(); i != values.constEnd(); ++i) {
        if (i->type() == QVariant::Bool && i->isValid() && !i->toBool())
            data += Consts::KEY_SEPARATOR; // логическое выражение FALSE рассматриваем как NULL или пустую строку
        else if (caseInsensitive.isEmpty())
            data += Consts::KEY_SEPARATOR + i->toString().trimmed();
        else
            data += Consts::KEY_SEPARATOR + (caseInsensitive.at(n) ? i->toString().trimmed().toLower() : i->toString().trimmed());
        n++;
    }
    // используем UUID т.к. он быстрее
    return generateGUIDString(data);
}

QString Utils::generateChecksum(QIODevice* device, qint64 size)
{
    Z_CHECK_NULL(device);
    Z_CHECK(device->isOpen() && device->isReadable() && !device->isSequential());

    device->seek(0);

    QByteArray hash;
    QByteArray data;
    qint64 readBytes = 0;
    qint64 needRead;
    static const qint64 _readBlockSize = 5000000; // 5 мегабайт
    do {
        if (size > 0)
            needRead = qMin(_readBlockSize, size - readBytes);
        else
            needRead = _readBlockSize;

        data.clear(); // Минимизация использования памяти
        data = device->read(needRead);
        readBytes += data.size();
        if (data.size() > 0)
            hash = QCryptographicHash::hash(hash + data, QCryptographicHash::Sha1);
    } while (data.size() == needRead && (size == 0 || size > readBytes));

    return QString(hash.toHex());
}

QString Utils::generateChecksum(const QString& s)
{
    auto data = s.toUtf8();
    QBuffer buf(&data);
    buf.open(QBuffer::ReadOnly);
    return generateChecksum(&buf);
}

QString Utils::generateChecksum(const QByteArray& b)
{
    auto data = b;
    QBuffer buf(&data);
    buf.open(QBuffer::ReadOnly);
    return generateChecksum(&buf);
}

AccessRights Utils::accessForAction(ObjectActionType action_type)
{
    switch (action_type) {
        case ObjectActionType::Create:
            return AccessFlag::Create;

        case ObjectActionType::CreateLevelDown:
            return AccessFlag::Create;

        case ObjectActionType::Remove:
            return AccessFlag::Remove;

        case ObjectActionType::Modify:
            return AccessFlag::Modify;

        case ObjectActionType::Export:
            return AccessFlag::Export;

        case ObjectActionType::Print:
            return AccessFlag::Print;

        case ObjectActionType::Configurate:
            return AccessFlag::Configurate;

        case ObjectActionType::Administrate:
            return AccessFlag::Administrate;

        default:
            return AccessRights();
    }
}

QWidget* Utils::getTopWindow()
{
    QWidget* w = QApplication::activeWindow();
    if (w != nullptr && !Core::fr()->isNonParentWidget(w))
        return w;

    w = QApplication::activeModalWidget();
    if (w != nullptr && !Core::fr()->isNonParentWidget(w))
        return w;

    return getMainWindow();
}

bool Utils::isModalWindow()
{
    QWidget* w = QApplication::activeModalWidget();
    return w != nullptr;
}

QWidget* Utils::getBaseChildWidget(QWidget* widget)
{
    if (widget == nullptr)
        return nullptr;

    if (getMainWidget(widget)->inherits("zf::ItemSelector"))
        return qobject_cast<ItemSelector*>(widget)->editor();

    if (getMainWidget(widget)->inherits("zf::RequestSelector"))
        return qobject_cast<RequestSelector*>(widget)->editor();

    if (getMainWidget(widget)->inherits("zf::FormattedEdit"))
        return qobject_cast<FormattedEdit*>(widget)->editor();

    return widget;
}

QWidget* Utils::getMainWidget(QWidget* widget)
{
    if (widget == nullptr)
        return nullptr;

    QWidget* w = findParentWidget<ItemSelector*>(widget);
    if (w == nullptr)
        w = findParentWidget<RequestSelector*>(widget);
    if (w == nullptr)
        w = findParentWidget<QAbstractItemView*>(widget);
    if (w == nullptr)
        w = findParentWidget<ImageList*>(widget);
    if (w == nullptr)
        w = findParentWidget<PictureSelector*>(widget);
    if (w == nullptr)
        w = findParentWidget<QTextEdit*>(widget);
    if (w == nullptr)
        w = findParentWidget<QPlainTextEdit*>(widget);
    if (w == nullptr)
        w = findParentWidget<FormattedEdit*>(widget);
    if (w == nullptr) {
        FrozenTableView* ftw = qobject_cast<FrozenTableView*>(widget);
        if (ftw == nullptr)
            ftw = findParentWidget<FrozenTableView*>(widget);
        if (ftw != nullptr)
            w = ftw->base();
    }

    if (w == nullptr)
        w = widget;

    return w;
}

const QStringList& Utils::containerWidgetClasses()
{
    return _container_widget_classes;
}

bool Utils::isContainerWidget(QWidget* widget)
{
    Z_CHECK_NULL(widget);
    return _container_widget_classes.contains(widget->metaObject()->className());
}

bool Utils::hasParentClass(const QObject* obj, const char* parent_class)
{
    Z_CHECK_NULL(obj);
    QObject* parent = obj->parent();
    while (parent != nullptr) {
        if (parent->inherits(parent_class))
            return true;

        parent = parent->parent();
    }
    return false;
}

bool Utils::hasParent(const QObject* obj, const QObject* parent)
{
    Z_CHECK_NULL(obj);
    Z_CHECK_NULL(parent);

    QObject* obj_parent = obj->parent();
    while (obj_parent != nullptr) {
        if (obj_parent == parent)
            return true;

        obj_parent = obj_parent->parent();
    }
    return false;
}

bool Utils::hasParentWindow(const QWidget* w, bool check_visible)
{
    auto parent = qobject_cast<QWidget*>(Utils::findParentByClass(w, "QDialog"));
    if (parent != nullptr)
        return check_visible ? parent->isVisible() : true;

    parent = qobject_cast<QWidget*>(Utils::findParentByClass(w, "QMdiSubWindow"));
    if (parent != nullptr)
        return check_visible ? parent->isVisible() : true;

    parent = qobject_cast<QWidget*>(Utils::findParentByClass(w, "QMainWindow"));
    if (parent != nullptr)
        return true;

    return false;
}

QObject* Utils::findTopParent(const QObject* obj, const QStringList& stop_on)
{
    Z_CHECK_NULL(obj);
    const QObject* parent = obj;
    const QObject* old_parent = nullptr;
    while (parent != nullptr) {
        for (auto& stop : stop_on) {
            if (parent->inherits(stop.toLatin1()))
                return const_cast<QObject*>(old_parent);
        }

        old_parent = parent;

        if (parent->parent() == nullptr)
            return const_cast<QObject*>(parent);

        parent = parent->parent();
    }
    return nullptr;
}

QWidget* Utils::findTopParentWidget(const QObject* obj, bool ignore_without_layout, const QStringList& stop_on_class, const QString& stop_on_name)
{
    Z_CHECK_NULL(obj);
    QObject* parent = obj->parent();

    // цепочка родителей
    QList<QWidget*> chain;
    while (parent != nullptr) {
        if (parent->isWidgetType()) {
            QWidget* w = qobject_cast<QWidget*>(parent);
            if (!ignore_without_layout || w->layout() != nullptr)
                chain << w;
        }
        parent = parent->parent();
    }

    if (!stop_on_name.isEmpty() || !stop_on_class.isEmpty()) {
        // идем сверху вниз
        for (int i = chain.count() - 1; i >= 0; i--) {
            QWidget* w = chain.at(i);
            if (!stop_on_name.isEmpty() && comp(w->objectName(), stop_on_name))
                return w;

            for (auto& stop : stop_on_class) {
                if (!w->inherits(stop.toLatin1()))
                    continue;
                // откатываемся на предыдущего
                return w;
            }
        }
    }

    return chain.isEmpty() ? nullptr : chain.constLast();
}

QObject* Utils::findParentByName(const QObject* obj, const QString& object_name)
{
    Z_CHECK_NULL(obj);
    Z_CHECK(!object_name.isEmpty());
    const QObject* parent = obj;
    while (parent != nullptr) {
        if (parent->objectName() == object_name)
            return const_cast<QObject*>(parent);

        parent = parent->parent();
    }
    return nullptr;
}

QObject* Utils::findParentByClass(const QObject* obj, const QString& class_name)
{
    Z_CHECK_NULL(obj);
    Z_CHECK(!class_name.isEmpty());
    const QObject* parent = obj;
    while (parent != nullptr) {
        if (parent->inherits(class_name.toLatin1()))
            return const_cast<QObject*>(parent);

        parent = parent->parent();
    }
    return nullptr;
}

QObjectList Utils::findAllParents(const QObject* obj, const QStringList& classes, const QStringList& names)
{
    Z_CHECK_NULL(obj);
    QObjectList res;
    QObject* parent = obj->parent();
    while (parent != nullptr) {
        if (!classes.isEmpty()) {
            bool found = false;
            for (auto& c : classes) {
                if (!parent->inherits(c.toLatin1().constData()))
                    continue;

                res << parent;
                found = true;
                break;
            }
            parent = parent->parent();
            if (found)
                continue;
            else if (names.isEmpty())
                continue;
        }

        if (!names.isEmpty()) {
            for (auto& c : qAsConst(names)) {
                if (c != parent->objectName())
                    continue;

                res << parent;
                break;
            }
            parent = parent->parent();
            continue;
        }

        res << parent;
        parent = parent->parent();
    }

    return res;
}

static const char* ro_props[] = {"readOnly", "enabled", nullptr};
static const bool ro_props_value[] = {true, false};
static const char* ignored_ro[] = {"QAbstractItemView", "QLabel", nullptr};
static const char* ignored_parent_ro[] = {"QAbstractItemView", "zf::PictureSelector", nullptr};

//! Какие виджеты игнорировать при установке readOnly
static bool isIgnoredROWidget(const QWidget* w)
{
    int i = 0;
    while (ignored_ro[i] != nullptr) {
        if (w->inherits(ignored_ro[i]))
            return true;

        int j = 0;
        while (ignored_parent_ro[j] != nullptr) {
            if (Utils::hasParentClass(w, ignored_parent_ro[i]))
                return true;
            j++;
        }

        i++;
    }
    return false;
}

bool Utils::isWidgetReadOnly(const QWidget* w)
{
    Z_CHECK_NULL(w);

    //    const QAbstractItemView* item_view = qobject_cast<const QAbstractItemView*>(w);
    //    if (item_view != nullptr)
    //        return item_view->editTriggers() == QAbstractItemView::NoEditTriggers;

    int i = 0;
    while (ro_props[i] != nullptr) {
        int p_index = w->metaObject()->indexOfProperty(ro_props[i]);
        if (p_index >= 0) {
            QMetaProperty to_property = w->metaObject()->property(p_index);
            if (to_property.isReadable()) {
                return w->property(ro_props[i]).toBool() == ro_props_value[i];
            }
        }
        i++;
    }

    return false;
}

bool Utils::setWidgetReadOnly(QWidget* w, bool read_only)
{
    Z_CHECK_NULL(w);

    if (Utils::getMainWidget(w) != w)
        return false; // контейнеры должны сами следить за своими дочерними виджетами

    if (isIgnoredROWidget(w))
        return false;

    if (isContainerWidget(w)) {
        // это контейнер, состоящий из другиз виджетов
        auto children = w->findChildren<QWidget*>();
        bool res = false;
        for (auto c : qAsConst(children)) {
            res |= setWidgetReadOnly(c, read_only);
        }
        return res;
    }

    int i = 0;
    while (ro_props[i] != nullptr) {
        int p_index = w->metaObject()->indexOfProperty(ro_props[i]);
        if (p_index >= 0) {
            QMetaProperty to_property = w->metaObject()->property(p_index);
            if (to_property.isReadable() && to_property.isWritable()) {
                bool target_value = read_only ? ro_props_value[i] : !ro_props_value[i];
                if (w->property(ro_props[i]).toBool() != target_value) {
                    if (to_property.write(w, target_value)) {
                        // setProperty работает очень странно. он шлет QDynamicPropertyChangeEvent, только если такого
                        // свойства у объекта нет вообще (метод возвращает false), поэтому шлем его сами
                        QDynamicPropertyChangeEvent event(ro_props[i]);
                        QApplication::sendEvent(w, &event);
                    }
                }
                return true;
            }
        }
        i++;
    }
    return false;
}

void Utils::setLayoutWidgetsVisible(QLayout* layout, bool visible, const QSet<QWidget*>& excluded_widgets, const QSet<QLayout*>& excluded_layouts)
{
    Z_CHECK_NULL(layout);
    Z_CHECK(!excluded_widgets.contains(nullptr));
    Z_CHECK(!excluded_layouts.contains(nullptr));

    const QObjectList& objList = layout->children();

    for (int i = 0; i < objList.size(); i++) {
        QLayout* la = qobject_cast<QLayout*>(objList.at(i));
        if (!la || excluded_layouts.contains(la))
            continue;

        setLayoutWidgetsVisible(la, visible, excluded_widgets, excluded_layouts);
    }

    for (int i = 0; i < layout->count(); i++) {
        if (layout->itemAt(i) && !layout->itemAt(i)->isEmpty() && layout->itemAt(i)->widget()) {
            if (excluded_widgets.contains(layout->itemAt(i)->widget()))
                continue;

            layout->itemAt(i)->widget()->setHidden(!visible);
        }
    }
}

void Utils::setLayoutWidgetsReadOnly(QLayout* layout, bool read_only, const QSet<QWidget*>& excluded_widgets, const QSet<QLayout*>& excluded_layouts)
{
    Z_CHECK_NULL(layout);
    Z_CHECK(!excluded_widgets.contains(nullptr));
    Z_CHECK(!excluded_layouts.contains(nullptr));

    const QObjectList& objList = layout->children();

    for (int i = 0; i < objList.size(); i++) {
        QLayout* la = qobject_cast<QLayout*>(objList.at(i));
        if (!la || excluded_layouts.contains(la))
            continue;

        setLayoutWidgetsReadOnly(la, read_only, excluded_widgets, excluded_layouts);
    }

    for (int i = 0; i < layout->count(); i++) {
        if (layout->itemAt(i) && !layout->itemAt(i)->isEmpty() && layout->itemAt(i)->widget()) {
            if (excluded_widgets.contains(layout->itemAt(i)->widget()))
                continue;

            setWidgetReadOnly(layout->itemAt(i)->widget(), read_only);
        }
    }
}

bool Utils::clearWidget(QWidget* w)
{
    Z_CHECK_NULL(w);

    if (auto x = qobject_cast<QLineEdit*>(w)) {
        x->clear();
        return true;
    }

    if (auto x = qobject_cast<FormattedEdit*>(w)) {
        x->clear();
        return true;
    }

    if (auto x = qobject_cast<QTextEdit*>(w)) {
        x->clear();
        return true;
    }

    if (auto x = qobject_cast<QPlainTextEdit*>(w)) {
        x->clear();
        return true;
    }

    if (auto x = qobject_cast<ItemSelector*>(w)) {
        x->clear();
        return true;
    }

    if (auto x = qobject_cast<RequestSelector*>(w)) {
        x->clear();
        return true;
    }

    if (auto x = qobject_cast<PictureSelector*>(w)) {
        x->clear();
        return true;
    }

    if (auto x = qobject_cast<ImageList*>(w)) {
        if (x->storage() != nullptr) {
            x->storage()->clear();
            return true;
        }
    }

    if (auto x = qobject_cast<QComboBox*>(w)) {
        x->setCurrentIndex(-1);
        if (x->isEditable())
            x->setEditText(QString());
        return true;
    }

    return false;
}

static bool isVisible_helper(QWidget* widget, bool res, bool check_window)
{
    if (!res || !check_window)
        return res;

    auto parent = qobject_cast<QWidget*>(Utils::findParentByClass(widget, "QDialog"));
    if (parent != nullptr)
        return parent->isVisible();

    parent = qobject_cast<QWidget*>(Utils::findParentByClass(widget, "QMdiSubWindow"));
    if (parent != nullptr)
        return parent->isVisible();

    parent = qobject_cast<QWidget*>(Utils::findParentByClass(widget, "QMainWindow"));
    if (parent != nullptr)
        return true;

    return false;
}

bool Utils::isVisible(QWidget* widget, bool check_window)
{
    Z_CHECK_NULL(widget);

    if (widget->isHidden())
        return false;

    if (QApplication::focusWidget() == widget)
        return isVisible_helper(widget, true, check_window);

    // Ищем родителей класса QTabWidget/QToolBox и проверяем родителей на видимость
    QWidget* w = widget->parentWidget();
    while (w != nullptr) {
        if (w->isHidden())
            return false;

        if (auto window = qobject_cast<QMdiSubWindow*>(w))
            return check_window ? window->isVisible() : true;

        if (auto window = qobject_cast<QDialog*>(w))
            return check_window ? window->isVisible() : true;

        if (qobject_cast<QMainWindow*>(w) != nullptr)
            return true;

        if (auto cw = qobject_cast<QSplitter*>(w)) {
            for (int i = 0; i < cw->count(); i++) {
                if (cw->widget(i) == widget || cw->widget(i)->isAncestorOf(widget)) {
                    if (cw->sizes().at(i) == 0)
                        return false;
                    break;
                }
            }
        }

        if (auto cw = qobject_cast<QTabWidget*>(w)) {
            for (int i = 0; i < cw->count(); i++) {
                if (cw->widget(i)->isAncestorOf(widget)) {
                    if (cw->currentIndex() != i)
                        return false; // Индекс текущей закладки не совпадает с закладкой виджета
                    break;
                }
            }

        } else if (auto cw = qobject_cast<QToolBox*>(w)) {
            for (int i = 0; i < cw->count(); i++) {
                if (cw->widget(i)->isAncestorOf(widget)) {
                    if (cw->currentIndex() != i)
                        return false; // Индекс текущей закладки не совпадает с закладкой виджета
                    break;
                }
            }

        } else if (auto cw = qobject_cast<CollapsibleGroupBox*>(w)) {
            if (!cw->isExpanded())
                return false;
        }

        w = w->parentWidget();
    }

    return isVisible_helper(widget, true, check_window);
}

class UiLoader : public QUiLoader
{
public:
    UiLoader(QObject* parent = nullptr)
        : QUiLoader(parent)
    {
    }

    QWidget* createWidget(const QString& className, QWidget* parent = nullptr, const QString& name = QString()) override
    {
        QWidget* w = QUiLoader::createWidget(className, parent, name);
        //        if (w != nullptr && w->styleSheet().isEmpty())
        //            w->setStyleSheet(QStringLiteral(".QWidget {}"));
        return w;
    }
};

QWidget* Utils::openUI(const QString& file_name, Error& error, QWidget* parent)
{
    Z_CHECK(!file_name.isEmpty());

    error.clear();

    UiLoader loader;
    if (!QFile::exists(file_name)) {
        error = Error::fileNotFoundError(file_name);
        return nullptr;
    }

    QFile file(file_name);
    if (!file.open(QFile::ReadOnly)) {
        error = Error::fileIOError(file_name);
        return nullptr;
    }

    QWidget* form = loader.load(&file, parent);
    if (!loader.errorString().isEmpty()) {
        error = Error(loader.errorString());
        return nullptr;
    }
    Z_CHECK_NULL(form);

    return form;
}

QLabel* Utils::createWaitingMovie(QWidget* parent)
{
    auto wait_movie_label = new QLabel(parent);
    wait_movie_label->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    wait_movie_label->setObjectName(QStringLiteral("zfwm"));

    auto wait_movie = new QMovie(QStringLiteral(":/share_icons/loader.gif"), {}, wait_movie_label);
    wait_movie_label->setMovie(wait_movie);

    QList<QSize> sizes = QIcon(wait_movie_label->movie()->fileName()).availableSizes();
    Z_CHECK(!sizes.isEmpty());
    wait_movie_label->setGeometry(0, 0, sizes.first().width(), sizes.first().height());
    // без такого шаманства бывают глюки
    wait_movie->start();
    wait_movie->stop();

    return wait_movie_label;
}

void Utils::alignGridLayouts(const QList<QGridLayout*>& layouts)
{
    int min_col = std::numeric_limits<int>::max();
    for (int i = 0; i < layouts.count(); i++) {
        min_col = qMin(min_col, layouts.at(i)->columnCount());
    }

    QVector<int> width(min_col - 1, 0);
    for (int i = 0; i < layouts.count(); i++) {
        QGridLayout* la = layouts.at(i);
        Z_CHECK_NULL(la);
        for (int col = 0; col < qMin(min_col, la->columnCount()) - 1; col++) {
            int max_width = 0;
            for (int row = 0; row < la->rowCount(); row++) {
                QLayoutItem* item = la->itemAtPosition(row, col);
                if (item->widget() == nullptr)
                    continue;

                int index = la->indexOf(item->widget());
                int row_w, column_w, rowSpan, columnSpan;
                la->getItemPosition(index, &row_w, &column_w, &rowSpan, &columnSpan);
                if (rowSpan > 1 || columnSpan > 1)
                    continue;

                if (auto w = qobject_cast<QLabel*>(item->widget())) {
                    max_width = qMax(max_width, stringWidth(w->text()));
                }
            }
            width[col] = qMax(width.at(col), max_width);
        }
    }

    for (int i = 0; i < layouts.count(); i++) {
        QGridLayout* la = layouts.at(i);
        for (int col = 0; col < qMin(min_col, la->columnCount()) - 1; col++) {
            if (width.at(col) > 0)
                la->setColumnMinimumWidth(col, width.at(col));
        }
    }
}

Utils::SplitterSavedData Utils::hideSplitterPart(QSplitter* splitter, int pos)
{
    Z_CHECK_NULL(splitter);
    Z_CHECK(pos >= 0 && splitter->count() > pos);

    SplitterSavedData data;

    data.pos = pos;
    data.sizes = splitter->sizes();
    data.handle_size = splitter->handleWidth();
    data.style = splitter->styleSheet();
    data.collapsible = splitter->isCollapsible(pos);

    splitter->setCollapsible(pos, true);
    splitter->widget(pos)->setHidden(true);

    QList<int> sizes;
    for (int i = 0; i < data.sizes.count(); i++) {
        if (i == pos)
            sizes << 0;
        else
            sizes << (data.sizes.at(i) == 0 ? 1 : data.sizes.at(i));
    }
    splitter->setSizes(sizes);

    // без этого извращения handle не скрывается
    splitter->setHandleWidth(1);
    for (int i = 0; i < splitter->count(); i++) {
        if (i != pos)
            continue;

        QSplitterHandle* hndl = splitter->handle(i);
        hndl->setEnabled(false);
        hndl->setHidden(true);
    }
    if (splitter->objectName().isEmpty())
        splitter->setStyleSheet(QStringLiteral("QSplitter::handle {image: url(images/zf_not_exists.png); }"));
    else
        splitter->setStyleSheet(QStringLiteral("QSplitter::handle#%1 {image: url(images/zf_not_exists.png); }").arg(splitter->objectName()));

    return data;
}

void Utils::showSplitterPart(QSplitter* splitter, const Utils::SplitterSavedData& data)
{
    Z_CHECK_NULL(splitter);
    if (!data.isValid())
        return;

    splitter->setSizes(data.sizes);
    for (int i = 0; i < splitter->count(); i++) {
        if (i != data.pos)
            continue;

        QSplitterHandle* hndl = splitter->handle(i);
        hndl->setEnabled(true);
        hndl->setVisible(true);
    }
    splitter->setHandleWidth(data.handle_size);
    splitter->setStyleSheet(data.style);
    splitter->setCollapsible(data.pos, data.collapsible);
}

void Utils::prepareTabBar(QTabWidget* tw, bool hide_disabled, int lines)
{
    Z_CHECK_NULL(tw);
    prepareTabBar(tw->tabBar(), hide_disabled, lines);
}

void Utils::prepareTabBar(QTabBar* tb, bool hide_disabled, int lines)
{
    Z_CHECK_NULL(tb);

    QStringList s;

    Qt::Orientation orientation;
    if (tb->shape() == QTabBar::RoundedNorth || tb->shape() == QTabBar::RoundedSouth || tb->shape() == QTabBar::TriangularNorth
        || tb->shape() == QTabBar::TriangularSouth)
        orientation = Qt::Horizontal;
    else
        orientation = Qt::Vertical;

    if (hide_disabled) {
        if (orientation == Qt::Horizontal)
            s << QStringLiteral("QTabBar::tab:disabled {min-width: 0px; max-width: 0px; color: rgba(0,0,0,0); background-color: rgba(0,0,0,0); }");
        else
            s << QStringLiteral("QTabBar::tab:disabled {min-height: 0px; max-height: 0px; color: rgba(0,0,0,0); background-color: rgba(0,0,0,0); }");
    }

    if (lines > 1) {
        if (orientation == Qt::Horizontal)
            s << QStringLiteral("QTabBar::tab { height: %1px; }").arg(tb->fontMetrics().height() * (lines + 1));
        else
            s << QStringLiteral("QTabBar::tab { width: %1px; }").arg(tb->fontMetrics().height() * (lines + 1));
    }

    if (!s.isEmpty())
        tb->setStyleSheet(s.join("\n"));
}

QFrame* Utils::createLine(Qt::Orientation orientation, int width)
{
    QFrame* line = new QFrame();
    line->setObjectName(QStringLiteral("zf_line_%1").arg(orientation));
    if (orientation == Qt::Horizontal) {
        line->setMinimumHeight(width + 1);
        line->setMaximumHeight(width + 1);
        line->setStyleSheet(QStringLiteral("border: 2px %1;"
                                           "border-top-style: ridge; "
                                           "border-right-style: none; "
                                           "border-bottom-style: none; "
                                           "border-left-style: none;")
                                .arg(Colors::uiLineColor(true).name()));
    } else {
        line->setMinimumWidth(width + 1);
        line->setMaximumWidth(width + 1);
        line->setStyleSheet(QStringLiteral("border: 2px %1;"
                                           "border-top-style: none; "
                                           "border-right-style: none; "
                                           "border-bottom-style: none; "
                                           "border-left-style: ridge;")
                                .arg(Colors::uiLineColor(true).name()));
    }
    return line;
}

bool Utils::isExpandingHelper(QLayout* la, Qt::Orientation orientation)
{
    for (int i = 0; i < la->count(); i++) {
        auto item = la->itemAt(i);
        if (item->spacerItem() != nullptr)
            continue;
        if (item->widget() != nullptr && isExpanding(item->widget(), orientation))
            return true;
        if (item->layout() != nullptr && isExpandingHelper(item->layout(), orientation))
            return true;
    }
    return false;
}

void Utils::debugDataset_helper(const QAbstractItemModel* model, int role, const QModelIndex& parent)
{
    for (int row = 0; row < model->rowCount(parent); row++) {
        QString rowText = QString::number(row) + ": ";
        for (int col = 0; col < model->columnCount(parent); col++) {
            if (col > 0)
                rowText += " $$ ";
            rowText += model->data(model->index(row, col, parent), role).toString();
        }
        QString s;
        if (parent.isValid())
            s = "parent(" + parent.data(role).toString() + ") " + rowText;
        else
            s = rowText;
        Core::logInfo(s.simplified());
        debugDataset_helper(model, role, model->index(row, 0, parent));
    }
}

bool Utils::isExpanding(const QWidget* w, Qt::Orientation orientation)
{
    Z_CHECK_NULL(w);
    if (orientation == Qt::Horizontal) {
        if (w->sizePolicy().horizontalPolicy() == QSizePolicy::Expanding || w->sizePolicy().horizontalPolicy() == QSizePolicy::MinimumExpanding)
            return true;

    } else if (orientation == Qt::Vertical) {
        if (w->sizePolicy().verticalPolicy() == QSizePolicy::Expanding || w->sizePolicy().verticalPolicy() == QSizePolicy::MinimumExpanding)
            return true;
    }

    if (w->layout() == nullptr)
        return false;

    return isExpandingHelper(w->layout(), orientation);
}

void Utils::getAllIndexes(const QAbstractItemModel* m, QModelIndexList& indexes)
{
    Z_CHECK_NULL(m);
    indexes.clear();
    getAllIndexesHelper(m, QModelIndex(), indexes);
}

int Utils::getAllRowCount(const QAbstractItemModel* m)
{
    int count = 0;
    getAllRowCountHelper(m, QModelIndex(), count);
    return count;
}

QModelIndex Utils::findLastDatasetIndex(const QAbstractItemModel* m, const QModelIndex& parent)
{
    Z_CHECK_NULL(m);

    if (m->rowCount(parent) == 0)
        return QModelIndex();
    else {
        QModelIndex idx = m->index(m->rowCount(parent) - 1, 0, parent);
        QModelIndex res = findLastDatasetIndex(m, idx);
        if (res.isValid())
            return res;
        else
            return idx;
    }
}

QModelIndex Utils::searchItemModel(const QModelIndex& searchFrom, const QVariant& value, bool forward, const I_DatasetVisibleInfo* converter)
{
    QString val = value.toString().trimmed().simplified();

    QModelIndex index = searchFrom;
    while (index.isValid()) {
        QModelIndex idx = index.model()->index(index.row(), searchFrom.column(), index.parent());

        QVariant original_value = idx.data(Qt::DisplayRole);
        QString cell_value = original_value.toString().trimmed().simplified();

        QList<ModelPtr> data_not_ready;
        if (converter != nullptr)
            converter->convertDatasetItemValue(idx, original_value, VisibleValueOption::Application, cell_value,
                data_not_ready); // ошибки игнорируем

        if (data_not_ready.isEmpty()) {
            cell_value = cell_value.trimmed().simplified();
            if ((val.isEmpty() && cell_value.isEmpty()) || (!val.isEmpty() && cell_value.contains(val, Qt::CaseInsensitive)))
                return idx;
        }
        index = getNextItemModelIndex(index, forward);
    }

    return QModelIndex();
}

QModelIndex Utils::getNextItemModelIndex(const QModelIndex& index, bool forward)
{
    if (!index.isValid())
        return QModelIndex();

    const QAbstractItemModel* model = index.model();
    // Индекс в той же строке, но в первой колонке. У него могут быть дочерние узлы
    QModelIndex idx = (index.column() == 0) ? index : model->index(index.row(), 0, index.parent());

    if (forward) { // Поиск "вперед
        if (model->rowCount(idx) > 0) {
            // Если есть дочерние, то выбираем первый дочерний
            return model->index(0, 0, idx);
        } else {
            /* Ищем узел, который не является последним на своем уровне.
             * При этом поднимаемся вверх по иерархии */
            while (idx.isValid()) {
                if (idx.row() < model->rowCount(idx.parent()) - 1) {
                    return model->index(idx.row() + 1, 0, idx.parent());
                }
                idx = idx.parent();
            }
            return QModelIndex();
        }
    } else { // Поиск "назад"
        if (idx.row() > 0) {
            // Если есть предыдущий на этом же уровне, то ищем для него самый нижний и последний в иерархии
            idx = model->index(idx.row() - 1, 0, idx.parent());
            while (model->rowCount(idx) > 0) {
                idx = model->index(model->rowCount(idx) - 1, 0, idx);
            }
            return idx;
        } else {
            // Узел первый на своем уровне - возвращаем родителя
            return idx.parent();
        }
    }
}

QModelIndex Utils::getTopSourceIndex(const QModelIndex& index)
{
    if (!index.isValid())
        return index;

    const QAbstractProxyModel* proxy;
    QModelIndex source_index = index;
    do {
        proxy = qobject_cast<const QAbstractProxyModel*>(source_index.model());
        if (proxy != nullptr)
            source_index = proxy->mapToSource(source_index);

    } while (proxy != nullptr);

    Z_CHECK(source_index.isValid());
    return source_index;
}

QAbstractItemModel* Utils::getTopSourceModel(const QAbstractItemModel* model)
{
    Z_CHECK_NULL(model);
    const QAbstractItemModel* source = model;
    while (true) {
        auto proxy = qobject_cast<const QAbstractProxyModel*>(source);
        if (proxy != nullptr)
            source = proxy->sourceModel();
        else
            return const_cast<QAbstractItemModel*>(source);
    }
    Z_HALT_INT;
    return nullptr;
}

QModelIndex Utils::alignIndexToModel(const QModelIndex& index, const QAbstractItemModel* model)
{
    Z_CHECK(index.isValid());
    Z_CHECK_NULL(model);
    if (index.model() == model)
        return index;

    // индекс имеет модель в предках по прокси?
    const QAbstractItemModel* m = index.model();
    QModelIndex idx = index;
    while (true) {
        if (m == model) {
            Z_CHECK(idx.model() == model);
            return idx;
        }

        const QAbstractProxyModel* proxy = qobject_cast<const QAbstractProxyModel*>(m);
        if (proxy == nullptr)
            break;

        m = proxy->sourceModel();
        idx = proxy->mapToSource(idx);
    }

    // модель имеет индекс в предках по прокси?
    m = model;
    QList<const QAbstractProxyModel*> proxy_chain;
    while (true) {
        if (m == index.model()) {
            // поднимаемся назад по цепочке прокси
            QModelIndex idx = index;
            for (int i = 0; i < proxy_chain.count(); i++) {
                idx = proxy_chain.at(i)->mapFromSource(idx);
                if (!idx.isValid())
                    return QModelIndex(); // отфильтровано
            }

            Z_CHECK(idx.model() == model);
            return idx;
        }
        const QAbstractProxyModel* proxy = qobject_cast<const QAbstractProxyModel*>(m);
        if (proxy == nullptr)
            return QModelIndex();

        m = proxy->sourceModel();
        proxy_chain.prepend(proxy);
    }

    return QModelIndex();
}

static void _textToConsole(const QString& text, bool new_line)
{
    static QMutex mutex;
    QMutexLocker lock(&mutex);

    ConsoleTextStream st;
    st << HtmlTools::plain(text);
    if (new_line)
        st << endl;
    st << flush;
}

void Utils::infoToConsole(const QString& text, bool new_line)
{
    Core::writeToLogStorage(text, InformationType::Information);
    _textToConsole(text, new_line);
}

void Utils::errorToConsole(const QString& text, bool new_line)
{
    Core::writeToLogStorage(text, InformationType::Error);
    _textToConsole(text, new_line);
}

void Utils::errorToConsole(const Error& error, bool new_line)
{
    errorToConsole(error.fullText(), new_line);
}

void Utils::cloneItemModel(const QAbstractItemModel* source, QAbstractItemModel* destination)
{
    cloneItemModel_helper(source, destination, QModelIndex(), QModelIndex());
}

void Utils::itemModelSetRowCount(QAbstractItemModel* model, int count, const QModelIndex& parent)
{
    Z_CHECK_NULL(model);

    int current_count = model->rowCount(parent);
    if (count == current_count)
        return;

    if (count > current_count)
        Z_CHECK(model->insertRows(current_count, count - current_count, parent));
    else
        Z_CHECK(model->removeRows(count, current_count - count, parent));

#if !defined(RELEASE_MODE) && defined(QT_DEBUG)
    if (count != model->rowCount(parent)) {
        qDebug() << count << model->rowCount(parent);
        Z_HALT_INT;
    }
#endif
}

void Utils::itemModelSetColumnCount(QAbstractItemModel* model, int count, const QModelIndex& parent)
{
    Z_CHECK_NULL(model);

    int current_count = model->columnCount(parent);
    if (count == current_count)
        return;

    if (count > current_count)
        Z_CHECK(model->insertColumns(current_count, count - current_count, parent));
    else
        Z_CHECK(model->removeColumns(count, current_count - count, parent));

#if !defined(RELEASE_MODE) && defined(QT_DEBUG)
    if (count != model->columnCount(parent)) {
        qDebug() << count << model->columnCount(parent);
        Z_HALT_INT;
    }
#endif
}

void Utils::selectLineEditAll(QLineEdit* le)
{
    Z_CHECK_NULL(le);
    if (le->text().isEmpty() || (le->selectionLength() == le->text().length() && le->cursorPosition() == 0))
        return;

    // особое извращение путем эмитации нажатия кнопок. Иначе не получится сделать чтобы курсор стоял в начале
    le->setCursorPosition(le->text().length());
    if (le->hasSelectedText())
        le->deselect();

    QKeyEvent* keyEvent = new QKeyEvent(QEvent::KeyPress, Qt::Key_Home, Qt::ShiftModifier);
    QApplication::postEvent(le, keyEvent);
}

int Utils::_wait_count = 0;
void Utils::setWaitCursor(bool processEvents, int callCount)
{
    Z_CHECK(callCount >= 0);

    if (isBlockWaitCursor())
        return;

    if (_wait_count > 0) {
        if (callCount > 0)
            _wait_count = callCount;
        else
            _wait_count++;
        return;
    }

    // Меняем курсор
    QApplication::setOverrideCursor(Qt::WaitCursor);

    if (callCount > 0)
        _wait_count = callCount;
    else
        _wait_count++;

    if (processEvents) {
        /* Без этого курсор может не отобразится.
         * QApplication::processEvents(QEventLoop::ExcludeUserInputEvents) не поможет */
        Utils::processEvents(QEventLoop::AllEvents);
    }
}

void Utils::restoreCursor()
{
    if (_wait_count == 0 || isBlockWaitCursor())
        return;

    _wait_count--;

    if (_wait_count == 0)
        QApplication::restoreOverrideCursor();
}

int Utils::removeWaitCursor()
{
    int count = _wait_count;
    _wait_count = 0;

    if (count > 0)
        QApplication::restoreOverrideCursor();

    return count;
}

bool Utils::isWaitCursor()
{
    return _wait_count > 0;
}

bool Utils::_is_block_wait_cursor = false;
void Utils::blockWaitCursor(bool b)
{
    _is_block_wait_cursor = b;
    if (_is_block_wait_cursor) {
        removeWaitCursor();
    }
}

bool Utils::isBlockWaitCursor()
{
    return _is_block_wait_cursor;
}

QSize Utils::stringSize(const QString& s, bool html_size, int ideal_width, const QFont& font)
{
    if (s.isEmpty())
        return QSize(0, 0);

    if (!html_size) {
        Z_CHECK(ideal_width <= 0);
        return QFontMetrics(font).size(0, s);
    }

    QTextDocument doc;
    doc.setDefaultFont(font);

    double last_ratio = -1;
    double calc_width = -1;
    QSize found_size;
    int count = 0;
    while (true) {
        if (ideal_width > 0)
            doc.setTextWidth(ideal_width);
        else if (calc_width > 0)
            doc.setTextWidth(calc_width);

        if (HtmlTools::isHtml(s))
            doc.setHtml(s);
        else
            doc.setPlainText(s);

        QSizeF size = doc.size();

        if (ideal_width > 0)
            return size.toSize();

        double ratio = size.width() / size.height();

        // смотрим на соотношение ширины и высоты и пытаемся приблизиться идеалу (золотое сечение тут, как выяснилось - не очень)
        static const double golden_ratio = 2;
        static const double target_diff = 0.1;

        double diff = qAbs(ratio - golden_ratio);

        if (diff < target_diff && ratio > golden_ratio)
            return size.toSize();

        if (last_ratio > 0) {
            // перестали приближаться к идеалу
            if (diff >= qAbs(last_ratio - golden_ratio) || ratio < golden_ratio)
                return found_size;
        }

        calc_width = (size.width() + size.height()) / (1.0 + golden_ratio);
        found_size = size.toSize();
        last_ratio = ratio;

        // защита на случай собственной криворукости
        if (count > 100)
            return found_size;
        count++;
    }
}

int Utils::stringWidth(const QString& s, bool html_size, int ideal_width, const QFont& font)
{
    return stringSize(s, html_size, ideal_width, font).width();
}

int Utils::stringHeight(const QString& s, bool html_size, int ideal_width, const QFont& font)
{
    return stringSize(s, html_size, ideal_width, font).height();
}

int Utils::fontHeight(const QFontMetrics& fm)
{
    return fm.ascent() + fm.descent() + fm.leading();
}

int Utils::scaleUI(int base_size)
{
    return zf::round(qMin((double)base_size * (double)fontHeight() / 16.0, (double)QWIDGETSIZE_MAX));
}

QString Utils::alignMultilineString(const QString& s)
{
    QString text = " " + s.trimmed();
    text.replace("\n", "\n ");
    return text;
}

QFont Utils::fontBold(const QFont& f)
{
    QFont font = f;
    font.setBold(true);
    return font;
}

QFont Utils::fontItalic(const QFont& f)
{
    QFont font = f;
    font.setItalic(true);
    return font;
}

QFont Utils::fontUnderline(const QFont& f)
{
    QFont font = f;
    font.setUnderline(true);
    return font;
}

QFont Utils::fontSize(const QFont& f, int newSize)
{
    QFont font = f;
    font.setPointSize(newSize);
    return font;
}

void Utils::changeFontSize(QWidget* w, int increment)
{
    Z_CHECK_NULL(w);
    QFont f = w->font();
    f.setPointSize(f.pointSize() + increment);
    w->setFont(f);
}

QSize Utils::maxPixmapSize(const QIcon& icon)
{
    auto sizes = icon.availableSizes();
    std::sort(sizes.begin(), sizes.end(), [&](const QSize& s1, const QSize& s2) -> bool { return s1.width() * s1.height() < s2.width() * s2.height(); });

    QSize size;
    if (!sizes.isEmpty())
        size = sizes.last();

    return size;
}

QSize Utils::bestPixmapSize(const QIcon& icon, const QSize& target_size)
{
    auto sizes = icon.availableSizes();
    std::sort(sizes.begin(), sizes.end(), [&](const QSize& s1, const QSize& s2) -> bool {
        qint64 s = (qint64)target_size.width() * (qint64)target_size.height();
        qint64 d1 = qAbs(s - (qint64)s1.width() * (qint64)s1.height());
        qint64 d2 = qAbs(s - (qint64)s2.width() * (qint64)s2.height());
        return d1 > d2; // чем размер ближе к size, тем лучше
    });

    QSize size;
    if (!sizes.isEmpty())
        size = sizes.last();
    else
        size = target_size;

    return size;
}

void Utils::chartToPNG(QtCharts::QChart* chart, QIODevice* device, const QSize& output_size, bool transparent)
{
    Z_CHECK_NULL(device);
    Z_CHECK(device->isOpen() && device->isWritable());

    chartToImage(chart, output_size, transparent).save(device);
}

void Utils::chartToSVG(QtCharts::QChart* chart, QIODevice* device, const QSize& output_size, bool transparent)
{
    Z_CHECK_NULL(chart);
    Z_CHECK_NULL(device);
    Z_CHECK(device->isOpen() && device->isWritable());

    QRectF output_rect(QPointF(0, 0), QSizeF(output_size));

    QSvgGenerator svg;
    svg.setOutputDevice(device);
    svg.setTitle(chart->title());
    svg.setSize(output_size);
    svg.setViewBox(output_rect);

    QBrush old_brush;
    if (transparent) {
        old_brush = chart->backgroundBrush();
        chart->setBackgroundBrush(QBrush(Qt::NoBrush));
    }

    auto original_size = chart->size();
    chart->resize(output_rect.size());

    QPainter painter;
    painter.begin(&svg);

    chart->scene()->render(&painter, output_rect, output_rect, Qt::IgnoreAspectRatio);
    painter.end();

    chart->resize(original_size);
    if (transparent)
        chart->setBackgroundBrush(old_brush);
}

QImage Utils::chartToImage(QtCharts::QChart* chart, const QSize& output_size, bool transparent)
{
    Z_CHECK_NULL(chart);

    QRectF output_rect(QPointF(0, 0), QSizeF(output_size));

    QImage image(output_size, QImage::Format_ARGB32);
    image.fill(Qt::transparent);

    QBrush old_brush;
    if (transparent) {
        old_brush = chart->backgroundBrush();
        chart->setBackgroundBrush(QBrush(Qt::NoBrush));
    }

    auto original_size = chart->size();
    chart->resize(output_rect.size());

    QPainter painter;
    painter.begin(&image);

    painter.setRenderHint(QPainter::Antialiasing);

    chart->scene()->render(&painter, output_rect, output_rect, Qt::IgnoreAspectRatio);
    painter.end();

    chart->resize(original_size);
    if (transparent)
        chart->setBackgroundBrush(old_brush);

    return image;
}

void Utils::chartToPDF(QPainter* painter, QtCharts::QChart* chart, QPdfWriter* device, const QRectF& target_rect, bool transparent)
{
    Z_CHECK_NULL(painter);
    Z_CHECK_NULL(chart);
    Z_CHECK_NULL(device);

    QBrush old_brush;
    if (transparent) {
        old_brush = chart->backgroundBrush();
        chart->setBackgroundBrush(QBrush(Qt::NoBrush));
    }

    qreal pdf_dpi = device->logicalDpiX();
    qreal screen_dpi = QApplication::primaryScreen()->logicalDotsPerInch();
    qreal scaling = pdf_dpi / screen_dpi;

    QSizeF old_size = chart->size();
    QRectF input_rect(0, 0, target_rect.width() / scaling, target_rect.height() / scaling);
    chart->resize(input_rect.size());

    if (painter->isActive())
        Z_CHECK(painter->device() == device);
    else
        painter->begin(device);

    chart->scene()->render(painter, target_rect, input_rect, Qt::IgnoreAspectRatio);

    chart->resize(old_size);
    if (transparent)
        chart->setBackgroundBrush(old_brush);
}

QImage Utils::scalePicture(const QImage& img, int max_width, int max_height, bool fill)
{
    return img.scaled(max_width, max_height, fill ? Qt::KeepAspectRatioByExpanding : Qt::KeepAspectRatio, Qt::SmoothTransformation);
}

QImage Utils::scalePicture(const QImage& img, const QSize& max_size, bool fill)
{
    return scalePicture(img, max_size.width(), max_size.height(), fill);
}

QPixmap Utils::scalePicture(const QPixmap& img, int max_width, int max_height, bool fill)
{
    return img.scaled(max_width, max_height, fill ? Qt::KeepAspectRatioByExpanding : Qt::KeepAspectRatio, Qt::SmoothTransformation);
}

QPixmap Utils::scalePicture(const QPixmap& img, const QSize& max_size, bool fill)
{
    return scalePicture(img, max_size.width(), max_size.height(), fill);
}

QByteArray Utils::pictureToByteArray(const QImage& img, FileType format)
{
    QByteArray data;
    QBuffer buffer(&data);
    buffer.open(QIODevice::WriteOnly);
    img.save(&buffer, format != FileType::Undefined ? fileExtensions(format).constFirst().toLocal8Bit().constData() : nullptr);
    buffer.close();
    return data;
}

QByteArray Utils::pictureToByteArray(const QPixmap& img, FileType format)
{
    return pictureToByteArray(img.toImage(), format);
}

QImage Utils::generatePreview(const QByteArray& data, const QSize& size, FileType format)
{
    if (format == FileType::Pdf) {
        QBuffer buffer;
        buffer.setData(data);
        buffer.open(QIODevice::ReadOnly);
        QPdfDocument pdf_doc;
        pdf_doc.load(&buffer);
        if (pdf_doc.error() == QPdfDocument::NoError) {
            QPdfDocumentRenderOptions opt;
            opt.setRenderFlags(QPdf::RenderFlags(QPdf::RenderOptimizedForLcd | QPdf::RenderTextAliased | QPdf::RenderImageAliased | QPdf::RenderPathAliased));
            return generatePreview(pdf_doc.render(0, size, opt), size);
        }
    }

    return generatePreview(QImage::fromData(data), size);
}

QImage Utils::generatePreview(const QImage& image, const QSize& size)
{
    if (image.isNull())
        return QImage();

    QSize target_size;
    if (image.width() > image.height()) {
        target_size.setWidth(qMax(size.width(), size.height()));
        target_size.setHeight(qMin(size.width(), size.height()));

    } else {
        target_size.setWidth(qMin(size.width(), size.height()));
        target_size.setHeight(qMax(size.width(), size.height()));
    }

    QImage preview_image = Utils::scalePicture(image, target_size);

    QImage target_image(preview_image.size(), preview_image.format());
    target_image.fill(Qt::white);

    QPainter painter(&target_image);
    painter.drawImage(0, 0, preview_image);

    return target_image;
}

QIcon Utils::fileTypePreviewIcon(FileType type)
{
    QString image;

    if (isImage(type, false)) {
        image = ":/share_icons/picture.svg";

    } else {
        switch (type) {
            case FileType::Pdf:
                image = ":/share_icons/pdf_format.svg";
                break;
            case FileType::Xls:
            case FileType::Xlsx:
                image = ":/share_icons/blue/xls_format.svg";
                break;
            case FileType::Doc:
            case FileType::Docx:
                image = ":/share_icons/doc_format.svg";
                break;
            case FileType::Json:
                image = ":/share_icons/json_format.svg";
                break;
            case FileType::Xml:
                image = ":/share_icons/xml_format.svg";
                break;
            default:
                image = ":/share_icons/document.svg";
                break;
        }
    }

    return QIcon(image);
}

QString Utils::putInQuotes(const QString& v, const QChar& quote)
{
    QString val = v;
    return quote + val.replace(quote, quote + quote) + quote;
}

QString Utils::removeQuotes(const QString& v, const QChar& quote)
{
    QString val = v;
    return val.replace(quote + quote, quote).simplified();
}

QStringList Utils::extractFromDelimeted(const QString& v, const QChar& quote, const QChar& delimeter, bool remove_internal_double_quotes)
{
    QStringList data;

    if (!v.contains(quote))
        // Нет двойных ковычек - можно разбить сепаратором
        data = v.split(delimeter, QString::KeepEmptyParts);

    else {
        // Ищем ячейки, закрытые  ковычками
        bool openFound = false;
        QList<QPair<int, int>> positions; // Начальная и конечная позиция ячеек закрытых двойными кавычками
        QPair<int, int> current;
        for (int i = 0; i < v.length(); i++) {
            if (openFound) {
                // В состоянии поиска закрывающей
                if (v.at(i) != quote)
                    continue;

                // Найден претендент на закрывающую двойную ковычку

                if (i < v.length() - 1 && v.at(i + 1) == quote) {
                    // Это дублирующая двойная кавычка. Игнорируем
                    i++;
                    continue;
                }

                if (i == v.length() - 1 || // Конец строки
                    v.at(i + 1) == delimeter) { // Разделитель
                    // Это закрывающая
                    current.second = i;
                    positions << current;
                    openFound = false;
                    continue;
                }

            } else if (v.at(i) == quote) {
                // Ковычка открывается
                openFound = true;
                current.first = i;
            }
        }

        // Формируем окончательный список
        int shift = 0;
        for (int i = 0; i < positions.count(); i++) {
            QString leftLine = v.mid(shift, positions.at(i).first - shift).trimmed();

            if (!leftLine.isEmpty())
                // Удаляем сепаратор справа
                leftLine.remove(leftLine.length() - 1, 1);

            if (!leftLine.isEmpty()) {
                // Строка слева не содержит внутри открывающих и закрывающих двойных ковычек, а значит и
                // сепараторов (в виде текста) внутри ячеек
                QStringList leftSplitted = leftLine.split(delimeter, QString::KeepEmptyParts);
                data << leftSplitted;
            }

            // Добавляем кусок закрытый двойными ковычками
            data << v.mid(positions.at(i).first + 1, positions.at(i).second - positions.at(i).first - 1);

            shift = positions.at(i).second + 1;
        }

        // Остаток справа
        if (positions.last().second < v.length() - 1) {
            QString rightLine = v.right(v.length() - positions.last().second - 1);

            if (!rightLine.isEmpty())
                // Удаляем сепаратор слева
                rightLine.remove(0, 1);

            if (!rightLine.isEmpty()) {
                QStringList rightSplitted = rightLine.split(delimeter, QString::KeepEmptyParts);
                data << rightSplitted;
            }
        }
    }

    if (remove_internal_double_quotes) {
        for (auto i = data.begin(); i != data.end(); ++i) {
            *i = removeQuotes(*i, quote);
        }
    }

    return data;
}

QVariant::Type Utils::dataTypeToVariant(DataType t)
{
    switch (t) {
        case DataType::Undefined:
        case DataType::Table:
        case DataType::Tree:
        case DataType::Uid:
        case DataType::Variant:
            return QVariant::Invalid;

        case DataType::String:
        case DataType::Memo:
        case DataType::RichMemo:
            return QVariant::String;

        case DataType::Integer:
            return QVariant::LongLong;

        case DataType::UInteger:
            return QVariant::ULongLong;

        case DataType::Float:
        case DataType::UFloat:
            return QVariant::Double;

        case DataType::Numeric:
        case DataType::UNumeric:
            return QVariant::String;

        case DataType::Date:
            return QVariant::Date;

        case DataType::DateTime:
            return QVariant::DateTime;

        case DataType::Time:
            return QVariant::Time;

        case DataType::Bool:
            return QVariant::Bool;

        case DataType::Image:
        case DataType::Bytes:
            return QVariant::ByteArray;

        default:
            return QVariant::Invalid;
    }
}

DataType Utils::dataTypeFromVariant(QVariant::Type t)
{
    switch (t) {
        case QVariant::Invalid:
            return DataType::Undefined;
        case QVariant::String:
            return DataType::String;
        case QVariant::Int:
        case QVariant::LongLong:
            return DataType::Integer;
        case QVariant::UInt:
        case QVariant::ULongLong:
            return DataType::UInteger;
        case QVariant::Double:
            return DataType::Float;
        case QVariant::Date:
            return DataType::Date;
        case QVariant::DateTime:
            return DataType::DateTime;
        case QVariant::Time:
            return DataType::Time;
        case QVariant::Bool:
            return DataType::Bool;
        case QVariant::ByteArray:
            return DataType::Bytes;

        default:
            return DataType::Undefined;
    }
}

bool Utils::dataTypeCanConvert(DataType from, DataType to, bool allow_int_to_bool)
{
    Z_CHECK(from != DataType::Undefined);
    Z_CHECK(to != DataType::Undefined);

    switch (from) {
        case DataType::Table:
        case DataType::Tree:
        case DataType::Uid:
        case DataType::Variant:
            return to == from;

        case DataType::String:
        case DataType::Memo:
        case DataType::RichMemo:
            return to == DataType::String || to == DataType::Memo || to == DataType::RichMemo;

        case DataType::Integer:
        case DataType::UInteger:
        case DataType::Numeric:
        case DataType::UNumeric: {
            if (to == DataType::Bool && allow_int_to_bool)
                return true;

            return to == DataType::String || to == DataType::Integer || to == DataType::UInteger || to == DataType::Numeric || to == DataType::UNumeric
                   || to == DataType::Float || to == DataType::UFloat;
        }

        case DataType::Float:
        case DataType::UFloat:
            return to == DataType::String || to == DataType::Float || to == DataType::UFloat;

        case DataType::Date:
            return to == DataType::String || to == DataType::Date || to == DataType::DateTime;

        case DataType::Time:
            return to == DataType::String || to == DataType::Time;

        case DataType::DateTime:
            return to == DataType::String || to == DataType::DateTime;

        case DataType::Bool:
            return to == DataType::Bool || to == DataType::Integer || to == DataType::UInteger || to == DataType::Numeric || to == DataType::UNumeric;

        case DataType::Image:
        case DataType::Bytes:
            return to == DataType::Image || to == DataType::Bytes;

        default:
            Z_HALT_INT;
            return false;
    }
}

QString Utils::dataTypeToString(DataType t)
{
    return qtEnumToString(t);
}

// взято из QVariant::load
QVariant Utils::variantFromSerialized(const QString& value, Error& error)
{
    QVariant res;
    QByteArray ba = QByteArray::fromBase64(value.toUtf8());
    QDataStream s(ba);
    s.setVersion(Consts::DATASTREAM_VERSION);

    quint32 typeId;
    s >> typeId;

    qint8 is_null = false;
    s >> is_null;

    if (typeId == QVariant::UserType) {
        QByteArray name;
        s >> name;
        typeId = QMetaType::type(name.constData());
        if (typeId == QMetaType::UnknownType) {
            error = Error(QStringLiteral("variantFromSerialized unknown user type with name %1").arg(QString::fromUtf8(name)));
            return QVariant();
        }
    }

    res = QVariant(static_cast<QVariant::Type>(typeId));
    if (!res.isValid())
        return QVariant();

    if (!QMetaType::load(s, res.type(), const_cast<void*>(res.constData()))) {
        error = Error(QStringLiteral("variantFromSerialized unable to load type %1.").arg(res.type()));
        return QVariant();
    }

    error.clear();
    return res;
}

// взято из QVariant::save
QString Utils::variantToSerialized(const QVariant& value, Error& error)
{
    quint32 typeId = value.type();
    QByteArray ba;
    QDataStream s(&ba, QIODevice::WriteOnly);
    s.setVersion(Consts::DATASTREAM_VERSION);

    s << typeId;
    s << qint8(value.isNull());
    if (value.type() >= QVariant::UserType)
        s << QMetaType::typeName(value.userType());

    if (value.isValid()) {
        if (!QMetaType::save(s, value.type(), value.constData())) {
            error
                = Error(QStringLiteral("variantToSerialized unable to save type '%1' (type id: %2)").arg(QMetaType::typeName(value.type())).arg(value.type()));
            return QString();
        }
    }

    error.clear();
    return QString::fromUtf8(ba.toBase64());
}

QByteArray Utils::iconToByteArray(const QIcon& icon)
{
    if (icon.isNull())
        return QByteArray();

    QByteArray ba;
    QDataStream s(&ba, QIODevice::WriteOnly);
    s.setVersion(Consts::DATASTREAM_VERSION);
    s << icon;
    return ba;
}

QIcon Utils::iconFromByteArray(const QByteArray& ba, Error& error)
{
    error.clear();

    if (ba.isEmpty())
        return QIcon();

    QIcon icon;
    QDataStream s(ba);
    s.setVersion(Consts::DATASTREAM_VERSION);
    s >> icon;

    if (s.status() != QDataStream::Ok)
        error = Error::corruptedDataError();

    return icon;
}

QIcon Utils::resizeSvg(const QString& svg_file, const QSize& size, bool use_png, const QString& instance_key)
{
    return _svg_cache.object(svg_file, size, use_png, instance_key);
}

QIcon Utils::resizeSvg(const QString& svg_file, int size, bool use_png, const QString& instance_key)
{
    return _svg_cache.object(svg_file, size, use_png, instance_key);
}

QString Utils::resizeSvgFileName(const QString& svg_file, const QSize& size, bool use_png, const QString& instance_key)
{
    return _svg_cache.fileName(svg_file, size, use_png, instance_key);
}

QString Utils::resizeSvgFileName(const QString& svg_file, int size, bool use_png, const QString& instance_key)
{
    return _svg_cache.fileName(svg_file, size, use_png, instance_key);
}

QIcon Utils::buttonIcon(const QString& svg_file, bool use_png, const QString& instance_key)
{
    return resizeSvg(svg_file, qApp->style()->pixelMetric(QStyle::PM_ButtonIconSize), use_png, instance_key);
}

QIcon Utils::toolButtonIcon(const QString& svg_file, ToolbarType toolbar_type, bool use_png, const QString& instance_key)
{
    return resizeSvg(svg_file, UiSizes::defaultToolbarIconSize(toolbar_type), use_png, instance_key);
}

QByteArray Utils::resizeSvgToByteArray(const QString& svg_file, const QSize& size, const QString& instance_key)
{
    return loadByteArray(resizeSvgFileName(svg_file, size, true, instance_key));
}

QSize Utils::screenSize(const QPoint& screen_point)
{
    return screenRect(screen_point).size();
}

QSize Utils::screenSize(const QWidget* w, Qt::Alignment alignment)
{
    return screenRect(w, alignment).size();
}

QRect Utils::screenRect(const QPoint& screen_point)
{
    QPoint pos = screen_point.isNull() ? QCursor::pos() : screen_point;

#if QT_VERSION < QT_VERSION_CHECK(5, 10, 0)
    QDesktopWidget dw;
    QRect rect = dw.availableGeometry(dw.screenNumber(pos));
#else
    QScreen* screen = QGuiApplication::screenAt(pos);
    if (screen == nullptr)
        return {};

    QRect rect = screen->availableGeometry();
#endif

    return rect;
}

QRect Utils::screenRect(const QWidget* w, Qt::Alignment alignment)
{
    Z_CHECK_NULL(w);

    enum
    {
        Center,
        TopRight,
        BottomRight,
        TopLeft,
        BottomLeft,
    };

    QList<int> check;

#define ALIGN_CHECK(x) (alignment & (x)) == (x)

    if (ALIGN_CHECK(Qt::AlignCenter))
        check = {Center, TopRight, BottomRight, TopLeft, BottomLeft};
    else if (ALIGN_CHECK(Qt::AlignTop | Qt::AlignRight))
        check = {TopRight, BottomRight, TopLeft, BottomLeft, Center};
    else if (ALIGN_CHECK(Qt::AlignBottom | Qt::AlignRight))
        check = {BottomRight, TopRight, BottomLeft, TopLeft, Center};
    else if (ALIGN_CHECK(Qt::AlignTop | Qt::AlignLeft))
        check = {TopLeft, BottomLeft, TopRight, BottomRight, Center};
    else if (ALIGN_CHECK(Qt::AlignBottom | Qt::AlignLeft))
        check = {BottomLeft, TopLeft, BottomRight, TopRight, Center};
    else
        check = {Center, TopRight, BottomRight, TopLeft, BottomLeft};

    QRect screen;
    for (auto c : qAsConst(check)) {
        if (c == Center)
            screen = Utils::screenRect(w->mapToGlobal(w->rect().center()));
        else if (c == TopRight)
            screen = Utils::screenRect(w->mapToGlobal(w->rect().topRight()));
        else if (c == BottomRight)
            screen = Utils::screenRect(w->mapToGlobal(w->rect().bottomRight()));
        else if (c == TopLeft)
            screen = Utils::screenRect(w->mapToGlobal(w->rect().topLeft()));
        else if (c == BottomLeft)
            screen = Utils::screenRect(w->mapToGlobal(w->rect().bottomLeft()));
        else
            Z_HALT_INT;

        if (!screen.isNull())
            break;
    }

    if (screen.isNull())
        screen = Utils::screenRect();

    return screen;
}

QJsonValue Utils::variantToJsonValue(const QVariant& value, DataType data_type)
{
    if (value.isNull() || !value.isValid())
        return QJsonValue();

    QVariant::Type v_type = (data_type == DataType::Undefined ? value.type() : dataTypeToVariant(data_type));
    if (v_type == QVariant::Invalid) {
        // нестандартный тип
        if (QMetaType::isRegistered(value.type())) {
            // зарегистрирован, надеемся что также имеется регистрация через qRegisterMetaTypeStreamOperators
            QByteArray data;
            QDataStream st(&data, QIODevice::WriteOnly);
            st.setVersion(Consts::DATASTREAM_VERSION);
            st << value;
            if (st.status() != QDataStream::Ok) {
                Core::logError(QStringLiteral("variantToJsonValue: unsupported variant %1").arg(value.type()));
                return value.toString();
            }

            return QString::fromUtf8(data.toBase64());

        } else {
            Core::logError(QStringLiteral("variantToJsonValue: unsupported variant %1").arg(value.type()));
            v_type = QVariant::String;
        }
    }

    QVariant converted;

    if (Numeric::isNumeric(value)) {
        converted = value.value<Numeric>().toString(Core::fr()->c_locale());

    } else {
        converted = value;
        if (v_type != QVariant::Invalid && !converted.convert(v_type))
            converted = value;
    }

    switch (converted.type()) {
        case QVariant::Bool:
            return QJsonValue(value.toBool());
            break;
        case QVariant::Int:
        case QVariant::UInt:
        case QVariant::LongLong:
        case QVariant::ULongLong:
            return QJsonValue(value.toLongLong());

        case QVariant::Double:
            return QJsonValue(value.toDouble());

        case QVariant::Char:
        case QVariant::String:
            return QJsonValue(value.toString());

        case QVariant::Date:
            return QJsonValue(value.toDate().toString(Qt::ISODate));

        case QVariant::Time:
            return QJsonValue(value.toTime().toString(Qt::ISODate));

        case QVariant::DateTime:
            return QJsonValue(value.toDateTime().toString(Qt::ISODate));

        case QVariant::ByteArray:
            return QJsonValue(QString::fromUtf8(value.toByteArray().toBase64()));

        default:
            Core::logError(QStringLiteral("variantToJsonValue - unknown type %1").arg(converted.type()));
            return QJsonValue();
    }
}

QVariant Utils::variantFromJsonValue(const QJsonValue& value, DataType data_type)
{
    if (value.isNull() || value.isUndefined())
        return QVariant();

    if (data_type == DataType::Undefined)
        return value.toVariant();

    switch (data_type) {
        case DataType::Bool:
            return value.toBool();

        case DataType::Integer:
        case DataType::UInteger:
            return value.toInt();

        case DataType::Float:
        case DataType::UFloat:
            return value.toDouble();

        case DataType::Numeric:
        case DataType::UNumeric:
            return QVariant::fromValue(Numeric::fromString(value.toString(), nullptr, Core::fr()->c_locale()));

        case DataType::String:
            return value.toString();

        case DataType::Date:
            return QDate::fromString(value.toString(), Qt::ISODate);

        case DataType::DateTime:
            return QDateTime::fromString(value.toString(), Qt::ISODate);

        case DataType::Time:
            return QTime::fromString(value.toString(), Qt::ISODate);

        case DataType::Image:
            return QByteArray::fromBase64(value.toString().toUtf8());

        default:
            // надеемся что также имеется регистрация через qRegisterMetaTypeStreamOperators
            auto data = QByteArray::fromBase64(value.toString().toUtf8());
            QVariant v;
            QDataStream st(data);
            st.setVersion(Consts::DATASTREAM_VERSION);
            st >> v;
            if (st.status() != QDataStream::Ok) {
                Core::logError(QStringLiteral("variantFromJsonValue - unknown type %1").arg(static_cast<int>(data_type)));
                return value.toString();
            }
            return v;
    }
}

QString Utils::variantToString(const QVariant& value, LocaleType lang_type, int max_list_count)
{
    return variantToStringHelper(value, Core::locale(lang_type), max_list_count);
}

QString Utils::variantToString(const QVariant& value, QLocale::Language language, int max_list_count)
{
    QLocale locale(language);
    return variantToStringHelper(value, &locale, max_list_count);
}

QVariant Utils::variantFromString(const QString& value, LocaleType lang_type)
{
    return variantFromStringHelper(value, Core::locale(lang_type));
}

QVariant Utils::variantFromString(const QString& value, QLocale::Language language)
{
    QLocale locale(language);
    return variantFromStringHelper(value, &locale);
}

QString Utils::uidsPropsToString(const UidList& uids, const QList<DataPropertySet>& props)
{
    QString text;
    for (int i = 0; i < uids.count(); i++) {
        if (!text.isEmpty())
            text += ", ";
        QString props_text;
        if (!props.isEmpty() && i < props.count() && !props.at(i).isEmpty()
            && props.at(i).count() != DataStructure::structure(uids.at(i))->propertiesMain().count()) {
            props_text = Utils::containerToString(props.at(i), 20);
        } else {
            props_text = QStringLiteral("all");
        }

        text += QStringLiteral("%1 [%2]").arg(uids.at(i).toPrintable(), props_text);
    }

    return text;
}

Error Utils::convertValue(const QVariant& value, DataType type, const QLocale* locale_from, ValueFormat format_to, const QLocale* locale_to, QVariant& result)
{
    Z_CHECK(format_to != ValueFormat::Undefined);

    Error error;
    result.clear();

    if (value.isNull() || !value.isValid())
        return error;

    if (InvalidValue::isInvalidValueVariant(value)) {
        // некорректное значение оставляем как есть
        result = value;
        return {};
    }

    if (locale_from == nullptr)
        locale_from = Core::locale(LocaleType::UserInterface);
    if (locale_to == nullptr)
        locale_to = Core::locale(LocaleType::UserInterface);

    QVariant::Type t = value.type();

    Uid* uid = nullptr;
    QVariant* uid_value = nullptr;
    if (Uid::isUidVariant(value)) {
        uid = new Uid(Uid::fromVariant(value));
        if (!uid->isValid()) {
            delete uid;
            return error;
        }

        // если на вход идет простой Uid, то возможно нам надо просто вытащить из него значение
        if (uid->isGeneral() && uid->count() == 1) {
            QVariant::Type v_type = uid->asVariant(0).type();
            if (v_type == QVariant::String || v_type == QVariant::Int || v_type == QVariant::UInt || v_type == QVariant::LongLong
                || v_type == QVariant::ULongLong)
                uid_value = new QVariant(uid->asVariant(0));
        }
    }

    switch (type) {
        case DataType::String:
        case DataType::Memo:
        case DataType::RichMemo: {
            if (uid != nullptr) {
                if (uid_value != nullptr)
                    result = *uid_value;
                else
                    result = uid->serialize();

            } else {
                result = (t == QVariant::String ? value : variantToStringHelper(value, locale_to, -1));
            }
            break;
        }
        case DataType::UInteger:
        case DataType::Integer: {
            if (t == QVariant::Int || t == QVariant::UInt || t == QVariant::LongLong || t == QVariant::ULongLong) {
                result = value;

            } else if (Numeric::isNumeric(value)) {
                result = static_cast<qlonglong>(value.value<Numeric>().toDouble());

            } else {
                const QVariant* value_to_convert;
                if (uid_value != nullptr)
                    value_to_convert = uid_value;
                else
                    value_to_convert = &value;

                bool b;
                QVariant v;
                if (t == QVariant::String) {
                    QString val_str = value_to_convert->toString().trimmed();
                    if (val_str.isEmpty()) {
                        b = true;
                        v = QVariant();
                    } else {
                        v = locale_to->toLongLong(val_str, &b);
                    }
                } else {
                    v = value_to_convert->toLongLong(&b);
                }
                if (b)
                    result = v;
                else
                    error = Error::conversionError(ZF_TR(ZFT_BAD_VALUE) + ": <b>" + variantToString(*value_to_convert) + "</b>");
            }
            break;
        }
        case DataType::Bool: {
            if (t == QVariant::Bool)
                result = value;
            else {
                if (t == QVariant::String) {
                    QString s = value.toString().trimmed();
                    if (comp(s, Z_STRING_LITERAL("да")) || comp(s, Z_STRING_LITERAL("истина")) || comp(s, QStringLiteral("yes"))
                        || comp(s, QStringLiteral("true")))
                        result = true;
                    if (comp(s, Z_STRING_LITERAL("нет")) || comp(s, Z_STRING_LITERAL("ложь")) || comp(s, QStringLiteral("no"))
                        || comp(s, QStringLiteral("false")))
                        result = false;
                } else if (t == QVariant::Int || t == QVariant::UInt || t == QVariant::LongLong || t == QVariant::ULongLong)
                    result = (value.toLongLong() == 0) ? false : true;
            }

            if (!result.isValid())
                error = Error::conversionError(ZF_TR(ZFT_BAD_VALUE) + ": <b>" + variantToString(value) + "</b>");
            break;
        }
        case DataType::Float:
        case DataType::UFloat: {
            if (t == QVariant::Double) {
                result = value;

            } else if (Numeric::isNumeric(value)) {
                result = (double)value.value<Numeric>().toDouble();

            } else {
                const QVariant* value_to_convert;
                if (uid_value != nullptr)
                    value_to_convert = uid_value;
                else
                    value_to_convert = &value;

                bool b;
                QVariant v;
                if (t == QVariant::String) {
                    QString val_str = value_to_convert->toString().trimmed();
                    if (val_str.isEmpty()) {
                        b = true;
                        v = QVariant();
                    } else {
                        v = locale_to->toDouble(val_str, &b);
                    }
                } else {
                    v = value_to_convert->toDouble(&b);
                }
                if (b)
                    result = v;
                else
                    error = Error::conversionError(ZF_TR(ZFT_BAD_VALUE) + ": <b>" + variantToString(*value_to_convert) + "</b>");
            }
            break;
        }
        case DataType::Numeric:
        case DataType::UNumeric: {
            if (t == QVariant::String && value.toString().simplified().trimmed().isEmpty()) {
                result = QVariant();
                break;
            }

            const QVariant* value_to_convert;
            if (uid_value != nullptr)
                value_to_convert = uid_value;
            else
                value_to_convert = &value;

            if (format_to == ValueFormat::Internal) {
                if (Numeric::isNumeric(value)) {
                    result = value;

                } else {
                    bool b;
                    Numeric v;
                    if (t == QVariant::Double)
                        v = Numeric::fromDouble(value_to_convert->toDouble(&b), Consts::DOUBLE_DECIMALS);
                    else if (t == QVariant::Int || t == QVariant::UInt || t == QVariant::LongLong || t == QVariant::ULongLong)
                        v = Numeric(value_to_convert->toLongLong(&b), Consts::DOUBLE_DECIMALS);
                    else
                        v = Numeric::fromString(value_to_convert->toString(), &b, locale_from, Consts::DOUBLE_DECIMALS);

                    if (b)
                        result = QVariant::fromValue(v);
                    else
                        error = Error::conversionError(ZF_TR(ZFT_BAD_VALUE) + ": <b>" + variantToString(*value_to_convert) + "</b>");
                }

            } else if (format_to == ValueFormat::Edit) {
                if (Numeric::isNumeric(value)) {
                    result = value.value<Numeric>().toString(locale_to);

                } else {
                    result = value.toString();
                }

            } else if (format_to == ValueFormat::Calculate) {
                if (Numeric::isNumeric(value)) {
                    result = (double)value.value<Numeric>().toDouble();

                } else {
                    bool b;
                    double v;
                    if (t == QVariant::String)
                        v = locale_from->toDouble(value_to_convert->toString(), &b);
                    else
                        v = value_to_convert->toDouble(&b);

                    if (b)
                        result = v;
                    else
                        error = Error::conversionError(ZF_TR(ZFT_BAD_VALUE) + ": <b>" + variantToString(*value_to_convert) + "</b>");
                }

            } else {
                Z_HALT_INT;
            }
            break;
        }
        case DataType::Date: {
            QDate date;
            if (value.type() == QVariant::Date)
                date = value.toDate();
            else if (value.type() == QVariant::DateTime)
                date = value.toDateTime().date();
            else
                date = locale_from->toDate(value.toString());

            result = (!date.isNull() && date.isValid()) ? QVariant(date) : QVariant();
            break;
        }
        case DataType::DateTime: {
            QDateTime date_time;
            if (value.type() == QVariant::Date)
                date_time = QDateTime(value.toDate(), QTime());
            else if (value.type() == QVariant::DateTime)
                date_time = value.toDateTime();
            else
                date_time = locale_from->toDateTime(value.toString());

            result = (!date_time.isNull() && date_time.isValid()) ? QVariant(date_time) : QVariant();
            break;
        }
        case DataType::Time: {
            QTime time;
            if (value.type() == QVariant::Date)
                time = QTime();
            else if (value.type() == QVariant::DateTime)
                time = value.toDateTime().time();
            else if (value.type() == QVariant::Time)
                time = value.toTime();
            else
                time = locale_from->toTime(value.toString());

            result = (!time.isNull() && time.isValid()) ? QVariant(time) : QVariant();
            break;
        }
        case DataType::Uid: {
            if (uid != nullptr) {
                result = value;

            } else {
                // создаем Uid общего типа на основании строкового или целого значения
                if (value.type() == QVariant::String)
                    result = Uid::general(value.toString()).variant();
                else if (value.type() == QVariant::Int || value.type() == QVariant::UInt || value.type() == QVariant::LongLong
                         || value.type() == QVariant::ULongLong)
                    result = Uid::general(value.toLongLong()).variant();
            }

            break;
        }
        case DataType::Bytes: {
            if (t == QVariant::String)
                result = Utils::variantFromSerialized(value.toString(), error);
            else
                result = value.toByteArray();

            break;
        }
        case DataType::Image: {
            if (t == QVariant::Icon)
                result = value;
            else if (t == QVariant::ByteArray)
                result = value;
            else if (t == QVariant::Pixmap)
                result = QIcon(qvariant_cast<QPixmap>(value));
            else if (t == QVariant::String)
                result = Utils::variantFromSerialized(value.toString(), error);

            break;
        }
        case DataType::Variant:
            result = value;
            break;

        default: {
            Z_HALT_INT;
            break;
        }
    }

    if (uid != nullptr) {
        delete uid;
        if (uid_value != nullptr)
            delete uid_value;
    }

    if (error.isOk()) {
        // Проверка значения на допустимость с точки зрения сервера
        if (!checkVariantSrv(result)) {
            QString str_value = variantToString(result);
            if (str_value.isEmpty())
                error = Error::conversionError(ZF_TR(ZFT_BAD_VALUE));
            else
                error = Error::conversionError(ZF_TR(ZFT_BAD_VALUE) + ": " + variantToString(result));
        }
    }

    if (error.isError())
        result.clear();

    return error;
}

bool Utils::checkVariantSrv(const QVariant& value)
{
    //TODO нужен интерфейс проверки, а не хардкодить здесь
    if (value.isNull() || !value.isValid())
        return true;

    if (value.type() == QVariant::Date || value.type() == QVariant::DateTime) {
        auto date = value.toDate();
        return date >= QDate(1900, 1, 1) && date <= QDate(2100, 12, 31);
    }

    return true;
}

QString Utils::variantToStringHelper(const QVariant& value, const QLocale* locale, int max_list_count)
{
    // в идеале надо писать конверторы через QMetaType::registerConverter, а не расширять этот метод

    if (!value.isValid() || value.isNull())
        return QString();

    if (value.userType() == Numeric::metaType())
        return value.value<Numeric>().toString(locale);

    if (value.userType() == DataProperty::metaType())
        return value.value<DataProperty>().toPrintable();

    if (Uid::isUidVariant(value))
        return Uid::fromVariant(value).toPrintable();

    if (Uid::isUidListVariant(value))
        return containerToString(Uid::fromVariantList(value));

    if (InvalidValue::isInvalidValueVariant(value))
        return variantToStringHelper(InvalidValue::fromVariant(value).value(), locale, max_list_count);

    if (Numeric::isNumeric(value)) {
        return Numeric::fromVariant(value).toString(locale);
    }

    if (value.userType() == FlatItemModel::metaType()) {
        //можно вывести содержимое таблицы, хотя зачем?
        return QStringLiteral("dataset");
    }

    switch (value.type()) {
        case QVariant::StringList:
            return containerToString(value.toStringList(), max_list_count);
        case QVariant::List:
            return containerToString(value.toList(), max_list_count);

        case QVariant::Time:
            return locale->toString(value.toTime(), QLocale::ShortFormat);

        case QVariant::Date:
            return locale->toString(value.toDate(), QLocale::ShortFormat);

        case QVariant::DateTime:
            return locale->toString(value.toDateTime(), QLocale::ShortFormat);

        case QVariant::ULongLong:
        case QVariant::UInt:
            return locale->toString(value.toULongLong());

        case QVariant::LongLong:
        case QVariant::Int:
            return locale->toString(value.toLongLong());

        case QVariant::Double:
            return locale->toString(value.toDouble(), 'f', Consts::DOUBLE_DECIMALS);

        case QVariant::String:
        case QVariant::Char:
            return value.toString();

        default: {
            if (auto list = value.value<QList<int>>(); !list.isEmpty())
                return containerToString(list, max_list_count);
            if (auto list = value.value<DataPropertySet>(); !list.isEmpty())
                return containerToString(list, max_list_count);
            if (auto list = value.value<DataPropertyList>(); !list.isEmpty())
                return containerToString(list, max_list_count);
            if (auto list = value.value<PropertyIDList>(); !list.isEmpty())
                return containerToString(list, max_list_count);
            if (auto list = value.value<PropertyIDSet>(); !list.isEmpty())
                return containerToString(list, max_list_count);

            return value.toString();
        }
    }
}

QVariant Utils::variantFromStringHelper(const QString& value, const QLocale* locale)
{
    if (value.isEmpty())
        return {};

    if (value.length() > 50)
        return value; // очевидно, что длинная строка - это просто строка

    bool ok;

    qlonglong l = locale->toLongLong(value, &ok);
    if (ok)
        return l;

    double d = locale->toDouble(value, &ok);
    if (ok)
        return d;

    QDateTime dt = locale->toDateTime(value, QLocale::FormatType::LongFormat);
    if (dt.isValid())
        return dt;
    dt = locale->toDateTime(value, QLocale::FormatType::ShortFormat);
    if (dt.isValid())
        return dt;

    QDate date = locale->toDate(value, QLocale::FormatType::LongFormat);
    if (date.isValid())
        return date;
    date = locale->toDate(value, QLocale::FormatType::ShortFormat);
    if (date.isValid())
        return date;

    QTime time = locale->toTime(value, QLocale::FormatType::LongFormat);
    if (time.isValid())
        return time;
    time = locale->toTime(value, QLocale::FormatType::ShortFormat);
    if (time.isValid())
        return time;

    return value;
}

struct _ZVariantToString
{
    QVariant::Type type;
    const char* value;
};

const _ZVariantToString _variantToString[] = {{QVariant::Bool, "Bool"}, {QVariant::LongLong, "Int"}, {QVariant::ULongLong, "Int"}, {QVariant::Int, "Int"},
    {QVariant::UInt, "Int"}, {QVariant::Double, "Double"}, {QVariant::String, "Text"}, {QVariant::Char, "Text"}, {QVariant::Date, "Date"},
    {QVariant::Time, "Time"}, {QVariant::DateTime, "DateTime"}};

bool Utils::isSimpleType(QVariant::Type type)
{
    if (type == QVariant::Invalid)
        return true;

    for (uint i = 0; i < count_of(_variantToString); i++)
        if (_variantToString[i].type == type) {
            return true;
        }
    return false;
}

static const char hexDigits[] = "0123456789ABCDEF";
// Метод заимствован из кода QSettings
void Utils::toUnescapedString(const QString& in, QString& out, QTextCodec* codec)
{
    static const char escapeCodes[][2]
        = {{'a', '\a'}, {'b', '\b'}, {'f', '\f'}, {'n', '\n'}, {'r', '\r'}, {'t', '\t'}, {'v', '\v'}, {'"', '"'}, {'?', '?'}, {'\'', '\''}, {'\\', '\\'}};
    static const int numEscapeCodes = sizeof(escapeCodes) / sizeof(escapeCodes[0]);

    QByteArray data = in.toUtf8();

    bool inQuotedString = false;
    bool currentValueIsQuoted = false;
    int escapeVal = 0;
    int i = 0;
    int to = data.length();
    char ch;

StSkipSpaces:
    while (i < to && ((ch = data.at(i)) == ' ' || ch == '\t'))
        ++i;
    // fallthrough

StNormal:
    while (i < to) {
        switch (data.at(i)) {
            case '\\':
                ++i;
                if (i >= to)
                    goto end;

                ch = data.at(i++);
                for (int j = 0; j < numEscapeCodes; ++j) {
                    if (ch == escapeCodes[j][0]) {
                        out += QLatin1Char(escapeCodes[j][1]);
                        goto StNormal;
                    }
                }

                if (ch == 'x') {
                    escapeVal = 0;

                    if (i >= to)
                        goto end;

                    ch = data.at(i);
                    if ((ch >= '0' && ch <= '9') || (ch >= 'A' && ch <= 'F') || (ch >= 'a' && ch <= 'f'))
                        goto StHexEscape;
                } else if (ch >= '0' && ch <= '7') {
                    escapeVal = ch - '0';
                    goto StOctEscape;
                } else if (ch == '\n' || ch == '\r') {
                    if (i < to) {
                        char ch2 = data.at(i);
                        // \n, \r, \r\n, and \n\r are legitimate line terminators in INI files
                        if ((ch2 == '\n' || ch2 == '\r') && ch2 != ch)
                            ++i;
                    }
                } else {
                    // the character is skipped
                }
                break;
            case '"':
                ++i;
                currentValueIsQuoted = true;
                inQuotedString = !inQuotedString;
                if (!inQuotedString)
                    goto StSkipSpaces;
                break;
            default: {
                int j = i + 1;
                while (j < to) {
                    ch = data.at(j);
                    if (ch == '\\' || ch == '"' || ch == ',')
                        break;
                    ++j;
                }

                if (codec) {
                    out += codec->toUnicode(data.constData() + i, j - i);
                } else {
                    int n = out.size();
                    out.resize(n + (j - i));
                    QChar* resultData = out.data() + n;
                    for (int k = i; k < j; ++k)
                        *resultData++ = QLatin1Char(data.at(k));
                }
                i = j;
            }
        }
    }
    goto end;

StHexEscape:
    if (i >= to) {
        out += QChar(escapeVal);
        goto end;
    }

    ch = data.at(i);
    if (ch >= 'a')
        ch -= 'a' - 'A';
    if ((ch >= '0' && ch <= '9') || (ch >= 'A' && ch <= 'F')) {
        escapeVal <<= 4;
        escapeVal += strchr(hexDigits, ch) - hexDigits;
        ++i;
        goto StHexEscape;
    } else {
        out += QChar(escapeVal);
        goto StNormal;
    }

StOctEscape:
    if (i >= to) {
        out += QChar(escapeVal);
        goto end;
    }

    ch = data.at(i);
    if (ch >= '0' && ch <= '7') {
        escapeVal <<= 3;
        escapeVal += ch - '0';
        ++i;
        goto StOctEscape;
    } else {
        out += QChar(escapeVal);
        goto StNormal;
    }

end:
    if (!currentValueIsQuoted)
        chopTrailingSpacesHelper(out);
}

QString Utils::toUnescapedString(const QString& in)
{
    QString s;
    toUnescapedString(in, s);
    return s;
}

// Метод заимствован из кода QSettings
void Utils::toEscapedString(const QString& in, QString& out, QTextCodec* codec)
{
    // bool needsQuotes = false;
    bool escapeNextIfDigit = false;
    int i;
    out.clear();

    out.reserve(0 + in.size() * 3 / 2);
    for (i = 0; i < in.size(); ++i) {
        uint ch = in.at(i).unicode();
        if (escapeNextIfDigit && ((ch >= '0' && ch <= '9') || (ch >= 'a' && ch <= 'f') || (ch >= 'A' && ch <= 'F'))) {
            out += "\\x";
            out += QByteArray::number(ch, 16);
            continue;
        }

        escapeNextIfDigit = false;

        switch (ch) {
            case '\0':
                out += "\\0";
                escapeNextIfDigit = true;
                break;
            case '\a':
                out += "\\a";
                break;
            case '\b':
                out += "\\b";
                break;
            case '\f':
                out += "\\f";
                break;
            case '\n':
                out += "\\n";
                break;
            case '\r':
                out += "\\r";
                break;
            case '\t':
                out += "\\t";
                break;
            case '\v':
                out += "\\v";
                break;
            case '"':
            case '\\':
                out += '\\';
                out += static_cast<char>(ch);
                break;
            default:
                if (ch <= 0x1F || (ch >= 0x7F && !codec)) {
                    out += "\\x";
                    out += QByteArray::number(ch, 16);
                    escapeNextIfDigit = true;
                } else if (codec) {
                    // slow
                    out += codec->fromUnicode(in.at(i));
                } else {
                    out += static_cast<char>(ch);
                }
        }
    }
}

QString Utils::toEscapedString(const QString& in)
{
    QString s;
    toEscapedString(in, s);
    return s;
}

static void lookAhead(const QStringRef& s, QString& replace, int& length)
{
    static const QVector<QPair<QString, QChar>> to_replace = {
        {"&amp;", '&'},
        {"&apos;", '\''},
        {"&quot;", '"'},
        {"&gt;", '>'},
        {"&lt;", '<'},
        {"&#x0A;", '\n'},
    };

    for (auto i = to_replace.constBegin(); i != to_replace.constEnd(); ++i) {
        if (s.length() < i->first.length())
            continue;

        if (s.left(i->first.length()) == i->first) {
            replace = i->second;
            length = i->first.length();
            return;
        }
    }

    length = 0;
    replace.clear();
    return;
}

QString Utils::unescapeXML(const QString& s)
{
    QString res;
    QString replace;
    int length;
    for (int i = 0; i < s.count();) {
        lookAhead(s.rightRef(s.count() - i), replace, length);
        if (length == 0) {
            res += s.at(i);
            i++;

        } else {
            res += replace;
            i += length;
        }
    }

    return res;
}

const QString Utils::_translitRus[74]
    = {utf("А"), utf("а"), utf("Б"), utf("б"), utf("В"), utf("в"), utf("Г"), utf("г"), utf("Ґ"), utf("ґ"), utf("Д"), utf("д"), utf("Е"), utf("е"), utf("Є"),
        utf("є"), utf("Ж"), utf("ж"), utf("З"), utf("з"), utf("И"), utf("и"), utf("І"), utf("і"), utf("Ї"), utf("ї"), utf("Й"), utf("й"), utf("К"), utf("к"),
        utf("Л"), utf("л"), utf("М"), utf("м"), utf("Н"), utf("н"), utf("О"), utf("о"), utf("П"), utf("п"), utf("Р"), utf("р"), utf("С"), utf("с"), utf("Т"),
        utf("т"), utf("У"), utf("у"), utf("Ф"), utf("ф"), utf("Х"), utf("х"), utf("Ц"), utf("ц"), utf("Ч"), utf("ч"), utf("Ш"), utf("ш"), utf("Щ"), utf("щ"),
        utf("Ь"), utf("ь"), utf("Ю"), utf("ю"), utf("Я"), utf("я"), utf("Ы"), utf("ы"), utf("Ъ"), utf("ъ"), utf("Ё"), utf("ё"), utf("Э"), utf("э")};

const QString Utils::_translitEng[74] = {"A", "a", "B", "b", "V", "v", "G", "g", "G", "g", "D", "d", "E", "e", "E", "E", "Zh", "zh", "Z", "z", "I", "i", "I",
    "I", "Yi", "yi", "J", "j", "K", "k", "L", "l", "M", "m", "N", "n", "O", "o", "P", "p", "R", "r", "S", "s", "T", "t", "U", "u", "F", "f", "H", "h", "Ts",
    "ts", "ch", "ch", "Sh", "sh", "Shh", "shh", "", "", "Yu", "yu", "Ya", "ya", "Y", "y", "", "", "Yo", "yo", "E", "e"};

QString Utils::translit(const QString& s)
{
    bool find = false;
    QString ret;

    for (int i = 0; i < s.length(); i++) {
        find = false;

        for (int j = 0; j < 74; j++) {
            if (s[i] == _translitRus[j]) {
                ret += _translitEng[j];
                find = true;
                break;
            }
        }

        if (!find)
            ret += s[i];
    }

    return ret;
}

const QString Utils::_ambiguities = R"(`~!@#$%^&*()_-+{}:;"'|\/<,>.?/№)";

QString Utils::removeAmbiguities(const QString& s, bool translit, const QChar& replace)
{
    QString res = s;
    for (int i = 0; i < s.length(); i++) {
        if (_ambiguities.contains(s.at(i)))
            res[i] = replace;
    }
    return translit ? Utils::translit(res) : res;
}

QChar Utils::prepareRussianStringHelper(const QChar& c)
{
    if (c.unicode() == 0x0041)
        return QChar(0x0410); // A
    if (c.unicode() == 0x0061)
        return QChar(0x0430); // a
    if (c.unicode() == 0x0042)
        return QChar(0x0412); // B
    if (c.unicode() == 0x0045)
        return QChar(0x0415); // E
    if (c.unicode() == 0x0065)
        return QChar(0x0435); // e
    if (c.unicode() == 0x004B)
        return QChar(0x041A); // K
    if (c.unicode() == 0x006B)
        return QChar(0x043A); // k
    if (c.unicode() == 0x004D)
        return QChar(0x041C); // M
    if (c.unicode() == 0x0048)
        return QChar(0x041D); // H
    if (c.unicode() == 0x004F)
        return QChar(0x041E); // O
    if (c.unicode() == 0x006F)
        return QChar(0x043E); // o
    if (c.unicode() == 0x0050)
        return QChar(0x0420); // P
    if (c.unicode() == 0x0070)
        return QChar(0x0440); // p
    if (c.unicode() == 0x0043)
        return QChar(0x0421); // C
    if (c.unicode() == 0x0063)
        return QChar(0x0441); // c
    if (c.unicode() == 0x0054)
        return QChar(0x0422); // T
    if (c.unicode() == 0x0058)
        return QChar(0x041A); // X
    if (c.unicode() == 0x0078)
        return QChar(0x0445); // x

    return c;
}

QString Utils::prepareRussianString(const QString& s)
{
    QString res = s;
    for (int i = 0; i < s.length(); i++) {
        res[i] = prepareRussianStringHelper(s.at(i));
    }
    return res;
}

QString Utils::prepareMemoString(const QString& s)
{
    QString vs = s.trimmed();
    vs.replace("\r\n", "\n");
    QString simplified;
    bool space_found = false;
    for (const QChar& c : qAsConst(vs)) {
        bool new_line = (c == '\n' || c == '\r');
        if (new_line || (!c.isSpace() && c != '\t' && c != '\v' && c != '\f')) {
            simplified += c;
            space_found = false;
            continue;
        }

        if (space_found)
            continue;

        simplified += ' ';
        space_found = true;
    }

    return simplified;
}

QString Utils::simplified(const QString& s)
{
    QString res = s.simplified();
    for (int i = res.length() - 1; i >= 0; i--) {
        const QChar& c = res.at(i);
        if (!c.isPrint() ||
            // question mark
            c == 65533)
            res.remove(i, 1);
    }

    return res;
}

QVariantList Utils::splitIntValues(const QString& s)
{
    QVariantList res;

    QString part;
    bool is_int = false;
    for (int i = 0; i < s.length(); i++) {
        const QChar& c = s.at(i);

        if (part.isEmpty()) {
            // начало новой части
            is_int = c.isDigit();

        } else {
            // продолжение накопления
            if (c.isDigit()) {
                if (is_int) {
                    // продолжаем накопление цифр

                } else {
                    // накопление строк прервано числом
                    res << part;
                    part.clear();
                    // начинаем накопление чисел
                    is_int = true;
                }

            } else {
                if (is_int) {
                    // накопление цифр прервано текстом
                    bool ok;
                    int v = part.toInt(&ok);
                    Z_CHECK(ok);
                    res << v;
                    part.clear();
                    // начинаем накопление строк
                    is_int = false;

                } else {
                    // продолжаем накопление текста
                }
            }
        }
        part += c;
    }

    if (!part.isEmpty()) {
        if (is_int) {
            bool ok;
            int v = part.toInt(&ok);
            Z_CHECK(ok);
            res << v;

        } else {
            res << part;
        }
    }

    return res;
}

bool Utils::addressSortFunc(const QPair<QString, QString>& d1, const QPair<QString, QString>& d2)
{
    QVariantList vals1 = splitIntValues(d1.second.trimmed());
    QVariantList vals2 = splitIntValues(d2.second.trimmed());

    if (vals1.isEmpty() && vals2.isEmpty())
        return false;

    if (!vals1.isEmpty() && vals2.isEmpty())
        return false;

    if (vals1.isEmpty() && !vals2.isEmpty())
        return true;

    for (int i = 0; i < qMin(vals1.count(), vals2.count()); i++) {
        const auto& v1 = vals1.at(i);
        const auto& v2 = vals2.at(i);

        bool ok1;
        bool ok2;
        int i1 = v1.toInt(&ok1);
        int i2 = v2.toInt(&ok2);

        if (ok1 && ok2) {
            if (i1 != i2)
                return i1 < i2;
            continue;
        }

        QString s1 = zf::Utils::variantToString(v1).toLower();
        QString s2 = zf::Utils::variantToString(v2).toLower();
        if (s1 != s2)
            return s1 < s2;
    }

    return vals1.count() < vals2.count();
}

QMap<QChar::Script, double> Utils::getTextScripts(const QString& s)
{
    QString prepared = s.simplified();
    if (prepared.isEmpty())
        return {};

    QMap<QChar::Script, int> count_map;
    int count = 0;
    for (int i = 0; i < prepared.length(); i++) {
        const QChar& c = prepared.at(i);
        if (c.isSpace())
            continue;

        count_map[c.script()]++;
        count++;
    }

    QMap<QChar::Script, double> res;
    for (auto i = count_map.constBegin(); i != count_map.constEnd(); ++i) {
        res[i.key()] = (double)i.value() / (double)count;
    }

    double sum = 0;
    for (auto i = res.constBegin(); i != res.constEnd(); ++i) {
        sum += i.value();
    }

    if (res.isEmpty())
        return {};

    res.first() += 1.0 - sum;
    return res;
}

QLocale::Language Utils::detectLanguage(const QString& s, double percent)
{
    static QMap<QChar::Script, QLocale::Language> lang_map = {
        {QChar::Script_Cyrillic, QLocale::Russian},
        {QChar::Script_Latin, QLocale::English},
    };

    auto scripts = getTextScripts(s);
    if (scripts.isEmpty())
        return QLocale::AnyLanguage;

    double common = scripts.value(QChar::Script_Common) + scripts.value(QChar::Script_Inherited);
    QLocale::Language max_lang = QLocale::AnyLanguage;
    double max_percent = 0;

    for (auto lang = lang_map.constBegin(); lang != lang_map.constEnd(); ++lang) {
        double p = common + scripts.value(lang.key());
        if ((p > percent || fuzzyCompare(p, percent)) && (p > max_percent || max_lang == QLocale::AnyLanguage)) {
            max_lang = lang.value();
            max_percent = p;
        }
    }

    return max_lang;
}

const QSet<QChar> Utils::_mask_values_mask = {'A', 'a', 'N', 'n', 'X', 'x', '9', '0', 'D', 'd', '#', 'H', 'h', 'B', 'b'};
const QSet<QChar> Utils::_mask_values_meta = {'>', '<', '!', '[', ']', '{', '}'};

QString Utils::extractMaskValue(const QString& text, const QString& mask)
{
    QString s = text.trimmed();
    if (s.isEmpty() || mask.isEmpty())
        return s;

    QString res;

    int space_pos = mask.indexOf(';');
    QChar space_char;
    if (space_pos >= 0 && mask.count() > space_pos + 1) {
        space_char = mask.at(space_pos + 1);
    }

    int index = 0;
    bool force_separator = false;
    for (int i = 0; i < mask.count(); i++) {
        if (index >= s.count())
            break;

        QChar m = mask.at(i);
        QChar c = s.at(index);

        if (force_separator) {
            // на прошлом шаге был слеш
            force_separator = false;
            res.append(s.at(index));
            index++;
            continue;
        }

        if (m == ';')
            break; // дошли дол конца маски

        if (c == space_char) {
            // пустое значение
            res += ' ';
            index++;
            continue;
        }

        if (m == '\\') {
            force_separator = true;
            continue;
        }

        if (_mask_values_mask.contains(m)) {
            // какой-то текст
            res.append(c);
            index++;
            continue;
        }

        if (_mask_values_meta.contains(m))
            continue; // изменение регистра и т.п. - игнорируем

        // какая-то фигня - пропускаем
        index++;
    }

    return res.trimmed();
}

void Utils::fixUnsavedData()
{
    QWidget* focused = QApplication::focusWidget();
    if (focused != nullptr) {
        if (focused->parent() && focused->parent()->inherits("QAbstractItemView")) {
            QAbstractItemView* view = qobject_cast<QAbstractItemView*>(focused->parent());
            QMetaObject::invokeMethod(view, "closeEditor", Q_ARG(QWidget*, focused));

        } else {
            focused->clearFocus();
            focused->setFocus();
        }
    }
}

int Utils::getAge(const QDate& date_birth, const QDate& target_date)
{
    if (date_birth.month() > target_date.month() || (date_birth.month() == target_date.month() && date_birth.day() > target_date.day()))
        return qMax(0, target_date.year() - date_birth.year() - 1);
    else
        return qMax(0, target_date.year() - date_birth.year());
}

QString Utils::getAgeText(const QDate& date_birth, const QDate& target_date)
{
    int age = getAge(date_birth, target_date);
    if (age != 0)
        return QStringLiteral("%1 %2").arg(age).arg(zf::translate(TR::ZFT_AGE));

    return zf::translate(TR::ZFT_AGE_ZERO);
}

QIcon Utils::icon(const QStringList& files)
{
    QIcon icon;
    for (auto& f : files) {
        icon.addFile(f);
    }
    return icon;
}

QString Utils::iconToSerialized(const QIcon& icon)
{
    Error error;
    return variantToSerialized(QVariant::fromValue(icon), error);
}

QIcon Utils::iconFromSerialized(const QString& s, Error& error)
{
    return variantFromSerialized(s, error).value<QIcon>();
}

void Utils::resizeWindowToScreen(QWidget* w, const QSize& default_size, bool to_center, int reserve_percent)
{
    QScreen* screen = QGuiApplication::screenAt(QCursor::pos());
    if (screen == nullptr)
        return;

    resizeWindow(w, screen->geometry(), default_size, to_center, reserve_percent);
}

void Utils::resizeWindow(QWidget* widget, const QRect& restriction, const QSize& default_size, bool to_center, int reserve_percent)
{
    Z_CHECK_NULL(widget);
    Z_CHECK(reserve_percent >= 0);

    if (widget->windowState() & Qt::WindowMaximized)
        return;

    QSize max_size = restriction.size();

    int w_corrected = max_size.width() - max_size.width() * reserve_percent / 100;
    int h_corrected = max_size.height() - max_size.height() * reserve_percent / 100;

    QSize d_size;
    if (default_size.isEmpty())
        d_size = QSize(w_corrected, h_corrected);
    else
        d_size = default_size;

    QSize s(qMin(d_size.width(), w_corrected), qMin(d_size.height(), h_corrected));
    s.setWidth(qMax(widget->minimumWidth(), s.width()));
    s.setHeight(qMax(widget->minimumHeight(), s.height()));

    QSize old_size = widget->size();
    widget->resize(s);

    int x_shift = s.width() - old_size.width();
    int y_shift = s.height() - old_size.height();

    widget->move(qMax(0, widget->geometry().x() - qMax(0, x_shift)), qMax(0, widget->geometry().y() - qMax(0, y_shift)));

    if (to_center)
        widget->setGeometry(QStyle::alignedRect(Qt::LeftToRight, Qt::AlignCenter, widget->size(), restriction));
}

void Utils::centerWindow(QWidget* widget, bool to_main_window)
{
    Z_CHECK_NULL(widget);

    QMainWindow* main_window = nullptr;
    if (to_main_window)
        main_window = getMainWindow();

    if (main_window != nullptr) {
        const QPoint global = getMainWindow()->mapToGlobal(main_window->rect().center());

        int posX = global.x() - widget->width() / 2;
        int posY = global.y() - widget->height() / 2;
        if (qAbs(widget->pos().x() - posX) > 10 || qAbs(widget->pos().y() - posY) > 100)
            widget->move(posX, posY);

    } else {
        resizeWindowToScreen(widget, widget->size(), true);
    }
}

void Utils::showAbout(QWidget* parent, const QString& title, const QString& text)
{
    Q_UNUSED(parent);
    MessageBox msg(MessageBox::MessageType::About, text);
    if (!title.isEmpty())
        msg.setWindowTitle(title);
    msg.exec();

    /*QMessageBox* msgBox = new QMessageBox(title, text, QMessageBox::Information, QDialogButtonBox::Ok, 0, 0, parent);
    msgBox->setAttribute(Qt::WA_DeleteOnClose);

    int height = fontHeight() * (1 + text.count('\n'));
    msgBox->setIconPixmap(qApp->windowIcon().pixmap(height));

    msgBox->exec();*/
}

void Utils::setFocus(QWidget* widget, bool direct)
{
    Z_CHECK_NULL(widget);

    if (QApplication::focusWidget() == widget)
        return;

    QObjectList chain = findAllParents(widget, {"QTabWidget"});
    chain.prepend(widget);
    for (int i = chain.count() - 1; i > 0; i--) {
        QTabWidget* w = qobject_cast<QTabWidget*>(chain.at(i));
        QWidget* w_prev = qobject_cast<QWidget*>(chain.at(i - 1));
        for (int tab = 0; tab < w->count(); tab++) {
            if (w->widget(tab)->isAncestorOf(w_prev)) {
                w->setCurrentIndex(tab);
                break;
            }
        }
    }

    chain = findAllParents(widget, {"QStackedWidget"});
    chain.prepend(widget);
    for (int i = chain.count() - 1; i > 0; i--) {
        QStackedWidget* w = qobject_cast<QStackedWidget*>(chain.at(i));
        QWidget* w_prev = qobject_cast<QWidget*>(chain.at(i - 1));
        for (int tab = 0; tab < w->count(); tab++) {
            if (w->widget(tab)->isAncestorOf(w_prev)) {
                w->setCurrentIndex(tab);
                break;
            }
        }
    }

    chain = findAllParents(widget, {"QToolBox"});
    chain.prepend(widget);
    for (int i = chain.count() - 1; i > 0; i--) {
        QToolBox* w = qobject_cast<QToolBox*>(chain.at(i));
        QWidget* w_prev = qobject_cast<QWidget*>(chain.at(i - 1));
        for (int tab = 0; tab < w->count(); tab++) {
            if (w->widget(tab)->isAncestorOf(w_prev)) {
                w->setCurrentIndex(tab);
                break;
            }
        }
    }

    chain = findAllParents(widget, {"zf::CollapsibleGroupBox"});
    for (int i = chain.count() - 1; i >= 0; i--) {
        qobject_cast<CollapsibleGroupBox*>(chain.at(i))->expand();
    }

    chain = findAllParents(widget, {"QScrollArea"});
    //    for (int i = chain.count() - 1; i >= 0; i--) {
    //        updateGeometry(qobject_cast<QScrollArea*>(chain.at(i)));
    //    }

    auto w_p = QPointer(widget);
    for (int i = chain.count() - 1; i >= 0; i--) {
        auto sa = qobject_cast<QScrollArea*>(chain.at(i));
        //        sa->ensureWidgetVisible(widget);

        QTimer::singleShot(0, sa, [sa, w_p]() {
            // если вызвать ensureWidgetVisible сразу, то не успеет отработать zf::CollapsibleGroupBox::expand
            // можно либо принудительно вызвать updateGeometry для QScrollArea, либо вот так через таймер
            if (!w_p.isNull())
                sa->ensureWidgetVisible(w_p);
        });
    }

    if (direct)
        widget->setFocus();
    else
        QMetaObject::invokeMethod(widget, "setFocus", Qt::QueuedConnection);
}

bool Utils::setFocus(const DataProperty& p, HighlightMapping* mapping)
{
    Z_CHECK(p.isValid());
    Z_CHECK_NULL(mapping);

    if (p.propertyType() == PropertyType::Field || p.propertyType() == PropertyType::Dataset) {
        QWidget* w = mapping->getHighlightWidget(p);
        if (w == nullptr)
            w = mapping->widgets()->object<QWidget>(p, false);

        if (w != nullptr) {
            Utils::setFocus(w);
            return true;
        }

    } else if (p.propertyType() == PropertyType::Cell) {
        QAbstractItemView* iv = qobject_cast<QAbstractItemView*>(mapping->getHighlightWidget(p.dataset()));
        if (iv == nullptr)
            iv = mapping->widgets()->object<QAbstractItemView>(p.dataset(), false);

        if (iv != nullptr) {
            Utils::setFocus(iv);                        
            QModelIndex idx = mapping->widgets()->data()->findDatasetRowID(p.dataset(), p.rowId());
            if (idx.isValid()) {
                auto dest_ds_prop = mapping->getDestinationHighlightProperty(p.dataset());
                auto index = mapping->widgets()->data()->cellIndex(idx.row(), p.column(), idx.parent());                
                if (dest_ds_prop != p.dataset() && !index.parent().isValid()) {
                    // возможность иерархии не учитыватся в связи с ленью и невостребованностью оного
                    auto dest_ds = mapping->widgets()->data()->dataset(dest_ds_prop);
                    if (dest_ds->rowCount() > idx.row() && dest_ds->columnCount() > idx.column())
                        index = dest_ds->index(idx.row(), p.column().pos());
                }

                mapping->widgets()->setCurrentIndex(dest_ds_prop, index);
                return true;
            }
        }
    }
    return false;
}

bool Utils::hasFocus(QWidget* widget)
{
    Z_CHECK_NULL(widget);

    QWidget* w = getMainWidget(widget);

    if (auto x = qobject_cast<ItemSelector*>(w))
        return x->hasFocus();
    if (auto x = qobject_cast<RequestSelector*>(w))
        return x->hasFocus();
    if (auto x = qobject_cast<FormattedEdit*>(w))
        return x->hasFocus();

    return w->hasFocus();
}

QKeySequence Utils::toKeySequence(QKeyEvent* event)
{
    Z_CHECK_NULL(event);
    return QKeySequence(event->key() | event->modifiers());
}

void Utils::updateGeometry(QWidget* widget)
{
    Z_CHECK_NULL(widget);
    // такое извращение нужно потому что QWidget::updateGeometry только просит обновить размеры, а обновляются они потом по таймеру
    // возможно вместо этого можно использовать QLayout::invalidate (не проверено)

    QEvent la_event(QEvent::LayoutRequest);
    auto ch = widget->findChildren<QWidget*>();
    for (auto* child : qAsConst(ch)) {
        QCoreApplication::sendEvent(child, &la_event);
    }
    QCoreApplication::sendEvent(widget, &la_event);
}

void Utils::fatalErrorHelper(const char* text, const char* file, const char* function, int line)
{
    fatalErrorHelper(utf(text), file, function, line);
}

void Utils::fatalErrorHelper(const QString& text, const char* file, const char* function, int line)
{
    if (_is_app_halted)
        return;

    if (!isFatalHaltBlocked()) {
        QString msg = (file == nullptr)
                          ? text
                          : QString("Critical error. %1. File %2, line %3, function %4").arg(HtmlTools::plain(text, false)).arg(file).arg(line).arg(function);

#ifndef _MSC_VER
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-security"
#endif
        if (Utils::isMainThread() && Core::isBootstraped()) {
            _is_app_halted = true;

            if (Core::messageDispatcher() != nullptr)
                Core::messageDispatcher()->stop();
            CallbackManager::stopAll();
            qFatal(msg.toUtf8().constData()); //-V618

        } else {
#ifdef QT_DEBUG
            // для отслеживания проблемы в не основном потоке под отладчиком, останавливаем сразу не выводя текстовое
            // сообщение
            if (Framework::defaultMessageHandler() != nullptr)
                Framework::defaultMessageHandler()(QtCriticalMsg, QMessageLogContext(file, line, function, "assert not in the main thread"), text);
            else
                qCritical() << msg;
#endif
            Core::writeToLogStorage(
                QString("file: %1, line: %2, function: %3, error: %4").arg(file).arg(line).arg(function).arg(text.simplified()), InformationType::Critical);

#ifdef QT_DEBUG
            debug_break();
#endif
            // Отправляем в основной поток сообщение и останавливаем текущий поток
            Core::messageDispatcher()->postMessage(Core::fr(), Core::fr(), MessageCriticalError(Error(msg)));
            QThread::sleep(UINT_MAX);
        }
#ifndef _MSC_VER
#pragma GCC diagnostic pop
#endif

    } else {
        QString msg = (file == nullptr) ? text : QString("Critical error. File %1, line: %2, function: %3").arg(file).arg(line).arg(function);
        Core::logError(msg, text);

        throw FatalException(Error(msg, text));
    }
}

void Utils::fatalErrorHelper(const Error& error, const char* file, const char* function, int line)
{
    Z_CHECK(error.isError());

    fatalErrorHelper(QString("<%1> %2").arg(static_cast<int>(error.type())).arg(error.fullText()), file, function, line);
}

bool Utils::isAppHalted()
{
    return _is_app_halted;
}

void Utils::blockFatalHalt()
{
    _fatal_halt_block_counter++;
}

void Utils::unBlockFatalHalt()
{
    _fatal_halt_block_counter--;
    if (_fatal_halt_block_counter < 0)
        Z_HALT_INT;
}

bool Utils::isFatalHaltBlocked()
{
    return _fatal_halt_block_counter > 0;
}

static QString _name(const QString& s, QChar sep)
{
    QString sTmp = s.trimmed().simplified();
    int i = sTmp.indexOf(sep);
    while (i >= 0) {
        if (i < sTmp.length() - 1)
            sTmp[i + 1] = sTmp[i + 1].toUpper();
        i = sTmp.indexOf(sep, i + 1);
    }
    return sTmp;
}

QString Utils::name(const QString& s)
{        
    QString sTmp = s.trimmed().simplified().toLower();
    if (sTmp.isEmpty())
        return sTmp;

    sTmp = _name(sTmp, ' ');
    sTmp = _name(sTmp, '-');
    sTmp = _name(sTmp, '(');
    // неразрывный пробел
    sTmp = _name(sTmp, QChar(0x00A0));
    sTmp = _name(sTmp, QChar(0x2007));
    sTmp = _name(sTmp, QChar(0x202F));
    sTmp = _name(sTmp, QChar(0x2060));

    sTmp[0] = sTmp[0].toUpper();
    return sTmp;
}

QString Utils::getFIO(const QString& f, const QString& i, const QString& o)
{
    return QString(name(f) + QStringLiteral(" ") + name(i) + QStringLiteral(" ") + name(o)).simplified();
}

QString Utils::getShortFIO(const QString& f, const QString& i, const QString& o)
{
    if (f.isEmpty() && i.isEmpty() && o.isEmpty())
        return f;

    if (i.isEmpty() && o.isEmpty())
        return f;

    if (i.isEmpty())
        return f + QStringLiteral(" ") + o.at(0) + QStringLiteral(".");

    if (o.isEmpty())
        return f + QStringLiteral(" ") + i.at(0) + QStringLiteral(".");

    return f + QStringLiteral(" ") + i.at(0) + QStringLiteral(".") + o.at(0) + QStringLiteral(".");
}

void Utils::extractFIO(const QString& fio, QString& lastName, QString& name, QString& middleName)
{
    QStringList f_list = fio.split(" ", QString::SkipEmptyParts);
    lastName = (f_list.count() >= 1) ? Utils::name(f_list.at(0)) : "";
    name = (f_list.count() >= 2) ? Utils::name(f_list.at(1)) : "";
    middleName = (f_list.count() >= 3) ? Utils::name(f_list.at(2)) : "";
}

QString Utils::prepareSearchString(const QString& s, bool to_lower)
{
    QString prepared = s.simplified().trimmed();
    for (auto& c : USER_SEPARATORS) {
        if (c == Consts::COMPLEX_SEPARATOR)
            continue;

        if (prepared.startsWith(c))
            prepared.remove(0, 1);
        if (prepared.endsWith(c))
            prepared.remove(prepared.length() - 1, 1);

        prepared.replace(c, Consts::COMPLEX_SEPARATOR);
    }

    prepared = prepared.simplified().trimmed();
    if (prepared.isEmpty())
        return prepared;

    if (s.rightRef(1) == " ")
        prepared += ' '; // поисковым сервисам нужен пробел в конце, чтобы выдать доп. варианты

    return to_lower ? prepared.toLower() : prepared;
}

qint64 Utils::fileSize(const QString& f, Error& error)
{
    error.clear();

    QFileInfo fi(f);
    if (!fi.exists()) {
        error = Error::fileNotFoundError(f);
        return -1;
    }

    return fi.size();
}

// взято из кода QLibraryInfo::QLibraryInfo
QString Utils::compilerString()
{
#if defined(Q_CC_INTEL) // must be before GNU, Clang and MSVC because ICC/ICL claim to be them
#ifdef __INTEL_CLANG_COMPILER
#define ICC_COMPAT "Clang"
#elif defined(__INTEL_MS_COMPAT_LEVEL)
#define ICC_COMPAT "Microsoft"
#elif defined(__GNUC__)
#define ICC_COMPAT "GCC"
#else
#define ICC_COMPAT "no"
#endif
#if __INTEL_COMPILER == 1300
#define ICC_VERSION "13.0"
#elif __INTEL_COMPILER == 1310
#define ICC_VERSION "13.1"
#elif __INTEL_COMPILER == 1400
#define ICC_VERSION "14.0"
#elif __INTEL_COMPILER == 1500
#define ICC_VERSION "15.0"
#else
#define ICC_VERSION QT_STRINGIFY(__INTEL_COMPILER)
#endif
#ifdef __INTEL_COMPILER_UPDATE
#define COMPILER_STRING                                                                                                                                        \
    "Intel(R) C++ " ICC_VERSION "." QT_STRINGIFY(__INTEL_COMPILER_UPDATE) " build " QT_STRINGIFY(__INTEL_COMPILER_BUILD_DATE) " [" ICC_COMPAT " compatibility" \
                                                                                                                              "]"
#else
#define COMPILER_STRING "Intel(R) C++ " ICC_VERSION " build " QT_STRINGIFY(__INTEL_COMPILER_BUILD_DATE) " [" ICC_COMPAT " compatibility]"
#endif
#elif defined(Q_CC_CLANG) // must be before GNU, because clang claims to be GNU too
#ifdef __apple_build_version__ // Apple clang has other version numbers
#define COMPILER_STRING "Clang " __clang_version__ " (Apple)"
#else
#define COMPILER_STRING "Clang " __clang_version__
#endif
#elif defined(Q_CC_GHS)
#define COMPILER_STRING "GHS " QT_STRINGIFY(__GHS_VERSION_NUMBER)
#elif defined(Q_CC_GNU)
#define COMPILER_STRING "GCC " __VERSION__
#elif defined(Q_CC_MSVC)
#if _MSC_VER < 1910
#define COMPILER_STRING "MSVC 2015"
#elif _MSC_VER < 1917
#define COMPILER_STRING "MSVC 2017"
#elif _MSC_VER < 1930
#define COMPILER_STRING "MSVC 2019"
#elif _MSC_VER < 2000
#define COMPILER_STRING "MSVC 2022"
#else
#define COMPILER_STRING "MSVC _MSC_VER " QT_STRINGIFY(_MSC_VER)
#endif
#else
#define COMPILER_STRING "<unknown compiler>"
#endif

    return COMPILER_STRING;
}

QString Utils::compilerVersion()
{
    return QStringLiteral("Qt %1 (%2, %3 bit)").arg(qVersion(), compilerString(), QString::number(QSysInfo::WordSize));
}

QVariant Utils::variantToDigitalHelper(const QVariant& v, bool invalid_if_error, const QLocale* locale)
{
    Z_CHECK_NULL(locale);

    QVariant value_prepared;

    if (v.isNull() || !v.isValid()) {
        value_prepared = 0;

    } else {
        bool ok_int = true;
        bool ok_float = true;
        double d_value = 0.0;
        qlonglong i_value = 0;

        if (v.type() == QVariant::Int || v.type() == QVariant::UInt || v.type() == QVariant::LongLong || v.type() == QVariant::ULongLong
            || v.type() == QVariant::Bool)
            i_value = v.toLongLong();
        else
            i_value = locale->toLongLong(v.toString(), &ok_int);

        if (v.type() == QVariant::Double)
            i_value = v.toDouble();
        else
            d_value = locale->toDouble(v.toString(), &ok_float);

        if (ok_int && !ok_float)
            value_prepared = i_value;
        else if (!ok_int && ok_float)
            value_prepared = d_value;
        else if (ok_int && ok_float) {
            if (fuzzyCompare(static_cast<double>(i_value), d_value))
                value_prepared = i_value;
            else
                value_prepared = d_value;
        }
    }
    if (value_prepared.isNull()) {
        if (invalid_if_error)
            return QVariant();
        else
            return 0;
    } else
        return value_prepared;
}

void Utils::cloneItemModel_helper(
    const QAbstractItemModel* source, QAbstractItemModel* destination, const QModelIndex& source_index, const QModelIndex& destination_index)
{
    int row_count = source->rowCount(source_index);
    int col_count = source->columnCount(source_index);

    itemModelSetRowCount(destination, row_count, destination_index);
    itemModelSetColumnCount(destination, col_count, destination_index);

    for (int row = 0; row < row_count; row++) {
        for (int col = 0; col < col_count; col++) {
            QModelIndex d_index = destination->index(row, col, destination_index);
            QModelIndex s_index = source->index(row, col, source_index);
#ifdef QT_DEBUG
//            qDebug() << row << col << source->itemData(s_index);
#endif

            if (!destination->setItemData(d_index, source->itemData(s_index))) {
                qDebug() << destination->rowCount(destination_index) << source->rowCount(source_index);
                qDebug() << destination->columnCount(destination_index) << source->columnCount(source_index);
                qDebug() << d_index << s_index << source->itemData(s_index);
                Z_HALT_INT;
            }
        }
        if (source_index.isValid())
            cloneItemModel_helper(source, destination, source->index(row, 0, source_index), destination->index(0, row, destination_index));
    }
}

QStringList zf::Utils::splitArgsHelper(const QString& s, int idx)
{
    int l = s.length();
    Z_CHECK(l > 0);
    Z_CHECK(s.at(idx) == QLatin1Char('('));
    Z_CHECK(s.at(l - 1) == QLatin1Char(')'));

    QStringList result;
    QString item;

    for (++idx; idx < l; ++idx) {
        QChar c = s.at(idx);
        if (c == QLatin1Char(')')) {
            Z_CHECK(idx == l - 1);
            result.append(item);
        } else if (c == QLatin1Char(' ')) {
            result.append(item);
            item.clear();
        } else {
            item.append(c);
        }
    }

    return result;
}

void zf::Utils::chopTrailingSpacesHelper(QString& str)
{
    int n = str.size() - 1;
    QChar ch;
    while (n >= 0 && ((ch = str.at(n)) == QLatin1Char(' ') || ch == QLatin1Char('\t')))
        str.truncate(n--);
}

#ifdef Q_OS_LINUX
bool Utils::checkRootPassword(const QString& root_password, const QString& test_folder)
{
    if (root_password.trimmed().isEmpty())
        return false;

    QString temp_file = generateUniqueString();
    QDir target_dir(test_folder.trimmed().isEmpty() ? "/opt" : test_folder);
    if (!target_dir.exists())
        return false;

    QString temp_file_path = target_dir.absoluteFilePath(temp_file);
    bool is_ok = execute_sudo_comand_helper(QStringLiteral("touch %1").arg(temp_file_path), root_password);
    if (is_ok)
        remove_file_helper(temp_file_path, root_password);
    return is_ok;
}

bool Utils::isRoot()
{
    return getuid() == 0;
}

#endif

void Utils::headerConfig(int& frozen_group, HeaderView* header_view, const QPoint& pos, const zf::DatasetConfigOptions& options, bool first_column_movable,
    const View* view, const DataProperty& dataset_property)
{
    Z_CHECK_NULL(header_view);

    if (options == DatasetConfigOptions())
        return;

    QModelIndex index = header_view->indexAt(pos);
    if (!index.isValid())
        return;

    int new_frozen_group = frozen_group;
    frozen_group = -1;

    HeaderItem* item = header_view->rootItem()->findByPos(index, true);
    Z_CHECK(item != nullptr);

    QMenu menu(header_view);

    if (options.testFlag(DatasetConfigOption::Visible)) {
        QString hide_caption = item->label(true).isEmpty() ? QString::number(item->id()) : item->label(true);
        hide_caption = ZF_TR(ZFT_TMC_VISIBILITY) + ": " + hide_caption;
        QAction* a_hide = menu.addAction(hide_caption, header_view, [&]() { item->setHidden(true); });
        a_hide->setCheckable(true);
        a_hide->setChecked(true);
        a_hide->setEnabled(header_view->rootItem()->canHideSection(item));

        menu.addSeparator();

        menu.addAction(ZF_TR(ZFT_TMC_SETUP_COLUMNS), header_view,
                [&]() {
                    ItemViewHeaderConfigDialog dlg;
                    if (dlg.run(new_frozen_group, header_view, first_column_movable))
                        frozen_group = new_frozen_group;
                })
            ->setIcon(resizeSvg(":/share_icons/grid.svg", Z_PM(PM_SmallIconSize)));

        menu.addAction(ZF_TR(ZFT_TMC_SHOW_HIDDEN), header_view, [&]() { header_view->rootItem()->resetHidden(); })
            ->setEnabled(header_view->rootItem()->hasHiddenSections());

        menu.addAction(ZF_TR(ZFT_TMC_RESET_COLUMNS), header_view, [&]() { header_view->rootItem()->resetOrder(); })
            ->setEnabled(header_view->rootItem()->isOrderChanged());
    }

    bool has_filter = false;
    if (options.testFlag(DatasetConfigOption::Filter) && view != nullptr && view->hasConditionFilter(dataset_property)) {
        for (auto& p : dataset_property.columns()) {
            if (p.options() & PropertyOption::Filtered) {
                has_filter = true;
                break;
            }
        }

        if (has_filter) {
            menu.addSeparator();
            menu.addAction(ZF_TR(ZFT_TMC_SETUP_CONDITIONS), view,
                    [&]() {
                        ConditionsDialog dlg(view->structure().get(), dataset_property, view->conditionFilter(dataset_property));
                        if (dlg.exec() == QDialogButtonBox::Save) {
                            Error error
                                = Core::fr()->saveConditionFilter(view->conditionFilterUniqueKey(), dataset_property, view->conditionFilter(dataset_property));
                            if (error.isError())
                                Core::logError(error);
                        }
                    })
                ->setIcon(resizeSvg(":/share_icons/filter.svg", Z_PM(PM_SmallIconSize)));

            menu.addAction(ZF_TR(ZFT_TMC_CLEAR_CONDITIONS), view,
                    [&]() {
                        view->conditionFilter(dataset_property)->clear();
                        Error error
                            = Core::fr()->saveConditionFilter(view->conditionFilterUniqueKey(), dataset_property, view->conditionFilter(dataset_property));
                        if (error.isError())
                            Core::logError(error);
                    })
                ->setEnabled(!view->conditionFilter(dataset_property)->isEmpty());
        }
    }

    if (options.testFlag(DatasetConfigOption::Export) && view != nullptr) {
        if (has_filter || options.testFlag(DatasetConfigOption::Visible))
            menu.addSeparator();
        menu.addAction(ZF_TR(ZFT_TMC_EXPORT_TABLE_EXCEL), view, [&]() { view->exportToExcel(dataset_property); })
            ->setIcon(resizeSvg(":/share_icons/blue/xls_format.svg", Z_PM(PM_SmallIconSize)));
    }

    if (!menu.actions().isEmpty())
        menu.exec(QCursor::pos());
}

void Utils::paintHeaderDragHandle(QAbstractScrollArea* area, HeaderView* header)
{
    if (header->isColumnsDragging() && header->dragAllowed() && header->dragToHidden() >= 0) {
        int drag_col = header->logicalIndex(header->dragToHidden());
        if (!header->isSectionHidden(drag_col)) {
            QPainter painter(area->viewport());
            painter.save();

            static QPixmap* pixmap = nullptr;
            if (pixmap == nullptr) {
                pixmap = new QPixmap;
                QIcon icon = resizeSvg(":/share_icons/green/insert-here-cercle.svg", scaleUI(16), true);
                *pixmap = icon.pixmap(scaleUI(16), scaleUI(16));
            }

            int drag_x = header->sectionViewportPosition(drag_col);
            const int shift = 3;
            if (header->dragLeft())
                drag_x += shift;
            else
                drag_x += header->sectionSize(drag_col) - pixmap->width() - shift;

            QRect pixmap_rect(drag_x, 2, pixmap->width(), pixmap->height());

            painter.drawPixmap(pixmap_rect, *pixmap, QRect(0, 0, pixmap_rect.width(), pixmap_rect.height()));

            painter.restore();
        }
    }
}

static const int _header_data_structure_version = 1;
Error Utils::saveHeader(QIODevice* device, HeaderItem* root_item, int frozen_group_count)
{
    Z_CHECK(device && root_item && frozen_group_count >= 0);
    Z_CHECK_NULL(root_item);
    Z_CHECK(root_item->isRoot());
    if (device->isOpen())
        device->close();

    if (!device->open(QIODevice::WriteOnly | QIODevice::Truncate))
        return Error("open error");

    QDataStream st(device);
    st.setVersion(Consts::DATASTREAM_VERSION);
    st << _header_data_structure_version;
    st << frozen_group_count;

    auto all = root_item->allChildren();
    st << all.count();

    for (auto h : all) {
        st << h->parent()->id();
        st << h->id();
        st << h->visualPos();
        st << (h->isHidden() && !h->isPermanentHidded());
        st << h->sectionSize();
    }

    if (st.status() != QDataStream::Ok) {
        device->close();
        return Error("write error");
    }

    device->close();

    return Error();
}

Error Utils::loadHeader(QIODevice* device, HeaderItem* root_item, int& frozen_group_count)
{
    frozen_group_count = 0;

    Z_CHECK(device && root_item);
    Z_CHECK(root_item->isRoot());

    if (device->isOpen())
        device->close();

    if (!device->open(QIODevice::ReadOnly))
        return Error("open error");

    QDataStream st(device);
    st.setVersion(Consts::DATASTREAM_VERSION);

    Error error;

    int version;
    st >> version;
    if (version != _header_data_structure_version)
        error = Error("version error");

    int f_count = 0;
    if (error.isOk()) {
        st >> f_count;
        if (st.status() != QDataStream::Ok)
            error = Error("read error");
    }

    int data_count = 0;
    if (error.isOk()) {
        st >> data_count;
        if (st.status() != QDataStream::Ok)
            error = Error("read error");
    }

    struct Data
    {
        int parent_id;
        int id;
        int pos;
        bool hidden;
        int size;
    };

    QList<Data> data;
    if (error.isOk()) {
        for (int i = 0; i < data_count; i++) {
            int parent_id;
            int id;
            int pos;
            bool hidden;
            int size;

            st >> parent_id;
            st >> id;
            st >> pos;
            st >> hidden;
            st >> size;

            data << Data {parent_id, id, pos, hidden, size};
        }
        if (st.status() != QDataStream::Ok)
            error = Error("read error");
    }

    device->close();

    if (error.isOk()) {
        frozen_group_count = f_count;

        std::sort(data.begin(), data.end(), [](const Data& d1, const Data& d2) -> bool {
            if (d1.parent_id == d2.parent_id)
                return d1.pos < d2.pos;
            else
                return d1.parent_id < d2.parent_id;
        });

        root_item->beginUpdate();
        for (const Data& d : qAsConst(data)) {
            if (d.id < 0)
                continue;

            HeaderItem* item = root_item->item(d.id, false);
            if (item == nullptr)
                continue;

            HeaderItem* parent_item = d.parent_id < 0 ? root_item : root_item->item(d.parent_id, false);
            if (parent_item == nullptr || item->parent() != parent_item || d.pos >= item->parent()->count())
                continue;

            HeaderItem* move_to_item = parent_item->childVisual(d.pos);
            item->move(move_to_item, true);

            if (d.size > 0 && !d.hidden)
                item->setSectionSize(d.size);
            else
                item->setSectionSize(item->defaultSectionSize());

            if (!item->isPermanentHidded())
                item->setHidden(d.hidden);
        }

        // проверяем не получилось ли так, что скрыто все (например после изменения структуры заголовка программистом)
        if (root_item->childrenVisual(Qt::AscendingOrder, true).isEmpty()) {
            // отображаем все
            root_item->setHidden(false);
        }

        root_item->endUpdate();
    }

    return error;
}

void Utils::updatePalette(bool read_only, bool warning, QWidget* widget)
{
    Z_CHECK(widget != nullptr);

    static bool is_updating = false;

    if (is_updating)
        return;

    is_updating = true;

    QPalette palette = qApp->palette();
    QColor text_color;
    QColor highlighted_text;
    QColor highlight;
    QColor background_color;
    QColor shadow_color;

    text_color = warning ? Qt::red : palette.color(QPalette::Active, QPalette::WindowText);
    highlighted_text = palette.color(QPalette::Active, QPalette::HighlightedText);
    highlight = palette.color(QPalette::Active, QPalette::Highlight);
    shadow_color = palette.color(QPalette::Active, QPalette::Button);

    if (read_only)
        background_color = Colors::readOnlyBackground();
    else
        background_color = Colors::background();

    palette.setColor(QPalette::Active, QPalette::WindowText, text_color);
    palette.setColor(QPalette::Active, QPalette::Text, text_color);
    palette.setColor(QPalette::Active, QPalette::ButtonText, text_color);
    palette.setColor(QPalette::Active, QPalette::HighlightedText, highlighted_text);
    palette.setColor(QPalette::Active, QPalette::Highlight, highlight);
    palette.setColor(QPalette::Active, QPalette::Midlight, background_color);
    palette.setColor(QPalette::Active, QPalette::Base, background_color);
    palette.setColor(QPalette::Active, QPalette::Shadow, shadow_color);

    palette.setColor(QPalette::Inactive, QPalette::WindowText, text_color);
    palette.setColor(QPalette::Inactive, QPalette::Text, text_color);
    palette.setColor(QPalette::Inactive, QPalette::ButtonText, text_color);
    palette.setColor(QPalette::Inactive, QPalette::HighlightedText, highlighted_text);
    palette.setColor(QPalette::Inactive, QPalette::Highlight, highlight);
    palette.setColor(QPalette::Inactive, QPalette::Midlight, background_color);
    palette.setColor(QPalette::Inactive, QPalette::Base, background_color);
    palette.setColor(QPalette::Inactive, QPalette::Shadow, shadow_color);

    palette.setColor(QPalette::Disabled, QPalette::WindowText, text_color);
    palette.setColor(QPalette::Disabled, QPalette::Text, text_color);
    palette.setColor(QPalette::Disabled, QPalette::ButtonText, text_color);
    palette.setColor(QPalette::Disabled, QPalette::HighlightedText, highlighted_text);
    palette.setColor(QPalette::Disabled, QPalette::Highlight, highlight);
    palette.setColor(QPalette::Disabled, QPalette::Midlight, background_color);
    palette.setColor(QPalette::Disabled, QPalette::Base, background_color);
    palette.setColor(QPalette::Disabled, QPalette::Shadow, shadow_color);

    widget->setPalette(palette);

    is_updating = false;
}

void Utils::paintBorder(QWidget* widget, const QRect& rect, bool top, bool bottom, bool left, bool right)
{
    Z_CHECK_NULL(widget);

    QStylePainter painter(widget);
    paintBorder(&painter, rect.isValid() ? rect : widget->rect(), top, bottom, left, right);
}

void Utils::paintBorder(QPainter* painter, const QRect& rect, bool top, bool bottom, bool left, bool right)
{
    Z_CHECK_NULL(painter);
    painter->save();
    painter->setPen(Colors::uiDarkColor());

    if (top)
        painter->drawLine(rect.left(), rect.top(), rect.right(), rect.top());
    if (bottom)
        painter->drawLine(rect.left(), rect.bottom(), rect.right(), rect.bottom());
    if (left)
        painter->drawLine(rect.left(), rect.top(), rect.left(), rect.bottom());
    if (right)
        painter->drawLine(rect.right(), rect.top(), rect.right(), rect.bottom());

    //    painter->drawRect(rect.adjusted(0, 0, -1, -1));

    painter->restore();
}

void Utils::createTabOrderWidgetHelper(QWidget* w, QList<QWidget*>& widgets)
{
    if (auto widget = qobject_cast<CollapsibleGroupBox*>(w)) {
        if (widget->layout())
            createTabOrder(widget->layout(), widgets);

    } else if (qobject_cast<QLineEdit*>(w) || qobject_cast<QAbstractSpinBox*>(w) || qobject_cast<QAbstractButton*>(w) || qobject_cast<QComboBox*>(w)
               || qobject_cast<QPlainTextEdit*>(w) || qobject_cast<QTextEdit*>(w) || qobject_cast<QAbstractItemView*>(w)) {
        widgets << getBaseChildWidget(w);

    } else if (qobject_cast<zf::FormattedEdit*>(w)) {
        widgets << getBaseChildWidget(w);

    } else if (qobject_cast<zf::ItemSelector*>(w)) {
        widgets << w;

    } else if (qobject_cast<zf::RequestSelector*>(w)) {
        widgets << w;

    } else if (auto* sp = qobject_cast<QSplitter*>(w)) {
        for (int i = 0; i < sp->count(); i++) {
            createTabOrderWidgetHelper(sp->widget(i), widgets);
        }

    } else if (auto* scroll = qobject_cast<QScrollArea*>(w)) {
        if (scroll->widget())
            createTabOrderWidgetHelper(scroll->widget(), widgets);

    } else if (auto* tb = qobject_cast<QTabWidget*>(w)) {
        for (int i = 0; i < tb->count(); i++)
            createTabOrderWidgetHelper(tb->widget(i), widgets);

    } else if (auto* tb = qobject_cast<QToolBox*>(w)) {
        for (int i = 0; i < tb->count(); i++)
            createTabOrderWidgetHelper(tb->widget(i), widgets);

    } else if (auto sw = qobject_cast<QStackedWidget*>(w)) {
        for (int i = 0; i < sw->count(); i++)
            createTabOrderWidgetHelper(sw->widget(i), widgets);

    } else {
        if (w->layout())
            createTabOrder(w->layout(), widgets);
    }
}

void Utils::createTabOrder(QLayout* la, QList<QWidget*>& widgets)
{
    if (la->inherits("QBoxLayout")) {
        QBoxLayout* layout = qobject_cast<QBoxLayout*>(la);
        for (int i = 0; i < layout->count(); i++) {
            QLayoutItem* item = layout->itemAt(i);
            if (!item)
                continue;
            if (item->layout())
                createTabOrder(item->layout(), widgets);
            else if (item->widget()) {
                createTabOrderWidgetHelper(item->widget(), widgets);
            }
        }

    } else if (la->inherits("QFormLayout")) {
        QFormLayout* layout = qobject_cast<QFormLayout*>(la);
        for (int i = 0; i < layout->rowCount(); i++) {
            QLayoutItem* item = layout->itemAt(i, QFormLayout::FieldRole);
            if (!item)
                continue;

            if (item->layout())
                createTabOrder(item->layout(), widgets);
            else if (item->widget()) {
                createTabOrderWidgetHelper(item->widget(), widgets);
            }
        }

    } else if (la->inherits("QGridLayout")) {
        QGridLayout* layout = qobject_cast<QGridLayout*>(la);
        for (int row = 0; row < layout->rowCount(); row++) {
            for (int col = 0; col < layout->columnCount(); col++) {
                QLayoutItem* item = layout->itemAtPosition(row, col);
                if (!item)
                    continue;
                if (item->layout())
                    createTabOrder(item->layout(), widgets);
                else if (item->widget()) {
                    createTabOrderWidgetHelper(item->widget(), widgets);
                }
            }
        }
    }
}

void Utils::setTabOrder(const QList<QWidget*> widgets)
{
    if (widgets.isEmpty())
        return;

    QWidget* first = nullptr;
    QWidget* prev = nullptr;
    QWidget* cur = nullptr;

    for (int i = 0; i < widgets.count(); i++) {
        QWidget* w = widgets.at(i);

        cur = (!w->isHidden() && (w->focusPolicy() & Qt::TabFocus) > 0) ? w : nullptr;

        if (cur && !first)
            first = cur;

        if (prev && cur)
            QWidget::setTabOrder(prev, cur);

        if (cur)
            prev = cur;
    }
}

QVariant Utils::valueToLanguage(const LanguageMap* value, QLocale::Language language)
{
    Z_CHECK_NULL(value);

    QLocale::Language def_lang = Core::language(LocaleType::UserInterface);

    if (!value->isEmpty()) {
        // всего одна запись - нет выбора
        if (value->count() == 1)
            return value->first();

        if (language == QLocale::AnyLanguage)
            language = def_lang;

        // точное совпадение
        auto it = value->find(language);
        if (it != value->end())
            return it.value();

        // язык UI
        it = value->find(def_lang);
        if (it != value->end())
            return it.value();

        // сохранено как значение для любого языка
        it = value->find(QLocale::AnyLanguage);
        if (it != value->end())
            return it.value();

        // сохранено под языком по умолчанию
        it = value->find(Core::defaultLanguage());
        if (it != value->end())
            return it.value();

        // может есть русский
        if (def_lang != QLocale::Russian && Core::defaultLanguage() != QLocale::Russian) {
            it = value->find(QLocale::Russian);
            if (it != value->end())
                return it.value();
        }

        // может есть английский
        if (def_lang != QLocale::English && Core::defaultLanguage() != QLocale::English) {
            it = value->find(QLocale::English);
            if (it != value->end())
                return it.value();
        }

        // первый попавшийся
        return value->first();

    } else
        return QVariant();
}

QVariant Utils::valueToLanguage(const LanguageMap& value, QLocale::Language language)
{
    return valueToLanguage(&value, language);
}

void Utils::haltInternal(const Error& error)
{
    Z_HALT(error);
}

QObject* Utils::getInterfaceHelper(const Uid& uid)
{
    Z_CHECK(uid.isValid());

    QObject* ptr = nullptr;
    Error error;
    if (uid.isUniqueEntity()) {
        // ищем плагин
        ptr = Core::getPlugin<QObject>(uid.entityCode(), error);
    }

    if (ptr == nullptr) {
        // плагин не нашли, ищем модель
        auto model = Core::getModelFull<Model>(uid, error);
        if (model != nullptr) {
            ptr = model.get();
            Core::modelKeeper()->keepModels({model});
        }
    }

    return ptr;
}

void Utils::getAllIndexesHelper(const QAbstractItemModel* m, const QModelIndex& parent, QModelIndexList& indexes)
{
    for (int i = 0; i < m->rowCount(parent); i++) {
        QModelIndex index = m->index(i, 0, parent);
        indexes << index;
        getAllIndexesHelper(m, index, indexes);
    }
}

void Utils::getAllRowCountHelper(const QAbstractItemModel* m, const QModelIndex& parent, int& count)
{
    for (int i = 0; i < m->rowCount(parent); i++) {
        count++;
        getAllRowCountHelper(m, m->index(i, 0, parent), count);
    }
}

// оптимизация для исключения постоянного вызова CallbackManager::startAll()/stopAll
class MessagePauseHelper_timeout : public QObject
{
public:
    MessagePauseHelper_timeout()
    {
        connect(&timer, &FeedbackTimer::timeout, &timer, [&]() {
            if (Utils::isAppHalted() || Core::messageDispatcher() == nullptr)
                return;

            while (counter > 0) {
                counter--;
                CallbackManager::startAll();
                Core::messageDispatcher()->start();
            }
        });
    }

    void start()
    {
        if (counter > 0)
            timer.start();
    }
    void stop()
    {
        counter++;
        Core::messageDispatcher()->stop();
        CallbackManager::stopAll();
    }

    FeedbackTimer timer;
    int counter = 0;
};

std::unique_ptr<MessagePauseHelper_timeout> MessagePauseHelper::_timeout = nullptr;
MessagePauseHelper::MessagePauseHelper() : _use_timeout(Utils::_disable_process_events_pause_counter == 0)
{
}

MessagePauseHelper::~MessagePauseHelper()
{
    if (!_paused)
        return;

    if (_use_timeout) {
        _timeout->start();

    } else {
        CallbackManager::startAll();
        Core::messageDispatcher()->start();
    }
}

void MessagePauseHelper::pause()
{
    if (_use_timeout) {
        if (_timeout == nullptr) {
            _timeout = std::make_unique<MessagePauseHelper_timeout>();
        }

        _timeout->stop();

    } else {
        Core::messageDispatcher()->stop();
        CallbackManager::stopAll();
    }

    _paused = true;
}

WaitCursor::WaitCursor(bool is_show)
{
    if (is_show)
        show(false);
}

void WaitCursor::show(bool processEvents)
{
    Utils::setWaitCursor(processEvents);
    _initialized = true;
}

void WaitCursor::hide()
{
    if (_initialized)
        Utils::restoreCursor();
    _initialized = false;
}

WaitCursor::~WaitCursor()
{
    if (_initialized)
        Utils::restoreCursor();
}

bool fuzzyCompare(double a, double b)
{
    return std::nextafter(a, std::numeric_limits<double>::lowest()) <= b && std::nextafter(a, std::numeric_limits<double>::max()) >= b;
}

bool fuzzyIsNull(double a)
{
    return fuzzyCompare(a, 0);
}

bool fuzzyCompare(long double a, long double b)
{
    return std::nextafter(a, std::numeric_limits<long double>::lowest()) <= b && std::nextafter(a, std::numeric_limits<long double>::max()) >= b;
}

bool fuzzyIsNull(long double a)
{
    return fuzzyCompare(a, (long double)0);
}

ObjectHierarchy::ObjectHierarchy()
{
}

ObjectHierarchy::ObjectHierarchy(QObject* obj)
    : _object(obj)
{
    Z_CHECK_NULL(_object);
}

QObject* ObjectHierarchy::object() const
{
    return _object;
}

void ObjectHierarchy::toDebug(QDebug debug) const
{
    if (_object.isNull()) {
        debug << "nullptr";
        return;
    }

    Core::beginDebugOutput();
    toDebugHelper(debug, _object);
    Core::endDebugOutput();
}

void ObjectHierarchy::toDebugHelper(QDebug debug, QObject* obj)
{
    debug << "\n";
    QString info;
    if (obj->objectName().isEmpty())
        info = obj->metaObject()->className();
    else
        info = QStringLiteral("%1(%2)").arg(obj->metaObject()->className()).arg(obj->objectName());

    debug << Core::debugIndent() << info;

    QWidget* w = qobject_cast<QWidget*>(obj);
    if (w != nullptr) {
        debug << ", " << w->geometry();
        debug << ", " << w->sizePolicy();
        debug << ", Hidden(" << w->isHidden() << ")";
    } else {
        QLayout* la = qobject_cast<QLayout*>(obj);
        if (la != nullptr) {
            debug << ", " << la->contentsMargins();
            debug << ", Spacing(" << la->spacing() << ")";
        }
    }

    if (!obj->children().isEmpty()) {
        Core::beginDebugOutput();
        for (int i = 0; i < obj->children().count(); i++) {
            toDebugHelper(debug, obj->children().at(i));
        }
        Core::endDebugOutput();
    }
}

QString utf(const char* s)
{
    return QString::fromUtf8(s);
}

QString utf(const QString& s)
{
    return s;
}

QString utf(const std::wstring& s)
{
    return QString::fromStdWString(s);
}

QString utf(const std::string& s)
{
    return QString::fromStdString(s);
}

Error utf(const Error& err)
{
    return err;
}

bool comp(const QString& s1, const QString& s2, Qt::CaseSensitivity cs)
{
    return s1.compare(s2, cs) == 0;
}

bool comp(const char* s1, const QString& s2, Qt::CaseSensitivity cs)
{
    return utf(s1).compare(s2, cs) == 0;
}

bool comp(const QString& s1, const char* s2, Qt::CaseSensitivity cs)
{
    return s1.compare(utf(s2), cs) == 0;
}

bool comp(const char* s1, const char* s2, Qt::CaseSensitivity cs)
{
    return utf(s1).compare(utf(s2), cs) == 0;
}

bool comp(const QStringRef& s1, const QStringRef& s2, Qt::CaseSensitivity cs)
{
    return s1.compare(s2, cs) == 0;
}

bool comp(const QString& s1, const QStringRef& s2, Qt::CaseSensitivity cs)
{
    return s1.compare(s2, cs) == 0;
}

bool comp(const QStringRef& s1, const QString& s2, Qt::CaseSensitivity cs)
{
    return s1.compare(s2, cs) == 0;
}

bool comp(const char* s1, const QStringRef& s2, Qt::CaseSensitivity cs)
{
    return utf(s1).compare(s2, cs) == 0;
}

bool comp(const QStringRef& s1, const char* s2, Qt::CaseSensitivity cs)
{
    return s1.compare(utf(s2), cs) == 0;
}

bool comp(const QModelIndex& i1, const QModelIndex& i2, int role, const QLocale* locale, CompareOptions options)
{
    if (i1.parent() == i2.parent()) {
        if (role >= 0)
            return Utils::compareVariant(i1.data(role), i2.data(role), CompareOperator::Less, locale, options);

        if (i1.row() != i2.row())
            return i1.row() < i2.row();

        return i1.column() < i2.column();
    }

    // находим цепочку родителей
    QModelIndexList i1_parents = {i1};
    while (i1_parents.constFirst().isValid()) {
        i1_parents.prepend(i1_parents.constFirst().parent());
    }

    QModelIndexList i2_parents = {i2};
    while (i2_parents.constFirst().isValid()) {
        i2_parents.prepend(i2_parents.constFirst().parent());
    }

    Z_CHECK(i2_parents.constFirst() == i1_parents.constFirst());
    Z_CHECK(!i2_parents.isEmpty() && !i1_parents.isEmpty());

    // ищем первое расхождение в родителях
    for (int i = 1;; i++) {
        if (i1_parents.at(i - 1) == i2_parents.at(i - 1)) {
            if (i2_parents.count() == i && i1_parents.count() > i)
                return false; // узел 1 вложен в узел 2
            if (i1_parents.count() == i && i2_parents.count() > i)
                return true; // узел 2 вложен в узел 1

            continue;
        }

        Z_CHECK(i1_parents.at(i - 1).parent() == i2_parents.at(i - 1).parent());
        return comp(i1_parents.at(i - 1), i2_parents.at(i - 1), role, locale, options);
    }

    Z_HALT_INT; // тут быть не должны
    return false;
}

qlonglong round(qreal v, RoundOption options)
{
    return static_cast<qlonglong>(roundReal(static_cast<long double>(v), options));
}

qreal roundPrecision(qreal v, int precision, RoundOption options)
{
    long double mul = static_cast<long double>(qPow(10, precision));
    return static_cast<qreal>(roundReal(static_cast<long double>(v) * mul, options) / mul);
}

bool Utils::isProxyExists()
{
    return proxyForUrl(QStringLiteral("www.google.com")).type() != QNetworkProxy::NoProxy;
}

QNetworkProxy Utils::proxyForUrl(const QUrl& url)
{
    auto proxies = QNetworkProxyFactory::systemProxyForQuery(QNetworkProxyQuery(url));
    for (auto& p : qAsConst(proxies)) {
        if (p.type() == QNetworkProxy::ProxyType::DefaultProxy)
            return p;
    }
    return proxies.isEmpty() ? QNetworkProxy() : proxies.constFirst();
}

QNetworkProxy Utils::proxyForUrl(const QString& url)
{
    return proxyForUrl(QUrl::fromUserInput(url));
}

int Utils::extractSvnRevision(const QString& svn_revision)
{
    QString s = svn_revision.simplified();
    if (s.right(1) == 'M')
        s.resize(s.length() - 1); // убираем признак модификации из ревизии SVN
    auto ver_split = s.split(":"); // убираем "двойную" ревизию (хрен знает что это такое)
    if (s.count() > 0)
        s = ver_split.constLast();

    return s.toInt();
}

Version Utils::getVersion()
{
    return Version(QString(MAJOR_VERSION_STR).simplified().toInt(), QString(MINOR_VERSION_STR).simplified().toInt(), extractSvnRevision(BUILD_VERSION_STR));
}

WaitWindow::WaitWindow(const QString& text, bool is_show)
    : _text(text)
{
    Z_CHECK(!_text.isEmpty());
    if (is_show)
        show();
}

WaitWindow::WaitWindow(WaitText type, bool is_show)
    : WaitWindow(waitText(type), is_show)
{
}

WaitWindow::WaitWindow(bool is_show)
    : WaitWindow(waitText(Waiting), is_show)
{
}

void WaitWindow::show()
{
    if (_window == nullptr)
        _window = Core::createSyncWaitWindow(_text);
    if (!_window->isVisible())
        _window->run();
}

void WaitWindow::hide()
{
    if (_window != nullptr)
        _window->hide();
}

WaitWindow::~WaitWindow()
{
    if (_window != nullptr)
        delete _window;
}

QString WaitWindow::waitText(WaitText type)
{
    switch (type) {
        case Loading:
            return ZF_TR(ZFT_LOADING);
        case Saving:
            return ZF_TR(ZFT_SAVING);
        case Waiting:
            return ZF_TR(ZFT_DATA_PROCESSING);
    }
    Z_HALT_INT;
    return {};
}

} // namespace zf

QDebug operator<<(QDebug debug, const zf::ObjectHierarchy& c)
{
    QDebugStateSaver saver(debug);
    debug.nospace();
    debug.noquote();

    c.toDebug(debug);

    return debug;
}
