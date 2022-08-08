#include "zf_highlight_view.h"
#include "zf_core.h"
#include "zf_html_tools.h"
#include "zf_framework.h"
#include "private/zf_highlight_p.h"
#include "zf_highlight_mapping.h"
#include "zf_ui_size.h"

#include <QPainter>
#include <qdrawutil.h>

namespace zf
{
HighlightView::HighlightView(HighlightProcessor* processor, I_HighlightView* interface, HighlightMapping* mapping, QWidget* parent)
    : QWidget(parent)
    , _highlight_processor(processor)
    , _interface(interface)
    , _mapping(mapping)
    , _object_extension(new ObjectExtension(this))
{
    Z_CHECK_NULL(_highlight_processor);
    Z_CHECK_NULL(_interface);
    Z_CHECK_NULL(_mapping);

    QObject* interface_object = dynamic_cast<QObject*>(interface);
    Z_CHECK_NULL(interface_object);
    connect(interface_object, SIGNAL(sg_highlightViewSortOrderChanged()), this, SLOT(sl_highlightSortOrderChanged()));

    Framework::internalCallbackManager()->registerObject(this, "sl_callbackManager");

    _view_model = new ItemModel(_highlight_processor->highlight()->count(), 1, this);

    setLayout(new QVBoxLayout);
    layout()->setMargin(0);

    _ok_image = new QLabel;

    int pixmap_size = Utils::scaleUI(64);
    _ok_image->setPixmap(Utils::resizeSvg(":/share_icons/green/check.svg", pixmap_size).pixmap(pixmap_size, pixmap_size));

    _ok_image->setAlignment(Qt::AlignCenter);
    _ok_image->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    _view = new HighlightTableView(this);

    _view->setFocusPolicy(Qt::NoFocus);
    _view->setMinimumWidth(100);
    _view->setFrameShape(QFrame::NoFrame);
    _view->setEditTriggers(QListView::NoEditTriggers);
    _view->setSelectionMode(QListView::SingleSelection);
    _view->setSelectionBehavior(QListView::SelectRows);    
    _view->setWordWrap(true);
    _view->setHidden(true);
    _view->setShowGrid(false);
    _view->setCornerButtonEnabled(false);
    _view->horizontalHeader()->setVisible(false);
    _view->horizontalHeader()->setHighlightSections(false);
    _view->horizontalHeader()->setStretchLastSection(true);
    _view->horizontalHeader()->setCascadingSectionResizes(false);
    _view->verticalHeader()->setDefaultSectionSize(UiSizes::defaultTableRowHeight());
    _view->verticalHeader()->setVisible(false);
    _view->verticalHeader()->setHighlightSections(false);
    _view->verticalHeader()->setStretchLastSection(false);
    _view->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
    _view->setItemDelegate(new HighlightDelegate(this));

    connect(_view->horizontalHeader(), &QHeaderView::sectionResized, this, &HighlightView::sl_resizeToContent);

    _view->setModel(_view_model);
    layout()->addWidget(_view);
    layout()->addWidget(_ok_image);

    connect(_highlight_processor, &HighlightProcessor::sg_highlightItemInserted, this, &HighlightView::sl_itemInserted);
    connect(_highlight_processor, &HighlightProcessor::sg_highlightItemRemoved, this, &HighlightView::sl_itemRemoved);
    connect(_highlight_processor, &HighlightProcessor::sg_highlightItemChanged, this, &HighlightView::sl_itemChanged);
    connect(_highlight_processor, &HighlightProcessor::sg_highlightBeginUpdate, this, &HighlightView::sl_beginUpdate);
    connect(_highlight_processor, &HighlightProcessor::sg_highlightEndUpdate, this, &HighlightView::sl_endUpdate);

    connect(_view->selectionModel(), &QItemSelectionModel::currentRowChanged, this, &HighlightView::sl_currentRowChanged);
    connect(_view, &QListView::clicked, this, &HighlightView::sl_clicked);

    updateControls();
}

HighlightView::~HighlightView()
{
    delete _object_extension;
}

bool HighlightView::objectExtensionDestroyed() const
{
    return _object_extension->objectExtensionDestroyed();
}

void HighlightView::objectExtensionDestroy()
{
    _object_extension->objectExtensionDestroy();
}

QVariant HighlightView::objectExtensionGetData(const QString& data_key) const
{
    return _object_extension->objectExtensionGetData(data_key);
}

bool HighlightView::objectExtensionSetData(const QString& data_key, const QVariant& value, bool replace) const
{
    return _object_extension->objectExtensionSetData(data_key, value, replace);
}

void HighlightView::objectExtensionRegisterUseExternal(I_ObjectExtension* i)
{
    _object_extension->objectExtensionRegisterUseExternal(i);
}

void HighlightView::objectExtensionUnregisterUseExternal(I_ObjectExtension* i)
{
    _object_extension->objectExtensionUnregisterUseExternal(i);
}

void HighlightView::objectExtensionDeleteInfoExternal(I_ObjectExtension* i)
{
    _object_extension->objectExtensionDeleteInfoExternal(i);
}

void HighlightView::objectExtensionRegisterUseInternal(I_ObjectExtension* i)
{
    _object_extension->objectExtensionRegisterUseInternal(i);
}

void HighlightView::objectExtensionUnregisterUseInternal(I_ObjectExtension* i)
{
    _object_extension->objectExtensionUnregisterUseInternal(i);
}

DataProperty HighlightView::currentItem() const
{
    return _current_item;
}

void HighlightView::setCurrentItem(const DataProperty& item)
{
    if (_current_item == item)
        return;

    _previous_item = _current_item;
    _current_item = item;
    _change_current_requested = true;

    if (Framework::internalCallbackManager()->isRequested(this, Framework::HIGHLIGHT_VIEW_UPDATE_KEY))
        return;

    setCurrentItemHelper();
}

void HighlightView::setCurrentItemHelper()
{
    if (!_change_current_requested)
        return;

    _change_current_requested = false;

    DataProperty view_prop = rowProperty(_view->currentIndex().row());
    if (_current_item != view_prop) {
        int row = _position_hash.value(_current_item, -1);
        if (row >= 0) {
            Z_CHECK(_current_item.isValid());
            //            Z_CHECK(row < _view_model->rowCount());
            if (row >= _view_model->rowCount())
                return;

            _view->setCurrentIndex(_view_model->index(row, 0));

        } else {
            _view->setCurrentIndex(QModelIndex());
        }
    }

    emit sg_currentChanged(_previous_item, _current_item);

    if (_user_event_counter > 0)
        emit sg_currentEdited(_previous_item, _current_item);
}

HighlightProcessor* HighlightView::highlightProcessor() const
{
    return _highlight_processor;
}

void HighlightView::sync()
{
    if (objectExtensionDestroyed())
        return;

    if (Framework::internalCallbackManager()->isRequested(this, Framework::HIGHLIGHT_VIEW_UPDATE_KEY))
        syncInternal();
}

void HighlightView::syncInternal()
{
    if (objectExtensionDestroyed())
        return;

    if (_update_counter > 0) {
        Framework::internalCallbackManager()->addRequest(this, Framework::HIGHLIGHT_VIEW_UPDATE_KEY);
        return;
    }

    Framework::internalCallbackManager()->removeRequest(this, Framework::HIGHLIGHT_VIEW_UPDATE_KEY);

    beginUpdate();
    endUpdate();

    if (_change_current_requested)
        setCurrentItemHelper();

    sl_resizeToContent();
}

void HighlightView::updateControls()
{
    _ok_image->setVisible(_position_hash.isEmpty());
    _view->setVisible(!_position_hash.isEmpty());
}

void HighlightView::onStartKeyPressEvent(QKeyEvent* event)
{
    Q_UNUSED(event)
    _user_event_counter++;
}

void HighlightView::onEndKeyPressEvent(QKeyEvent* event)
{
    Q_UNUSED(event)
    _user_event_counter--;
}

void HighlightView::onStartMousePressEvent(QMouseEvent* event)
{
    Q_UNUSED(event)
    _user_event_counter++;
}

void HighlightView::onEndMousePressEvent(QMouseEvent* event)
{
    Q_UNUSED(event)
    _user_event_counter--;
}

bool HighlightView::eventFilter(QObject* watched, QEvent* event)
{
    return QWidget::eventFilter(watched, event);
}

void HighlightView::sl_itemInserted(const HighlightItem& item)
{
    Q_UNUSED(item)
    if (objectExtensionDestroyed())
        return;
    Framework::internalCallbackManager()->addRequest(this, Framework::HIGHLIGHT_VIEW_UPDATE_KEY);
}

void HighlightView::sl_itemRemoved(const HighlightItem& item)
{
    Q_UNUSED(item)
    if (objectExtensionDestroyed())
        return;
    Framework::internalCallbackManager()->addRequest(this, Framework::HIGHLIGHT_VIEW_UPDATE_KEY);
}

void HighlightView::sl_itemChanged(const HighlightItem& item)
{
    Q_UNUSED(item)
    if (objectExtensionDestroyed())
        return;
    Framework::internalCallbackManager()->addRequest(this, Framework::HIGHLIGHT_VIEW_UPDATE_KEY);
}

void HighlightView::sl_beginUpdate()
{    
}

void HighlightView::sl_endUpdate()
{
    if (objectExtensionDestroyed())
        return;
    Framework::internalCallbackManager()->addRequest(this, Framework::HIGHLIGHT_VIEW_UPDATE_KEY);
}

void HighlightView::sl_currentRowChanged(const QModelIndex& current, const QModelIndex& previous)
{
    Q_UNUSED(previous)
    setCurrentItem(rowProperty(current.row()));
}

void HighlightView::sl_clicked(const QModelIndex& index)
{
    Q_UNUSED(index)
}

void HighlightView::sl_resizeToContent()
{
    if (objectExtensionDestroyed())
        return;
    Framework::internalCallbackManager()->addRequest(this, Framework::HIGHLIGHT_VIEW_RESIZE_KEY);
}

void HighlightView::sl_callbackManager(int key, const QVariant& data)
{
    Q_UNUSED(data)

    if (objectExtensionDestroyed())
        return;

    if (key == Framework::HIGHLIGHT_VIEW_UPDATE_KEY) {
        syncInternal();

    } else if (key == Framework::HIGHLIGHT_VIEW_RESIZE_KEY) {        
        _view->resizeRowsToContents();        
    }
}

void HighlightView::sl_highlightSortOrderChanged()
{
    if (objectExtensionDestroyed())
        return;
    Framework::internalCallbackManager()->addRequest(this, Framework::HIGHLIGHT_VIEW_UPDATE_KEY);
}

void HighlightView::reload()
{    
    calcPositions();

    _view_model->setRowCount(_position_hash.count());

    for (int i = 0; i < _position_hash.count(); i++) {
        auto prop = _position_hash.key(i);
        setItem(prop, i);
    }
}

void HighlightView::setItem(const DataProperty& prop, int row)
{
    DataProperty source_prop = _mapping->getSourceHighlightProperty(prop);
    DataProperty dest_prop = _mapping->getDestinationHighlightProperty(prop);

    QList<HighlightItem> items = _highlight_processor->highlight()->items(source_prop, true);
    if (dest_prop != source_prop)
        items << _highlight_processor->highlight()->items(dest_prop, true);

    QStringList texts;
    QList<InformationType> types;
    DataProperty target_prop;
    for (auto& i : qAsConst(items)) {
        types << i.type();
        texts << i.text();
        if (!target_prop.isValid())
            target_prop = i.property();
    }

    Z_CHECK(target_prop.isValid());

    InformationType type = Utils::getTopErrorLevel(types);
    QString html_text = HtmlTools::table(texts);

    _view_model->setData(row, 0, html_text);
    _view_model->setData(row, 0, Utils::informationTypeIcon(type, Z_PM(PM_SmallIconSize)), Qt::DecorationRole);
    _view_model->setData(row, 0, target_prop.toVariant(), ZF_HIGHLIGHT_PROPERTY_ROLE);
}

void HighlightView::calcPositions()
{
    _position_hash.clear();

    DataPropertyList props = _highlight_processor->highlight()->properties(false);
    DataPropertyList props_prepared;
    for (auto& p : qAsConst(props)) {
        props_prepared << _mapping->getDestinationHighlightProperty(p);
    }

    std::sort(props_prepared.begin(), props_prepared.end(), [this](const DataProperty& property1, const DataProperty& property2) -> bool {
        return _interface->highlightViewGetSortOrder(property1, property2);
    });

    for (int i = 0; i < props_prepared.count(); i++) {
        int n_check = _position_hash.count();
        auto source = _mapping->getSourceHighlightProperty(props_prepared.at(i));
        Z_CHECK(source.isValid());
        _position_hash[source] = i;
        // если ошибка тут, то что-то не так с установкой маппинга setHighlightPropertyMapping и setHighlightWidgetMapping
        // скорее всего после установки маппинга, ошибки вставляются в оба замапленых свойства, вместо одного из них
        Z_CHECK_X(_position_hash.count() > n_check, QString("%1; %2").arg(source.toPrintable(), props_prepared.at(i).toPrintable()));
    }
}

DataProperty HighlightView::rowProperty(int row) const
{
    Z_CHECK(row < _view_model->rowCount());

    if (row < 0)
        return {};

    auto prop = DataProperty::fromVariant(_view_model->data(row, 0, ZF_HIGHLIGHT_PROPERTY_ROLE));
    Z_CHECK(prop.isValid());
    return prop;
}

void HighlightView::beginUpdate()
{
    if (_update_counter == 0)
        _current_before_update = _current_item;

    _update_counter++;
}

void HighlightView::endUpdate()
{
    Z_CHECK(_update_counter > 0);
    _update_counter--;

    if (_update_counter > 0)
        return;

    reload();

    int row = -1;
    if (_current_before_update.isValid())
        row = _position_hash.value(_current_before_update, -1);

    DataProperty current;
    if (row >= 0)
        current = rowProperty(row);

    setCurrentItem(current);

    updateControls();
}

} // namespace zf
