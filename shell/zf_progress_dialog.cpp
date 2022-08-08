#include "zf_progress_dialog.h"
#include "ui_zf_progress_dialog.h"
#include "zf_core.h"
#include "zf_main_window.h"
#include "zf_colors_def.h"
#include "zf_translation.h"

#include <QDebug>
#include <QKeyEvent>
#include <QMovie>
#include <QWindow>

#ifdef Q_OS_WIN
#include <QWinTaskbarButton>
#include <QWinTaskbarProgress>
#endif

#define WAIT_IMAGE ":/share_icons/loader.gif"
#define Z_PROGRESS_COLOR QStringLiteral("#3CA111")

// Задержка начального отображения прогресса в мс
#define PROGRESS_DELAY 1000
// Интервал после которого принимается решение о визуальном завершении последней операции
#define VISUAL_FINISH_DELAY 5000
// Задержка обновления отображения прогресса в мс
#define UPDATE_PERIOD 500
// Задержка вызова processEvents в мс
#define UPDATE_PROCESS_EVENTS 50
//! Задержка скрытия общего прогресса
#define HIDE_OVERRAL_PERIOD 4000

namespace zf
{
ProgressDialog::ProgressDialog(MainWindow* window)
    : QDialog(window, Qt::FramelessWindowHint)
    , _ui(new Ui::ProgressDialog)
    , _window(window)
{
    _ui->setupUi(this);
    _ui->cancell_button->setText(zf::translate(TR::ZFT_STOP));

    Core::registerNonParentWidget(this);

#ifdef Q_OS_WIN
    setAttribute(Qt::WA_TranslucentBackground, true);
#endif
    /*
    _ui->outer_frame->setStyleSheet(QString("#outer_frame {"
                                            "background-color: %2;"
                                            "border: 1px solid %1;"
#ifdef Q_OS_WIN
                                            "border-radius: 5px;"
#endif
                                            "}")
                                        .arg(Colors::uiWindowColor().name(), Colors::uiButtonColor().name()));

    _ui->inner_frame->setStyleSheet(QString("#inner_frame {"
                                            "background-color: %2;"
                                            "border: 1px %1;"
                                            "border-top-style: solid;"
                                            "border-right-style: solid; "
                                            "border-bottom-style: solid; "
                                            "border-left-style: solid;"
#ifdef Q_OS_WIN
                                            "border-radius: 5px;"
#endif
                                            "}")
                                        .arg(Colors::uiDarkColor().name(), Colors::uiButtonColor().name()));
    */

    _ui->outer_frame->setStyleSheet(QString("#outer_frame {"
                                            "border: none;"
                                            "}"));

    _ui->inner_frame->setStyleSheet(QString("#inner_frame {"
                                            "font-size:%1pt; font-family: %2; "
                                            "border: 1px %3;"
                                            "border-top-style: solid;"
                                            "border-right-style: solid; "
                                            "border-bottom-style: solid; "
                                            "border-left-style: solid;"
                                            "background-color: %4;"
#ifdef Q_OS_WIN
                                            "border-radius: 5px;"
#endif
                                            "}")
                                        .arg(qApp->font().weight())
                                        .arg(qApp->font().family())
                                        .arg(Colors::uiLineColor(true).name())
                                        .arg(Colors::uiButtonColor().name()));

    connect(_ui->cancell_button, &QPushButton::clicked, this, &ProgressDialog::sl_cancel_click);

    auto movie = Utils::createWaitingMovie();
    _ui->loader_la->addWidget(movie);
    movie->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
    int movie_size = qApp->fontMetrics().capHeight() * 3;
    movie->movie()->setScaledSize({movie_size, movie_size});

    movie->show();
    movie->movie()->start();

    connect(Core::progress(), &ProgressObject::sg_startProgress, this, &ProgressDialog::sl_startProgress);
    connect(Core::progress(), &ProgressObject::sg_progressChange, this, &ProgressDialog::sl_progressChange);

    _relaxed_progress = new RelaxedProgress(Core::progress());
    _relaxed_progress->setRelaxedTimeout(PROGRESS_DELAY);
    connect(_relaxed_progress, &RelaxedProgress::sg_startProgress, this, &ProgressDialog::sl_startRelaxedProgress);
    connect(_relaxed_progress, &RelaxedProgress::sg_finishProgress, this, &ProgressDialog::sl_finishProgress);
    connect(_relaxed_progress, &RelaxedProgress::sg_progressChange, this, &ProgressDialog::sl_progressRelaxedChange);
    connect(_relaxed_progress, &RelaxedProgress::sg_cancelSupportActivated, this, &ProgressDialog::sl_cancelSupportActivated);

    _last_process_events_time = QDateTime::currentDateTime();
    reset();
}

ProgressDialog::~ProgressDialog()
{
    delete _relaxed_progress;
    delete _ui;
}

void ProgressDialog::keyPressEvent(QKeyEvent* e)
{
    switch (e->key()) {
        case Qt::Key_Escape:
        case Qt::Key_Enter:
        case Qt::Key_Return:
            if (_ui->cancell_button->isEnabled())
                sl_cancel_click();

            e->ignore();
            return;
        default:
            break;
    }

    QDialog::keyPressEvent(e);
}

void ProgressDialog::sl_cancel_click()
{
    if (Core::progress()->isProgress() && Core::progress()->isCancelProgressSupported()) {
        Core::progress()->cancelProgress();
        updateProgress();
    }
}

void ProgressDialog::sl_startProgress()
{
    if (Core::progress()->isLongOperation() && isHidden()) {
        _relaxed_progress->force();
    }
}

void ProgressDialog::sl_progressChange()
{
    if (Core::progress()->isLongOperation() && isHidden()) {
        _relaxed_progress->force();
    }
}

void ProgressDialog::sl_startRelaxedProgress()
{
    updateProgress();
}

void ProgressDialog::sl_finishProgress() //-V524
{
    updateProgress();
}

void ProgressDialog::sl_progressRelaxedChange()
{
    updateProgress();
}

void ProgressDialog::sl_cancelSupportActivated()
{
    updateProgress();
}

void ProgressDialog::reset()
{
    _last_progress_key.clear();
    _ui->overral_progress->setHidden(true);
    _overral_progress_hide_request = QDateTime();

    setOverralProgressValue(0, true);
    setProgressStyleLess50();
    setProgressValue(0);

    _last_update_time_valid = false;
    _canceled = false;
    _ui->cancell_button->setHidden(true);
    _ui->cancell_button->setText("Остановить");
    _ui->button_spacer->changeSize(0, 0, QSizePolicy::Minimum, QSizePolicy::Fixed);

    _ui->label->setText(QString());
}

void ProgressDialog::updateProgress()
{
    ProgressObject* progress = Core::progress();

    if (!progress->isProgress()) {
        reset();
        updatePosition(true);
        hide();
        emit sg_hide();
        setModal(false);
        setWindowOpacity(0); // Борьба с глюком Qt (подвисание диалога после закрытия)
#ifdef Q_OS_WIN
        if (_window != nullptr && _window->taskbarButton() != nullptr)
            _window->taskbarButton()->progress()->setVisible(false);
#endif
        return;
    }

    if (isHidden()) {
        if (_window != nullptr)
            setWindowTitle(_window->windowTitle());
        updatePosition(false);
        setModal(true);
        setWindowOpacity(100); // Борьба с глюком Qt (подвисание диалога после закрытия)
        show();
        emit sg_show();
#ifdef Q_OS_WIN
        if (_window != nullptr && _window->taskbarButton() != nullptr)
            _window->taskbarButton()->progress()->setVisible(true);
#endif
        processEvents(true);

    } else {
        processEvents(false);
    }

    bool cancel_changed = false;
    if (_ui->cancell_button->isHidden() && progress->isCancelProgressSupported()) {
        cancel_changed = true;
        _ui->cancell_button->setVisible(true);
        _ui->button_spacer->changeSize(0, 10, QSizePolicy::Minimum, QSizePolicy::Fixed);
    }
    bool cancel_enabled = progress->isCancelProgressSupported() && !progress->isCanceled();
    if (_ui->cancell_button->isEnabled() != cancel_enabled) {
        _ui->cancell_button->setEnabled(cancel_enabled);
        if (cancel_enabled)
            _ui->cancell_button->setFocus();
        cancel_changed = true;
    }
    if (_ui->cancell_button->isVisible() && !_ui->cancell_button->isEnabled() && _canceled != progress->isCanceled()) {
        cancel_changed = true;
        _canceled = true;
        _ui->cancell_button->setText("Остановка...");
    }
    QDateTime current_time = QDateTime::currentDateTime();
    std::shared_ptr<ProgressObject::Progress> top_item;
    std::shared_ptr<ProgressObject::Progress> bottom_item;
    // ищем операцию верхнего уровня
    for (auto i = progress->progressStack().begin(); i != progress->progressStack().end(); ++i) {
        auto item = (*i);
        if (item->percent > 0) {
            top_item = item;
            break;
        }
    }
    // ищем операцию нижнего уровня
    for (auto i = progress->progressStack().rbegin(); i != progress->progressStack().rend(); ++i) {
        auto item = (*i);
        if (item == top_item || item->start_time.msecsTo(current_time) > PROGRESS_DELAY) {
            bottom_item = item;
            break;
        }
    }
    // если не нашли ничего нижнего уровня, то берем верхнего
    if (bottom_item == nullptr) {
        bottom_item = progress->progressStack().first();
        top_item = bottom_item;
    }

    if (_last_progress_key == bottom_item->key && (_last_update_time_valid && _last_update_time.msecsTo(current_time) < UPDATE_PERIOD)) {
        if (cancel_changed)
            updatePosition(true);
        return;
    }
    _last_update_time = current_time;
    _last_update_time_valid = true;
    _last_progress_key = bottom_item->key;

    bool text_changed = false;
    if (_ui->label->text() != bottom_item->text) {
        text_changed = true;
        _ui->label->setText(bottom_item->text);
    }

    transition(bottom_item->percent);

    // отображение общего прогресса
    bool overral_changed = false;
    if (top_item != nullptr && bottom_item != top_item) {
        overral_changed = _ui->overral_progress->isHidden();
        _ui->overral_progress->setVisible(true);
    }

    if (_ui->overral_progress->isVisible()) {
        Z_CHECK_NULL(top_item);
        setOverralProgressValue(top_item->percent, overral_changed);

        if (top_item == bottom_item || top_item->percent == 0) {
            // надо запустить ожидание скрытия общего прогресса, т.к. он уже не нужен
            if (!_overral_progress_hide_request.isNull()) {
                if (_overral_progress_hide_request.msecsTo(current_time) > HIDE_OVERRAL_PERIOD) {
                    _overral_progress_hide_request = QDateTime();
                    _ui->overral_progress->setHidden(true);
                    setOverralProgressValue(0);
                }
            } else {
                _overral_progress_hide_request = current_time;
            }
        } else {
            _overral_progress_hide_request = QDateTime();
        }

    } else {
        _overral_progress_hide_request = QDateTime();
    }

    if (cancel_changed || text_changed || overral_changed)
        updatePosition(true);
}

void ProgressDialog::updatePosition(bool process_events)
{
    if (process_events)
        processEvents(true);
    updateGeometry();
    if (process_events)
        processEvents(true);
    adjustSize();
    if (process_events)
        processEvents(true);

    QPoint global;
    if (_window != nullptr)
        global = _window->mapToGlobal(_window->rect().center());
    else
        global = Utils::screenRect().center();

    int posX = global.x() - width() / 2;
    int posY = global.y() - height() / 2;
    if (qAbs(pos().x() - posX) > 10 || qAbs(pos().y() - posY) > 100)
        move(posX, posY);

    if (process_events)
        processEvents(true);
}

void ProgressDialog::processEvents(bool force)
{
    QDateTime current_time = QDateTime::currentDateTime();
    if (!force && _last_process_events_time.msecsTo(current_time) < UPDATE_PROCESS_EVENTS)
        return;

    _last_process_events_time = current_time;
    Utils::processEvents();
}

bool ProgressDialog::transition(int percent)
{
    bool changed = false;

    int current = progressValue();

    if (current > percent) {
        setProgressValue(0);
        processEvents(true);
        current = 0;
        changed = true;
    }

    for (int i = current + 1; i <= percent; i++) {
        setProgressValue(i);
        processEvents(true);
        changed = true;
    }

    return changed;
}

void ProgressDialog::setProgressValue(int i, bool force)
{
    if (i <= 52) {
        if (force || _progressValue > 52)
            setProgressStyleLess50();
    } else {
        if (force || _progressValue <= 52)
            setProgressStyleMore50();
    }

    _progressValue = i;
    // Иначе не понятно что что-то происходит
    _ui->current_progress->setValue(i < 1 ? 1 : i);

#ifdef Q_OS_WIN
    if (_window != nullptr && _window->taskbarButton() != nullptr)
        _window->taskbarButton()->progress()->setValue(_ui->current_progress->value());
#endif
}

void ProgressDialog::setOverralProgressValue(int i, bool force)
{
    if (i <= 52) {
        if (force || _ui->overral_progress->value() > 52)
            setOverralProgressStyleLess50();
    } else {
        if (force || _ui->overral_progress->value() <= 52)
            setOverralProgressStyleMore50();
    }

    // Иначе не понятно что что-то происходит
    _ui->overral_progress->setValue(i < 1 ? 1 : i);
}

void ProgressDialog::setProgressStyleLess50()
{
    int bottom_radius = _ui->overral_progress->isHidden() ? 2 : 0;

    _ui->current_progress->setStyleSheet(QStringLiteral("QProgressBar {color: black; border: 1px solid %1;"
                                                        "border-top-left-radius: 2px; border-top-right-radius: 2px; "
                                                        "border-bottom-left-radius: %3px; border-bottom-right-radius: %3px;}"
                                                        "QProgressBar::chunk {background-color: %2;width: 1px;}")
                                             .arg(Colors::uiDarkColor().name(), Z_PROGRESS_COLOR)
                                             .arg(bottom_radius));
}

void ProgressDialog::setProgressStyleMore50()
{
    int bottom_radius = _ui->overral_progress->isHidden() ? 2 : 0;

    _ui->current_progress->setStyleSheet(QStringLiteral("QProgressBar {color: white; border: 1px solid %1;"
                                                        "border-top-left-radius: 2px; border-top-right-radius: 2px; "
                                                        "border-bottom-left-radius: %3px; border-bottom-right-radius: %3px;}"
                                                        "QProgressBar::chunk {background-color: %2;width: 1px;}")
                                             .arg(Colors::uiDarkColor().name(), Z_PROGRESS_COLOR)
                                             .arg(bottom_radius));
}

void ProgressDialog::setOverralProgressStyleLess50()
{
    _ui->overral_progress->setStyleSheet(QStringLiteral("QProgressBar {color: black; border: 1px solid %1;"
                                                        "border-top: none;"
                                                        "border-top-left-radius: 0px; border-top-right-radius: 0px; "
                                                        "border-bottom-left-radius: 2px; border-bottom-right-radius: 2px;}"
                                                        "QProgressBar::chunk {background-color: %2;width: 1px; margin-top: 1;}")
                                             .arg(Colors::uiDarkColor().name(), Z_PROGRESS_COLOR));
}

void ProgressDialog::setOverralProgressStyleMore50()
{
    _ui->overral_progress->setStyleSheet(QStringLiteral("QProgressBar {color: white; border: 1px solid %1;"
                                                        "border-top: none;"
                                                        "border-top-left-radius: 0px; border-top-right-radius: 0px; "
                                                        "border-bottom-left-radius: 2px; border-bottom-right-radius: 2px;}"
                                                        "QProgressBar::chunk {background-color: %2;width: 1px; margin-top: 1;}")
                                             .arg(Colors::uiDarkColor().name(), Z_PROGRESS_COLOR));
}

} // namespace zf
