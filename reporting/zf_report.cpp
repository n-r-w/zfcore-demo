#include "zf_report.h"
#include "zf_core.h"
#include "zf_xml.h"
#include "zf_translation.h"

#include <QDebug>
#include <QDir>
#include <QDesktopServices>

namespace zf
{
//! Имя последнего успешно созданного файла
QString ReportGenerator::_last_ok_file;

QAtomicInt _counter_test = 0;

ReportGenerator::ReportGenerator()
{
    Z_DEBUG_NEW("ReportGenerator");
}

ReportGenerator::~ReportGenerator()
{
    clear();
    Z_DEBUG_DELETE("ReportGenerator");
}

void ReportGenerator::clear()
{
    _properties.clear();
    _blocks.clear();
    if (_document != nullptr)
        _document.reset();
}

Error ReportGenerator::createError(const QString& text) const
{
    return Error(QString("Template error. File: %1, %2").arg(QDir::toNativeSeparators(document()->name()), text));
}

XMLDocument* ReportGenerator::document() const
{
    Z_CHECK_NULL(_document);
    return _document.get();
}

Error ReportGenerator::splitText(const QString& text, bool search_next, const QList<QPair<QString, QString>>& marks,
                                 const QList<ReportGenerator::TagType>& types, QString& text_before, QString& tag_text, QString& text_after,
                                 TagType& type, SplitTextResult& result) const
{
    Q_UNUSED(search_next);
    Z_CHECK(!marks.isEmpty() && marks.count() == types.count());

    type = TagType::Invalid;
    result = SplitTextResult::NoTag;
    text_before.clear();
    tag_text.clear();
    text_after.clear();

    QString n_text = text.trimmed().toLower();
    if (n_text.isEmpty())
        return Error();

    for (int i = 0; i < marks.count(); i++) {
        QString begin_m = marks.at(i).first;
        Z_CHECK(!begin_m.isEmpty());
        QString end_m = marks.at(i).second;
        Z_CHECK(!end_m.isEmpty());
        TagType type_m = types.at(i);
        Z_CHECK(type_m != TagType::Invalid);

        int start_pos = n_text.indexOf(begin_m);
        int end_pos = n_text.indexOf(end_m);
        if (start_pos >= 0 && end_pos >= 0 && end_pos <= start_pos)
            return createError(QString("intersection of the beginning and end marks: %1").arg(n_text));

        int before_start = -1;
        int before_end = -1;
        int tag_start = -1;
        int tag_end = -1;
        int after_start = -1;
        int after_end = -1;

        if (start_pos >= 0) {
            before_start = 0;
            tag_start = start_pos + begin_m.length();
            before_end = start_pos - 1;

        } else {
            tag_start = 0;
        }

        if (end_pos >= 0) {
            after_start = end_pos + end_m.length();
            tag_end = end_pos - 1;
            after_end = n_text.length() - 1;
        } else {
            tag_end = n_text.length() - 1;
        }

        if (start_pos < 0 && end_pos < 0)
            continue;

        tag_text = n_text.mid(tag_start, tag_end - tag_start + 1);
        text_before = n_text.mid(before_start, before_end - before_start + 1);
        text_after = n_text.mid(after_start, after_end - after_start + 1);

        type = types.at(i);

        if (start_pos >= 0 && end_pos < 0)
            result = SplitTextResult::TagBegin;
        else if (start_pos < 0 && end_pos >= 0)
            result = SplitTextResult::TagEnd;
        else if (start_pos >= 0 && end_pos >= 0)
            result = SplitTextResult::FullTag;
        else
            Z_HALT_INT;

        // надо дополнить text_before и text_after пробелами если они есть
        for (int i = 0; i < text.length(); i++) {
            if (text.at(i) == ' ')
                text_before.prepend(' ');
            else
                break;
        }
        for (int i = text.length() - 1; i >= 0; i--) {
            if (text.at(i) == ' ')
                text_after.append(' ');
            else
                break;
        }

        return Error();
    }

    return Error();
}

QString ReportGenerator::prepareTargetFileName(const QString& file_name) const
{
    return file_name;
}

Error ReportGenerator::prepareTree(const XMLNodePtr& root)
{
    Q_UNUSED(root);
    return {};
}

Error ReportGenerator::parseHelper(const XMLNodePtr& parent, const QHash<QString, DataProperty>& property_hash,
    QStack<ReportGenerator::BlockInfoPtr>& current_blocks, ReportGenerator::TagInfoPtr& current_tag)
{
    if (!parent->hasChildren())
        return Error();

    XMLNodePtr node = parent->firstChild();

    Error error;
    while (node != nullptr) {
        bool is_scan_next = false;
        if (current_tag == nullptr) {
            QString text;
            QString text_before;
            // нет ранее найденного начала тэга, ищем новые
            TagType type = TagType::Invalid;
            error = extractText(node, text, text_before, type, is_scan_next);
            if (error.isError())
                return error;

            if (type != TagType::Invalid) {
                // найдено начало тэга
                current_tag = Z_MAKE_SHARED(TagInfo);
                current_tag->tag_text = text.trimmed().toLower();
                current_tag->text_before = text_before;
                current_tag->type = type;
                current_tag->node = node;

                if (type == TagType::BlockStart || type == TagType::BlockFinish) {
                    XMLNodePtr main_node;
                    error = getTagMainNode(node, current_tag->type, current_tag->tag_text, main_node);
                    if (error.isError())
                        return error;
                    Z_CHECK_NULL(main_node);
                    current_tag->main_node = main_node;
                }
            }

        } else {
            // находимся в процессе поиска закрывающих символов для тэга
            Z_CHECK_NULL(current_tag->node);
            QString tag_text;
            QString text_after;
            error = extractTextNext(
                current_tag->node, current_tag->type, current_tag->tag_text, node, tag_text, text_after, is_scan_next);
            if (error.isError())
                return error;

            tag_text = tag_text.trimmed();
            if (!tag_text.isEmpty() || is_scan_next) {
                if (is_scan_next) {
                    // в узле найдено продолжение тэга
                    current_tag->tag_text += tag_text.toLower();
                } else {
                    // найден сам тэг
                    current_tag->tag_text = tag_text.toLower();
                }
                // удаляем из узла содержимое тэга
                if (!text_after.isEmpty())
                    Z_CHECK(node->type() == XML_NodeType::XML_TEXT_NODE);
                if (node->type() == XML_NodeType::XML_TEXT_NODE)
                    node->setContent(text_after);
            }

            if (!text_after.isEmpty()) {
                Z_CHECK(!is_scan_next); // не может быть, что тэг закончился, а мы хотим искать дальше
                current_tag->text_after += text_after;
            }

            if (tag_text.isEmpty() && !is_scan_next && node != current_tag->node) {
                // надо удалить закрывающий текст
                if (node->type() == XML_NodeType::XML_TEXT_NODE)
                    node->setContent(QString());
            }
        }

        if (!is_scan_next && current_tag != nullptr) {
            // дальше сканировать группу не надо, запоминаем результат парсинга тэга
            QString key = current_tag->tag_text;
            DataProperty property;
            int cell_row = -1;
            DataProperty cell_column_property;

            if (current_tag->type == TagType::Cell) {
                // извлекаем из тэга код набора данных, строку и колонку
                QStringList parts = key.split(":", QString::SkipEmptyParts);
                bool parse_error = false;
                if (parts.count() == 3) {
                    key = parts.at(0);
                    bool ok;
                    cell_row = parts.at(1).toInt(&ok);
                    if (ok && cell_row >= 0) {
                        const QString& cell_column_key = parts.at(2);
                        error = findDataProperty(cell_column_key, property_hash, cell_column_property);
                        if (error.isError())
                            return error;

                    } else {
                        parse_error = true;
                    }
                }

                if (parse_error)
                    return createError(
                        QString("ReportGenerator::parseHelper. Wrong tag <%1> for data type <Cell>").arg(current_tag->tag_text));
            }

            error = findDataProperty(key, property_hash, property);
            if (error.isError())
                return error;

            // Корректируем тип тэга
            if (current_tag->type == TagType::Field && property.isColumn()) {
                if (current_blocks.isEmpty()) {
                    Core::logError(QString("ReportGenerator::parseHelper. Wrong tag <%1> for data type <Column>").arg(key));
                    property.clear();

                } else {
                    current_tag->type = TagType::Column;
                }
            }

            if (property.isValid()) {
                // проверяем на ошибки
                if (property.propertyType() == PropertyType::Field || property.isColumn()
                    || property.propertyType() == PropertyType::Dataset) {
                    if (property.propertyType() == PropertyType::Field && current_tag->type != TagType::Field) {
                        return createError(QString("ReportGenerator::parseHelper. Wrong tag <%1> for data type <Field>").arg(key));
                    }

                    if (property.isColumn() && current_tag->type != TagType::Column) {
                        return createError(QString("ReportGenerator::parseHelper. Wrong tag <%1> for data type <Column>").arg(key));
                    }

                    if (property.propertyType() == PropertyType::Dataset) {
                        if (current_tag->type != TagType::Cell && current_tag->type != TagType::BlockStart
                            && current_tag->type != TagType::BlockFinish) {
                            return createError(QString("ReportGenerator::parseHelper. Wrong tag <%1> for data type <Dataset>").arg(key));
                        }
                    }

                } else {
                    // свойство найдено, но оно имеет тип данных, который не поддерживается генератором отчетов
                    return createError(QString("ReportGenerator::parseHelper. Unsupported tag <%1> for data type <%2>")
                                           .arg(key)
                                           .arg(static_cast<int>(property.propertyType())));
                }
            }
            if (current_tag->type == TagType::BlockFinish) {
                if (current_blocks.isEmpty()) {
                    // нашли закрывающий тэг, но для него нет открывающего
                    return createError(QString("No open block tag for <%1>").arg(current_tag->tag_text));
                }
                if (current_blocks.last()->first_tag->tag_text != current_tag->tag_text) {
                    // нашли закрывающий тэг, но для него не совпадает тэг в последним в стеке
                    return createError(QString("Open block tag mismatch <%1> for <%2>")
                                           .arg(current_blocks.last()->first_tag->tag_text, current_tag->tag_text));
                }
            }

            current_tag->property = property;
            current_tag->cell_row = cell_row;
            current_tag->cell_column = cell_column_property;
            Z_CHECK_NULL(current_tag->node);

            if (current_tag->type == TagType::BlockFinish) {
                // закрываем блок
                Z_CHECK(!current_blocks.isEmpty());
                BlockInfoPtr closing_block = current_blocks.pop();
                closing_block->second_tag = current_tag;

            } else {
                // создаем новый блок
                BlockInfoPtr new_block = Z_MAKE_SHARED(BlockInfo);
                new_block->first_tag = current_tag;
                if (current_blocks.isEmpty()) {
                    // верхний уровень блоков запоминаем
                    _blocks << new_block;

                } else {
                    // нижние уровни добавляем к верхним
                    current_blocks.last()->children << new_block;
                }

                if (current_tag->type == TagType::BlockStart) {
                    new_block->type = BlockType::Complex;
                    current_blocks.push(new_block);

                } else if (current_tag->type == TagType::Field) {
                    new_block->type = BlockType::Simple;

                } else if (current_tag->type == TagType::Cell) {
                    new_block->type = BlockType::Simple;

                } else if (current_tag->type == TagType::Column) {
                    new_block->type = BlockType::Simple;

                    // проверяем что колонка входит в текущий блок (набор данных)
                    if (property.isValid() && !current_blocks.last()->first_tag->property.contains(property))
                        return createError(QString("Column <%1> not found in dataset <%2>")
                                               .arg(property.name(), current_blocks.last()->first_tag->property.name()));
                }
            }

            // найден конец текущего тега - сбрасываем
            current_tag.reset();
        }

        // парсим дочерние узлы
        error = parseHelper(node, property_hash, current_blocks, current_tag);
        if (error.isError())
            return error;

        node = node->next();
    }

    return Error();
}

Error ReportGenerator::findDataProperty(const QString& key, const QHash<QString, DataProperty>& property_hash, DataProperty& property)
{
    auto prop_it = property_hash.find(key);

    if (prop_it == property_hash.end()) {
        if (_auto_map) {
            bool ok;
            int id = key.toInt(&ok);
            if (ok && id >= Consts::MINUMUM_PROPERTY_ID)
                property = _data->structure()->property(PropertyID(id), false);
        }

        if (!property.isValid())
            return Error();

    } else
        property = prop_it.value();

    return Error();
}

Error ReportGenerator::generateHelper(const ReportGenerator::BlockInfoPtr& block)
{
    Z_CHECK_NULL(block);

    // заполняем данными дочерние блоки
    Error error;
    for (auto& b : block->children) {
        error = generateHelper(b);
        if (error.isError())
            return error;
    }
    // дочерние блоки (если они были) заполнены данными, можно заполнять данными сам этот блок
    if (block->type == BlockType::Simple) {
        if (block->first_tag->type == TagType::Field || block->first_tag->type == TagType::Cell) {
            // поле данных, не зависящее от набора данных - заполняем
            TagInfoPtr field_tag = block->first_tag;

            if (field_tag->property.isValid() && !_data->structure()->contains(field_tag->property))
                return createError(QString("Property <%1> not found").arg(field_tag->property.name()));

            // заполняем данными
            std::unique_ptr<QString> value_text;
            std::unique_ptr<QVariant> value_image;
            QString image_id;

            if (field_tag->property.isValid()) {
                if (block->first_tag->type == TagType::Field) {
                    if (field_tag->property.dataType() == DataType::Image) {
                        value_image
                            = std::make_unique<QVariant>(_data->value(field_tag->property, fieldLanguage(field_tag->property), true));
                        image_id = field_tag->property.id().string() + "_"
                                   + QString::number(static_cast<int>(fieldLanguage(field_tag->property)));

                    } else {
                        value_text = std::make_unique<QString>(
                            Utils::variantToString(_data->value(field_tag->property, fieldLanguage(field_tag->property), true),
                                                   fieldLanguage(field_tag->property)));
                    }

                } else if (block->first_tag->type == TagType::Cell) {
                    Z_CHECK(block->first_tag->cell_row >= 0 && block->first_tag->cell_column.isValid());
                    if (block->first_tag->cell_row < _data->rowCount(block->first_tag->property)) {
                        if (block->first_tag->cell_column.dataType() == DataType::Image) {
                            value_image = std::make_unique<QVariant>(_data->cell(block->first_tag->cell_row, block->first_tag->cell_column,
                                                                                 Qt::DisplayRole, QModelIndex(),
                                                                                 fieldLanguage(block->first_tag->cell_column)));
                            image_id = field_tag->cell_column.id().string() + "_" + QString::number(block->first_tag->cell_row) + "_"
                                       + QString::number(static_cast<int>(fieldLanguage(block->first_tag->cell_column)));

                        } else {
                            value_text = std::make_unique<QString>(Utils::variantToString(
                                _data->cell(block->first_tag->cell_row, block->first_tag->cell_column, Qt::DisplayRole, QModelIndex(),
                                            fieldLanguage(block->first_tag->cell_column), true),
                                fieldLanguage(block->first_tag->cell_column)));
                        }
                    }

                } else
                    Z_HALT_INT;
            }

            if (value_text != nullptr)
                error = setTextFieldValue(field_tag->node, field_tag->text_before + *value_text + field_tag->text_after);
            else if (value_image != nullptr)
                error = setImageFieldValue(field_tag->node, field_tag->text_before, field_tag->text_after, *value_image, image_id);
            else
                error = setTextFieldValue(field_tag->node, field_tag->text_before + field_tag->text_after);

            if (error.isError())
                return error;

        } else if (block->first_tag->type == TagType::Column) {
            // колонка набора данных - будет заполняться уровнем выше при заполнении набора данных, в который она входит

        } else
            Z_HALT_INT;

    } else if (block->type == BlockType::Complex) {
        // составной блок (набор данных)

        // копируем структуру записи для ее последующего клонирования по строкам набора данных
        QList<XMLNodePtr> row_nodes;

        Z_CHECK_NULL(block->first_tag->main_node);
        Z_CHECK_NULL(block->second_tag->main_node);

        XMLNodePtr internal_start_node = block->first_tag->main_node->next();
        XMLNodePtr internal_finish_node = block->second_tag->main_node;
        while (internal_start_node != nullptr && internal_start_node != internal_finish_node) {
            row_nodes << internal_start_node;
            internal_start_node = internal_start_node->next();
            row_nodes.last()->unlink(); // изъятие из дерева
        }
        QMap<QString, XMLNodePtr> row_nodes_map = XMLNode::createNodesMap(row_nodes);

        // заполняем построчно
        DataProperty dataset_property = block->first_tag->property;
        int row_count = _data->dataset(dataset_property)->rowCount();

        for (int row = row_count - 1; row >= 0; row--) {
            error = generateDatasetRow(block, block->second_tag->main_node, row_nodes, row_nodes_map, row);
            if (error.isError())
                return error;
        }

        // удаляем начало и конец блока
        XMLDocument::removeNodes({block->first_tag->main_node});
        XMLDocument::removeNodes({block->second_tag->main_node});

    } else
        Z_HALT_INT;

    return Error();
}

Error ReportGenerator::generateDatasetRow(const BlockInfoPtr& block, const XMLNodePtr& insert_after, const QList<XMLNodePtr>& row_nodes,
    const QMap<QString, XMLNodePtr>& row_nodes_map, int row)
{
    // создаем узлы новой строки
    QList<XMLNodePtr> cloned_nodes;
    for (const XMLNodePtr& node : row_nodes) {
        cloned_nodes << node->clone(XMLNode::CloneProperties);
    }
    QMap<QString, XMLNodePtr> cloned_nodes_map = XMLNode::createNodesMap(cloned_nodes);

    // добавляем новые узлы в документ
    for (const XMLNodePtr& node : qAsConst(cloned_nodes)) {
        Z_CHECK_NULL(insert_after->addNextSibling(node));
    }

    DataProperty dataset_property = block->first_tag->property;
    // заполняем данными по колонкам
    for (const BlockInfoPtr& child_block : qAsConst(block->children)) {
        if (child_block->type != BlockType::Simple || child_block->first_tag->type != TagType::Column)
            continue;

        // ищем куда будем вставлять значение
        QString key = row_nodes_map.key(child_block->first_tag->node);
        XMLNodePtr set_value_to = cloned_nodes_map.value(key);
        Z_CHECK_NULL(set_value_to);

        // вставляем значение
        Error error;
        DataProperty column_property = child_block->first_tag->property;
        QVariant value = _data->cell(row, column_property, Qt::DisplayRole, QModelIndex(), QLocale::AnyLanguage, true);
        if (column_property.dataType() == DataType::Image)
            error = setImageFieldValue(set_value_to, "", "", value,
                                       column_property.id().string() + "_" + QString::number(row) + "_"
                                           + QString::number(static_cast<int>(QLocale::AnyLanguage)));
        else
            error = setTextFieldValue(set_value_to, Utils::variantToString(value, fieldLanguage(column_property)));

        if (error.isError())
            return error;
    }

    return Error();
}

Error ReportGenerator::setTextFieldValue(const XMLNodePtr& set_to, const QString& value)
{
    return setText(value, set_to);
}

Error ReportGenerator::setImageFieldValue(const XMLNodePtr& set_to, const QString& text_before, const QString& text_after,
                                          const QVariant& value, const QString& image_id)
{
    Error error;
    QImage image;

    if (value.type() == QVariant::ByteArray) {
        image.loadFromData(value.toByteArray());

    } else if (value.type() == QVariant::Icon) {
        QIcon icon = qvariant_cast<QIcon>(value);
        image = icon.pixmap(Utils::maxPixmapSize(icon)).toImage();

    } else if (value.type() == QVariant::Pixmap) {
        image = qvariant_cast<QPixmap>(value).toImage();
    }

    if (image.isNull())
        return setTextFieldValue(set_to, "");

    Z_CHECK(!image_id.isEmpty());

    return setImage(set_to, text_before, text_after, image, image_id);
}

QLocale::Language ReportGenerator::fieldLanguage(const DataProperty& property) const
{
    return _field_languages.value(property, _language);
}

Error ReportGenerator::generate(const DataContainer* data, const QMap<DataProperty, QString>& field_names, bool auto_map,
    const QString& template_file, const QString& target_file, QLocale::Language language,
    const QMap<DataProperty, QLocale::Language>& field_languages)
{
    if (!QFile::exists(template_file))
        return Error::fileNotFoundError(template_file);

    QByteArray ba_template;
    QFile f(template_file);
    if (!f.open(QFile::ReadOnly))
        return Error::fileIOError(template_file);
    ba_template = f.readAll();
    if (ba_template.size() != f.size()) {
        f.close();
        return Error::fileIOError(template_file);
    }
    f.close();

    return generate(data, field_names, auto_map, ba_template, target_file, language, field_languages);
}

Error ReportGenerator::generate(const DataContainer* data, const QMap<PropertyID, QString>& field_names, bool auto_map,
                                const QString& template_file, const QString& target_file, QLocale::Language language,
                                const QMap<PropertyID, QLocale::Language>& field_languages)
{
    if (!QFile::exists(template_file))
        return Error::fileNotFoundError(template_file);

    QByteArray ba_template;
    QFile f(template_file);
    if (!f.open(QFile::ReadOnly))
        return Error::fileIOError(template_file);
    ba_template = f.readAll();
    if (ba_template.size() != f.size()) {
        f.close();
        return Error::fileIOError(template_file);
    }
    f.close();

    return generate(data, field_names, auto_map, ba_template, target_file, language, field_languages);
}

Error ReportGenerator::generate(const DataContainer* data, const QMap<DataProperty, QString>& field_names, bool auto_map,
                                const QByteArray& template_file, const QString& target_file, QLocale::Language language,
                                const QMap<DataProperty, QLocale::Language>& field_languages)
{
    Z_CHECK_NULL(data);

    _data = data;
    _language = (language == QLocale::AnyLanguage ? Core::language(LocaleType::Workflow) : language);
    _auto_map = auto_map;
    _field_languages = field_languages;

    _temp_folder = Utils::generateTempDirName();
    Error error = Utils::makeDir(_temp_folder);
    if (error.isError())
        return error;

    QString target_file_name = prepareTargetFileName(target_file);

    QStringList to_parse;
    error = unpackTemplate(template_file, _temp_folder, to_parse);
    if (error.isError())
        return error;

    // шаблон был упакован
    bool is_packed_template = !to_parse.isEmpty();
    if (!is_packed_template) {
        // надо создать временный файл для парсинга на основе неупакованных данных
        QString temp_file = _temp_folder + "/" + Utils::generateUniqueString();
        QFile f_temp(temp_file);
        if (!f_temp.open(QFile::WriteOnly | QFile::Truncate))
            return Error::fileIOError(temp_file);

        f_temp.write(template_file);

        f_temp.close();
        to_parse << temp_file;
    }

    for (auto& file_name : to_parse) {
        if (error.isOk())
            error = openDocument(file_name, _document);

        XMLNodePtr root;
        if (error.isOk()) {
            Z_CHECK_NULL(_document);
            root = _document->getRootElement();
            if (root == nullptr)
                error = createError("Empty template");
        }

        if (error.isOk()) {
            QHash<QString, DataProperty> property_hash;
            for (auto i = field_names.constBegin(); i != field_names.constEnd(); ++i) {
                Z_CHECK(i.key().isValid());
                Z_CHECK(!i.value().isEmpty());
                property_hash[i.value().toLower()] = i.key();
            }

            error = prepareTree(root);

            QStack<BlockInfoPtr> begin_blocks;
            TagInfoPtr current_tag;
            if (error.isOk())
                error = parseHelper(root, property_hash, begin_blocks, current_tag);

            if (error.isOk()) {
                if (current_tag != nullptr)
                    error = createError(QString("Found not closed tag <%1>").arg(current_tag->tag_text));

                if (!begin_blocks.isEmpty()) {
                    // есть не закрытые блоки
                    QStringList not_closed;
                    for (auto& b : begin_blocks) {
                        not_closed << b->first_tag->tag_text;
                    }
                    error << createError(QString("Found blocks without close tag <%1>").arg(not_closed.join(", ")));
                }
            }
        }

        if (error.isOk()) {
            // начинаем рекурсивно заполнять блоки от нижнего уровня к верхнему
            for (auto& b : _blocks) {
                error = generateHelper(b);
                if (error.isError())
                    break;
            }

            if (error.isOk()) {
                if (is_packed_template)
                    error = saveDocument(file_name, _document);
                else
                    error = saveDocument(target_file_name, _document);
            }
        }

        clear();
    }

    if (error.isOk() && is_packed_template)
        error = packTarget(_temp_folder, target_file_name);

    Utils::removeDir(_temp_folder);

    if (Utils::isMainThread()) {
        if (error.isOk())
            _last_ok_file = target_file_name;
        else
            _last_ok_file.clear();
    }

    return error;
}

Error ReportGenerator::generate(const DataContainer* data, const QMap<PropertyID, QString>& field_names, bool auto_map,
                                const QByteArray& template_file, const QString& target_file, QLocale::Language language,
                                const QMap<PropertyID, QLocale::Language>& field_languages)
{
    Z_CHECK_NULL(data);

    QMap<DataProperty, QString> f;
    for (auto it = field_names.constBegin(); it != field_names.constEnd(); ++it) {
        f[data->property(it.key())] = it.value();
    }

    QMap<DataProperty, QLocale::Language> f_l;
    for (auto it = field_languages.constBegin(); it != field_languages.constEnd(); ++it) {
        f_l[data->property(it.key())] = it.value();
    }

    return generate(data, f, auto_map, template_file, target_file, language, f_l);
}

void ReportGenerator::showLastOk()
{
    Z_CHECK(Utils::isMainThread());

    if (_last_ok_file.isEmpty())
        return;

    if (Core::ask(ZF_TR(ZFT_REPORT_OK).arg(QDir::toNativeSeparators(_last_ok_file))))
        QDesktopServices::openUrl(QUrl::fromLocalFile(_last_ok_file));

    _last_ok_file.clear();
}

} // namespace zf
