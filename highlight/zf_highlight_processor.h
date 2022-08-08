#pragma once

#include "zf.h"
#include "zf_hashed_dataset.h"
#include "zf_i_highlight_processor.h"
#include "zf_object_extension.h"

namespace zf
{
class DataContainer;
class HighlightModel;
class HighlightItem;
class ItemModel;

//! Реализация обработки реакции на изменение данных и заполнения модели ошибок
class ZCORESHARED_EXPORT HighlightProcessor : public QObject, public I_HashedDatasetCutomize, public I_ObjectExtension
{
    Q_OBJECT
public:
    //! Создается не запущенным, т.е. надо вызвать start для начала работы
    HighlightProcessor(const DataContainer* data,
        //! Обработка ключевых значений
        I_HighlightProcessorKeyValues* i_key_values,
        /*! Аналог ModuleDataOption::SimpleHighlight
                       * В дополнение к методу getHighlight, вызываются:
                       *   для полей: getFieldHighlight
                       *   для ячеек таблиц: getCellHighlight */
        bool simple = true);
    ~HighlightProcessor();

public: // реализация I_ObjectExtension
    //! Удален ли объект
    bool objectExtensionDestroyed() const final;

    //! Безопасно удалить объект
    void objectExtensionDestroy() override;

    //! Получить данные
    QVariant objectExtensionGetData(
        //! Ключ данных
        const QString& data_key) const final;
    //! Записать данные
    //! Возвращает признак - были ли записаны данные
    bool objectExtensionSetData(
        //! Ключ данных
        const QString& data_key, const QVariant& value,
        //! Замещение. Если false, то при наличии такого ключа, данные не замещаются и возвращается false
        bool replace) const final;

    //! Другой объект сообщает этому, что будет хранить указатель на него
    void objectExtensionRegisterUseExternal(I_ObjectExtension* i) final;
    //! Другой объект сообщает этому, что перестает хранить указатель на него
    void objectExtensionUnregisterUseExternal(I_ObjectExtension* i) final;

    //! Другой объект сообщает этому, что будет удален и надо перестать хранить указатель на него
    //! Реализация этого метода, кроме вызова ObjectExtension::objectExtensionDeleteInfoExternal должна
    //! очистить все ссылки на указанный объект
    void objectExtensionDeleteInfoExternal(I_ObjectExtension* i) final;

    //! Этот объект начинает использовать другой объект
    void objectExtensionRegisterUseInternal(I_ObjectExtension* i) final;
    //! Этот объект прекращает использовать другой объект
    void objectExtensionUnregisterUseInternal(I_ObjectExtension* i) final;

public:
    //! Модель ошибок
    const HighlightModel* highlight() const;

    //! Данные
    const DataContainer* data() const;
    //! Структура данных
    const DataStructure* structure() const;

    //! Реакция на изменение данных приостановлена
    bool isStopped() const;
    //! Приостановить реакцию на изменение данных
    void stop();
    //! Запустить реакцию на изменение данных
    void start();

    //! Запросить проверку на ошибки для данного свойства
    void registerHighlightCheck(const DataProperty& property);
    //! Запросить проверку на ошибки для данного свойства
    void registerHighlightCheck(const PropertyID& property_id);
    //! Запросить проверку на ошибки для всех свойств
    void registerHighlightCheck();

    //! Зарегистрировать проверку строк с дубликатами ключевых значений
    void registerDatasetRowKeyDuplicateCheck(const DataProperty& dataset);

    //! Принудительно запустить зарегистрирированные проверки на ошибки
    void executeHighlightCheckRequests();

    //! Очистить зарегистрирированные проверки на ошибки
    void clearHighlightCheckRequests();
    //! Очистить все ошибки
    void clearHighlights();

    //! Ошибки для ячейки, отсортированные от наивысшего приоритета к наименьшему
    QList<HighlightItem> cellHighlight(const QModelIndex& index, bool execute_check = true, bool halt_if_not_found = true) const;

    //! Перенаправить все вызовы перенаправляются в другой процессор
    //! При установке процессора текущий останавливается
    void installMasterProcessor(HighlightProcessor* master_processor);
    //! Удалить мастер-процессор
    void removeMasterProcessor(
        //! Запустить этот процессор после удаления
        bool start);
    HighlightProcessor* masterProcessor() const;

    //! Добавить интерфейс для внешней обработки ошибок. Должен быть наследником QObject
    void installExternalProcessing(I_HighlightProcessor* i_processor);
    //! Удалить интерфейс для внешней обработки ошибок. Должен быть наследником QObject
    void removeExternalProcessing(I_HighlightProcessor* i_processor);
    //! Установлен ли интерфейс для внешней обработки ошибок
    bool isExternalProcessingInstalled(I_HighlightProcessor* i_processor) const;

    //! Установить интерфейс для кастомизации HashedDataset. Должен быть наследником QObject
    void setHashedDatasetCutomization(I_HashedDatasetCutomize* i_hashed_customize);

    //! Блокировка выполнения автоматических проверок их плагина
    void blockHighlightAuto();
    //! Разблокировка выполнения автоматических проверок их плагина
    void unBlockHighlightAuto();

    //! Проверка ключевых значений на уникальность. Использовать для реализации нестандартных алгоритмов проверки
    //! ключевых полей По умолчанию используется алгоритм работы по ключевым полям, заданным через свойство
    //! PropertyOption::Key для колонок наборов данных
    void checkKeyValues(const DataProperty& dataset, int row, const QModelIndex& parent,
        //! Если ошибка есть, то текст не пустой
        QString& error_text,
        //! Ячейка где должна быть ошибка если она есть
        DataProperty& error_property) const;
    //! Проверить свойство на ошибки на основании PropertyConstraint и занести результаты проверки в HighlightInfo
    void getHighlightAuto(const DataProperty& property, HighlightInfo& info) const;

public: // интерфейс I_HashedDatasetCutomize
    //! Преобразовать набор значений ключевых полей в уникальную строку
    QString hashedDatasetkeyValuesToUniqueString(
        //! Ключ для идентификации при обратном вызове I_HashedDatasetCutomize
        const QString& key,
        //! Строка набора данных
        int row,
        //! Ролитель строки
        const QModelIndex& parent,
        //! Ключевые значения
        const QVariantList& keyValues) const final;

signals:
    //! Модель ошибок: Добавлен элемент
    void sg_highlightItemInserted(const zf::HighlightItem& item);
    //! Модель ошибок: Удален элемент
    void sg_highlightItemRemoved(const zf::HighlightItem& item);
    //! Модель ошибок: Изменен элемент
    void sg_highlightItemChanged(const zf::HighlightItem& item);
    //! Модель ошибок: Начато групповое изменение
    void sg_highlightBeginUpdate();
    //! Модель ошибок: Закончено групповое изменение
    void sg_highlightEndUpdate();

private slots:
    //! Менеджер обратных вызовов
    void sl_callbackManager(int key, const QVariant& data);

    //! Изменились ячейки в наборе данных
    void sl_dataset_dataChanged(const zf::DataProperty& p, const QModelIndex& topLeft, const QModelIndex& bottomRight, const QVector<int>& roles);
    //! После добавления строк в набор данных
    void sl_dataset_rowsInserted(const zf::DataProperty& p, const QModelIndex& parent, int first, int last);
    //! Перед удалением строк из набора данных
    void sl_dataset_rowsAboutToBeRemoved(const zf::DataProperty& p, const QModelIndex& parent, int first, int last);
    //! После удаления строк из набора данных
    void sl_dataset_rowsRemoved(const zf::DataProperty& p, const QModelIndex& parent, int first, int last);
    //! После добавления колонок в набор данных
    void sl_dataset_columnsInserted(const zf::DataProperty& p, const QModelIndex& parent, int first, int last);
    //! Перед удалением колонок в набор данных
    void sl_dataset_columnsAboutToBeRemoved(const zf::DataProperty& p, const QModelIndex& parent, int first, int last);
    //! После удаления колонок из набора данных
    void sl_dataset_columnsRemoved(const zf::DataProperty& p, const QModelIndex& parent, int first, int last);
    //! Модель данных была сброшена
    void sl_dataset_modelReset(const zf::DataProperty& p);

    //! Изменилось состояние валидности данных
    void sl_invalidateChanged(const zf::DataProperty& p, bool invalidate);

    //! Все свойства были разблокированы
    void sl_allPropertiesUnBlocked();
    //! Свойство были разаблокировано
    void sl_propertyUnBlocked(const zf::DataProperty& p);

    //! Изменилось значение свойства. Не вызывается если свойство заблокировано
    void sl_propertyChanged(const zf::DataProperty& p,
        //! Старые значение (работает только для полей)
        const zf::LanguageMap& old_values);

    //! Модель ошибок: Добавлен элемент
    void sl_highlightItemInserted(const zf::HighlightItem& item);
    //! Модель ошибок: Удален элемент
    void sl_highlightItemRemoved(const zf::HighlightItem& item);
    //! Модель ошибок: Изменен элемент
    void sl_highlightItemChanged(const zf::HighlightItem& item);
    //! Модель ошибок: Начато групповое изменение
    void sl_highlightBeginUpdate();
    //! Модель ошибок: Закончено групповое изменение
    void sl_highlightEndUpdate();

    //! Модель ошибок (головная): Добавлен элемент
    void sl_masterHighlightItemInserted(const zf::HighlightItem& item);
    //! Модель ошибок (головная): Удален элемент
    void sl_masterHighlightItemRemoved(const zf::HighlightItem& item);
    //! Модель ошибок (головная): Изменен элемент
    void sl_masterHighlightItemChanged(const zf::HighlightItem& item);
    //! Модель ошибок (головная): Начато групповое изменение
    void sl_masterHighlightBeginUpdate();
    //! Модель ошибок (головная): Закончено групповое изменение
    void sl_masterHighlightEndUpdate();

    void sl_highlightProcessorDestroyed(QObject* obj);
    void sl_hashedDatasetCutomizeDestroyed(QObject* obj);

private:
    struct DatasetInfo;

    //! Реакция на изменение данных приостановлена
    bool isStoppedHelper() const;
    //! Приостановить реакцию на изменение данных
    void stopHelper();
    //! Запустить реакцию на изменение данных
    void startHelper();

    //! Очистить зарегистрирированные проверки на ошибки
    void clearHighlightCheckRequestsHelper();
    //! Очистить все ошибки
    void clearHighlightsHelper();

    //! Проверка значения свойства
    static void checkHightlightValue(
        const DataContainer* data, const QVariant& value, const DataProperty& property, PropertyConstraint* constraint, HighlightInfo& info);
    //! Проверка на InvalidValue
    static bool checkInvalidHighlightValue(const DataProperty& property, const QVariant& value, HighlightInfo& info);
    //! Вернуть значения ключевых полей строки
    void getKeyValues(DatasetInfo* info, int row, const QModelIndex& parent,
        //! Значения ключевых полей  (PropertyOption::KeyColumn)
        QVariantList& key_values,
        //! Значения базовых ключевых полей (PropertyOption::KeyColumnBase)
        QVariantList& key_base_values) const;
    //! Проверка набора данных на ошибки
    void getDatasetHighlightHelper(I_HighlightProcessor* processor, const DataProperty& dataset, zf::HighlightInfo& info);

private: // эти функции должны вызывать внешний интерфейс
    //! Преобразовать набор значений ключевых полей в уникальную строку.
    //! Если возвращена пустая строка, то проверка ключевых полей для данной записи набора данных игнорируется
    QString keyValuesToUniqueString(const DataProperty& dataset, int row, const QModelIndex& parent, const QVariantList& key_values) const;
    //! Проверить поле на ошибки
    void getFieldHighlight(I_HighlightProcessor* processor, const DataProperty& field, HighlightInfo& info);
    //! Проверить ячейку набора данных на ошибки
    void getCellHighlight(
        I_HighlightProcessor* processor, const DataProperty& dataset, int row, const DataProperty& column, const QModelIndex& parent, HighlightInfo& info);
    //! Проверить свойство на ошибки и занести результаты проверки в HighlightInfo
    void getHighlight(I_HighlightProcessor* processor, const DataProperty& p, HighlightInfo& info,
        //! Наборы данных, по которым нужна дополнительная отдельная проверка. Проверка именно по самому свойству, а не по ячейкам
        DataPropertySet& direct_datasets);

private:
    void bootstrap();

    //! Информация о наборе данных
    struct DatasetInfo
    {
        DatasetInfo(const HighlightProcessor* _processor, const DataProperty& _dataset);

        //! Хэш для контроля уникальности строк
        HashedDataset* getKeyColumnsCheckHash() const;
        //! Указатель на данные для оптимизации скорости доступа
        ItemModel* item_model() const;

        const HighlightProcessor* processor = nullptr;
        //! Набор данных
        DataProperty dataset;
        //! Ключевые колонки (PropertyOption::KeyColumn) - порядковые номера (для контроля уникальности). Заполняются для оптимизации
        QList<int> key_columns;
        //! Ключевые колонки (PropertyOption::KeyColumnBase) - порядковые номера (которые надо заполнить чтобы контроль уникальности начал работать).
        //! Заполняются для оптимизации
        QList<int> key_columns_base;
        //! Колонка, отвечающая за отображение ошибок проверки на уникальность
        int key_column_error = -1;

        //! Хэш для контроля уникальности строк
        mutable std::unique_ptr<HashedDataset> key_columns_check_hash;

    private:
        //! Указатель на данные для оптимизации скорости доступа
        mutable QPointer<ItemModel> _item_model;
    };
    mutable QMap<DataProperty, std::shared_ptr<DatasetInfo>> _dataset_info;
    DatasetInfo* datasetInfo(const DataProperty& dataset, bool null_if_not_exists) const;

    //! Модель ошибок
    HighlightModel* _highlight = nullptr;
    //! Структура данных
    const DataStructure* _structure = nullptr;
    //! Данные
    const DataContainer* _data = nullptr;
    //! Обработка ключевых значений
    I_HighlightProcessorKeyValues* _i_key_values = nullptr;

    //! Внешние обработчики I_HighlightProcessor
    QList<I_HighlightProcessor*> _i_processors;
    QMap<QObject*, I_HighlightProcessor*> _i_processors_map;

    //! Внешний обработчик I_HashedDatasetCutomize
    I_HashedDatasetCutomize* _i_hashed_customize = nullptr;

    //! Запросы на проверку ошибок
    QSet<DataProperty> _registered_highlight_check;

    /*! Аналог ModuleDataOption::SimpleHighlight
                       * В дополнение к методу getHighlight, вызываются:
                       *   для полей: getFieldHighlight
                       *   для ячеек таблиц: getCellHighlight */
    bool _is_simple = true;
    //! Счетчик остановки. Изначально остановлен
    int _stop_counter = 1;

    //! Если задано, то все вызовы перенаправляются в другой процессор
    QPointer<HighlightProcessor> _master_processor;
    //! Есть ли наборы данных с ключевыми колонками
    mutable QVariant _has_key_columns;

    //! Потокобезопасное расширение возможностей QObject
    ObjectExtension* _object_extension = nullptr;

    //! Блокировка getHighlightAuto
    int _block_highlight_auto = 0;
};

} // namespace zf
