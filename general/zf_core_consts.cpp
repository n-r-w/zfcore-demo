#include "zf_core_consts.h"

namespace zf
{
//! Некорректный идентификатор БД
const int Consts::INVALID_DATABASE_ID = 0;
//! Не существующий код идентификатора
const int Consts::INVALID_ENTITY_CODE = 0;
//! Код идентификатора независимой сущности
const int Consts::INDEPENDENT_ENTITY_CODE = 1;
//! Не существующий ID сущности
const int Consts::INVALID_ENTITY_ID = 0;
//! Минимально допустимый id свойства
const int Consts::MINUMUM_PROPERTY_ID = 1;
//! Минимальный номер кода (для любых идентификаторов). Номера меньше этого зарезервированы под нужды ядра
const int Consts::MIN_ENTITY_CODE = 500;
//! Максимальный номер кода сущности
const int Consts::MAX_ENTITY_CODE = USHRT_MAX;
//! Максимальный номер кода базы данных
const int Consts::MAX_DATABASE_ID = USHRT_MAX;

//! Разделитель
const QChar Consts::KEY_SEPARATOR = QChar(QChar::ParagraphSeparator);
//! Количество знаков после запятой для дробных чисел
const int Consts::DOUBLE_DECIMALS = 2;
//! Размер групп цифр для деления на классы (Пример: 1 000 000,00)
const int Consts::DIGITS_GROUP_SIZE = 3;
//! Код конфигурации ядра (для внутреннего использования)
const QString Consts::CORE_INTERNAL_CODE = "metastaff";
//! Код конфигурации разработчика (для внутреннего использования)
const QString Consts::CORE_DEV_CODE = "core";
//! Разделитель при выводе в CSV файлы
const QChar Consts::CSV_SEPARATOR = ';';

/*! Роль в которой хранится уникальный идентификатор строки набора данных. Для хранения выбирается колонка с
 * минимальным номером */
const int Consts::UNIQUE_ROW_COLUMN_ROLE = Qt::UserRole + USHRT_MAX;

//! Версия QDataStream
const QDataStream::Version Consts::DATASTREAM_VERSION = QDataStream::Qt_5_12;

//! Символ для подсчета размера строки (N)
const QChar Consts::AVERAGE_CHAR = 'N';

//! Разделитель составных значений
const QChar Consts::COMPLEX_SEPARATOR = ',';

//! Задержка после нажатия кнопки пользователем, после которой начинается поиск, выбор нового элемента и т.п.
const int Consts::USER_INPUT_TIMEOUT_MS = 300;

//! Минимальный номер id при вызовах HighlightInfo::insert/remove/empty/set и при прочей работе с HighlightModel
//! id меньше чем этот, зарезервированы на нужны ядра
const int Consts::MINIMUM_HIGHLIGHT_ID = 1000;

//! Кодировка при выгрузке в файл
const QString Consts::FILE_ENCODING = "Windows-1251";

const Command ModuleCommands::Load(1);
const Command ModuleCommands::Save(2);
const Command ModuleCommands::Remove(3);
const int ModuleCommands::CustomCommand = 500;

} // namespace zf
