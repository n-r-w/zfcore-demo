#pragma once

#include "zf_report.h"

namespace zf
{
/*!
 * \brief The DocxReportGenerator class
 * Генерация документов на основании шаблонов DOCX
 *
 * Описание полей в шаблоне:    {{текстовый код поля}}
 *
 * Описание строк таблиц в шаблоне:
 * Начало строки таблицы:       {[текстовый код набора данных]}
 * Окончание строки таблицы:    {#текстовый код набора данных#}
 *
 * Описание ячеек таблицы
 * Если надо динамически вводить строки таблицы:
 *  1) Закрыть таблицу {[ТАБЛИЦА]} .... {#ТАБЛИЦА#}
 *  2) Добавить в середину одну строку таблицы word с кодами колонок {{КОЛОНКА}}
 * Если надо указать конкретную ячейку:
 *  1) Не оформлять таблицу через {[ТАБЛИЦА]}...{#ТАБЛИЦА#}
 *  2) Просто указать: {<текстовый_код_набора_данных:номер_строки:текстовый_код_колонки_набора_данных]>}
 *
 * Параграфы, в которых находятся начало и конец строки таблицы, полностью вырезаются из документа при генерации,
 * поэтому в них нельзя помещать любую важную информацию (например перенос страницы).
 *
 * Метод ReportGenerator::generate принимает на вход шаблон в формате DOCX. При этом он распаковывается и обрабатываются
 * все файлы с расширением XML (тело документа, заголовки и т.п.)
 */
class ZCORESHARED_EXPORT DocxReportGenerator : public ReportGenerator
{
public:
    DocxReportGenerator();

protected:
    /*! Распаковать исходный шаблон если он упакован и поместить во временные файлы (будут удалены после обработки)
     * Если шаблон не упакован, то возвратить пустой список */
    Error unpackTemplate(const QByteArray& template_file, const QString& temp_folder, QStringList& to_parse) override;
    //! Упаковать папку в целевой файл (только для изначально упакованных шаблонов)
    Error packTarget(const QString& temp_folder, const QString& target_file) override;

    //! Подготовка названия целевого файла (например добавление расширения и т.п.)
    QString prepareTargetFileName(const QString& file_name) const override;

    //! Открыть документ
    Error openDocument(const QString& template_file, XMLDocumentPtr& document) override;
    //! Записать документ в файл
    Error saveDocument(const QString& target_file, const XMLDocumentPtr& document) override;    
    //! Извлечь текст из элемента и определить его тип. Необходимо возвращать только тот текст, который может подходить
    //! под тэг, для остальных TagType = Invalid
    Error extractText(
        //! Проверяемый узел
        const XMLNodePtr& node,
        //! Текст этого узла, который относится в тэгу
        QString& text,
        //! Текст этого узла, который находится до начала тэга
        QString& text_before,
        //! Тип найденного в узле тэга
        TagType& type,
        //! Тэг содержится в нескольких узлах. Надо проверить следующий
        bool& is_scan_next) override;
    //! Извлеч текст из элемента при условии, что он является дополнением в найденному ранее элементу с тэгом
    Error extractTextNext(
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
        bool& is_scan_next) override;
    //! Определить его родителя, который отвечает за хранение информации о тэге
    Error getTagMainNode(
        //! Проверяемый узел
        const XMLNodePtr& node,
        //! Тип тэга
        TagType type,
        //! Текст тэга
        const QString& tag_text,
        //! Родительский узел, который надо вырезать при замене тэга на значение
        XMLNodePtr& main_node) override;
    //! Записать указанный текст в элемент
    Error setText(const QString& text, const XMLNodePtr& element) override;
    //! Записать изображение в указанный элемент.
    Error setImage(const XMLNodePtr& set_to, const QString& text_before, const QString& text_after, const QImage& value,
                   const QString& image_id) override;

private:
    //! Конвертация размера из пикселей в EMU (English Metric Units)
    static int pixelsToEMU(int n);
    //! Создание узла XML
    XMLNodePtr createNode(const XMLNodePtr& parent, const QString& name, const QMap<QString, QVariant>& values = {});
    //! Подготовка файлов во временном каталоге перед архивацией
    Error prepareFiles(const QString& temp_folder) const;
    //! Добавить строку в xml
    Error appendToXML(const QString& xml_file, const QString& target_node, const QString& name, const QMap<QString, QString>& params,
                      //! Исключения. Ключ: имя узла, значение: <имя атрибута, значение атрибута>
                      const QMap<QString, QPair<QString, QString>>& exclude) const;

    //! Компрессия переменных
    static Error compress(const QString& source_data, QString& result_data);

    //! Добавленные картинки. Ключ - id
    QMap<QString, QImage> _image_info;

    //! Открывающие и закрывающие маркеры
    static const QList<QPair<QString, QString>> _MARKS;
};

} // namespace zf
