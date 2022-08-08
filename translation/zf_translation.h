#ifndef ZF_TRANSLATION_H
#define ZF_TRANSLATION_H

#include <QtCore>

//! Макрос для упрощения перевода констант ядра
#define ZF_TR(_value_) zf::translate(zf::TR::_value_)
//! Макрос для упрощения перевода констант ядра со значением по умолчанию
#define ZF_TR_EX(_value_, _default_)                                                                                                       \
    (zf::Core::isBootstraped() ? zf::translate(zf::TR::_value_, (QString(_default_).isEmpty() ? true : false), _default_)                  \
                               : QString::fromUtf8(_default_))

namespace zf
{
//! Константы ядра для перевода. Сканируются с помощью lupdate
struct TR
{
    //: ZFT_OK
    //% "ОК"
    inline static const char* ZFT_OK = QT_TRID_NOOP("ZFT_OK");
    //: ZFT_SAVE
    //% "Сохранить"
    inline static const char* ZFT_SAVE = QT_TRID_NOOP("ZFT_SAVE");
    //: ZFT_DONE
    //% "Завершить"
    inline static const char* ZFT_DONE = QT_TRID_NOOP("ZFT_DONE");
    //: ZFT_SEARCH
    //% "Поиск"
    inline static const char* ZFT_SEARCH = QT_TRID_NOOP("ZFT_SEARCH");
    //: ZFT_NEXT
    //% "Далее"
    inline static const char* ZFT_NEXT = QT_TRID_NOOP("ZFT_NEXT");
    //: ZFT_PREV
    //% "Назад"
    inline static const char* ZFT_PREV = QT_TRID_NOOP("ZFT_PREV");
    //: ZFT_CLOSE
    //% "Закрыть"
    inline static const char* ZFT_CLOSE = QT_TRID_NOOP("ZFT_CLOSE");
    //: ZFT_OPEN
    //% "Открыть"
    inline static const char* ZFT_OPEN = QT_TRID_NOOP("ZFT_OPEN");
    //: ZFT_SELECT
    //% "Выбрать"
    inline static const char* ZFT_SELECT = QT_TRID_NOOP("ZFT_SELECT");
    //: ZFT_SELECTION
    //% "Поиск"
    inline static const char* ZFT_SELECTION = QT_TRID_NOOP("ZFT_SELECTION");
    //: ZFT_CANCEL
    //% "Отменить"
    inline static const char* ZFT_CANCEL = QT_TRID_NOOP("ZFT_CANCEL");
    //: ZFT_APPLY
    //% "Применить"
    inline static const char* ZFT_APPLY = QT_TRID_NOOP("ZFT_APPLY");
    //: ZFT_RESET
    //% "Сбросить"
    inline static const char* ZFT_RESET = QT_TRID_NOOP("ZFT_RESET");
    //: ZFT_HELP
    //% "Помощь"
    inline static const char* ZFT_HELP = QT_TRID_NOOP("ZFT_HELP");
    //: ZFT_DISCARD
    //% "Отказ"
    inline static const char* ZFT_DISCARD = QT_TRID_NOOP("ZFT_DISCARD");
    //: ZFT_YES
    //% "Да"
    inline static const char* ZFT_YES = QT_TRID_NOOP("ZFT_YES");
    //: ZFT_NO
    //% "Нет"
    inline static const char* ZFT_NO = QT_TRID_NOOP("ZFT_NO");
    //: ZFT_YES_ALL
    //% "Да, для всех"
    inline static const char* ZFT_YES_ALL = QT_TRID_NOOP("ZFT_YES_ALL");
    //: ZFT_NO_ALL
    //% "Нет, для всех"
    inline static const char* ZFT_NO_ALL = QT_TRID_NOOP("ZFT_NO_ALL");
    //: ZFT_SAVE_ALL
    //% "Сохранить все"
    inline static const char* ZFT_SAVE_ALL = QT_TRID_NOOP("ZFT_SAVE_ALL");
    //: ZFT_ABORT
    //% "Прервать"
    inline static const char* ZFT_ABORT = QT_TRID_NOOP("ZFT_ABORT");
    //: ZFT_RETRY
    //% "Повторить"
    inline static const char* ZFT_RETRY = QT_TRID_NOOP("ZFT_RETRY");
    //: ZFT_IGNORE
    //% "Игнорировать"
    inline static const char* ZFT_IGNORE = QT_TRID_NOOP("ZFT_IGNORE");
    //: ZFT_RESTORE_DEFAULTS
    //% "Восстановить"
    inline static const char* ZFT_RESTORE_DEFAULTS = QT_TRID_NOOP("ZFT_RESTORE_DEFAULTS");
    //: ZFT_ENTER
    //% "Войти"
    inline static const char* ZFT_ENTER = QT_TRID_NOOP("ZFT_ENTER");
    //: ZFT_CLEAR
    //% "Очистить"
    inline static const char* ZFT_CLEAR = QT_TRID_NOOP("ZFT_CLEAR");
    //: ZFT_SELECT_CALENDAR
    //% "Выбрать из календаря"
    inline static const char* ZFT_SELECT_CALENDAR = QT_TRID_NOOP("ZFT_SELECT_CALENDAR");
    //: ZFT_GOTO
    //% "Перейти к <b>%1</b>"
    inline static const char* ZFT_GOTO = QT_TRID_NOOP("ZFT_GOTO");
    //: ZFT_CLOSE_NO_SAVE
    //% "Закрыть и отменить изменения?"
    inline static const char* ZFT_CLOSE_NO_SAVE = QT_TRID_NOOP("ZFT_CLOSE_NO_SAVE");

    //: ZFT_SETTINGS_CHANGED
    //% "Изменения настроек будут применены при следующем запуске программы"
    inline static const char* ZFT_SETTINGS_CHANGED = QT_TRID_NOOP("ZFT_SETTINGS_CHANGED");
    //: ZFT_SETTINGS
    //% "Настройки программы"
    inline static const char* ZFT_SETTINGS = QT_TRID_NOOP("ZFT_SETTINGS");
    //: ZFT_CHANGELOG
    //% "История изменений"
    inline static const char* ZFT_CHANGELOG = QT_TRID_NOOP("ZFT_CHANGELOG");
    //: ZFT_HOST
    //% "Сервер"
    inline static const char* ZFT_HOST = QT_TRID_NOOP("ZFT_HOST");
    //: ZFT_LANGUAGE
    //% "Язык"
    inline static const char* ZFT_LANGUAGE = QT_TRID_NOOP("ZFT_LANGUAGE");
    //: ZFT_UI_LANGUAGE
    //% "Интерфейс"
    inline static const char* ZFT_UI_LANGUAGE = QT_TRID_NOOP("ZFT_UI_LANGUAGE");
    //: ZFT_DOC_LANGUAGE
    //% "Документооборот"
    inline static const char* ZFT_DOC_LANGUAGE = QT_TRID_NOOP("ZFT_DOC_LANGUAGE");
    //: ZFT_SETTINGS_FONT_PANGRAM
    //% "В чащах юга жил бы цитрус? Да, но фальшивый экземпляр!"
    inline static const char* ZFT_SETTINGS_FONT_PANGRAM = QT_TRID_NOOP("ZFT_SETTINGS_FONT_PANGRAM");
    //: ZFT_FONT
    //% "Шрифт"
    inline static const char* ZFT_FONT = QT_TRID_NOOP("ZFT_FONT");
    //: ZFT_FONT_FAMILY
    //% "Семейство"
    inline static const char* ZFT_FONT_FAMILY = QT_TRID_NOOP("ZFT_FONT_FAMILY");
    //: ZFT_FONT_SIZE
    //% "Размер"
    inline static const char* ZFT_FONT_SIZE = QT_TRID_NOOP("ZFT_FONT_SIZE");
    //: ZFT_SHOW_TEXT_MAIN_MENU
    //% "Показывать названия кнопок главного меню"
    inline static const char* ZFT_SHOW_TEXT_MAIN_MENU = QT_TRID_NOOP("ZFT_SHOW_TEXT_MAIN_MENU");

    //: ZFT_REPORT_OK
    //% "Файл <b>%1</b> создан успешно. Открыть для просмотра?"
    inline static const char* ZFT_REPORT_OK = QT_TRID_NOOP("ZFT_REPORT_OK");

    //: ZFT_LOGIN
    //% "Пользователь"
    inline static const char* ZFT_LOGIN = QT_TRID_NOOP("ZFT_LOGIN");
    //: ZFT_PASSWORD
    //% "Пароль"
    inline static const char* ZFT_PASSWORD = QT_TRID_NOOP("ZFT_PASSWORD");

    //: ZFT_EQUAL
    //% "Равно"
    inline static const char* ZFT_EQUAL = QT_TRID_NOOP("ZFT_EQUAL");
    //: ZFT_NOT_EQUAL
    //% "Не равно"
    inline static const char* ZFT_NOT_EQUAL = QT_TRID_NOOP("ZFT_NOT_EQUAL");
    //: ZFT_MORE
    //% "Больше"
    inline static const char* ZFT_MORE = QT_TRID_NOOP("ZFT_MORE");
    //: ZFT_MORE_EQUAL
    //% "Больше или равно"
    inline static const char* ZFT_MORE_EQUAL = QT_TRID_NOOP("ZFT_MORE_EQUAL");
    //: ZFT_LESS
    //% "Меньше"
    inline static const char* ZFT_LESS = QT_TRID_NOOP("ZFT_LESS");
    //: ZFT_LESS_EQUAL
    //% "Меньше или равно"
    inline static const char* ZFT_LESS_EQUAL = QT_TRID_NOOP("ZFT_LESS_EQUAL");
    //: ZFT_CONTAINS
    //% "Содержит"
    inline static const char* ZFT_CONTAINS = QT_TRID_NOOP("ZFT_CONTAINS");
    //: ZFT_STARTS_WITH
    //% "Начинается с"
    inline static const char* ZFT_STARTS_WITH = QT_TRID_NOOP("ZFT_STARTS_WITH");
    //: ZFT_ENDS_WITH
    //% "Заканчивается на"
    inline static const char* ZFT_ENDS_WITH = QT_TRID_NOOP("ZFT_ENDS_WITH");

    //: ZFT_SEARCH_WIDGET_PLACEHOLDER
    //% "Выберите значение"
    inline static const char* ZFT_SEARCH_WIDGET_PLACEHOLDER = QT_TRID_NOOP("ZFT_SEARCH_WIDGET_PLACEHOLDER");

    //: ZFT_BAD_VALUE
    //% "Недопустимое значение"
    inline static const char* ZFT_BAD_VALUE = QT_TRID_NOOP("ZFT_BAD_VALUE");
    //: ZFT_BAD_FIELD_VALUE
    //% "Недопустимое значение поля <b>%1</b>"
    inline static const char* ZFT_BAD_FIELD_VALUE = QT_TRID_NOOP("ZFT_BAD_FIELD_VALUE");

    //: ZFT_CONNECTION_CLOSED
    //% "Потеряно соединение с сервером"
    inline static const char* ZFT_CONNECTION_CLOSED = QT_TRID_NOOP("ZFT_CONNECTION_CLOSED");

    //: ZFT_CHANGED_BY_OTHER_USER
    //% "Данные были изменены другим пользователем"
    inline static const char* ZFT_CHANGED_BY_OTHER_USER = QT_TRID_NOOP("ZFT_CHANGED_BY_OTHER_USER");

    //: ZFT_BAD_CHECKSUM
    //% "Ошибка контрольной суммы"
    inline static const char* ZFT_BAD_CHECKSUM = QT_TRID_NOOP("ZFT_BAD_CHECKSUM");

    //: ZFT_CONDITION_AND
    //% "Выполнены все условия:"
    inline static const char* ZFT_CONDITION_AND = QT_TRID_NOOP("ZFT_CONDITION_AND");
    //: ZFT_CONDITION_OR
    //% "Выполнено любое условие:"
    inline static const char* ZFT_CONDITION_OR = QT_TRID_NOOP("ZFT_CONDITION_OR");
    //: ZFT_CONDITION_REQUIRED
    //% "Требуется значение"
    inline static const char* ZFT_CONDITION_REQUIRED = QT_TRID_NOOP("ZFT_CONDITION_REQUIRED");
    //: ZFT_CONDITION_COMPARE
    //% "Сравнение"
    inline static const char* ZFT_CONDITION_COMPARE = QT_TRID_NOOP("ZFT_CONDITION_COMPARE");

    // Диалог настройки условий фильтрации
    //: ZFT_COND_DIAL_TYPE
    //% "Вид условия"
    inline static const char* ZFT_COND_DIAL_TYPE = QT_TRID_NOOP("ZFT_COND_DIAL_TYPE");
    //: ZFT_COND_DIAL_FIELD
    //% "Колонка"
    inline static const char* ZFT_COND_DIAL_FIELD = QT_TRID_NOOP("ZFT_COND_DIAL_FIELD");
    //: ZFT_COND_DIAL_OPERATOR
    //% "Сравнение"
    inline static const char* ZFT_COND_DIAL_OPERATOR = QT_TRID_NOOP("ZFT_COND_DIAL_OPERATOR");
    //: ZFT_COND_DIAL_VALUE
    //% "Значение"
    inline static const char* ZFT_COND_DIAL_VALUE = QT_TRID_NOOP("ZFT_COND_DIAL_VALUE");
    //: ZFT_COND_DIAL_COMPARE_COLUMNS
    //% "Сравнение двух колонок"
    inline static const char* ZFT_COND_DIAL_COMPARE_COLUMNS = QT_TRID_NOOP("ZFT_COND_DIAL_COMPARE_COLUMNS");
    //: ZFT_COND_DIAL_COMPARE_VALUE
    //% "Сравнение колонки со значением"
    inline static const char* ZFT_COND_DIAL_COMPARE_VALUE = QT_TRID_NOOP("ZFT_COND_DIAL_COMPARE_VALUE");

    // Меню настройки таблиц
    //: ZFT_TMC_SHOW_HIDDEN
    //% "Показать скрытые колонки"
    inline static const char* ZFT_TMC_SHOW_HIDDEN = QT_TRID_NOOP("ZFT_TMC_SHOW_HIDDEN");
    //: ZFT_TMC_SETUP_COLUMNS
    //% "Настроить колонки..."
    inline static const char* ZFT_TMC_SETUP_COLUMNS = QT_TRID_NOOP("ZFT_TMC_SETUP_COLUMNS");
    //: ZFT_TMC_RESET_COLUMNS
    //% "Восстановить порядок колонок"
    inline static const char* ZFT_TMC_RESET_COLUMNS = QT_TRID_NOOP("ZFT_TMC_RESET_COLUMNS");
    //: ZFT_TMC_SETUP_CONDITIONS
    //% "Настроить фильтр..."
    inline static const char* ZFT_TMC_SETUP_CONDITIONS = QT_TRID_NOOP("ZFT_TMC_SETUP_CONDITIONS");
    //: ZFT_TMC_CLEAR_CONDITIONS
    //% "Очистить фильтр"
    inline static const char* ZFT_TMC_CLEAR_CONDITIONS = QT_TRID_NOOP("ZFT_TMC_CLEAR_CONDITIONS");
    //: ZFT_TMC_VISIBILITY
    //% "Видимость"
    inline static const char* ZFT_TMC_VISIBILITY = QT_TRID_NOOP("ZFT_TMC_VISIBILITY");
    //: ZFT_TMC_EXPORT_TABLE_EXCEL
    //% "Выгрузить таблицу в Excel..."
    inline static const char* ZFT_TMC_EXPORT_TABLE_EXCEL = QT_TRID_NOOP("ZFT_TMC_EXPORT_TABLE_EXCEL");
    //: ZFT_TMC_EXPORT_TABLE_EXCEL_1
    //% "Выгрузка в Excel"
    inline static const char* ZFT_TMC_EXPORT_TABLE_EXCEL_1 = QT_TRID_NOOP("ZFT_TMC_EXPORT_TABLE_EXCEL_1");
    //: ZFT_TMC_EXPORTING_TABLE_EXCEL
    //% "Выгрузка в Excel, пожалуйста подождите..."
    inline static const char* ZFT_TMC_EXPORTING_TABLE_EXCEL = QT_TRID_NOOP("ZFT_TMC_EXPORTING_TABLE_EXCEL");

    //: ZFT_TABLE
    //% "Таблица"
    inline static const char* ZFT_TABLE = QT_TRID_NOOP("ZFT_TABLE");
    //: ZFT_RELOAD
    //% "Обновить"
    inline static const char* ZFT_RELOAD = QT_TRID_NOOP("ZFT_RELOAD");
    //: ZFT_EDIT
    //% "Изменить"
    inline static const char* ZFT_EDIT = QT_TRID_NOOP("ZFT_EDIT");
    //: ZFT_REMOVE
    //% "Удалить"
    inline static const char* ZFT_REMOVE = QT_TRID_NOOP("ZFT_REMOVE");
    //: ZFT_ADD_ROW
    //% "Добавить строку"
    inline static const char* ZFT_ADD_ROW = QT_TRID_NOOP("ZFT_ADD_ROW");
    //: ZFT_ADD_ROW_LEVEL
    //% "Добавить подчиненную строку"
    inline static const char* ZFT_ADD_ROW_LEVEL = QT_TRID_NOOP("ZFT_ADD_ROW_LEVEL");
    //: ZFT_EDIT_ROW
    //% "Изменить"
    inline static const char* ZFT_EDIT_ROW = QT_TRID_NOOP("ZFT_EDIT_ROW");
    //: ZFT_VIEW_ROW
    //% "Открыть"
    inline static const char* ZFT_VIEW_ROW = QT_TRID_NOOP("ZFT_VIEW_ROW");
    //: ZFT_REMOVE_ROW
    //% "Удалить строку"
    inline static const char* ZFT_REMOVE_ROW = QT_TRID_NOOP("ZFT_REMOVE_ROW");
    //: ZFT_REMOVE_PAGE
    //% "Удалить страницу"
    inline static const char* ZFT_REMOVE_PAGE = QT_TRID_NOOP("ZFT_REMOVE_PAGE");

    //: ZFT_FILTER
    //% "Фильтрация"
    inline static const char* ZFT_FILTER = QT_TRID_NOOP("ZFT_FILTER");
    //: ZFT_LOADING
    //% "Загрузка..."
    inline static const char* ZFT_LOADING = QT_TRID_NOOP("ZFT_LOADING");
    //: ZFT_SAVING
    //% "Сохранение..."
    inline static const char* ZFT_SAVING = QT_TRID_NOOP("ZFT_SAVING");
    //: ZFT_COPY
    //% "Копировать"
    inline static const char* ZFT_COPY = QT_TRID_NOOP("ZFT_COPY");
    //: ZFT_CUT
    //% "Вырезать"
    inline static const char* ZFT_CUT = QT_TRID_NOOP("ZFT_CUT");
    //: ZFT_PASTE
    //% "Вставить"
    inline static const char* ZFT_PASTE = QT_TRID_NOOP("ZFT_PASTE");
    //: ZFT_SELECT_ALL
    //% "Выделить все"
    inline static const char* ZFT_SELECT_ALL = QT_TRID_NOOP("ZFT_SELECT_ALL");
    //: ZFT_STOP
    //% "Остановить"
    inline static const char* ZFT_STOP = QT_TRID_NOOP("ZFT_STOP");

    //: ZFT_OPEN_IMAGE
    //% "Выбрать изображение"
    inline static const char* ZFT_OPEN_IMAGE = QT_TRID_NOOP("ZFT_OPEN_IMAGE");
    //: ZFT_CLEAR_IMAGE
    //% "Очистить изображение"
    inline static const char* ZFT_CLEAR_IMAGE = QT_TRID_NOOP("ZFT_CLEAR_IMAGE");
    //: ZFT_PASTE_IMAGE
    //% "Вставить изображение"
    inline static const char* ZFT_PASTE_IMAGE = QT_TRID_NOOP("ZFT_PASTE_IMAGE");
    //: ZFT_IMAGE_FILES
    //% "Файлы изображений (*.jpg *.png)"
    inline static const char* ZFT_IMAGE_FILES = QT_TRID_NOOP("ZFT_IMAGE_FILES");
    //: ZFT_WRONG_IMAGE_FORMAT
    //% "Неверный формат изображения"
    inline static const char* ZFT_WRONG_IMAGE_FORMAT = QT_TRID_NOOP("ZFT_WRONG_IMAGE_FORMAT");

    //: ZFT_QUESTION
    //% "Вопрос"
    inline static const char* ZFT_QUESTION = QT_TRID_NOOP("ZFT_QUESTION");
    //: ZFT_CHOICE
    //% "Выбор"
    inline static const char* ZFT_CHOICE = QT_TRID_NOOP("ZFT_CHOICE");
    //: ZFT_UNKNOWN_ERROR
    //% "Неизвестная ошибка"
    inline static const char* ZFT_UNKNOWN_ERROR = QT_TRID_NOOP("ZFT_UNKNOWN_ERROR");
    //: ZFT_NO_MEMORY
    //% "Недостаточно оперативной памяти"
    inline static const char* ZFT_NO_MEMORY = QT_TRID_NOOP("ZFT_NO_MEMORY");
    //: ZFT_DATA_PROCESSING
    //% "Пожалуйста подождите..."
    inline static const char* ZFT_DATA_PROCESSING = QT_TRID_NOOP("ZFT_DATA_PROCESSING");

    //: ZFT_EXECUTE_QUESTION
    //% "Выполнить <b>%1</b>?"
    inline static const char* ZFT_EXECUTE_QUESTION = QT_TRID_NOOP("ZFT_EXECUTE_QUESTION");

    //: ZFT_MODULE_NOT_FOUND
    //% "Не найден модуль <b>%1<b>"
    inline static const char* ZFT_MODULE_NOT_FOUND = QT_TRID_NOOP("ZFT_MODULE_NOT_FOUND");
    //: ZFT_MODULE_LOAD_ERROR
    //% "Ошибка загрузки модуля <b>%1<b>"
    inline static const char* ZFT_MODULE_LOAD_ERROR = QT_TRID_NOOP("ZFT_MODULE_LOAD_ERROR");
    //: ZFT_NOT_DEFINED
    //% "Не задано <b>%1</b>"
    inline static const char* ZFT_NOT_DEFINED = QT_TRID_NOOP("ZFT_NOT_DEFINED");
    //: ZFT_NOT_DEFINED_VALUE
    //% "Не задано значение"
    inline static const char* ZFT_NOT_DEFINED_VALUE = QT_TRID_NOOP("ZFT_NOT_DEFINED_VALUE");
    //! Не заполнены обязательные поля
    //: ZFT_EMPTY_REQUIRED_FIELDS
    //% "Не заполнены обязательные поля"
    inline static const char* ZFT_EMPTY_REQUIRED_FIELDS = QT_TRID_NOOP("ZFT_EMPTY_REQUIRED_FIELDS");
    //: ZFT_NO_ACCESS_RIGHTS
    //% "Нет прав доступа к форме <b>%1<b>"
    inline static const char* ZFT_NO_ACCESS_RIGHTS = QT_TRID_NOOP("ZFT_NO_ACCESS_RIGHTS");

    //: ZFT_FIELD_EXCEEDED_MAX_LENGTH
    //% "Превышено максимально допустимое количество символов <b>%2</b> для поля <b>%1</b>"
    inline static const char* ZFT_FIELD_EXCEEDED_MAX_LENGTH = QT_TRID_NOOP("ZFT_FIELD_EXCEEDED_MAX_LENGTH");
    //: ZFT_EXCEEDED_MAX_LENGTH
    //% "Превышено допустимое количество символов"
    inline static const char* ZFT_EXCEEDED_MAX_LENGTH = QT_TRID_NOOP("ZFT_EXCEEDED_MAX_LENGTH");
    //: ZFT_BAD_LOOKUP_VALUE
    //% "Недопустимое значение: <b>%1</b>"
    inline static const char* ZFT_BAD_LOOKUP_VALUE = QT_TRID_NOOP("ZFT_BAD_LOOKUP_VALUE");

    //: ZFT_ASK_APP_QUIT
    //% "Завершить работу?"
    inline static const char* ZFT_ASK_APP_QUIT = QT_TRID_NOOP("ZFT_ASK_APP_QUIT");

    //: ZFT_DATABASE_DRIVER_NOT_FOUND
    //% "Не найден драйвер базы данных <b>%1<b>"
    inline static const char* ZFT_DATABASE_DRIVER_NOT_FOUND = QT_TRID_NOOP("ZFT_DATABASE_DRIVER_NOT_FOUND");
    //: ZFT_DATABASE_DRIVER_LOAD_ERROR
    //% "Ошибка загрузки драйвера базы данных <b>%1<b>"
    inline static const char* ZFT_DATABASE_DRIVER_LOAD_ERROR = QT_TRID_NOOP("ZFT_DATABASE_DRIVER_LOAD_ERROR");

    //! Виды информации - Успех
    //: ZFT_SUCCESS
    //% "Успех"
    inline static const char* ZFT_SUCCESS = QT_TRID_NOOP("ZFT_SUCCESS");
    //! Виды информации - Ошибка
    //: ZFT_ERROR
    //% "Ошибка"
    inline static const char* ZFT_ERROR = QT_TRID_NOOP("ZFT_ERROR");
    //! Виды информации - Критическая ошибка
    //: ZFT_CRITICAL_ERROR
    //% "Критическая ошибка"
    inline static const char* ZFT_CRITICAL_ERROR = QT_TRID_NOOP("ZFT_CRITICAL_ERROR");
    //! Виды информации - Фатальная ошибка
    //: ZFT_FATAL_ERROR
    //% "Фатальная ошибка"
    inline static const char* ZFT_FATAL_ERROR = QT_TRID_NOOP("ZFT_FATAL_ERROR");
    //! Виды информации - Предупреждение
    //: ZFT_WARNING
    //% "Предупреждение"
    inline static const char* ZFT_WARNING = QT_TRID_NOOP("ZFT_WARNING");
    //! Виды информации - Информация
    //: ZFT_INFORMATION
    //% "Информация"
    inline static const char* ZFT_INFORMATION = QT_TRID_NOOP("ZFT_INFORMATION");
    //! Виды информации - Требуется
    //: ZFT_REQUIRED
    //% "Требуется"
    inline static const char* ZFT_REQUIRED = QT_TRID_NOOP("ZFT_REQUIRED");

    //! Логический признак - ИСТИНА
    //: ZFT_TRUE
    //% "Истина"
    inline static const char* ZFT_TRUE = QT_TRID_NOOP("ZFT_TRUE");
    //! Логический признак - ЧАСТИЧНО
    //: ZFT_PARTIAL
    //% "Частично"
    inline static const char* ZFT_PARTIAL = QT_TRID_NOOP("ZFT_PARTIAL");
    //! Логический признак - ЛОЖЬ
    //: ZFT_FALSE
    //% "Ложь"
    inline static const char* ZFT_FALSE = QT_TRID_NOOP("ZFT_FALSE");

    //! Неверная версия протокола
    //: ZFT_WRONG_PROTOCOL_VERSION
    //% "Неверная версия протокола, обновите программное обеспечение"
    inline static const char* ZFT_WRONG_PROTOCOL_VERSION = QT_TRID_NOOP("ZFT_WRONG_PROTOCOL_VERSION");
    //! Неверные данные
    //: ZFT_WRONG_DATA
    //% "Неверные данные"
    inline static const char* ZFT_WRONG_DATA = QT_TRID_NOOP("ZFT_WRONG_DATA");
    //! Не найдено значение для расшифровки lookup list
    //: ZFT_LOOKUP_LIST_NOT_FOUND
    //% "%1 (Значение не найдено)"
    inline static const char* ZFT_LOOKUP_LIST_NOT_FOUND = QT_TRID_NOOP("ZFT_LOOKUP_LIST_NOT_FOUND");
    //! Имеются не сохраненные изменения
    //: ZFT_HAS_UNSAVED_DATA
    //% "Имеются не сохраненные изменения"
    inline static const char* ZFT_HAS_UNSAVED_DATA = QT_TRID_NOOP("ZFT_HAS_UNSAVED_DATA");
    //! Сущность не найдена
    //: ZFT_ENTITY_NOT_FOUND
    //% "Объект не существует в базе данных"
    inline static const char* ZFT_ENTITY_NOT_FOUND = QT_TRID_NOOP("ZFT_ENTITY_NOT_FOUND");
    //! Журнал ошибок
    //: ZFT_ERROR_LOG
    //% "Журнал ошибок"
    inline static const char* ZFT_ERROR_LOG = QT_TRID_NOOP("ZFT_ERROR_LOG");

    //! Файл не найден
    //: ZFT_FILE_NOT_FOUND
    //% "Файл <b>%1</b> не найден"
    inline static const char* ZFT_FILE_NOT_FOUND = QT_TRID_NOOP("ZFT_FILE_NOT_FOUND");
    //! Ошибка открытия файла
    //: ZFT_BAD_FILE
    //% "Ошибка открытия файла <b>%1</b>"
    inline static const char* ZFT_BAD_FILE = QT_TRID_NOOP("ZFT_BAD_FILE");
    //! Неизвестный тип файла
    //: ZFT_BAD_FILE_TYPE
    //% "Неизвестный тип файла"
    inline static const char* ZFT_BAD_FILE_TYPE = QT_TRID_NOOP("ZFT_BAD_FILE_TYPE");
    //! Ошибка записи/чтения файла
    //: ZFT_FILE_IO_ERROR
    //% "Ошибка записи/чтения файла <b>%1</b>"
    inline static const char* ZFT_FILE_IO_ERROR = QT_TRID_NOOP("ZFT_FILE_IO_ERROR");
    //: ZFT_FILE_SAVE_ERROR
    //% "Ошибка сохранения файла <b>%1</b>"
    inline static const char* ZFT_FILE_SAVE_ERROR = QT_TRID_NOOP("ZFT_FILE_SAVE_ERROR");
    //! Ошибка копирования файла
    //: ZFT_FILE_COPY_ERROR
    //% "Ошибка копирования файла <b>%1</b> в <b>%2</b>"
    inline static const char* ZFT_FILE_COPY_ERROR = QT_TRID_NOOP("ZFT_FILE_COPY_ERROR");
    //! Ошибка удаления файла
    //: ZFT_FILE_REMOVE_ERROR
    //% "Ошибка удаления файла <b>%1</b>"
    inline static const char* ZFT_FILE_REMOVE_ERROR = QT_TRID_NOOP("ZFT_FILE_REMOVE_ERROR");
    //! "Ошибка переименования файла
    //: ZFT_FILE_RENAME_ERROR
    //% "Ошибка переименования файла <b>%1</b> в <b>%2</b>"
    inline static const char* ZFT_FILE_RENAME_ERROR = QT_TRID_NOOP("ZFT_FILE_RENAME_ERROR");
    //! Ошибка создания папки
    //: ZFT_FOLDER_CREATE_ERROR
    //% "Ошибка создания папки <b>%1</b>"
    inline static const char* ZFT_FOLDER_CREATE_ERROR = QT_TRID_NOOP("ZFT_FOLDER_CREATE_ERROR");
    //! Ошибка удаления папки
    //: ZFT_FOLDER_REMOVE_ERROR
    //% "Ошибка удаления папки <b>%1</b>"
    inline static const char* ZFT_FOLDER_REMOVE_ERROR = QT_TRID_NOOP("ZFT_FOLDER_REMOVE_ERROR");
    //! Ошибка - превышено время ожидания
    //: ZFT_TIMEOUT_ERROR
    //% "Превышено время ожидания"
    inline static const char* ZFT_TIMEOUT_ERROR = QT_TRID_NOOP("ZFT_TIMEOUT_ERROR");
    //! Ошибка - не поддерживается
    //: ZFT_UNSUPPORTED_ERROR
    //% "Операция не поддерживается"
    inline static const char* ZFT_UNSUPPORTED_ERROR = QT_TRID_NOOP("ZFT_UNSUPPORTED_ERROR");
    //! Ошибка в JSON документе
    //: ZFT_JSON_ERROR
    //% "Некорректное содержимое JSON"
    inline static const char* ZFT_JSON_ERROR = QT_TRID_NOOP("ZFT_JSON_ERROR");
    //! Ошибка - испорченные данные
    //: ZFT_CORRUPTED_ERROR
    //% "Некорректное содержимое"
    inline static const char* ZFT_CORRUPTED_ERROR = QT_TRID_NOOP("ZFT_CORRUPTED_ERROR");
    //! Ошибка - запрещено
    //: ZFT_FORBIDDEN_ERROR
    //% "Запрещено"
    inline static const char* ZFT_FORBIDDEN_ERROR = QT_TRID_NOOP("ZFT_FORBIDDEN_ERROR");
    //! Ошибка - не найдено
    //: ZFT_NOT_FOUND_ERROR
    //% "Не найдено"
    inline static const char* ZFT_NOT_FOUND_ERROR = QT_TRID_NOOP("ZFT_NOT_FOUND_ERROR");

    //: ZFT_FILE_SAVED
    //% "Файл сохранен в <b>%1</b>"
    inline static const char* ZFT_FILE_SAVED = QT_TRID_NOOP("ZFT_FILE_SAVED");

    //! Выберите файлы
    //: ZFT_OPEN_FILES
    //% "Выберите файлы"
    inline static const char* ZFT_OPEN_FILES = QT_TRID_NOOP("ZFT_OPEN_FILES");

    //: ZFT_CATEGORY
    //% "Категория"
    inline static const char* ZFT_CATEGORY = QT_TRID_NOOP("ZFT_CATEGORY");
    //: ZFT_MESSAGE
    //% "Сообщение"
    inline static const char* ZFT_MESSAGE = QT_TRID_NOOP("ZFT_MESSAGE");
    //: ZFT_DETAIL
    //% "Подробности"
    inline static const char* ZFT_DETAIL = QT_TRID_NOOP("ZFT_DETAIL");
    //: ZFT_STATUS
    //% "Статус"
    inline static const char* ZFT_STATUS = QT_TRID_NOOP("ZFT_STATUS");

    //: ZFT_OPEN_DOCUMENT
    //% "Открыть документ"
    inline static const char* ZFT_OPEN_DOCUMENT = QT_TRID_NOOP("ZFT_OPEN_DOCUMENT");

    //: ZFT_FILE_COPY
    //% "Копирование файлов"
    inline static const char* ZFT_FILE_COPY = QT_TRID_NOOP("ZFT_FILE_COPY");
    //: ZFT_FOLDER_NOT_EXISTS
    //% "Папка <b>%1</b> не существует"
    inline static const char* ZFT_FOLDER_NOT_EXISTS = QT_TRID_NOOP("ZFT_FOLDER_NOT_EXISTS");
    //: ZFT_FOLDER_ALREADY_EXISTS
    //% "Папка <b>%1</b> уже существует"
    inline static const char* ZFT_FOLDER_ALREADY_EXISTS = QT_TRID_NOOP("ZFT_FOLDER_ALREADY_EXISTS");

    //: ZFT_ROW_ALREADY_EXISTS
    //% "Строка с такими значениями <b>уже существует</b>"
    inline static const char* ZFT_ROW_ALREADY_EXISTS = QT_TRID_NOOP("ZFT_ROW_ALREADY_EXISTS");

    //*************** Диалог настройки колонок таблиц
    //: ZFT_COLDLG_NAME
    //% "Настройка колонок таблицы"
    inline static const char* ZFT_COLDLG_NAME = QT_TRID_NOOP("ZFT_COLDLG_NAME");
    //: ZFT_COLDLG_COLUMNS
    //% "Колонки таблицы"
    inline static const char* ZFT_COLDLG_COLUMNS = QT_TRID_NOOP("ZFT_COLDLG_COLUMNS");
    //: ZFT_COLDLG_VISABILITY
    //% "Видимость"
    inline static const char* ZFT_COLDLG_VISABILITY = QT_TRID_NOOP("ZFT_COLDLG_VISABILITY");
    //: ZFT_COLDLG_VISABILITY_TOOLTIP
    //% "Видимость заголовка таблицы"
    inline static const char* ZFT_COLDLG_VISABILITY_TOOLTIP = QT_TRID_NOOP("ZFT_COLDLG_VISABILITY_TOOLTIP");
    //: ZFT_COLDLG_FIX
    //% "Фиксация"
    inline static const char* ZFT_COLDLG_FIX = QT_TRID_NOOP("ZFT_COLDLG_FIX");
    //: ZFT_COLDLG_FIX_TOOLTIP
    //% "Фиксация заголовка в левой части таблицы"
    inline static const char* ZFT_COLDLG_FIX_TOOLTIP = QT_TRID_NOOP("ZFT_COLDLG_FIX_TOOLTIP");
    //: ZFT_COLDLG_BEGIN
    //% "Переместить в начало"
    inline static const char* ZFT_COLDLG_BEGIN = QT_TRID_NOOP("ZFT_COLDLG_BEGIN");
    //: ZFT_COLDLG_END
    //% "Переместить в конец"
    inline static const char* ZFT_COLDLG_END = QT_TRID_NOOP("ZFT_COLDLG_END");
    //: ZFT_COLDLG_UP
    //% "Переместить выше"
    inline static const char* ZFT_COLDLG_UP = QT_TRID_NOOP("ZFT_COLDLG_UP");
    //: ZFT_COLDLG_DOWN
    //% "Переместить ниже"
    inline static const char* ZFT_COLDLG_DOWN = QT_TRID_NOOP("ZFT_COLDLG_DOWN");

    //*************** Главное окно
    //: ZFT_ABOUT (меню "О программе" главного окна)
    //% "О программе..."
    inline static const char* ZFT_ABOUT = QT_TRID_NOOP("ZFT_ABOUT");
    //: ZFT_ALL_WINDOWS
    //% "Список всех окон"
    inline static const char* ZFT_ALL_WINDOWS = QT_TRID_NOOP("ZFT_ALL_WINDOWS");
    //: ZFT_CLOSE_ALL_WINDOWS
    //% "Закрыть все окна"
    inline static const char* ZFT_CLOSE_ALL_WINDOWS = QT_TRID_NOOP("ZFT_CLOSE_ALL_WINDOWS");
    //: ZFT_TILE_WINDOWS
    //% "Расположить рядом"
    inline static const char* ZFT_TILE_WINDOWS = QT_TRID_NOOP("ZFT_TILE_WINDOWS");
    //: ZFT_CASCADE_WINDOWS
    //% "Расположить друг за другом"
    inline static const char* ZFT_CASCADE_WINDOWS = QT_TRID_NOOP("ZFT_CASCADE_WINDOWS");
    //: ZFT_MINIMIZE_WINDOWS
    //% "Свернуть\nокна"
    inline static const char* ZFT_MINIMIZE_WINDOWS = QT_TRID_NOOP("ZFT_MINIMIZE_WINDOWS");
    //: ZFT_RESTORE_WINDOWS
    //% "Восстановить\nокна"
    inline static const char* ZFT_RESTORE_WINDOWS = QT_TRID_NOOP("ZFT_RESTORE_WINDOWS");
    //: ZFT_WORKPLACE
    //% "Рабочий\nстол"
    inline static const char* ZFT_WORKPLACE = QT_TRID_NOOP("ZFT_WORKPLACE");

    //: ZFT_WINDOWS
    //% "Операции\nс окнами"
    inline static const char* ZFT_WINDOWS = QT_TRID_NOOP("ZFT_WINDOWS");
    //: ZFT_MISC
    //% "Настройки\nпрограммы"
    inline static const char* ZFT_MISC = QT_TRID_NOOP("ZFT_MISC");
    //: ZFT_MISC_EXT
    //% "Дополнительные возможности программы"
    inline static const char* ZFT_MISC_EXT = QT_TRID_NOOP("ZFT_MISC_EXT");
    //: ZFT_QUIT
    //% "Завершить работу..."
    inline static const char* ZFT_QUIT = QT_TRID_NOOP("ZFT_QUIT");

    //: _UI_ZFT_REMOVED_FROM_DB
    //% "Удален из базы данных"
    inline static const char* UI_ZFT_REMOVED_FROM_DB = QT_TRID_NOOP("_UI_ZFT_REMOVED_FROM_DB");

    //: ZFT_DOCPREVIEW
    //% "Просмотр файла"
    inline static const char* ZFT_DOCPREVIEW = QT_TRID_NOOP("ZFT_DOCPREVIEW");

    //: ZFT_DIALOG_DOCPREVIEW_TITLE
    //% "Просмотр файла"
    inline static const char* ZFT_DIALOG_DOCPREVIEW_TITLE = QT_TRID_NOOP("ZFT_DIALOG_DOCPREVIEW_TITLE");

    //: ZFT_SCRIPT_PLAYER_SAVE_STATE
    //% "Сохранить состояние и закрыть окно?"
    inline static const char* ZFT_SCRIPT_PLAYER_SAVE_STATE = QT_TRID_NOOP("ZFT_SCRIPT_PLAYER_SAVE_STATE");
    //: ZFT_SCRIPT_PLAYER_DONE
    //% "Завершить диалог и закрыть окно?"
    inline static const char* ZFT_SCRIPT_PLAYER_DONE = QT_TRID_NOOP("ZFT_SCRIPT_PLAYER_DONE");
    //: ZFT_SCRIPT_PLAYER_CANCEL
    //% "Отменить все изменения и закрыть окно?"
    inline static const char* ZFT_SCRIPT_PLAYER_CANCEL = QT_TRID_NOOP("ZFT_SCRIPT_PLAYER_CANCEL");

    //: ZFT_REVISION
    //% "Ревизия"
    inline static const char* ZFT_REVISION = QT_TRID_NOOP("ZFT_REVISION");
    //: ZFT_REVISION
    //% "Версия"
    inline static const char* ZFT_VERSION = QT_TRID_NOOP("ZFT_VERSION");
    //: ZFT_PROXY_SERVER
    //% "Прокси-сервер"
    inline static const char* ZFT_PROXY_SERVER = QT_TRID_NOOP("ZFT_PROXY_SERVER");

    //: ZFT_ID
    //% "ID"
    inline static const char* ZFT_ID = QT_TRID_NOOP("ZFT_ID");
    //: ZFT_CREATE
    //% "Создание"
    inline static const char* ZFT_CREATE = QT_TRID_NOOP("ZFT_CREATE");

    //: ZFT_SAVING_DOC
    //% "Сохранение документа, пожалуйста подождите..."
    inline static const char* ZFT_SAVING_DOC = QT_TRID_NOOP("ZFT_SAVING_DOC");
    //: ZFT_DOWNLOADING_DOC
    //% "Загрузка документа, пожалуйста подождите..."
    inline static const char* ZFT_DOWNLOADING_DOC = QT_TRID_NOOP("ZFT_DOWNLOADING_DOC");
    //: ZFT_DOWNLOAD_FILE
    //% "Загрузить файл"
    inline static const char* ZFT_DOWNLOAD_FILE = QT_TRID_NOOP("ZFT_DOWNLOAD_FILE");
    //: ZFT_DOWNLOAD_DOCUMENT
    //% "Загрузить документ"
    inline static const char* ZFT_DOWNLOAD_DOCUMENT = QT_TRID_NOOP("ZFT_DOWNLOAD_DOCUMENT");

    //: ZFT_XLSX_SAVE_FORMAT
    //% "Файлы Excel (*.xlsx);;"
    inline static const char* ZFT_XLSX_SAVE_FORMAT = QT_TRID_NOOP("ZFT_XLSX_SAVE_FORMAT");

    //: ZFT_AGE
    //% "лет"
    inline static const char* ZFT_AGE = QT_TRID_NOOP("ZFT_AGE");
    //: ZFT_AGE_ZERO
    //% "до года"
    inline static const char* ZFT_AGE_ZERO = QT_TRID_NOOP("ZFT_AGE_ZERO");
};

} // namespace zf

#endif // ZF_TRANSLATION_H
