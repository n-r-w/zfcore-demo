#pragma once

#include "zf_i_data_change_processor.h"
#include "zf_data_container.h"

namespace zf
{
//! Отслеживание измнений данных в DataContainer
class ZCORESHARED_EXPORT DataChangeProcessor : public QObject
{
    Q_OBJECT
public:
    DataChangeProcessor(
        //! Данные
        DataContainer* data,
        //! Интерфейс, методы которого будут вызываться при изменениях данных
        I_DataChangeProcessor* i_processor, QObject* parent = nullptr);

    const DataStructure* structure() const;
    DataContainer* data() const;

private slots:
    //! Данные помечены как невалидные. Вызывается без учета изменения состояния валидности
    void sl_invalidate(const zf::DataProperty& p, bool invalidate, const zf::ChangeInfo& info);
    //! Изменилось состояние валидности данных
    void sl_invalidateChanged(const zf::DataProperty& p, bool invalidate, const zf::ChangeInfo& info);

    //! Сменился текущий язык данных контейнера
    void sl_languageChanged(QLocale::Language language);

    //! Все свойства были заблокированы
    void sl_allPropertiesBlocked();
    //! Все свойства были разблокированы
    void sl_allPropertiesUnBlocked();

    //! Свойство были заблокировано
    void sl_propertyBlocked(const zf::DataProperty& p);
    //! Свойство были разаблокировано
    void sl_propertyUnBlocked(const zf::DataProperty& p);

    //! Свойство было инициализировано
    void sl_propertyInitialized(const zf::DataProperty& p);
    //! Свойство стало неинициализировано
    void sl_propertyUnInitialized(const zf::DataProperty& p);

    //! Изменилось значение свойства
    void sl_propertyChanged(const zf::DataProperty& p,
                            //! Старые значение (работает только для полей)
                            const zf::LanguageMap& old_values);

    //! Сигналы наборов данных
    void sl_dataset_dataChanged(const zf::DataProperty& p, const QModelIndex& topLeft, const QModelIndex& bottomRight,
                                const QVector<int>& roles);
    void sl_dataset_headerDataChanged(const zf::DataProperty& p, Qt::Orientation orientation, int first, int last);

    void sl_dataset_rowsAboutToBeInserted(const zf::DataProperty& p, const QModelIndex& parent, int first, int last);
    void sl_dataset_rowsInserted(const zf::DataProperty& p, const QModelIndex& parent, int first, int last);

    void sl_dataset_rowsAboutToBeRemoved(const zf::DataProperty& p, const QModelIndex& parent, int first, int last);
    void sl_dataset_rowsRemoved(const zf::DataProperty& p, const QModelIndex& parent, int first, int last);

    void sl_dataset_columnsAboutToBeInserted(const zf::DataProperty& p, const QModelIndex& parent, int first, int last);
    void sl_dataset_columnsInserted(const zf::DataProperty& p, const QModelIndex& parent, int first, int last);

    void sl_dataset_columnsAboutToBeRemoved(const zf::DataProperty& p, const QModelIndex& parent, int first, int last);
    void sl_dataset_columnsRemoved(const zf::DataProperty& p, const QModelIndex& parent, int first, int last);

    void sl_dataset_modelAboutToBeReset(const zf::DataProperty& p);
    void sl_dataset_modelReset(const zf::DataProperty& p);

    void sl_dataset_rowsAboutToBeMoved(const zf::DataProperty& p, const QModelIndex& sourceParent, int sourceStart, int sourceEnd,
                                       const QModelIndex& destinationParent, int destinationRow);
    void sl_dataset_rowsMoved(const zf::DataProperty& p, const QModelIndex& parent, int start, int end, const QModelIndex& destination,
                              int row);

    void sl_dataset_columnsAboutToBeMoved(const zf::DataProperty& p, const QModelIndex& sourceParent, int sourceStart, int sourceEnd,
                                          const QModelIndex& destinationParent, int destinationColumn);
    void sl_dataset_columnsMoved(const zf::DataProperty& p, const QModelIndex& parent, int start, int end, const QModelIndex& destination,
                                 int column);

private:
    DataContainer* _data;
    I_DataChangeProcessor* _i_processor;
};

} // namespace zf
