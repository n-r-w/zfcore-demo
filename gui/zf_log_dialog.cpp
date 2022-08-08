#include "zf_log_dialog.h"
#include "ui_zf_log_dialog.h"
#include "zf_core.h"
#include "zf_log.h"
#include "zf_log_widget.h"
#include "zf_translation.h"

#include <QFileDialog>
#include <QFileInfo>
#include <QIcon>
#include <QLabel>
#include <QVBoxLayout>

namespace zf
{
LogDialog::LogDialog(const Log* log, const QIcon& messageIcon, const QString& message)
    : Dialog(getButtonsList(), getButtonText(), getButtonRoles())
    , _ui(new Ui::LogDialogUI)
    , _log(log)
{
    Z_CHECK(!messageIcon.isNull());
    Z_CHECK_NULL(_log);

    Core::registerNonParentWidget(this);

    setWindowFlags(windowFlags() | Qt::WindowMaximizeButtonHint);

    setBottomLineVisible(false);

    QString s = message.trimmed().isEmpty() ? _log->name() : message.trimmed();

    _ui->setupUi(workspace());
    _logWidget = new LogWidget(_log);
    _logWidget->setObjectName(QStringLiteral("zf_log"));

    connect(_logWidget, &LogWidget::sg_beforeRefilter, this, &LogDialog::sl_beforeRefilter);
    connect(_logWidget, &LogWidget::sg_afterRefilter, this, &LogDialog::sl_afterRefilter);

    _ui->tableLayout->addWidget(_logWidget);
    _ui->icon->setPixmap(messageIcon.pixmap(Z_PM(PM_LargeIconSize)));
    _ui->message->setText(s);

    if (_log->detail().isEmpty()) {
        _ui->extendedInfo->setVisible(false);
        _ui->extendedInfoLine->setVisible(false);
    } else {
        _ui->extendedInfo->setText(_log->detail());

        QFont font = _ui->message->font();
        font.setUnderline(false);
        _ui->message->setFont(font);
    }

    // Если журнал содержит только однотипные записи без ошибок, то фильтр не нужен
    int total = log->count();
    if (log->count(InformationType::Success) == total || log->count(InformationType::Information) == total) {
        _ui->filterPanel->setVisible(false);
        _ui->infoPanel->setVisible(false);
        _ui->extendedInfoLine->setVisible(false);
        _logWidget->hideRecordTypeTypeColumn();
    }

    connect(_ui->filter, qOverload<int>(&QComboBox::currentIndexChanged), this, &LogDialog::filterIndexChanged);
    setModal(true);
}

LogDialog::~LogDialog()
{
    delete _ui;
}

void LogDialog::setVisible(bool visible)
{
    Dialog::setVisible(visible);
    if (visible)
        _logWidget->setFocus();
}

void LogDialog::adjustDialogSize()
{
    setMinimumWidth(500);
    resize(700, 500);
}

bool LogDialog::onButtonClick(QDialogButtonBox::StandardButton button)
{
    if (button == QDialogButtonBox::Save) {
        QString fName = Core::lastUsedDirectory() + "/" + Utils::validFileName(_log->name()) + ".csv";

        fName = Utils::getSaveFileName(_log->name(), fName, "CSV (*.csv)");
        if (fName.isEmpty())
            return true;

        QFile file(fName);
        if (!file.open(QIODevice::ReadWrite | QIODevice::Truncate)) {
            Core::error(ZF_TR(ZFT_FILE_SAVE_ERROR).arg(QDir::toNativeSeparators(fName)));
            return true;
        }
        Core::updateLastUsedDirectory(QFileInfo(fName).absolutePath());

        Error error = _log->saveCSV(&file, _logWidget->filter());
        file.close();

        if (error.isError())
            Core::error(error);
        else
            Core::info(ZF_TR(ZFT_FILE_SAVED).arg(QDir::toNativeSeparators(fName)));

        return true;
    } else
        return Dialog::onButtonClick(button);
}

void LogDialog::doLoad()
{
    _ui->total->setText(QString::number(_log->count()));

    _ui->errors->setText(QString::number(_log->count(InformationType::Error) + _log->count(InformationType::Critical)
        + _log->count(InformationType::Fatal) + _log->count(InformationType::Required)));

    _ui->warnings->setText(QString::number(_log->count(InformationType::Warning)));

    _ui->informations->setText(QString::number(_log->count(InformationType::Information)));

    _ui->successes->setText(QString::number(_log->count(InformationType::Success)));
}

void LogDialog::filterIndexChanged(int index)
{
    if (index == 0)
        _logWidget->setFilter();
    else if (index == 1)
        _logWidget->setFilter(InformationType::Error);
    else if (index == 2)
        _logWidget->setFilter(InformationType::Warning);
    else if (index == 3)
        _logWidget->setFilter(InformationType::Information);
    else if (index == 4)
        _logWidget->setFilter(InformationType::Success);
    else
        Z_HALT_INT;
}

void LogDialog::sl_beforeRefilter()
{
    _savedRecord = _logWidget->currentRecord();
}

void LogDialog::sl_afterRefilter()
{
    if (_savedRecord.isValid())
        _logWidget->setCurrentRecord(_savedRecord);
}

QList<int> LogDialog::getButtonsList() const
{
    QList<int> res;
    res << QDialogButtonBox::Ok << QDialogButtonBox::Save;
    return res;
}

QStringList LogDialog::getButtonText() const
{
    QStringList res;
    res << utf("Закрыть") << utf("Сохранить в файл");
    return res;
}

QList<int> LogDialog::getButtonRoles() const
{
    QList<int> res;
    res << QDialogButtonBox::AcceptRole << QDialogButtonBox::ResetRole;
    return res;
}
} // namespace zf
