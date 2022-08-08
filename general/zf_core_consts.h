#pragma once

#include "zf_basic_types.h"

namespace zf
{
//! Общие константы
class ZCORESHARED_EXPORT Consts
{
public:
    //! Некорректный идентификатор БД
    static const int INVALID_DATABASE_ID; // 0
    //! Максимальный номер кода базы данных
    static const int MAX_DATABASE_ID; // USHRT_MAX

    //! Не существующий код идентификатора(сущности)
    static const int INVALID_ENTITY_CODE; // 0
    //! Код идентификатора независимой сущности
    static const int INDEPENDENT_ENTITY_CODE; // 1
    //! Минимальный номер кода (для любых идентификаторов). Номера меньше этого зарезервированы под нужды ядра
    static const int MIN_ENTITY_CODE; // 500

    //! Максимальный номер кода сущности
    static const int MAX_ENTITY_CODE; // USHRT_MAX

    //! Не существующий ID сущности
    static const int INVALID_ENTITY_ID; // 0

    //! Минимально допустимый id свойства
    static const int MINUMUM_PROPERTY_ID; // 1

    //! Разделитель ключей в строках
    static const QChar KEY_SEPARATOR;

    //! Количество знаков после запятой для дробных чисел
    static const int DOUBLE_DECIMALS;
    //! Размер групп цифр для деления на классы (Пример: 1 000 000,00)
    static const int DIGITS_GROUP_SIZE;

    //! Код конфигурации ядра (для внутреннего использования)
    static const QString CORE_INTERNAL_CODE;
    //! Код конфигурации разработчика (для внутреннего использования)
    static const QString CORE_DEV_CODE;

    /*! Роль в которой хранится уникальный идентификатор строки набора данных. Для хранения выбирается колонка с
     * номером 0 */
    static const int UNIQUE_ROW_COLUMN_ROLE; // Qt::UserRole + USHRT_MAX

    //! Версия QDataStream
    static const QDataStream::Version DATASTREAM_VERSION; // QDataStream::Qt_5_14

    //! Символ для подсчета размера строки (N)
    static const QChar AVERAGE_CHAR;

    //! Разделитель составных значений при отображении пользователю. Не путать с KEY_SEPARATOR !!!
    static const QChar COMPLEX_SEPARATOR;

    //! Задержка в мс. после нажатия кнопки пользователем, после которой начинается поиск, выбор нового элемента и т.п.
    static const int USER_INPUT_TIMEOUT_MS;

    //! Минимальный номер id при вызовах HighlightInfo::insert/remove/empty/set и при прочей работе с HighlightModel
    //! id меньше чем этот, зарезервированы на нужны ядра
    static const int MINIMUM_HIGHLIGHT_ID;

    //! Кодировка при выгрузке в файл
    static const QString FILE_ENCODING;

    //! Разделитель при выводе в CSV файлы
    static const QChar CSV_SEPARATOR;
};

//! Команды модулей
class ZCORESHARED_EXPORT ModuleCommands
{
public:
    //! Загрузка из БД
    static const Command Load;
    //! Сохранение в БД
    static const Command Save;
    //! Удаление из БД
    static const Command Remove;

    //! Начальный номер для создания собственных команд
    static const int CustomCommand;
};

} // namespace zf
