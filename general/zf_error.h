#pragma once

#include "zf_global.h"
#include "zf_basic_types.h"
#include <QList>
#include <QVariant>
#include <QSharedDataPointer>

namespace zf
{
class Error_data;
class Log;
class Error;
class Uid;

typedef QList<Error> ErrorList;

class ZCORESHARED_EXPORT Error
{
public:
    Error();
    //! Копирующий конструктор
    Error(const Error& e);
    //! Полный конструктор
    explicit Error(ErrorType type, int code, const QString& text, const QString& detail = "");
    //! Пользовательская ошибка без указания кода
    explicit Error(const QString& text, const QString& detail = "");
    explicit Error(const char* text);
    //! Пользовательская ошибка с указанием кода
    explicit Error(int code, const QString& text, const QString& detail = "");

    //! Ошибка на основе списка ошибок
    explicit Error(const QList<Error>& errors);
    explicit Error(int code, const QList<Error>& errors);
    //! Ошибка на основе журнала
    explicit Error(const Log& log);
    explicit Error(int code, const Log& log);

    ~Error();

    Error& operator=(const Error& e);

    //! Текст с отладочной информацией
    QString debString() const;
    //! Вывод отладочной информации в файл
    Error debPrint() const;

    //! Сбросить
    Error& clear();

    //! Вид ошибки
    ErrorType type() const;
    //! Смена вида ошибки (использовать аккуратно, а лучше вообще не использовать!)
    Error& setType(ErrorType type);

    //! Есть ли ошибка
    bool isError() const;
    //! Нет ошибки
    bool isOk() const;

    //! Текст ощибки
    QString text() const;
    //! Задать новый текст для существующей ошибки
    Error& setText(const QString& text);
    //! Подробности
    QString textDetail() const;
    //! Задать новые подробности для существующей ошибки
    Error& setTextDetail(const QString& detail);
    //! Сформировать полный текст ошибки с учетом основного и дочерних
    QString fullText(
            //! Ограничение на макс. количество символов в возвращаемом тексте
            int limit = 0,
            //! Разделитель ошибок в виде переноса строки <br>
            bool is_html = false) const;
    //! Сформировать detail текст ошибки с учетом дочерних ошибок
    QString textDetailFull(
            //! Разделитель ошибок в виде переноса строки <br>
            bool is_html = false) const;
    //! Сформировать текст ошибки из всех дочерних ошибок
    QString childErrorText(
        //! Разделитель ошибок в виде переноса строки <br>
        bool is_html = false) const;

    //! Преобразует fullText в qPrintable (для использования в QtTest)
    const char* pritable() const;

    //! Допинформация о некритической ошибке. Это просто возможность привязать к объекту ошибка другую ошибку
    //! Не влияет на isError и прочее
    Error nonCriticalError() const;
    Error&  setNonCriticalError(const zf::Error& err);

    //! Код ошибки
    int code() const;
    //! Заменить код ошибки
    Error& setCode(int code);

    //! Произвольные данные
    const QVariant& data() const;
    Error& setData(const QVariant& v);

    //! Служебная информация
    QString extendedInfo() const;
    Error& setExtendedInfo(const QString& text);

    //! Признак "Главная". Используется для отображения ошибок в ZCore::error
    bool isMain() const;
    Error& setMain(bool is_main = true);

    //! Признак "Скрытая". При отображения ошибок в ZCore::error, диалог показан не будет
    bool isHidden() const;
    Error& setHidden(bool is_hidden = true);

    //! Тип информации
    InformationType informationType() const;
    Error& setInformationType(InformationType t);

    //! Признак "Обширное поле". Используется для отображения ошибок в виде QPlainTextEdit вместо QLineEdit
    bool isNote() const;
    Error& setNote(bool is_note = true);

    //! Связанные с ошибкой идентификаторы
    QList<Uid> uids() const;
    Error& setUids(const QList<Uid>& uids);

    /*! Признак необходимости закрытия окна при ошибке
     * Используется для отображения ошибок в модальном окне редактирования
     * По умолчанию false */
    bool isCloseWindow() const;
    Error& setCloseWindow(bool b = true);

    /*! Признак отображения ошибки в модальном окне. По умолчанию true */
    bool isModalWindow() const;
    Error& setModalWindow(bool b = true);

    //! Признак необходимости перезагрузки данных
    bool needReload() const;
    Error& setNeedReload(bool b = true);

    //! Если истина, то методы показа ошибки, вместо отображения на экране, выводят ее в файлы, которые имеют порядковую нумерацию
    bool isFileDebug() const;
    //! Название файла при выводе в файл
    QString fileDebugModifier() const;
    Error& setFileDebug(bool is_file_debug, const QString& modifier = "error");

    //! Получить ошибку с признаком "Главная" из всех, включая дочерних
    Error extractMainError() const;
    //! Получить все ошибки с признаком "Не главная" из всех, включая дочерних
    Error extractNotMainError() const;

    //! Получить основную ошибку без дочерних
    Error extractParentError() const;

    //! Преобразовать в список текстовых ошибок
    QStringList toStringList() const;

    //! Преобразовать в QVariant
    QVariant variant() const;
    //! Создать из QVariant
    static Error fromVariant(const QVariant& v);
    //! Яаляется ли QVariant ZError
    static bool isErrorVariant(const QVariant& v);

    //! Создает список ошибок с учетом дочерних
    ErrorList toList() const;
    //! Создать одну составную ошибку из списка ошибок
    static Error fromList(const ErrorList& e,
                          //! Ищет в списке ошибок первую с признаком isMain и делает ее главной. Иначе делает главной первую в списке
                          bool auto_main = false);

    //! Дочерние ошибки
    const ErrorList& childErrors() const;
    //! Дочерние ошибки определенного типа. Включает проверку самого себя
    ErrorList childErrors(zf::ErrorType t) const;
    //! Есть ли дочерние ошибки определенного типа. Включает проверку самого себя
    bool containsChildErrors(zf::ErrorType t) const;
    //! Общее количество ошибок, включая саму ошибку и дочерние (не рекурсивно)
    int count() const;

    Error& addChildError(const Error& error);
    Error& addChildError(const ErrorList& errors);
    Error& addChildError(const QString& error);
    Error& addChildError(const char* error);
    Error& operator<<(const Error& e);
    Error& operator<<(const QString& e);
    Error& operator<<(const char* e);
    Error& operator<<(const ErrorList& errors);
    Error& operator+=(const Error& e);
    Error& operator+=(const QString& e);
    Error& operator+=(const char* e);
    Error& operator+=(const ErrorList& errors);
    Error operator+(const Error& e) const;
    Error operator+(const ErrorList& e) const;

    //! Убрать дочерние ошибки
    Error& removeChildErrors();
    //! Убрать дочерние ошибки
    Error withoutChildErrors() const;

    //! Объединяет одинаковые дочерние ошибки
    Error compressChildErrors() const;

    bool operator==(const Error& e) const;

    //! Создать отдельную копию объекта (только если несколько объектов ссылаются на общие данные)
    void detach();

    //! Скрытая ошибка. Не должна отображать на экране, т.к. отображение будет проводиться иначе (например по
    //! сигналу)
    bool isHiddenError() const;
    //! Общая системная ошибка. Сгенерирована ядром без привязки к конкретному действию
    bool isCoreError() const;
    //! Системная ошибка. Не приводит к остановке программы, но требует вмешательства разработчика
    bool isSystemError() const;
    //! Это не ошибка, а просто указание на отмену операции. Никакой информации пользователю не выводится
    bool isCancelError() const;
    //! Синтаксическая ошибка
    bool isSyntaxError() const;
    //! Требуется подтверждение от пользователя
    bool isNeedConfirmationError() const;
    //! Приложение в состоянии критической ошибки и будет закрыто
    bool isApplicationHaltedError() const;

    //! Ошибка загрузки драйвера БД
    bool isDatabaseDriverError() const;
    //! Ошибка загрузки модуля в целом
    bool isModuleError() const;
    //! Не найдено представление
    bool isViewNotFoundError() const;
    //! Плагин не найден
    bool isModuleNotFoundError() const;
    //! Ошибка с указанием (открытием) документа
    bool isDocumentError() const;

    /*! Ошибка взаимодействия с БД (ошибка драйвера) */
    bool isDatabaseError() const;

    //! В модели есть несохраненные изменения и она отказывает в проведении операции (например операции обновления
    //! без опции force)
    bool isHasUnsavedDataError() const;
    //! Сущность не найдена в БД
    bool isEntityNotFoundError() const;

    //! Ошибка со журналом ошибок
    bool isLogItemsError() const;

    //! Файл не найден
    bool isFileNotFoundError() const;
    //! Некоректный файл
    bool isBadFileError() const;
    //! Ошибка чтения/записи файла
    bool isFileIOError() const;
    //! Ошибка копирования файла
    bool isCopyFileError() const;
    //! Ошибка удаления файла
    bool isRemoveFileError() const;
    //! Ошибка переименования файла
    bool isRenameFileError() const;
    //! Ошибка создания директории
    bool isCreateDirError() const;
    //! Ошибка удаления директории
    bool isRemoveDirError() const;
    //! Превышен период ожидания
    bool isTimeoutError() const;
    //! Операция не поддерживается
    bool isUnsupportedError() const;
    //! Ошибка JSON
    bool isJsonError() const;
    //! Некорректные данные
    bool isCorruptedDataError() const;
    //! Запрещено
    bool isForbiddenError() const;
    //! Не найдено. Например не найдена информация при HTTP запросе и т.п. Т.е. любые общие ошибки такого вида
    bool isDataNotFound() const;
    //! Ошибка преобразования данных
    bool isConversionError() const;
    //! Потеряно соединение
    bool isConnectionClosedError() const;
    //! Данные изменены другим пользователем
    bool isChangedByOtherUserError() const;

    //! Скрытая ошибка. Не должна отображать на экране, т.к. отображение будет проводиться иначе (например по
    //! сигналу)
    static Error hiddenError(const QString& text, const QString& detail = QString());
    //! Общая системная ошибка. Сгенерирована ядром без привязки к конкретному действию
    static Error coreError(const QString& text, const QString& detail = QString());
    //! Системная ошибка. Не приводит к остановке программы, но требует вмешательства разработчика
    static Error systemError(const QString& text, const QString& detail = QString());
    //! Это не ошибка, а просто указание на отмену операции. Никакой информации пользователю не выводится
    static Error cancelError();
    //! Синтаксическая ошибка
    static Error syntaxError(const QString& text, const QString& detail = QString());
    //! Требуется подтверждение от пользователя
    static Error needConfirmationError(const QString& text, const QString& detail = QString());
    //! Приложение в состоянии критической ошибки и будет закрыто
    static Error applicationHaltedError();

    //! Ошибка загрузки драйвера БД
    static Error databaseDriverError(const QString& text);

    //! Ошибка загрузки модуля в целом
    static Error moduleError(const QString& text);
    //! Ошибка загрузки модуля в целом
    static Error moduleError(const EntityCode& code, const QString& text);
    //! Не найдено представление
    static Error viewNotFoundError(const EntityCode& code);
    //! Плагин не найден
    static Error moduleNotFoundError(const EntityCode& code);
    //! Ошибка с указанием (открытием) документа
    static Error documentError(const Uid& uid, const QString& text, const QString& detail = QString());

    /*! Ошибка взаимодействия с БД (ошибка драйвера) */
    static Error databaseError(const QString& text, const QString& detail = QString());

    //! В модели есть несохраненные изменения и она отказывает в проведении операции (например операции обновления
    //! без опции force)
    static Error hasUnsavedDataError(const Uid& uid);
    //! Сущность не найдена в БД
    static Error entityNotFoundError(const Uid& uid);

    //! Ошибка со журналом ошибок
    static Error logItemsError(const Log& log);

    //! Файл не найден
    static Error fileNotFoundError(const QString& fileName, const QString& text = QString());
    //! Некоректный файл
    static Error badFileError(const QString& fileName, const QString& text = QString());
    static Error badFileError(const QIODevice* device, const QString& text = QString());
    //! Ошибка чтения/записи файла
    static Error fileIOError(const QString& fileName, const QString& text = QString());
    static Error fileIOError(const QIODevice* device, const QString& text = QString());
    static Error fileIOError(const QIODevice& device, const QString& text = QString());
    //! Ошибка копирования файла
    static Error copyFileError(const QString& sourceFile, const QString& destFile, const QString& text = QString());
    //! Ошибка удаления файла
    static Error removeFileError(const QString& fileName, const QString& text = QString());
    //! Ошибка переименования файла
    static Error renameFileError(
            const QString& old_file_name, const QString& new_file_name, const QString& text = QString());
    //! Ошибка создания директории
    static Error createDirError(const QString& dir_name, const QString& text = QString());
    //! Ошибка удаления директории
    static Error removeDirError(const QString& dir_name, const QString& text = QString());
    //! Ошибка превышения периода ожидания
    static Error timeoutError(const QString& text = QString());
    //! Ошибка: операция не поддерживается
    static Error unsupportedError(const QString& text = QString());
    //! Ошибка JSON
    static Error jsonError(const QString& key);
    static Error jsonError(const QStringList& key_path);
    static Error jsonError(const QStringList& key_path, const QString& last_key);
    static Error jsonError(const QStringList& key_path, const QStringList& last_keys);
    //! Некорректные данные
    static Error corruptedDataError(const QString& text = QString());
    //! Запрещено
    static Error forbiddenError(const QString& text = QString());
    //! Не найдено. Например не найдена информация при HTTP запросе и т.п. Т.е. любые общие ошибки такого вида
    static Error dataNotFoundrror(const QString& text = QString());
    //! Ошибка преобразования данных
    static Error conversionError(const QString& text = QString());
    //! Потеряно соединение с сервером
    static Error connectionClosedError(const QString& text = QString(), int code = -1);
    static Error connectionClosedError(int code);
    //! Изменено другим пользователем
    static Error changedByOtherUserError(const QString& text = {});

private:
    QString hashValue() const;
    //! Инициализация
    void init();
    //! Очистка кэшированных значений при изменении данных
    void changed();

    //! Данные
    QSharedDataPointer<Error_data> _d;
    //! Содержимое fullText при запросе через printable
    mutable std::unique_ptr<QByteArray> _printable;

    static int _metatype_number;

    friend class Framework;
    friend ZCORESHARED_EXPORT QDataStream& operator<<(QDataStream& s, const zf::Error& e);
    friend ZCORESHARED_EXPORT QDataStream& operator>>(QDataStream& s, zf::Error& e);
};

typedef std::shared_ptr<Error> ErrorPtr;

ZCORESHARED_EXPORT QDataStream& operator<<(QDataStream& s, const zf::Error& e);
ZCORESHARED_EXPORT QDataStream& operator>>(QDataStream& s, zf::Error& e);

} // namespace zf

Q_DECLARE_METATYPE(zf::Error);
Q_DECLARE_METATYPE(zf::ErrorList)

ZCORESHARED_EXPORT QDebug operator<<(QDebug debug, const zf::Error& c);
ZCORESHARED_EXPORT QDebug operator<<(QDebug debug, const zf::ErrorList& c);
