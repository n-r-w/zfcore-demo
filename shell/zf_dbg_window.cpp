#include "zf_dbg_window.h"

#include "ui_zf_dbg_window.h"
#include "zf_core.h"
#include "zf_table_view.h"

#include <QTabWidget>
#include <QTableWidget>

namespace zf
{
DebugWindow::DebugWindow()
    : QMdiSubWindow()
    , _ui(new Ui::DebugWindow)
    , _scroll(new QTimer(this))
    , _object_extension(new ObjectExtension(this))

{
    setWindowTitle("Debug");
    setWindowIcon(qApp->windowIcon());

    QWidget* body = new QWidget;
    _ui->setupUi(body);
    setWidget(body);

    _data.setColumnCount(7);
    _view = new TableView;
    _view->setEditTriggers(QTableView::NoEditTriggers);
    _view->setModel(&_data);
    _view->verticalHeader()->hide();
    _view->horizontalRootHeaderItem()->append(0, "#");
    _view->horizontalRootHeaderItem()->append(1, "Отправитель");
    _view->horizontalRootHeaderItem()->append(2, "Получатель");
    _view->horizontalRootHeaderItem()->append(3, "Вид сообщения");
    _view->horizontalRootHeaderItem()->append(4, "ID");
    _view->horizontalRootHeaderItem()->append(5, "Feedback ID");
    _view->horizontalRootHeaderItem()->append(6, "Subscribe handle");

    _ui->verticalLayout->addWidget(_view);

    connect(_ui->clearButton, &QPushButton::clicked, this, [&]() {
        _data.setRowCount(0);
        _counter = 0;
    });

    connect(_ui->maxCount, qOverload<int>(&QSpinBox::valueChanged), this, [&](int) { _scroll->start(); });

    _scroll->setSingleShot(true);
    _scroll->setInterval(1000);
    connect(_scroll, &QTimer::timeout, this, [&]() {
        for (auto& b : _buffer) {
            _counter++;
            Z_CHECK(b.count() == 6);

            QList<QStandardItem*> items;
            items << new QStandardItem(QString::number(_counter));
            QStringList dbg_message;
            dbg_message << QString("#%1").arg(_counter);
            for (int i = 0; i < b.count(); i++) {
                QString s = b.at(i).toString();
                items << new QStandardItem(s);
                dbg_message << s;
            }

            _data.insertRow(0, items);

            Core::logInfo("MSGDEBUG", dbg_message.join(" | "));
        }
        _buffer.clear();

        checkMax();

        _view->scrollToTop();
    });

    Core::messageDispatcher()->registerObject(CoreUids::SHELL_DEBUG, this, "sl_message_dispatcher_inbound");
    Core::messageDispatcher()->registerChannel(CoreChannels::MESSAGE_DEBUG);
    Core::messageDispatcher()->subscribe(CoreChannels::MESSAGE_DEBUG, this);
}

DebugWindow::~DebugWindow()
{
    delete _object_extension;
}

bool DebugWindow::objectExtensionDestroyed() const
{
    return _object_extension->objectExtensionDestroyed();
}

void DebugWindow::objectExtensionDestroy()
{
    _object_extension->objectExtensionDestroy();
}

QVariant DebugWindow::objectExtensionGetData(const QString& data_key) const
{
    return _object_extension->objectExtensionGetData(data_key);
}

bool DebugWindow::objectExtensionSetData(const QString& data_key, const QVariant& value, bool replace) const
{
    return _object_extension->objectExtensionSetData(data_key, value, replace);
}

void DebugWindow::objectExtensionRegisterUseExternal(I_ObjectExtension* i)
{
    _object_extension->objectExtensionRegisterUseExternal(i);
}

void DebugWindow::objectExtensionUnregisterUseExternal(I_ObjectExtension* i)
{
    _object_extension->objectExtensionUnregisterUseExternal(i);
}

void DebugWindow::objectExtensionDeleteInfoExternal(I_ObjectExtension* i)
{
    _object_extension->objectExtensionDeleteInfoExternal(i);
}

void DebugWindow::objectExtensionRegisterUseInternal(I_ObjectExtension* i)
{
    _object_extension->objectExtensionRegisterUseInternal(i);
}

void DebugWindow::objectExtensionUnregisterUseInternal(I_ObjectExtension* i)
{
    _object_extension->objectExtensionUnregisterUseInternal(i);
}

void DebugWindow::sl_message_dispatcher_inbound(const Uid& sender, const Message& message, zf::SubscribeHandle subscribe_handle)
{
    Q_UNUSED(sender)
    Q_UNUSED(subscribe_handle)

    Core::messageDispatcher()->confirmMessageDelivery(message, this);

    if (message.messageType() != MessageType::VariantList)
        return;

    if (!isVisible())
        return;

    _buffer << VariantListMessage(message).data();

    if (!_scroll->isActive())
        _scroll->start();
}

int DebugWindow::getMaxCount() const
{
    return _ui->maxCount->value();
}

void DebugWindow::checkMax()
{
    if (_data.rowCount() > getMaxCount())
        _data.setRowCount(getMaxCount());
}

} // namespace zf
