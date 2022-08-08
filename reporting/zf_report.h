#pragma once

#include <QMutex>

#include "zf.h"
#include "zf_data_container.h"
#include "zf_error.h"
#include "zf_xml.h"

namespace zf
{
//! Базовый абстрактный класс для генерации документов по шаблонам XML (DOCX, ODF), XHTML, HTML4, HTML5
class ZCORESHARED_EXPORT ReportGenerator
{
    Q_DISABLE_COPY(ReportGenerator)
public:
    ReportGenerator();
    virtual ~ReportGenerator();

    //! Создать документ. Поля задаются через DataProperty
    Error generate(
        //! Источник данных
        const DataContainer* data,
        //! Соответствие между полями данных и текстовыми метками в шаблоне
        const QMap<DataProperty, QString>& field_names,
        /*! Автоматическое определение соответствия между полями данных и текстовыми метками в шаблоне по DataProperty::id
         * Ищет совпадение между целочисленными текстовыми метками и DataProperty::id. Работает как дополнение к field_names */
        bool auto_map,
        //! Шаблон
        const QString& template_file,
        //! Результат
        const QString& target_file,
        //! Язык по умолчанию для генерации отчета. По умолчанию - Core::languageWorkflow
        QLocale::Language language = QLocale::AnyLanguage,
        //! Язык для конкретных полей данных
        const QMap<DataProperty, QLocale::Language>& field_languages = {});
    //! Создать документ. Поля задаются через DataProperty
    Error generate(
        //! Источник данных
        const DataContainer* data,
        //! Соответствие между полями данных и текстовыми метками в шаблоне
        const QMap<DataProperty, QString>& field_names,
        /*! Автоматическое определение соответствия между полями данных и текстовыми метками в шаблоне по DataProperty::id
         * Ищет совпадение между целочисленными текстовыми метками и DataProperty::id. Работает как дополнение к field_names */
        bool auto_map,
        //! Шаблон
        const QByteArray& template_file,
        //! Результат
        const QString& target_file,
        //! Язык по умолчанию для генерации отчета. По умолчанию - Core::languageWorkflow
        QLocale::Language language = QLocale::AnyLanguage,
        //! Язык для конкретных полей данных
        const QMap<DataProperty, QLocale::Language>& field_languages = {});

    //! Создать документ. Поля задаются через коды DataProperty
    Error generate(
        //! Источник данных
        const DataContainer* data,
        //! Соответствие между полями данных и текстовыми метками в шаблоне. Ключ - id свойства
        const QMap<PropertyID, QString>& field_names,
        /*! Автоматическое определение соответствия между полями данных и текстовыми метками в шаблоне по DataProperty::id
         * Ищет совпадение между текстовыми метками и DataProperty::id. Работает как дополнение к field_names */
        bool auto_map,
        //! Шаблон
        const QString& template_file,
        //! Результат
        const QString& target_file,
        //! Язык по умолчанию для генерации отчета. По умолчанию - Core::languageWorkflow
        QLocale::Language language = QLocale::AnyLanguage,
        //! Язык для конкретных полей данных
        const QMap<PropertyID, QLocale::Language>& field_languages = {});
    //! Создать документ. Поля задаются через коды DataProperty
    Error generate(
        //! Источник данных
        const DataContainer* data,
        //! Соответствие между полями данных и текстовыми метками в шаблоне. Ключ - id свойства
        const QMap<PropertyID, QString>& field_names,
        /*! Автоматическое определение соответствия между полями данных и текстовыми метками в шаблоне по DataProperty::id
         * Ищет совпадение между текстовыми метками и DataProperty::id. Работает как дополнение к field_names */
        bool auto_map,
        //! Шаблон
        const QByteArray& template_file,
        //! Результат
        const QString& target_file,
        //! Язык по умолчанию для генерации отчета. По умолчанию - Core::languageWorkflow
        QLocale::Language language = QLocale::AnyLanguage,
        //! Язык для конкретных полей данных
        const QMap<PropertyID, QLocale::Language>& field_languages = {});

    /*! Показать стандартный диалог с информацией об успешном сохранении файла
     * Если была ошибка, то ничего не показывает */
    static void showLastOk();

protected:    
    //! Тип тэга
    enum class TagType
    {
        Invalid,
        //! Поле данных
        Field,
        //! Ячейка таблицы
        Cell,
        //! Колонка таблицы
        Column,
        //! Начало повтора блока
        BlockStart,
        //! Окончание повтора блока
        BlockFinish,
    };

    //! Сформировать ошибку
    Error createError(const QString& text) const;
    //! Документ XML
    XMLDocument* document() const;

    //! Результаты разбиения текста на основании признаков начала и конца тэга
    enum class SplitTextResult
    {
        //! Не найдено ничего
        NoTag,
        //! Найдено начало тэга
        TagBegin,
        //! Найдено окончание тэга
        TagEnd,
        //! Найдено начало и окончание тэга
        FullTag
    };

    //! Разбить текст на основании признаков начала и конца тэга
    Error splitText(
        //! Текст для анализа
        const QString& text,
        //! Поиск следующего теги
        bool search_next,
        //! Пары [начало тэга, окончание тэга].
        const QList<QPair<QString, QString>>& marks,
        //! Тип тэга, который соответствует marks
        const QList<TagType>& types,
        //! Текст перед тэгом
        QString& text_before,
        //! Текст тега
        QString& tag_text,
        //! Текст после тэга
        QString& text_after,
        //! Какой тип тэга найден
        TagType& type,
        //! Результаты разбиения текста
        SplitTextResult& result) const;

    /*! Распаковать исходный шаблон если он упакован и поместить во временные файлы (будут удалены после обработки)
     * Если шаблон не упакован, то возвратить пустой список */
    virtual Error unpackTemplate(const QByteArray& template_file, const QString& temp_folder, QStringList& to_parse) = 0;
    //! Упаковать папку в целевой файл (только для изначально упакованных шаблонов)
    virtual Error packTarget(const QString& temp_folder, const QString& target_file) = 0;

    //! Подготовка названия целевого файла (например добавление расширения и т.п.)
    virtual QString prepareTargetFileName(const QString& file_name) const;

    //! Открыть документ
    virtual Error openDocument(const QString& template_file, XMLDocumentPtr& document) = 0;
    //! Записать документ в файл
    virtual Error saveDocument(const QString& target_file, const XMLDocumentPtr& document) = 0;

    //! Подготовка дерева к разбору
    virtual Error prepareTree(const XMLNodePtr& root);
    //! Извлечь текст из элемента и определить его тип. Для ячеек можно возвращать тип Field, т.к. он автоматически
    //! преобразуется в Column. Необходимо возвращать только тот текст, который может подходить под тэг, для остальных
    //! TagType = Invalid
    virtual Error extractText(
        //! Проверяемый узел
        const XMLNodePtr& node,
        //! Текст этого узла, который относится в тэгу
        QString& text,
        //! Текст этого узла, который находится до начала тэга
        QString& text_before,
        //! Тип найденного в узле тэга
        TagType& type,
        //! Тэг содержится в нескольких узлах. Надо проверить следующий
        bool& is_scan_next)
        = 0;
    //! Извлеч текст из элемента при условии, что он является дополнением в найденному ранее элементу с тэгом
    virtual Error extractTextNext(
        //! Элемент, в котором находится начало тэга
        const XMLNodePtr& start_node,
        //! Тип тэга
        TagType type,
        //! Текст тэга, найденный путем слияния предыдущих узлов
        const QString& previous_text,
        //! Проверяемый узел
        const XMLNodePtr& node,
        //! Текст этого узла, который относится в тэгу
        QString& text,
        //! Текст этого узла, который находится после окончания тэга
        QString& text_after,
        //! Надо проверить следующий, т.к. конец тэга не найден
        bool& is_scan_next)
        = 0;
    //! Определить родителя, который отвечает за хранение информации о тэге. Только для начала или окончания блока
    //! Родительский тэг и все его содержимое будет удалено в процессе генерации выходного документа
    virtual Error getTagMainNode(        
        //! Проверяемый узел
        const XMLNodePtr& node,
        //! Тип тэга
        TagType type,
        //! Текст тэга
        const QString& tag_text,
        //! Родительский узел, который надо вырезать при замене тэга на значение
        XMLNodePtr& main_node)
        = 0;
    //! Записать указанный текст в элемент
    virtual Error setText(const QString& text, const XMLNodePtr& element) = 0;
    //! Записать изображение в указанный элемент.
    virtual Error setImage(const XMLNodePtr& set_to, const QString& text_before, const QString& text_after, const QImage& value,
                           const QString& image_id)
        = 0;

private:
    //! Результат парсинга для одного тэга
    struct TagInfo
    {        
        //! Свойство данных
        DataProperty property;
        //! Строка набора данных (для TagType::Cell)
        int cell_row = -1;
        //! Колонка набора данных (для TagType::Cell)
        DataProperty cell_column;
        //! Текст тэга
        QString tag_text;
        //! Текст этого узла, который находится до начала тэга
        QString text_before;
        //! Текст этого узла, который находится после окончания тэга
        QString text_after;
        //! Тип узла
        TagType type = TagType::Invalid;
        //! Узел, в котором содержится тэг
        XMLNodePtr node;
        //! Узел для node, который надо использовать для удаления тэга (т.е. вырезается не node, а main_node)
        XMLNodePtr main_node;
    };
    typedef std::shared_ptr<TagInfo> TagInfoPtr;

    //! Вид блока
    enum class BlockType
    {
        Invalid,
        //! Простой (поле)
        Simple,
        //! Составной (два тэга - начало и конец)
        Complex,
    };
    //! Результаты парсинга по блокам
    struct BlockInfo
    {
        //! Вид блока
        BlockType type = BlockType::Invalid;
        //! Начальный тэг для составных блоков или просто тэг для простых
        TagInfoPtr first_tag;
        //! Конечный тэг для составных блоков
        TagInfoPtr second_tag;        
        //! Дочерние блоки
        QList<std::shared_ptr<BlockInfo>> children;
    };
    typedef std::shared_ptr<BlockInfo> BlockInfoPtr;

    //! Рекурсивный парсинг исходного документа
    Error parseHelper(        
        //! Родительский узел для парсинга
        const XMLNodePtr& parent,
        //! Хэш для поиска полей данных по тэгам
        const QHash<QString, DataProperty>& property_hash,        
        //! Если не пусто, то находимся в поиске окончания блока
        QStack<BlockInfoPtr>& current_blocks,
        //! Если не nullptr, то находимся в режиме накопления узлов для найденного ранее тэга
        TagInfoPtr& current_tag);

    //! Поиск свойства по его ключу
    Error findDataProperty(const QString& key,        
        //! Хэш для поиска полей данных по тэгам
        const QHash<QString, DataProperty>& property_hash,        
        //! Найденное свойство
        DataProperty& property);

    //! Рукурсивная генерация выходного документа
    Error generateHelper(const BlockInfoPtr& block);
    //! Генерация строки набора данных
    Error generateDatasetRow(const BlockInfoPtr& block, const XMLNodePtr& insert_after, const QList<XMLNodePtr>& row_nodes,
        const QMap<QString, XMLNodePtr>& row_nodes_map, int row);

    //! Записать текстовое значение в поле
    Error setTextFieldValue(const XMLNodePtr& set_to, const QString& value);
    //! Записать изображение значение в поле
    Error setImageFieldValue(const XMLNodePtr& set_to, const QString& text_before, const QString& text_after, const QVariant& value,
                             const QString& image_id);
    //! Язык для поля
    QLocale::Language fieldLanguage(const DataProperty& property) const;

    //! Очистить распарсенные данные
    void clear();

    //! Документ
    XMLDocumentPtr _document;
    //! Используемые свойства
    QSet<DataProperty> _properties;    
    //! Результаты парсинга - иерархия блоков
    QList<BlockInfoPtr> _blocks;
    //! Временная папка
    QString _temp_folder;

    //! Источник данных
    const DataContainer* _data = nullptr;
    //! Язык по умолчанию для генерации отчета
    QLocale::Language _language = QLocale::AnyLanguage;
    /*! Автоматическое определение соответствия между полями данных и текстовыми метками в шаблоне по DataProperty::id
     * Ищет совпадение между целочисленными текстовыми метками и DataProperty::id. Работает как дополнение к field_names */
    bool _auto_map = false;
    //! Язык для конкретных полей данных
    QMap<DataProperty, QLocale::Language> _field_languages;

    //! Имя последнего успешно созданного файла
    static QString _last_ok_file;
};

} // namespace zf
