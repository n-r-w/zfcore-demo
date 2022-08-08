#include "zf_sync_ask.h"
#include "zf_colors_def.h"
#include "zf_core.h"
#include "NcFramelessHelper.h"
#include <QMovie>
#include <QtConcurrent>
#include <QMainWindow>

namespace zf
{
//! Минимальное время показа диалога
const int SyncWaitWindow::MINIMUM_TIME_MS = 1000;

bool SyncAskDialog::run(const QString& question, const QString& wait_info, SyncAskDialogFunction sync_function, Error& error,
                        const QString& result_info, bool show_error, bool run_in_thread, const zf::Uid& progress_uid)
{
    error.clear();

    SyncAskDialog* dlg = new SyncAskDialog(question, wait_info, sync_function, result_info, show_error, run_in_thread, progress_uid);
    if (dlg->exec() != QDialogButtonBox::Yes)
        return false;

    error = dlg->_error;
    dlg->objectExtensionDestroy();

    return true;
}

bool SyncAskDialog::onButtonClick(QDialogButtonBox::StandardButton button)
{
    if (button == QDialogButtonBox::Yes) {
        hide();

        SyncWaitWindow wait_dlg(_wait_info, _progress_uid);
        wait_dlg.run();

        QElapsedTimer timer;
        timer.start();

        if (_run_in_thread) {
            QEventLoop loop;
            QFutureWatcher<Error> watcher;
            watcher.connect(&watcher, &QFutureWatcher<int>::finished, &watcher, [&loop]() { loop.quit(); });
            QFuture<Error> future = QtConcurrent::run(_sync_function);
            watcher.setFuture(future);
            loop.exec();
            _error = future.result();

        } else {
            _error = _sync_function();
        }

        if (timer.elapsed() < SyncWaitWindow::MINIMUM_TIME_MS)
            Utils::delay(SyncWaitWindow::MINIMUM_TIME_MS - timer.elapsed());

        wait_dlg.hide();

        if (_error.isError()) {
            if (_show_error)
                zf::Core::error(_error);

        } else if (!_result_info.isEmpty()) {
            zf::Core::info(_result_info);
        }
    }

    return MessageBox::onButtonClick(button);
}

SyncAskDialog::SyncAskDialog(const QString& question, const QString& wait_info, SyncAskDialogFunction sync_function,
                             const QString& result_info, bool show_error, bool run_in_thread, const Uid& progress_uid)
    : MessageBox(MessageType::Ask, question)
    , _wait_info(wait_info)
    , _result_info(result_info)
    , _sync_function(sync_function)
    , _show_error(show_error)
    , _run_in_thread(run_in_thread)
    , _progress_uid(progress_uid)
{
    Z_CHECK(!wait_info.isEmpty());
    Z_CHECK_NULL(sync_function);
}

SyncWaitWindow::SyncWaitWindow(const QString& text, const Uid& progress_uid)
    : QDialog(Utils::getTopWindow(), Qt::FramelessWindowHint)
    , _progress_uid(progress_uid.isValid()? progress_uid : generateProgressUid())
    , _object_extension(new ObjectExtension(this))
{
    Z_CHECK(!text.isEmpty());

    connect(&_timer, &FeedbackTimer::timeout, this, [this]() { updateText(); });

    if (_progress_uid.isValid())
        Core::messageDispatcher()->registerObject(_progress_uid, this, "sl_message_dispatcher_inbound");

    Core::registerNonParentWidget(this);
    setModal(true);

    auto main_la = new QVBoxLayout;
    main_la->setMargin(0);
    setLayout(main_la);

    auto frame = new QFrame;
    frame->setObjectName("main_frame");
    auto frame_la = new QVBoxLayout;
    frame_la->setMargin(15);
    frame_la->setSpacing(10);

    frame->setLayout(frame_la);
    main_la->addWidget(frame);

    _text_lalel = new QLabel;
    frame_la->addWidget(_text_lalel);

    auto loader = new QLabel;
    loader->setAlignment(Qt::AlignCenter);
    auto wait_movie = new QMovie(":/share_icons/loader-horizontal-16.gif", {}, this);
    loader->setMovie(wait_movie);
    frame_la->addWidget(loader);
    wait_movie->start();

#ifdef Q_OS_WIN
    setAttribute(Qt::WA_TranslucentBackground, true);
#endif

    frame->setStyleSheet(QString("#%4 {"
                                 "font-size:%1pt; font-family: %2; "
                                 "border: 1px %3;"
                                 "border-top-style: solid;"
                                 "border-right-style: solid; "
                                 "border-bottom-style: solid; "
                                 "border-left-style: solid;"
                                 "background-color: %5;"
#ifdef Q_OS_WIN
                                 "border-radius: 5px;"
#endif
                                 "}")
                             .arg(qApp->font().weight())
                             .arg(qApp->font().family())
                             .arg(Colors::uiLineColor(true).name())
                             .arg(frame->objectName())
                             .arg(Colors::uiButtonColor().name()));

    setText(text);
}

SyncWaitWindow::~SyncWaitWindow()
{
    if (_progress_uid.isValid())
        Core::messageDispatcher()->unRegisterObject(this);
    delete _object_extension;
}

Uid SyncWaitWindow::progressUid() const
{
    return _progress_uid;
}

bool SyncWaitWindow::objectExtensionDestroyed() const
{
    return _object_extension->objectExtensionDestroyed();
}

void SyncWaitWindow::objectExtensionDestroy()
{
    _object_extension->objectExtensionDestroy();
}

QVariant SyncWaitWindow::objectExtensionGetData(const QString& data_key) const
{
    return _object_extension->objectExtensionGetData(data_key);
}

bool SyncWaitWindow::objectExtensionSetData(const QString& data_key, const QVariant& value, bool replace) const
{
    return _object_extension->objectExtensionSetData(data_key, value, replace);
}

void SyncWaitWindow::objectExtensionRegisterUseExternal(I_ObjectExtension* i)
{
    _object_extension->objectExtensionRegisterUseExternal(i);
}

void SyncWaitWindow::objectExtensionUnregisterUseExternal(I_ObjectExtension* i)
{
    _object_extension->objectExtensionUnregisterUseExternal(i);
}

void SyncWaitWindow::objectExtensionDeleteInfoExternal(I_ObjectExtension* i)
{
    _object_extension->objectExtensionDeleteInfoExternal(i);
}

void SyncWaitWindow::objectExtensionRegisterUseInternal(I_ObjectExtension* i)
{
    _object_extension->objectExtensionRegisterUseInternal(i);
}

void SyncWaitWindow::objectExtensionUnregisterUseInternal(I_ObjectExtension* i)
{
    _object_extension->objectExtensionUnregisterUseInternal(i);
}

void SyncWaitWindow::run()
{    
    show();
    raise();
    adjustSize();
    Utils::centerWindow(this, true);
}

void SyncWaitWindow::setText(const QString& text)
{
    _base_text = text;
    _timer.start();
}

SyncWaitWindow* SyncWaitWindow::run(const QString& text, const zf::Uid& progress_uid)
{
    auto w = new SyncWaitWindow(text, progress_uid);
    w->run();
    return w;
}

Uid SyncWaitWindow::generateProgressUid()
{
    return Uid::general(QVariantList { "SyncWaitWindow", Utils::generateGUID() });
}

void SyncWaitWindow::showEvent(QShowEvent* event)
{
    QWidget::showEvent(event);
}

void SyncWaitWindow::hideEvent(QHideEvent* event)
{
    QWidget::hideEvent(event);
}

bool SyncWaitWindow::event(QEvent* event)
{
    return QWidget::event(event);
}

void SyncWaitWindow::sl_message_dispatcher_inbound(const Uid& sender, const Message& message, SubscribeHandle subscribe_handle)
{
    Q_UNUSED(sender);
    Q_UNUSED(subscribe_handle);

    if (!isVisible())
        return;

    if (message.messageType() == MessageType::Progress) {
        ProgressMessage pm(message);
        _progress_percent = pm.percent();
        _progress_text = pm.info();
        _timer.start();
    }
}

void SyncWaitWindow::updateText()
{
    QString text;

    if (_progress_uid.isValid() && _progress_percent > 0) {
        text = (_progress_text.isEmpty() ? _base_text : _progress_text).trimmed();
        if (text.right(1) == QStringLiteral(":"))
            text.remove(text.length() - 1, 1);
        if (text.right(3) == QStringLiteral("..."))
            text.remove(text.length() - 3, 3);

        text = QStringLiteral("%1: %2%").arg(text).arg(_progress_percent);

    } else {
        text = _base_text;
    }

    _text_lalel->setText(text);
    adjustSize();
    Utils::centerWindow(this, true);
}

} // namespace zf
