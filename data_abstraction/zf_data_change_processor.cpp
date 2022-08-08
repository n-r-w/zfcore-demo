#include "zf_data_change_processor.h"
#include "zf_core.h"

namespace zf
{
DataChangeProcessor::DataChangeProcessor(DataContainer* data, I_DataChangeProcessor* i_processor, QObject* parent)
    : QObject(parent)
    , _data(data)
    , _i_processor(i_processor)
{
    Z_CHECK_NULL(_data);
    Z_CHECK_NULL(_i_processor);

    connect(_data, &DataContainer::sg_invalidate, this, &DataChangeProcessor::sl_invalidate);
    connect(_data, &DataContainer::sg_invalidateChanged, this, &DataChangeProcessor::sl_invalidateChanged);    
    connect(_data, &DataContainer::sg_languageChanged, this, &DataChangeProcessor::sl_languageChanged);
    connect(_data, &DataContainer::sg_allPropertiesBlocked, this, &DataChangeProcessor::sl_allPropertiesBlocked);
    connect(_data, &DataContainer::sg_allPropertiesUnBlocked, this, &DataChangeProcessor::sl_allPropertiesUnBlocked);
    connect(_data, &DataContainer::sg_propertyBlocked, this, &DataChangeProcessor::sl_propertyBlocked);
    connect(_data, &DataContainer::sg_propertyUnBlocked, this, &DataChangeProcessor::sl_propertyUnBlocked);
    connect(_data, &DataContainer::sg_propertyChanged, this, &DataChangeProcessor::sl_propertyChanged);

    connect(_data, &DataContainer::sg_propertyInitialized, this, &DataChangeProcessor::sl_propertyInitialized);
    connect(_data, &DataContainer::sg_propertyUnInitialized, this, &DataChangeProcessor::sl_propertyUnInitialized);

    connect(_data, &DataContainer::sg_dataset_dataChanged, this, &DataChangeProcessor::sl_dataset_dataChanged);
    connect(_data, &DataContainer::sg_dataset_headerDataChanged, this, &DataChangeProcessor::sl_dataset_headerDataChanged);
    connect(_data, &DataContainer::sg_dataset_rowsAboutToBeInserted, this, &DataChangeProcessor::sl_dataset_rowsAboutToBeInserted);
    connect(_data, &DataContainer::sg_dataset_rowsInserted, this, &DataChangeProcessor::sl_dataset_rowsInserted);
    connect(_data, &DataContainer::sg_dataset_rowsAboutToBeRemoved, this, &DataChangeProcessor::sl_dataset_rowsAboutToBeRemoved);
    connect(_data, &DataContainer::sg_dataset_rowsRemoved, this, &DataChangeProcessor::sl_dataset_rowsRemoved);
    connect(_data, &DataContainer::sg_dataset_columnsAboutToBeInserted, this, &DataChangeProcessor::sl_dataset_columnsAboutToBeInserted);
    connect(_data, &DataContainer::sg_dataset_columnsInserted, this, &DataChangeProcessor::sl_dataset_columnsInserted);
    connect(_data, &DataContainer::sg_dataset_columnsAboutToBeRemoved, this, &DataChangeProcessor::sl_dataset_columnsAboutToBeRemoved);
    connect(_data, &DataContainer::sg_dataset_columnsRemoved, this, &DataChangeProcessor::sl_dataset_columnsRemoved);
    connect(_data, &DataContainer::sg_dataset_modelAboutToBeReset, this, &DataChangeProcessor::sl_dataset_modelAboutToBeReset);
    connect(_data, &DataContainer::sg_dataset_modelReset, this, &DataChangeProcessor::sl_dataset_modelReset);
    connect(_data, &DataContainer::sg_dataset_rowsAboutToBeMoved, this, &DataChangeProcessor::sl_dataset_rowsAboutToBeMoved);
    connect(_data, &DataContainer::sg_dataset_rowsMoved, this, &DataChangeProcessor::sl_dataset_rowsMoved);
    connect(_data, &DataContainer::sg_dataset_columnsAboutToBeMoved, this, &DataChangeProcessor::sl_dataset_columnsAboutToBeMoved);
    connect(_data, &DataContainer::sg_dataset_columnsMoved, this, &DataChangeProcessor::sl_dataset_columnsMoved);
}

const DataStructure* DataChangeProcessor::structure() const
{
    return _data->structure().get();
}

DataContainer* DataChangeProcessor::data() const
{
    return _data;
}

void DataChangeProcessor::sl_invalidate(const DataProperty& p, bool invalidate, const ChangeInfo& info)
{
    _i_processor->onDataInvalidate(p, invalidate, info);
}

void DataChangeProcessor::sl_invalidateChanged(const DataProperty& p, bool invalidate, const ChangeInfo& info)
{
    _i_processor->onDataInvalidateChanged(p, invalidate, info);
}

void DataChangeProcessor::sl_languageChanged(QLocale::Language language)
{
    _i_processor->onLanguageChanged(language);
}

void DataChangeProcessor::sl_allPropertiesBlocked()
{
    _i_processor->onAllPropertiesBlocked();
}

void DataChangeProcessor::sl_allPropertiesUnBlocked()
{
    _i_processor->onAllPropertiesUnBlocked();
    for (const auto& p : structure()->propertiesMain()) {
        _i_processor->onPropertyUpdated(p, ObjectActionType::Modify);
    }
}

void DataChangeProcessor::sl_propertyBlocked(const DataProperty& p)
{
    _i_processor->onPropertyBlocked(p);
}

void DataChangeProcessor::sl_propertyUnBlocked(const DataProperty& p)
{
    _i_processor->onPropertyUnBlocked(p);
    _i_processor->onPropertyUpdated(p, ObjectActionType::Modify);
}

void DataChangeProcessor::sl_propertyInitialized(const DataProperty& p)
{
    _i_processor->onPropertyUpdated(p, ObjectActionType::Modify);
}

void DataChangeProcessor::sl_propertyUnInitialized(const DataProperty& p)
{
    Q_UNUSED(p)
}

void DataChangeProcessor::sl_propertyChanged(const DataProperty& p, const LanguageMap& old_values)
{
    _i_processor->onPropertyChanged(p, old_values);
    _i_processor->onPropertyUpdated(p, ObjectActionType::Modify);
}

void DataChangeProcessor::sl_dataset_dataChanged(const DataProperty& p, const QModelIndex& topLeft, const QModelIndex& bottomRight,
                                                 const QVector<int>& roles)
{
    _i_processor->onDatasetDataChanged(p, topLeft, bottomRight, roles);

    DataProperty left_column;
    if (p.columns().count() > topLeft.column())
        left_column = p.columns().at(topLeft.column());
    DataProperty right_column;
    if (p.columns().count() > bottomRight.column())
        right_column = p.columns().at(bottomRight.column());

    if (left_column.isValid() && right_column.isValid()) {
        Z_CHECK(topLeft.parent() == bottomRight.parent());
        _i_processor->onDatasetCellChanged(left_column, right_column, topLeft.row(), bottomRight.row(), topLeft.parent());
    }

    for (int row = topLeft.row(); row <= bottomRight.row(); row++) {
        for (int col = topLeft.column(); col <= bottomRight.column(); col++) {
            _i_processor->onPropertyUpdated(_data->propertyCell(p, row, col, topLeft.parent()), ObjectActionType::Modify);
        }
    }
}

void DataChangeProcessor::sl_dataset_headerDataChanged(const DataProperty& p, Qt::Orientation orientation, int first, int last)
{
    _i_processor->onDatasetHeaderDataChanged(p, orientation, first, last);
}

void DataChangeProcessor::sl_dataset_rowsAboutToBeInserted(const DataProperty& p, const QModelIndex& parent, int first, int last)
{
    _i_processor->onDatasetRowsAboutToBeInserted(p, parent, first, last);
}

void DataChangeProcessor::sl_dataset_rowsInserted(const DataProperty& p, const QModelIndex& parent, int first, int last)
{
    _i_processor->onDatasetRowsInserted(p, parent, first, last);

    for (int row = first; row <= last; row++) {
        _i_processor->onPropertyUpdated(_data->propertyRow(p, row, parent), ObjectActionType::Create);
    }
}

void DataChangeProcessor::sl_dataset_rowsAboutToBeRemoved(const DataProperty& p, const QModelIndex& parent, int first, int last)
{
    _i_processor->onDatasetRowsAboutToBeRemoved(p, parent, first, last);

    for (int row = first; row <= last; row++) {
        _i_processor->onPropertyUpdated(_data->propertyRow(p, row, parent), ObjectActionType::Remove);
    }
}

void DataChangeProcessor::sl_dataset_rowsRemoved(const DataProperty& p, const QModelIndex& parent, int first, int last)
{
    _i_processor->onDatasetRowsRemoved(p, parent, first, last);
}

void DataChangeProcessor::sl_dataset_columnsAboutToBeInserted(const DataProperty& p, const QModelIndex& parent, int first, int last)
{
    _i_processor->onDatasetColumnsAboutToBeInserted(p, parent, first, last);
}

void DataChangeProcessor::sl_dataset_columnsInserted(const DataProperty& p, const QModelIndex& parent, int first, int last)
{
    _i_processor->onDatasetColumnsInserted(p, parent, first, last);

    for (int col = first; col <= last; col++) {
        _i_processor->onPropertyUpdated(structure()->propertyColumn(p, col), ObjectActionType::Create);
    }
}

void DataChangeProcessor::sl_dataset_columnsAboutToBeRemoved(const DataProperty& p, const QModelIndex& parent, int first, int last)
{
    _i_processor->onDatasetColumnsAboutToBeRemoved(p, parent, first, last);

    for (int col = first; col <= last; col++) {
        _i_processor->onPropertyUpdated(structure()->propertyColumn(p, col), ObjectActionType::Remove);
    }
}

void DataChangeProcessor::sl_dataset_columnsRemoved(const DataProperty& p, const QModelIndex& parent, int first, int last)
{
    _i_processor->onDatasetColumnsRemoved(p, parent, first, last);
}

void DataChangeProcessor::sl_dataset_modelAboutToBeReset(const DataProperty& p)
{
    _i_processor->onDatasetModelAboutToBeReset(p);
}

void DataChangeProcessor::sl_dataset_modelReset(const DataProperty& p)
{
    _i_processor->onDatasetModelReset(p);
    _i_processor->onPropertyUpdated(p, ObjectActionType::Modify);
}

void DataChangeProcessor::sl_dataset_rowsAboutToBeMoved(const DataProperty& p, const QModelIndex& sourceParent, int sourceStart,
                                                        int sourceEnd, const QModelIndex& destinationParent, int destinationRow)
{
    _i_processor->onDatasetRowsAboutToBeMoved(p, sourceParent, sourceStart, sourceEnd, destinationParent, destinationRow);
}

void DataChangeProcessor::sl_dataset_rowsMoved(const DataProperty& p, const QModelIndex& parent, int start, int end,
                                               const QModelIndex& destination, int row)
{
    _i_processor->onDatasetRowsMoved(p, parent, start, end, destination, row);
    _i_processor->onPropertyUpdated(p, ObjectActionType::Modify);
}

void DataChangeProcessor::sl_dataset_columnsAboutToBeMoved(const DataProperty& p, const QModelIndex& sourceParent, int sourceStart,
                                                           int sourceEnd, const QModelIndex& destinationParent, int destinationColumn)
{
    _i_processor->onDatasetColumnsAboutToBeMoved(p, sourceParent, sourceStart, sourceEnd, destinationParent, destinationColumn);
}

void DataChangeProcessor::sl_dataset_columnsMoved(const DataProperty& p, const QModelIndex& parent, int start, int end,
                                                  const QModelIndex& destination, int column)
{
    _i_processor->onDatasetColumnsMoved(p, parent, start, end, destination, column);
    _i_processor->onPropertyUpdated(p, ObjectActionType::Modify);
}

} // namespace zf
