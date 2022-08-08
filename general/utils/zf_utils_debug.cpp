#include "zf_utils.h"

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
#include <QPdfDocument>
#include <QPdfDocumentRenderOptions>

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
#include "zf_data_container.h"

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

#ifdef _MSC_VER
#define PATH_MAX MAX_PATH
#endif

#ifdef __MINGW32__
#include <limits>
#endif

#else
#include <linux/limits.h>
#include <unistd.h>
#endif

namespace zf
{
void Utils::debugDataset(const QAbstractItemModel* model, int role, const QModelIndex& parent)
{
    Z_CHECK_NULL(model);
    Core::logInfo(QStringLiteral("DEBUG_DATASET BEGIN: NAME(%1) ROW_COUNT(%2) COLUMN_COUNT(%3)")
                      .arg(model->objectName())
                      .arg(model->rowCount())
                      .arg(model->columnCount()));
    debugDataset_helper(model, role, parent);
    Core::logInfo(QStringLiteral("DEBUG_DATASET END: NAME(%1)").arg(model->objectName()));
}

void Utils::debugDatasetShow(const QAbstractItemModel* model)
{
    Z_CHECK_NULL(model);

    Dialog dlg;
    dlg.setDialogCode("debugDatasetShow");
    dlg.setWindowTitle(utf("Отладка набора данных"));

    TableView* view = new TableView;
    view->setModel(const_cast<QAbstractItemModel*>(model));

    view->setEditTriggers(QAbstractItemView::NoEditTriggers);

    QVBoxLayout* la = new QVBoxLayout;
    la->setMargin(0);
    la->addWidget(view);

    dlg.setBottomLineVisible(false);
    dlg.workspace()->setLayout(la);
    dlg.exec();
}

void Utils::debugDatasetShow(const ModelPtr& model, const PropertyID& dataset)
{
    Z_CHECK_NULL(model);
    Z_CHECK(dataset.isValid());

    auto error = Core::waitForLoadModel(model);
    if (error.isError()) {
        Core::error(error);
        return;
    }

    debugDatasetShow(model->data(), dataset);
}

void Utils::debugDatasetShow(const DataContainerPtr& container, const PropertyID& dataset)
{
    Z_CHECK_NULL(container);
    Z_CHECK(dataset.isValid());

    auto ds_prop = container->property(dataset);

    Dialog dlg;
    dlg.setDialogCode("debugDatasetShow");
    dlg.setWindowTitle(ds_prop.name());

    ObjectExtensionPtr<DataWidgets> widgets = new DataWidgets(container);
    auto view = widgets->field<QAbstractItemView>(dataset);
    view->setEditTriggers(QAbstractItemView::NoEditTriggers);
    widgets->showAllDatasetHeaders(ds_prop);

    QVBoxLayout* la = new QVBoxLayout;
    la->setMargin(0);
    la->addWidget(view);

    dlg.setBottomLineVisible(false);
    dlg.workspace()->setLayout(la);
    dlg.exec();
}

void Utils::debugDataShow(const ModelPtr& model)
{
    Z_CHECK_NULL(model);

    auto error = Core::waitForLoadModel(model);
    if (error.isError()) {
        Core::error(error);
        return;
    }

    debugDataShow(model->data());
}

void Utils::debugDataShow(const DataContainerPtr& container)
{
    Z_CHECK_NULL(container);

    ObjectExtensionPtr<DataWidgets> widgets = new DataWidgets(container);
    WidgetScheme scheme(widgets.get());

    bool has_borders;
    QWidget* form = scheme.generate(QString::number(container->structure()->entityCode().value()), has_borders, false, true, true);

    QVBoxLayout* la = new QVBoxLayout();
    la->setMargin(0);
    la->setSpacing(0);

    auto deb_print = new QPushButton("DebPrint");
    deb_print->connect(deb_print, &QPushButton::clicked, deb_print, [container]() { container->debPrint(); });

    auto button_la = new QHBoxLayout;
    button_la->setMargin(6);
    button_la->addWidget(deb_print);
    button_la->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Expanding));

    la->addLayout(button_la);
    la->addWidget(Utils::createLine(Qt::Horizontal));
    la->addWidget(form);

    Dialog dlg;
    dlg.setDialogCode("debugDataShow");
    dlg.setWindowTitle("Container data");

    dlg.setBottomLineVisible(true);
    dlg.workspace()->setLayout(la);
    dlg.exec();
}

} // namespace zf
