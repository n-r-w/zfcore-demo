#pragma once

#include "zf.h"
#include "zf_defs.h"
#include "zf_uid.h"
#include "zf_error.h"
#include "zf_access_rights.h"
#include "zf_flat_item_model.h"
#include "zf_version.h"

#include <QRandomGenerator>
#include <QFileDialog>
#include <QEventLoop>
#include <QCryptographicHash>
#include <QElapsedTimer>
#include <QApplication>
#include <QDate>
#include <QValidator>
#include <QNetworkProxy>

class QFrame;
class QWidget;
class QAbstractItemModel;
class QIODevice;
class QMainWindow;
class QAbstractScrollArea;
class QPdfWriter;
class QLineEdit;
class QToolBar;
class QMenu;
class QHeaderView;
class QAbstractSocket;
class QToolButton;
class QLabel;
class QSplitter;
class QTabWidget;
class QTabBar;
class QGridLayout;
class QNetworkProxy;
class QCollator;
class SimpleCrypt;

namespace QtCharts
{
class QChart;
}

namespace QXlsx
{
class Workbook;
class Worksheet;
class Cell;
class Document;
} // namespace QXlsx

#ifndef _MSC_VER
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wattributes"
#endif

namespace zf
{
class ProgressObject;
class ItemModel;
class HeaderView;
class HeaderItem;
class ComplexCondition;
class I_DatasetVisibleInfo;
class HighlightMapping;
class SyncWaitWindow;
class SvgIconCache;

//! Перевод в UTF
QString ZCORESHARED_EXPORT utf(const char* s);
//! Для совместимости макроса Z_HALT
QString ZCORESHARED_EXPORT utf(const QString& s);
//! Поддержка стандартных строк
QString ZCORESHARED_EXPORT utf(const std::wstring& s);
QString ZCORESHARED_EXPORT utf(const std::string& s);
//! Для совместимости макроса Z_HALT
Error ZCORESHARED_EXPORT utf(const Error& err);

//! Сравнение двух double (qFuzzyCompare работает некорректно)
bool ZCORESHARED_EXPORT fuzzyCompare(double a, double b);
//! Проверка double на null
bool ZCORESHARED_EXPORT fuzzyIsNull(double a);
//! Сравнение двух double (qFuzzyCompare работает некорректно)
bool ZCORESHARED_EXPORT fuzzyCompare(long double a, long double b);
//! Проверка double на null
bool ZCORESHARED_EXPORT fuzzyIsNull(long double a);

//! Сравнение строк
bool ZCORESHARED_EXPORT comp(const QString& s1, const QString& s2, Qt::CaseSensitivity cs = Qt::CaseInsensitive);
bool ZCORESHARED_EXPORT comp(const char* s1, const QString& s2, Qt::CaseSensitivity cs = Qt::CaseInsensitive);
bool ZCORESHARED_EXPORT comp(const QString& s1, const char* s2, Qt::CaseSensitivity cs = Qt::CaseInsensitive);
bool ZCORESHARED_EXPORT comp(const char* s1, const char* s2, Qt::CaseSensitivity cs = Qt::CaseInsensitive);
bool ZCORESHARED_EXPORT comp(const QStringRef& s1, const QStringRef& s2, Qt::CaseSensitivity cs = Qt::CaseInsensitive);
bool ZCORESHARED_EXPORT comp(const QString& s1, const QStringRef& s2, Qt::CaseSensitivity cs = Qt::CaseInsensitive);
bool ZCORESHARED_EXPORT comp(const QStringRef& s1, const QString& s2, Qt::CaseSensitivity cs = Qt::CaseInsensitive);
bool ZCORESHARED_EXPORT comp(const char* s1, const QStringRef& s2, Qt::CaseSensitivity cs = Qt::CaseInsensitive);
bool ZCORESHARED_EXPORT comp(const QStringRef& s1, const char* s2, Qt::CaseSensitivity cs = Qt::CaseInsensitive);
//! Сравнение индексов с учетом иерархии. Равнозначен операции i1 < i2
bool ZCORESHARED_EXPORT comp(const QModelIndex& i1, const QModelIndex& i2,
    //! Сравнение на основании значения в указанной роли. Если -1, то сортировка по порядку в модели
    int role = -1,
    //! Локаль для сравнения по значению (по умолчанию - локаль пользовательского интерфейса)
    const QLocale* locale = nullptr,
    //! Параметры стравнения по значению
    CompareOptions options = CompareOption::NoOption);

//! Округление. Результат дробный
long double ZCORESHARED_EXPORT roundReal(long double v, RoundOption options = RoundOption::Nearest);
//! Округление. Результат целый
qlonglong ZCORESHARED_EXPORT round(qreal v, RoundOption options = RoundOption::Nearest);
//! Округление с указанием кол-ва знаков после запятой
qreal ZCORESHARED_EXPORT roundPrecision(qreal v, int precision = 2, RoundOption options = RoundOption::Nearest);
//! Преобразовать дробное число в строку и убрать лишние нули справа
QString ZCORESHARED_EXPORT doubleToString(qreal v, int precision = 2, const QLocale* locale = nullptr, RoundOption options = RoundOption::Nearest);

} // namespace zf

#ifndef _MSC_VER
#pragma GCC diagnostic pop
#endif

namespace zf
{
class ZCORESHARED_EXPORT Utils
{
public:
    //! Преобразование Qtшных enum в строковое представление
    template <typename QEnum> static QString qtEnumToString(const QEnum value) { return QMetaEnum::fromType<QEnum>().valueToKey(static_cast<int>(value)); }

    //! Аналог qDeleteAll с проверкой на nullptr
    template <typename Container> static void zDeleteAll(const Container& c)
    {
        auto begin = c.begin();
        auto end = c.end();

        while (begin != end) {
            if (*begin != nullptr)
                delete *begin;
            ++begin;
        }
    }
    //! Аналог qDeleteAll с проверкой на nullptr (кастомный аллокатор)
    template <typename Container> static void zDeleteAllCustom(const Container& c)
    {
        auto begin = c.begin();
        auto end = c.end();

        while (begin != end) {
            if (*begin != nullptr)
                Z_DELETE(*begin);
            ++begin;
        }
    }
    //! Изьять из вектора набор значений
    template <class T> QVector<T> static takeVector(QVector<T>& v, int position, int count)
    {
        Z_CHECK(position >= 0 && position + count <= v.size());
        QVector<T> res(count);
        for (int i = position + count - 1; i >= position; --i) {
            res[i - position] = v.takeAt(i);
        }

        return res;
    }
    //! Вставить в вектор набор значений
    template <class T> static QVector<T>& insertVector(QVector<T>& v, int position, const QVector<T>& values)
    {
        Z_CHECK(position >= 0 && position <= v.size());
        if (!values.isEmpty()) {
            for (int i = position; i < position + values.count(); ++i) {
                v.insert(i, values.at(i - position));
            }
        }

        return v;
    }
    //! Переместить значения в векторе
    template <class T> static void moveVector(QVector<T>& v, int source, int count, int destination)
    {
        Z_CHECK(source >= 0 && source + count <= v.size());
        Z_CHECK(destination >= 0 && destination + count <= v.size());

        if (source != destination && count > 0) {
            insertVector(v, destination, takeVector(v, source, count));
        }
    }
    //! Изьять из QList значения
    template <class T> static QList<T> takeList(QList<T>& v, int position, int count)
    {
        Z_CHECK(position >= 0 && position + count <= v.size());
        QList<T> res;
        for (int i = position + count - 1; i >= position; --i) {
            res.insert(0, v.takeAt(i));
        }

        return res;
    }
    //! Вставить в QList значения
    template <class T> static QList<T>& insertList(QList<T>& v, int position, const QList<T>& values)
    {
        Z_CHECK(position >= 0 && position <= v.size());
        if (!values.isEmpty()) {
            v.reserve(v.size() + values.count());
            for (int i = position; i < position + values.count(); ++i) {
                v.insert(i, values.at(i - position));
            }
        }

        return v;
    }
    //! Переместь значения в QList
    template <class T> static void moveList(QList<T>& v, int source, int count, int destination)
    {
        Z_CHECK(source >= 0 && source + count <= v.size());
        Z_CHECK(destination >= 0 && destination + count <= v.size());

        if (source != destination && count > 0) {
            insertList(v, destination, takeList(v, source, count));
        }
    }

    //! Заблокировать вызов processEvents
    static void blockProcessEvents();
    //! Разблокировать вызов processEvents
    static void unBlockProcessEvents();
    //! Идет выполнение processEvents
    static bool isProcessingEvent();
    //! Безопасная обработка сообщений для отзывчивости интерфейса
    static void processEvents(QEventLoop::ProcessEventsFlags flags = QEventLoop::ExcludeUserInputEvents | QEventLoop::ExcludeSocketNotifiers);
    //! Отключение пауз разблокировки обработки сообщени при вызове processEvents
    static void disableProcessEventsPause();
    //! Включение пауз разблокировки обработки сообщени при вызове processEvents
    static void enableProcessEventsPause();
    //! Будет ли произведена обработка при вызове processEvents
    static bool isProcessEventsInterval();

    //! Пауза в миллисекундах
    static void delay(int n, QEventLoop::ProcessEventsFlags flags = QEventLoop::ExcludeUserInputEvents | QEventLoop::ExcludeSocketNotifiers,
        //! Использовать QEventLoop
        bool use_event_loop = true);

    //! Находимся в главном потоке
    static bool isMainThread(
        //! Если не задано, то проверяется текущий поток
        QObject* object = nullptr);
    //! Логин в ОС
    static QString systemUserName();
    //! Получить текущий масштаб экранных элементов средствами ОС. У Qt это сделано криво
    static double screenScaleFactor(const QScreen* screen = nullptr);

    /*! Корректно синхронно отключает сокет
     * Согласно документации, вызов QAbstractSocket::waitForDisconnected не всегда срабатывает под Windows */
    static bool disconnectSocket(QAbstractSocket* socket, int wait_msecs = 30000);

    //! Отложенное удаление QObject (для унификации)
    static void deleteLater(QObject* obj);
    //! Отложенное удаление умных указателей. Надо не забыть вызвать reset у указателя после передачи в этот метод
    static void deleteLater(const std::shared_ptr<void>& obj);
    //! Безопасное удаление QObject и его детей. Проверяется наличие наследников I_ObjectExtension,
    //! которые убираются из детей и для них вызывается deleteLater
    static void safeDelete(QObject* obj);

    //! Текстовое представление вида информации
    static QString informationTypeText(InformationType type);
    static QColor informationTypeColor(InformationType type);
    // Определяет тип информации, как наиболее серьезный из присутствующих с точки зрения ошибки
    static InformationType getTopErrorLevel(const QList<InformationType>& types);
    //! Иконка для вида информации
    static QIcon informationTypeIcon(InformationType type,
        //! Размер
        int size = -1);

    //! Расшифровка вида сообщения
    static QString messageTypeName(MessageType type);

    //! Поиск layout в котором находится виджет
    static QLayout* findParentLayout(QWidget* w);

    //! Ищет первый дочерний QLayout с ненулевыми отступами
    static QLayout* findTopMargins(
        //! Виджет, с котрого начинается поиск
        QWidget* w);
    //! Ищет первый дочерний QLayout с ненулевыми отступами
    static QLayout* findTopMargins(
        //! Лайаут, с котрого начинается поиск
        QLayout* l);

    //! Найти все объекты, наследники className в parent, которые имеют objectName
    static void findObjectsByClass(QObjectList& list, QObject* parent, const QString& class_name, bool is_check_parent = false, bool has_names = true);

    //! Найти окно приложения
    static QMainWindow* getMainWindow();
    //! Найти верхнее окно приложения (диалог или MainWindow)
    static QWidget* getTopWindow();
    //! Истина, если открыто модальное окно
    static bool isModalWindow();

    //! Для составного виджета вернуть дочерний виджет, который выполняет основную роль (например lineEdit в
    //! ItemSelector)
    static QWidget* getBaseChildWidget(QWidget* widget);
    //! Для составного виджета вернуть виджет, который выполняет роль главного виджета (например ItemSelector)
    static QWidget* getMainWidget(QWidget* widget);

    //! Список классов, которые являются контейнерами для других виджетов
    static const QStringList& containerWidgetClasses();
    //! Является ли виджет конейнером для других виджетов
    static bool isContainerWidget(QWidget* widget);

    //! Вернуть родительский виджет, наследник определенного класса
    template <typename T>
    static inline T findParentWidget(QWidget* widget,
        //! На сколько уровней анализировать
        int level = -1)
    {
        if (!widget)
            return nullptr;
        QWidget* parent = widget->parentWidget();
        while (parent && (level == -1 || level > 0)) {
            T res = qobject_cast<T>(parent);
            if (res)
                return res;
            parent = parent->parentWidget();

            if (level != -1)
                level--;
        }
        return nullptr;
    }

    //! Вернуть родителя, наследника определенного класса
    template <typename T> static inline T findParentObject(QObject* obj)
    {
        if (!obj)
            return nullptr;
        QObject* parent = obj->parent();
        while (parent) {
            T res = qobject_cast<T>(parent);
            if (res)
                return res;
            parent = parent->parent();
        }
        return nullptr;
    }

    //! Есть ли у объекта родительский объект, наследник указанного класса
    static bool hasParentClass(const QObject* obj, const char* parent_class);
    //! Есть ли такой родитель у объекта
    static bool hasParent(const QObject* obj, const QObject* parent);
    //! Имеет ли данный виджет родителя в виде окна (QDialog, QMdiSubWindow, QMainWindow)
    static bool hasParentWindow(const QWidget* w,
        //! Проверять окно на видимость
        bool check_visible = false);
    //! Получить самый "верхний" parent
    static QObject* findTopParent(const QObject* obj,
        //! Остановить поиск при нахождении родителя - наследника указанных классов
        const QStringList& stop_on = {});
    //! Список всех родителей от нижнего к верхнему
    static QObjectList findAllParents(const QObject* obj,
        //! Только наследники указанных классов
        const QStringList& classes = {},
        //! Только с указанными именами
        const QStringList& names = {});

    //! Получить самый "верхний" parent - наследник QWidget
    static QWidget* findTopParentWidget(const QObject* obj,
        //! Игнорировать с незаданным QLayout
        bool ignore_without_layout,
        //! Остановить поиск при нахождении родителя - наследника указанных классов
        const QStringList& stop_on_class = {},
        //! Остановить поиск при нахождении родителя с указанным именем
        const QString& stop_on_name = {});

    //! Получить parent с указанным objectName
    static QObject* findParentByName(const QObject* obj, const QString& object_name);
    //! Получить parent наследник указанного класса
    static QObject* findParentByClass(const QObject* obj, const QString& class_name);

    //! Находится ли виджет в состоянии "только для чтения". Метод обобщает работу со свойствами readOnly и enabled
    static bool isWidgetReadOnly(const QWidget* w);
    //! Задать для виджета состояние "только для чтения". Метод обобщает работу со свойствами readOnly и enabled
    static bool setWidgetReadOnly(QWidget* w, bool read_only);

    //! Скрыть/показать все виджеты в лайауте
    static void setLayoutWidgetsVisible(QLayout* layout, bool visible,
        //! Список исключений виджетов
        const QSet<QWidget*>& excluded_widgets = QSet<QWidget*>(),
        //! Список исключений вложенных лайаутов
        const QSet<QLayout*>& excluded_layouts = QSet<QLayout*>());
    //! Установить свойство readOnly для всех виджетов в лайауте
    //! Для View нужно вызывать свой setLayoutWidgetsReadOnly из-за наличия пропертей у которых
    //! readOnly контролируется своим механизмом.
    static void setLayoutWidgetsReadOnly(QLayout* layout, bool read_only,
        //! Список исключений виджетов
        const QSet<QWidget*>& excluded_widgets = QSet<QWidget*>(),
        //! Список исключений вложенных лайаутов
        const QSet<QLayout*>& excluded_layouts = QSet<QLayout*>());
    //! Очистить содержимое виджета
    static bool clearWidget(QWidget* w);

    //! Проверка на видимость виджета с учетом возможного скрытия закладки QTabWidget/QToolBox на которой он находится
    static bool isVisible(QWidget* widget, bool check_window = false);

    //! Загрузить форму (ui файл) из файла
    static QWidget* openUI(const QString& file_name, Error& error, QWidget* parent = nullptr);

    //! Создать виджет с "крутилкой"
    static QLabel* createWaitingMovie(QWidget* parent = nullptr);

    //! Выровнять колонки QGridLayout
    static void alignGridLayouts(const QList<QGridLayout*>& layouts);

    //! Данные о состоянии сплиттера до скрытия
    struct SplitterSavedData
    {
        bool isValid() const { return !sizes.isEmpty() && pos >= 0 && handle_size >= 0; }

        //! Какой элемент сплиттера скрывать
        int pos = -1;
        //! Размеры сплиттера до скрытия
        QList<int> sizes;
        //! Размер хендла сплиттера до скрытия
        int handle_size = -1;
        //! Стиль сплиттера до скрытия
        QString style;
        //! Сворачиваемый
        bool collapsible = false;
    };
    //! Скрытие части сплиттера. Возвращает размеры до скрытия.
    //! Подразумевается что все виджеты в скрываемой части уже в состоянии hidden
    static SplitterSavedData hideSplitterPart(QSplitter* splitter,
        //! Какой элемент сплиттера скрывать
        int pos);
    //! Восстановление части сплиттера из скрытого состояния
    //! Не меняет состояние visible/hidden для виджетов
    static void showSplitterPart(QSplitter* splitter, const SplitterSavedData& data);

    //! Подготовка QTabWidget
    static void prepareTabBar(QTabWidget* tw,
        //! Скрывать заблокированные вкладки
        bool hide_disabled = true,
        //! Задать количество строк текста в заголовке
        int lines = -1);
    //! Подготовка QTabBar
    static void prepareTabBar(QTabBar* tb,
        //! Скрывать заблокированные вкладки
        bool hide_disabled = true,
        //! Задать количество строк текста в заголовке
        int lines = -1);

    //! Создать линию
    static QFrame* createLine(Qt::Orientation orientation, int width = 1);
    //! Есть ли у виджета или его дочерних свойство expanding
    static bool isExpanding(const QWidget* w, Qt::Orientation orientation);

    //! Вывод содержимого QAbstractItemModel в qDebug
    static void debugDataset(const QAbstractItemModel* model, int role = Qt::DisplayRole, const QModelIndex& parent = QModelIndex());
    //! Вывод содержимого QAbstractItemModel в окно просмотра
    static void debugDatasetShow(const QAbstractItemModel* model);
    //! Вывод содержимого набора данных в окно просмотра
    static void debugDatasetShow(const ModelPtr& model, const PropertyID& dataset);
    //! Вывод содержимого набора данных в окно просмотра
    static void debugDatasetShow(const DataContainerPtr& container, const PropertyID& dataset);

    //! Вывод содержимого модели в окно просмотра
    static void debugDataShow(const ModelPtr& model);
    //! Вывод содержимого контейнера в окно просмотра
    static void debugDataShow(const DataContainerPtr& container);

    //! Вернуть интерфейс, реализуемый объектом с указанным идентификатором
    //! ВАЖНО: если это модель, то она больше не будет выгружена из памяти
    //! Если не найдено - nullptr
    template <typename T>
    static inline T* getInterface(
        //! Идентификатор сущности, реализующей интерфейс
        const Uid& entity_uid)
    {
        QObject* ptr = getInterfaceHelper(entity_uid);
        if (ptr == nullptr)
            return nullptr;

        T* res = dynamic_cast<T*>(ptr);
        if (res == nullptr)
            haltInternal(Error("Wrong interface type"));

        return res;
    }

    //! Вернуть интерфейс, реализуемый объектом с указанным кодом сущности
    //! ВАЖНО: если это модель, то она больше не будет выгружена из памяти
    //! Если не найдено - nullptr
    template <typename T>
    static inline T* getInterface(
        //! Код сущности, реализующей интерфейс
        const EntityCode& entity_code)
    {
        return getInterface<T>(Uid::uniqueEntity(entity_code));
    }

    //! Получить все индексы нулевой колонки для модели
    static void getAllIndexes(const QAbstractItemModel* m, QModelIndexList& indexes);
    //! Получить количество строк, включая иерархию
    static int getAllRowCount(const QAbstractItemModel* m);

    //! Поиск последнего индекса в QAbstractItemModel
    static QModelIndex findLastDatasetIndex(const QAbstractItemModel* m, const QModelIndex& parent);

    //! Поиск по модели
    static QModelIndex searchItemModel(const QModelIndex& searchFrom, const QVariant& value, bool forward, const I_DatasetVisibleInfo* converter = nullptr);
    //! Поиск по модели следующего или предыдущего элемента. Возвращает всегда индекс в с колонкой 0
    static QModelIndex getNextItemModelIndex(const QModelIndex& index, bool forward);

    //! Возвращает самый верхий индекс по цепочке прокси
    static QModelIndex getTopSourceIndex(const QModelIndex& index);
    //! Возвращает самый верхюю itemModel по цепочке прокси
    static QAbstractItemModel* getTopSourceModel(const QAbstractItemModel* model);
    //! Преобразует уровень прокси индекса в уровень model. Если не может, то invalid
    static QModelIndex alignIndexToModel(const QModelIndex& index, const QAbstractItemModel* model);

    //! Название библиотеки в зависимости от версии ОС
    static QString libraryName(const QString& libraryBaseName);
    //! Название исполняемого в зависимости от версии ОС
    static QString executableName(const QString& executableBaseName);

    //! Скрыть дубликаты сепараторов в списке QAction. Возвращает пару: если остались видимые элементы, остались разрешенные элементы
    static QPair<bool, bool> compressActionList(const QList<QAction*>& al,
        //! Разрешать боковые сепараторы
        bool allowBorderSeparators);
    //! Скрыть дубликаты сепараторов в тулбаре и его подменю. Возвращает пару: если остались видимые элементы, остались разрешенные элементы
    static QPair<bool, bool> compressToolbarActions(QToolBar* toolbar,
        //! Разрешать боковые сепараторы
        bool allowBorderSeparators);

    //! Скрыть экшены, которые содержат меню и в этом меню нет видимых экшенов
    static bool hideEmptyMenuActions(const QList<QAction*>& al);

    //! Создать кнопку на тулбаре с привязанным к нему меню. Возвращает экшен созданной кнопки
    static QAction* createToolbarMenuAction(
        //! Стиль кнопки
        Qt::ToolButtonStyle button_style,
        //! Текст кнопки
        const QString& text,
        //! Рисунок кнопки
        const QIcon& icon,
        //! На какой тулбар поместить (может быть nullptr)
        QToolBar* toolbar,
        //! Перед каким экшеном тулбара (если nullptr, то в конец)
        QAction* before,
        //! Меню для привязки (может быть nullptr)
        QMenu* menu,
        //! objectName для QAction
        const QString& action_object_name,
        //! objectName для кнопки тулбара, созданной на основании action
        const QString& button_object_name);

    //! Выставить стандартные параметры для тулбара
    static QToolBar* prepareToolbar(QToolBar* toolbar, ToolbarType type);
    //! Выставить стандартные параметры для кнопки тулбара на основании ее action
    static QToolButton* prepareToolbarButton(QToolBar* toolbar, QAction* action);
    //! Выставить стандартные параметры для кнопки тулбара на основании ее action
    static void prepareToolbarButton(QToolButton* button);

    //! Подгонка размеров виджета на основе текущего масштаба
    static void prepareWidgetScale(QWidget* w);

    //! Проверка QAction на enabled без учета visible
    static bool isActionEnabled(QAction* a);

    //! Установить у виджета рамку
    static void setHighlightWidgetFrame(QWidget* w, InformationType type = InformationType::Error);
    //! Убрать у виджета рамку
    static void removeHighlightWidgetFrame(QWidget* w);

    //! Разбить строку на строки по размеру
    static QString stringToMultiline(const QString& value, int width, const QFontMetrics& fm = QApplication::fontMetrics());

    /*! Расхождение (расстояние) между двумя строками (расстояние Дамерау-Левенштейна):
     * количество действий пользователя для превращения одной строки в другую.
     * Метод быстрее чем фонетическое сравнение из zf_fuzzy_search.h, но хуже по "качеству" */
    static int editDistance(const QString& str1, const QString& str2);

    //! Является ли символ алфовитно-цифровым (английская раскладка)
    static bool isAlnum(const QChar& ch);
    //! Сравнение двух QVariant
    static bool compareVariant(const QVariant& v1, const QVariant& v2,
        //! Оператор сравнения
        CompareOperator op = CompareOperator::Equal,
        //! Локаль для сравнения (по умолчанию - локаль пользовательского интерфейса)
        const QLocale* locale = nullptr,
        //! Параметры сравнения
        CompareOptions options = CompareOption::NoOption);
    //! Сравнение двух QVariant
    static bool compareVariant(const QVariant& v1, const QVariant& v2,
        //! Оператор сравнения
        CompareOperator op,
        //! Для определения режима стравнения
        LocaleType locale_type,
        //! Параметры сравнения
        CompareOptions options = CompareOption::NoOption);

    //! Текстовое представление оператора сравнения
    static QString compareOperatorToText(CompareOperator op,
        //! Для использования в JavaScript
        bool for_java_script = false,
        //! Полное название (если for_java_script==true, то игнорируется)
        bool full_name = true);

    //! Текстовое представление вида условия
    static QString conditionTypeToText(ConditionType c);

    //! Преобразование QStringList в QVariantList
    static QVariantList toVariantList(const QStringList& list);
    //! Преобразование UidList в QVariantList
    static QVariantList toVariantList(const UidList& list);
    //! Преобразование QList<int> в QVariantList
    static QVariantList toVariantList(const QList<int>& list);

    //! Преобразование QVariantList в UidList
    static UidList toUidList(const QVariantList& list);
    //! Преобразование QVariantList в QStringList
    static QStringList toStringList(const QVariantList& list);
    //! Преобразование QList<int> в QStringList
    static QStringList toStringList(const QList<int>& list);
    //! Преобразование QVariantList в QList<int>
    static QList<int> toIntList(const QVariantList& list);

    //! Содержит ли список такое значение
    static bool containsVariantList(const QVariantList& list, const QVariant& value,
        //! Локаль для сравнения (по умолчанию - локаль пользовательского интерфейса)
        const QLocale* locale = nullptr,
        //! Параметры сравнения
        CompareOptions options = CompareOption::NoOption);
    //! В какой позиции список содержит такое значение. Если не найдено, то -1
    static int indexOfVariantList(const QVariantList& list, const QVariant& value,
        //! Локаль для сравнения (по умолчанию - локаль пользовательского интерфейса)
        const QLocale* locale = nullptr,
        //! Параметры сравнения
        CompareOptions options = CompareOption::NoOption);

    //! Проверить есть ли такой слот у объекта
    static bool hasSlot(QObject* obj, const char* member, QGenericReturnArgument ret, QGenericArgument val0 = QGenericArgument(nullptr),
        QGenericArgument val1 = QGenericArgument(), QGenericArgument val2 = QGenericArgument(), QGenericArgument val3 = QGenericArgument(),
        QGenericArgument val4 = QGenericArgument(), QGenericArgument val5 = QGenericArgument(), QGenericArgument val6 = QGenericArgument(),
        QGenericArgument val7 = QGenericArgument(), QGenericArgument val8 = QGenericArgument(), QGenericArgument val9 = QGenericArgument());
    //! Проверить есть ли такой слот у объекта
    static bool hasSlot(QObject* obj, const char* member, QGenericArgument val0 = QGenericArgument(nullptr), QGenericArgument val1 = QGenericArgument(),
        QGenericArgument val2 = QGenericArgument(), QGenericArgument val3 = QGenericArgument(), QGenericArgument val4 = QGenericArgument(),
        QGenericArgument val5 = QGenericArgument(), QGenericArgument val6 = QGenericArgument(), QGenericArgument val7 = QGenericArgument(),
        QGenericArgument val8 = QGenericArgument(), QGenericArgument val9 = QGenericArgument());

    //! Создать список последовательных значений за исключением указанных
    template <typename T>
    static inline QList<T> range(
        //! Начальное значение
        const T& begin_value,
        //! Конечное значение
        const T& end_value,
        //! Исключения
        const QList<T>& excluded = QList<T>())
    {
        QList<T> res;
        T counter = begin_value;
        while (counter <= end_value) {
            if (!excluded.contains(counter))
                res.append(counter);
            counter = counter + 1;
        }
        return res;
    }

    //! Создать список указанного размера
    template <typename T> static inline QList<T> makeList(int size, T default_value = T())
    {
        Z_CHECK(size >= 0);
        QList<T> res;
        for (int i = 0; i < size; i++) {
            res << default_value;
        }

        return res;
    }

    //! Перемешать элементы списка в случайном порядке (метод Фишера — Йетса)
    template <typename T>
    static inline void shuffleList(QList<T>& a,
        //! База для rand (если -1, то seed не вызывается)
        int seed = -1)
    {
        if (seed >= 0)
            QRandomGenerator::system()->seed(static_cast<quint32>(seed));

        int n = a.size();
        for (int i = n - 1; i > 0; --i) { // gist, note, i>0 not i>=0
            int r = QRandomGenerator::system()->generate() % (i + 1); // gist, note, i+1 not i. "rand() % (i+1)" means
                // generate rand numbers from 0 to i
            a.swap(i, r);
        }
    }

    //! Равномерно распределить число total на count частей
    static QList<int> distributeEqual(int total, int count,
        //! База для rand (если -1, то seed не вызывается)
        int seed = 9999);
    //! Распределить число total пропорционально распределению в base
    static QList<int> distribute( //! база распределения (процентное соотношение)
        const QList<int>& base,
        //! ограничения распределения (не больше)
        const QList<int>& limit,
        //! количество для распределения
        int total);

    //! Преобразование QList в QSet для совместимости версий Qt
    template <typename T> static QSet<T> toSet(const QList<T>& v)
    {
#if (QT_VERSION >= QT_VERSION_CHECK(5, 14, 0))
        return QSet(v.constBegin(), v.constEnd());
#else
        return v.toSet();
#endif
    }
    //! Преобразование QMap в QHash для совместимости версий Qt
    template <typename K, typename T> static QHash<K, T> toHash(const QMap<K, T>& v)
    {
#if (QT_VERSION >= QT_VERSION_CHECK(5, 14, 0))
        return QHash<K, T>(v.begin(), v.end());
#else
        QHash<K, T> res;
        for (auto i = v.constBegin(); i != v.constEnd(); ++i)
            res.insert(i.key(), i.value());
        return res;
#endif
    }
    //! Преобразование QHash в QMap для совместимости версий Qt
    template <typename K, typename T> static QMap<K, T> toMap(const QHash<K, T>& v)
    {
#if (QT_VERSION >= QT_VERSION_CHECK(5, 14, 0))
        return QMap<K, T>(v.begin(), v.end());
#else
        QMap<K, T> res;
        for (auto i = v.constBegin(); i != v.constEnd(); ++i)
            res.insert(i.key(), i.value());
        return res;
#endif
    }

    //! Содержит ли QMap со значениями QVariant v1 содержимое из v2
    template <typename K> static bool contains(const QMap<K, QVariant>& v1, const QMap<K, QVariant>& v2)
    {
        for (auto i = v2.constBegin(); i != v2.constEnd(); ++i) {
            auto val = v1.constFind(i.key());
            if (val == v1.constEnd())
                return false;
            if (!compareVariant(val.value(), i.value()))
                return false;
        }
        return true;
    }
    //! Равно ли QMap со значениями QVariant v1 содержимому из v2
    template <typename K> static bool equal(const QMap<K, QVariant>& v1, const QMap<K, QVariant>& v2)
    {
        if (v1.count() != v2.count())
            return false;

        return contains(v1, v2);
    }

    //! Содержит ли QHash со значениями QVariant v1 содержимое из v2
    template <typename K> static bool contains(const QHash<K, QVariant>& v1, const QMap<K, QVariant>& v2)
    {
        for (auto i = v2.constBegin(); i != v2.constEnd(); ++i) {
            auto val = v1.constFind(i.key());
            if (val == v1.constEnd())
                return false;
            if (!compareVariant(val.value(), i.value()))
                return false;
        }
        return true;
    }
    //! Равно ли QHash со значениями QVariant v1 содержимому из v2
    template <typename K> static bool equal(const QHash<K, QVariant>& v1, const QMap<K, QVariant>& v2)
    {
        if (v1.count() != v2.count())
            return false;

        return contains(v1, v2);
    }

    //! Содержит ли QSet со значениями QVariant v1 содержимое из v2
    static bool contains(const QSet<QVariant>& v1, const QSet<QVariant>& v2);
    //! Содержит ли QList со значениями QVariant v1 содержимое из v2
    static bool contains(const QList<QVariant>& v1, const QList<QVariant>& v2);
    //! Равно ли QSet со значениями QVariant v1 содержимому из v2
    static bool equal(const QSet<QVariant>& v1, const QSet<QVariant>& v2);
    //! Равно ли QList со значениями QVariant v1 содержимому из v2
    static bool equal(const QList<QVariant>& v1, const QList<QVariant>& v2);

    //! Для совместимости между версиями Qt
    static QSize grownBy(const QSize& size, const QMargins& m);

    //! Метод по умолчанию для генерации уникальных строк
    static QString generateUniqueStringDefault(const QString& data = QString());
    /*! Генерация уникальной строки
     * Если параметр задан - производное от data
     * Если параметр не задан - генерируется случайным образом на основе генерации UUID */
    static QString generateUniqueString(
        QCryptographicHash::Algorithm algorithm = QCryptographicHash::Sha224, const QString& data = QString(), bool remove_curly_braces = true);
    /*! Генерация Microsoft GUID вида {165C53A6-4B2E-4BE2-89BF-75D2952DE243}
     * Если data не задана, то генерируется случайным образом
     * Иначе производное от data.
     * Если не нужен именно Microsoft GUID, то лучше использовать generateUniqueString,
     * которое имеет гораздо меньшую вероятность коллизий. С другой стороны генерация GUID быстрее */
    static QString generateGUIDString(const QString& data = QString(), bool remove_curly_braces = true);
    static QUuid generateGUID(const QString& data = QString());
    //! Создать уникальную строку по набору значений (использует generateGUIDString)
    static QString generateUniqueString(
        //! Набор значений
        const QVariantList& values,
        //! Какие из значений не чувствительны к регистру (если не задано, то все чувствительны)
        const QList<bool>& caseInsensitive = QList<bool>());

    //! Получить чек-сумму QIODevice
    static QString generateChecksum(QIODevice* device, qint64 size = 0);
    //! Получить чек-сумму строки
    static QString generateChecksum(const QString& s);
    //! Получить чек-сумму QByteArray
    static QString generateChecksum(const QByteArray& b);

    //! Расшифровать произвольный QVariant зашифрованный через encode.
    //! На входе всегда QVariant с типом QByteArray
    static QVariant decodeVariant(const QVariant& v, quint64 key = 0);
    //! Зашифровать QVariant
    //! На выходе всегда QVariant с типом QByteArray
    static QVariant encodeVariant(const QVariant& v, quint64 key = 0);

    //! Возвращает требуемый набор прав, для указанного стандартного действия
    static AccessRights accessForAction(ObjectActionType action_type);

    //! Преобразовать MS Word в PDF
    static Error wordToPdf(const QString& source_file_name, const QString& dest_file_name);
    //! Преобразовать MS Excel в PDF
    static Error excelToPdf(const QString& source_file_name, const QString& dest_file_name);

    //! Получить имя файла для открытия
    static QString getOpenFileName(
        //! Заголовок диалога. По умолчанию берется название программы
        const QString& caption = QString(),
        //! Папка. По умолчанию берется Core::lastUsedDirectory()
        const QString& default_name = QString(),
        //! Фильтр вида: Images (*.png *.xpm *.jpg);;Text files (*.txt);;
        const QString& filter = QString());
    //! Получить имя нескольких файлов для открытия
    static QStringList getOpenFileNames(
        //! Заголовок диалога. По умолчанию берется название программы
        const QString& caption = QString(),
        //! Папка. По умолчанию берется Core::lastUsedDirectory()
        const QString& default_name = QString(),
        //! Фильтр вида: Images (*.png *.xpm *.jpg);;Text files (*.txt);;
        const QString& filter = QString());
    //! Получить имя файла для сохранения
    static QString getSaveFileName(
        //! Заголовок диалога. По умолчанию берется название программы
        const QString& caption = QString(),
        //! Папка или файл. По умолчанию берется Core::lastUsedDirectory()
        const QString& default_name = QString(),
        //! Фильтр вида: Images (*.png *.xpm *.jpg);;Text files (*.txt);;
        const QString& filter = QString());
    //! Получить имя существующей папки
    static QString getExistingDirectory(
        //! Заголовок диалога. По умолчанию берется название программы
        const QString& caption = QString(),
        //! Папка. По умолчанию берется Core::lastUsedDirectory()
        const QString& dir = QString(), QFileDialog::Options options = QFileDialog::ShowDirsOnly);

    //! Открыть каталог или файл средствами ОС
    static Error openFile(const QString& file_dir);
    //! Сохраняет файл во временный каталог tempDocsLocation под случайным именем и открывает его средствами ОС
    //! Очистка каталога tempDocsLocation происходит при каждом запуске программы
    static Error openFile(const QByteArray& file_content, const QString& extension);

    //! Открыть каталог средствами ОС
    static Error openDir(
        //! Папка или файл
        const QString& file_dir);

    //! Преобразовать строку в допустимое имя файла (убрать символы \ / : * ? " < > |)
    static QString validFileName(const QString& s,
        //! На какой символ заменить недопустимые символы
        const QChar& replace = '_');

    //! Вывод текста в консоль с учетом текущего языка кодировки
    static void infoToConsole(const QString& text, bool new_line = true);
    //! Вывод текста в консоль с учетом текущего языка кодировки
    static void errorToConsole(const QString& text, bool new_line = true);
    static void errorToConsole(const Error& error, bool new_line = true);

    //! Создать копию itemModel
    static void cloneItemModel(const QAbstractItemModel* source, QAbstractItemModel* destination);
    //! Задать количество строк для QAbstractItemModel
    static void itemModelSetRowCount(QAbstractItemModel* model, int count, const QModelIndex& parent = QModelIndex());
    //! Задать количество колонок для QAbstractItemModel
    static void itemModelSetColumnCount(QAbstractItemModel* model, int count, const QModelIndex& parent = QModelIndex());

    //! Вывести итем модель в файл csv
    static Error itemModelToCSV(const QAbstractItemModel* model, const QString& file_name,
        //! Информация о видимости и т.п.
        const I_DatasetVisibleInfo* visible_info = nullptr,
        //! Список колонок и их названий для вывода
        const QMap<int, QString>& header_map = QMap<int, QString>(),
        //! Выгружать идентификаторы как есть, без преобразования в визуальный формат
        bool original_uid = false,
        //! Таймаут в мс (0 - бесконечно)
        int timeout_ms = 0);
    static Error itemModelToCSV(const QAbstractItemModel* model, QIODevice* device,
        //! Информация о видимости и т.п.
        const I_DatasetVisibleInfo* visible_info = nullptr,
        //! Список колонок и их названий для вывода
        const QMap<int, QString>& header_map = QMap<int, QString>(),
        //! Выгружать идентификаторы как есть, без преобразования в визуальный формат
        bool original_uid = false,
        //! Таймаут в мс (0 - бесконечно)
        int timeout_ms = 0);

    //! Загрузить итем модель из файла csv
    static Error itemModelFromCSV(FlatItemModel* model, const QString& file_name,
        //! Колоноки для загрузки. Ключ - имя колонки в CSV
        //! значение - колонка в model
        const QMap<QString, int>& columns_mapping,
        //! Кодек
        const QString& codec = "UTF-8");
    //! Загрузить итем модель из файла csv
    static Error itemModelFromCSV(FlatItemModel* model, QIODevice* device,
        //! Колоноки для загрузки. Ключ - имя колонки в CSV
        //! значение - колонка в model
        const QMap<QString, int>& columns_mapping,
        //! Кодек
        const QString& codec = "UTF-8");

    static Error itemModelToExcel(const QAbstractItemModel* model, const QString& file_name,
        //! Информация о видимости и т.п.
        const I_DatasetVisibleInfo* visible_info = nullptr,
        //! Список колонок и их названий для вывода
        const QMap<int, QString>& header_map = QMap<int, QString>(),
        //! Выгружать идентификаторы как есть, без преобразования в визуальный формат
        bool original_uid = false,
        //! Таймаут в мс (0 - бесконечно)
        int timeout_ms = 0);
    static Error itemModelToExcel(const QAbstractItemModel* model, QIODevice* device,
        //! Информация о видимости и т.п.
        const I_DatasetVisibleInfo* visible_info = nullptr,
        //! Список колонок и их названий для вывода
        const QMap<int, QString>& header_map = QMap<int, QString>(),
        //! Выгружать идентификаторы как есть, без преобразования в визуальный формат
        bool original_uid = false,
        //! Таймаут в мс (0 - бесконечно)
        int timeout_ms = 0);

    //! Предопределенные форматы XSLX. Использовать в функции QXlsx::Format::setNumberFormatIndex
    //! https://docs.microsoft.com/en-us/dotnet/api/documentformat.openxml.spreadsheet.numberingformat?view=openxml-2.8.1
    static int xlsxFormat(QVariant::Type type);
    //! Предопределенные форматы XSLX. Использовать в функции QXlsx::Format::setNumberFormatIndex
    //! https://docs.microsoft.com/en-us/dotnet/api/documentformat.openxml.spreadsheet.numberingformat?view=openxml-2.8.1
    static int xlsxFormat(DataType type);

    /*! Сериализация QAbstractItemModel в QDataStream (без структуры) полная */
    static void itemModelToStream(QDataStream& s, const FlatItemModel* model);
    //! Десериализация QAbstractItemModel из QDataStream
    static Error itemModelFromStream(QDataStream& s, FlatItemModel* model);

    //! Выделить весь текст в QLineEdit, при этом поместить курсор в начало (стандартный метод помещает в конец)
    static void selectLineEditAll(QLineEdit* le);

    //! Управление курсором ожидания (борьба с глюком qt)
    static void setWaitCursor(bool processEvents = false,
        //! Установить счетчик вызовов
        int callCount = 0);
    //! Восстановить курсор
    static void restoreCursor();
    //! Убрать курсор ожидания и вернуть счетчик установки курсора, который был на момент вызова
    static int removeWaitCursor();
    //! Установлен ли курсор ожидания
    static bool isWaitCursor();
    /*! Игнорировать методы setWaitCursor, restoreCursor
     * Сбрасывает счетчик вложенности курсора в 0 */
    static void blockWaitCursor(bool b);
    static bool isBlockWaitCursor();

    //! Примерный размер строки
    static QSize stringSize(const QString& s,
        //! Использовать QTextDocument для вычисления размера. Работает медленнее
        bool html_size = false,
        //! Желаемая ширина (только для html_size true). Если не задана, то размеры подгоняются под золотое сечение
        int ideal_width = -1,
        //! Шрифт
        const QFont& font = QApplication::font());
    //! Вычислить длину строки в пикселах
    static int stringWidth(const QString& s,
        //! Использовать QTextDocument для вычисления размера. Работает медленнее
        bool html_size = false,
        //! Желаемая ширина (только для html_size true). Если не задана, то размеры подгоняются под золотое сечение
        int ideal_width = -1,
        //! Шрифт
        const QFont& font = QApplication::font());
    //! Вычислить высоту строки в пикселах
    static int stringHeight(const QString& s,
        //! Использовать QTextDocument для вычисления размера. Работает медленнее
        bool html_size = false,
        //! Желаемая ширина (только для html_size true). Если не задана, то размеры подгоняются под золотое сечение
        int ideal_width = -1,
        //! Шрифт
        const QFont& font = QApplication::font());

    //! Высота строки текста (шрифт + отступы)
    static int fontHeight(const QFontMetrics& fm = QApplication::fontMetrics());
    //! Скалировать размер UI в зависимости от DPI
    static int scaleUI(int base_size);

#define Z_PM(X) qApp->style()->pixelMetric(QStyle::X)

    //! Добавляет по одному пробелу к началу каждой строки текста. Нужно для борьбы с криворуким
    //! выравниванием Qt иконок по отношению к тексту на кнопках
    static QString alignMultilineString(const QString& s);

    //! Добавить "жирность" к шрифту
    static QFont fontBold(const QFont& f);
    //! Добавить наклон к шрифту
    static QFont fontItalic(const QFont& f);
    //! Добавить подчеркивание к шрифту
    static QFont fontUnderline(const QFont& f);
    //! Изменить размер шрифта
    static QFont fontSize(const QFont& f, int newSize);
    //! Изменить размер шрифта относительно текущего
    static void changeFontSize(QWidget* w, int increment);

    //! Размер максимальной картинки внутри QIcon
    static QSize maxPixmapSize(const QIcon& icon);
    //! Размер картинки внутри QIcon, которая максимально подходит к указанному размеру
    static QSize bestPixmapSize(const QIcon& icon, const QSize& target_size);

    //! Вывести чарт в файл PNG
    static void chartToPNG(
        //! Чарт
        QtCharts::QChart* chart,
        //! Файл PNG
        QIODevice* device,
        //! Размер картинки в пикселах
        const QSize& output_size,
        //! Прозрачный фон
        bool transparent = false);
    //! Вывести чарт в файл SVG
    static void chartToSVG(
        //! Чарт
        QtCharts::QChart* chart,
        //! Файл SVG
        QIODevice* device,
        //! Размер SVG в pt
        const QSize& output_size,
        //! Прозрачный фон
        bool transparent = false);
    static QImage chartToImage(
        //! Чарт
        QtCharts::QChart* chart,
        //! Размер картинки в пикселах
        const QSize& output_size,
        //! Прозрачный фон
        bool transparent = false);
    //! Вывести чарт в файл PDF
    static void chartToPDF(
        /*! QPainter может быть не активным (тогда он активируется device)
         * Либо активированным ранее с помощью device
         * В конце работы с PDF надо вызвать painter->end() иначе PDF не будет сформирован */
        QPainter* painter,
        //! Чарт
        QtCharts::QChart* chart,
        //! Куда выводить
        QPdfWriter* device,
        //! Положение на странице в единицах измерения QPdfWriter
        const QRectF& target_rect,
        //! Прозрачный фон
        bool transparent = false);

    //! Уменьшить картинку до заданного размера
    static QImage scalePicture(const QImage& img, int max_width, int max_height,
        //! Заполнить по ширине или высоте
        bool fill = false);
    static QImage scalePicture(const QImage& img, const QSize& max_size,
        //! Заполнить по ширине или высоте
        bool fill = false);
    static QPixmap scalePicture(const QPixmap& img, int max_width, int max_height,
        //! Заполнить по ширине или высоте
        bool fill = false);
    static QPixmap scalePicture(const QPixmap& img, const QSize& max_size,
        //! Заполнить по ширине или высоте
        bool fill = false);

    //! Вышрузить картинку в QByteArray
    static QByteArray pictureToByteArray(const QImage& img,
        //! Если задано, то будет изменен формат изображения
        FileType format = FileType::Undefined);
    //! Вышрузить картинку в QByteArray
    static QByteArray pictureToByteArray(const QPixmap& img,
        //! Если задано, то будет изменен формат изображения
        FileType format = FileType::Undefined);

    //! Генерация превью
    static QImage generatePreview(const QByteArray& data, const QSize& size,
        //! Тип файла. Если не задано, будет пыться определить автоматически
        FileType format = FileType::Undefined);
    //! Генерация превью
    static QImage generatePreview(const QImage& image, const QSize& size);

    //! Иконка превью лоя типа файла
    static QIcon fileTypePreviewIcon(FileType type);

    //! Заключить строку в двойные кавычки. Если в ней есть ковычки, они экранируются еще одними кавычками
    static QString putInQuotes(const QString& v,
        //! Символ, ограничивающий строки по краям (например кавычки)
        const QChar& quote = '"');
    //! Убрать двойные кавычки из строки
    static QString removeQuotes(const QString& v,
        //! Символ, ограничивающий строки по краям (например кавычки)
        const QChar& quote = '"');
    //! Разбить строку на элементы
    static QStringList extractFromDelimeted(const QString& v,
        //! Символ, ограничивающий строки по краям (например кавычки)
        const QChar& quote = '"',
        //! Разделитель строк
        const QChar& delimeter = ';',
        //! Удалять двойные quote из найденных строк
        bool remove_internal_double_quotes = true);

    //! Преобразовать zf::DataType в QVariant::Type
    static QVariant::Type dataTypeToVariant(DataType t);
    //! Преобразовать QVariant::Type в zf::DataType
    static DataType dataTypeFromVariant(QVariant::Type t);
    //! Можно преобразовать значение с такими типами
    static bool dataTypeCanConvert(DataType from, DataType to,
        //! Можно ли преобразовывать целые в bool
        bool allow_int_to_bool);
    //! Текстовое представление типа данных
    static QString dataTypeToString(DataType t);

    //! Сохранить QByteArray в файл
    static Error saveByteArray(const QString& file_name, const QByteArray& ba);
    //! Загрузить QByteArray из файла
    static Error loadByteArray(const QString& file_name, QByteArray& ba);
    //! Загрузить QByteArray из файла. Если не важна ошибка (например при загрузке из ресурсов которые точно есть)
    static QByteArray loadByteArray(const QString& file_name);

    //! Загрузить JSON
    static QJsonDocument openJson(const QByteArray& source, Error& error);
    //! Загрузить JSON
    static QJsonDocument openJson(const QString& file_name, Error& error);

    //! Преобразовать сериализованную строку в QVariant
    static QVariant variantFromSerialized(const QString& value, Error& error);
    //! Преобразовать QVariant в сериализованную строку
    static QString variantToSerialized(const QVariant& value, Error& error);

    //! Преобразовать QVariant в QJsonValue
    static QJsonValue variantToJsonValue(const QVariant& value, DataType data_type = DataType::Undefined);
    //! Преобразовать QJsonValue в QVariant
    static QVariant variantFromJsonValue(const QJsonValue& value, DataType data_type = DataType::Undefined);

    //! Преобразование QVariant в строку для отображения
    static QString variantToString(const QVariant& value, LocaleType conv = LocaleType::UserInterface,
        //! Максимальное количество выводимых элементов для списка
        int max_list_count = -1);
    static QString variantToString(const QVariant& value, QLocale::Language language,
        //! Максимальное количество выводимых элементов для списка
        int max_list_count = -1);
    //! Преобразование строку, введенную пользователем в QVariant
    static QVariant variantFromString(const QString& value, LocaleType conv = LocaleType::UserInterface);
    static QVariant variantFromString(const QString& value, QLocale::Language language);

    //! Перевод контейнера (QList, QSet) в строку
    template <typename T>
    static QString containerToString(
        //! Массив Qt
        const T& c,
        //! Максимальное количество отображаемых элементов
        int max_count = -1)
    {
        int count = max_count > 0 ? qMin(max_count, c.count()) : c.count();
        QString text;
        int n = 0;
        for (auto i = c.constBegin(); i != c.constEnd(); ++i) {
            if (n >= count)
                break;
            n++;

            if (!text.isEmpty())
                text += QStringLiteral(", ");
            text += variantToString(QVariant::fromValue(*i));
        }
        if (count < c.count())
            text += QStringLiteral("...(%1)").arg(c.count());

        return text;
    }

    //! Преобразовать список Uid и свойст в строку
    static QString uidsPropsToString(const UidList& uids, const QList<DataPropertySet>& props);

    //! Привести значение к типу данных
    static Error convertValue(
        //! Исходное значение
        const QVariant& value,
        //! Тип данных
        DataType type,
        //! Исходная локаль (может быть nullptr)
        const QLocale* locale_from,
        //! Целевой формат
        ValueFormat format_to,
        //! Целевая локаль (может быть nullptr)
        const QLocale* locale_to,
        //! Результат
        QVariant& result);

    //! Проверка значения на допустимость с точки зрения сервера
    static bool checkVariantSrv(const QVariant& value);

    //! Является ли тип данных Variant простым
    static bool isSimpleType(QVariant::Type type);

    //! Преобразовать все спецсимволы в читаемые значения. Метод не совместим с XML, HTML и подобным
    static void toUnescapedString(const QString& in, QString& out, QTextCodec* codec = nullptr);
    static QString toUnescapedString(const QString& in);
    //! Обратное к toUnescapedString преобразование. Метод не совместим с XML, HTML и подобным
    static void toEscapedString(const QString& in, QString& out, QTextCodec* codec = nullptr);
    static QString toEscapedString(const QString& in);

    //! Преобразовать XML спецсимволы в строке в нормальные значения
    static QString unescapeXML(const QString& s);

    //! Перевод русского текста в английский путем транслитерации
    static QString translit(const QString& s);
    //! Убрать неоднозначности из строки (транслитерация в английский, плюс конвертация знаков препинания)
    static QString removeAmbiguities(const QString& s, bool translit = true, const QChar& replace = '-');
    //! Преобразовать английские символы, похожие на аналогичные русские в русские
    static QString prepareRussianString(const QString& s);
    // Удаляем лишние пробелы, символы табуляции и т.п.
    static QString prepareMemoString(const QString& s);
    //! Оставляет в строке только символы, цифры и т.п. Убирает всякие странные символы.
    static QString simplified(const QString& s);

    /*! Разбиение строки на текстовые и целочисленные части. После чего результат можно использовать
     *  для сортировки (сравнения) с учетом числовых значений. Например : дом 12 к 1 > дом 11 к 2 */
    static QVariantList splitIntValues(const QString& s);
    //! Функция сортировки элементов адреса. Пары ID/название. Сортировка идет на основе splitIntValues
    static bool addressSortFunc(const QPair<QString, QString>& d1, const QPair<QString, QString>& d2);

    //! Категории символов для строки с указанием процента нахождения. Процент 0-1
    static QMap<QChar::Script, double> getTextScripts(const QString& s);
    //! Определение языка текста. Поддерживается: Кирилица->QLocale::Russian, Латиница->QLocale::English
    static QLocale::Language detectLanguage(const QString& s,
        //! Процент текста на определенном языке, после которого выбирается этот язык
        double percent = 0.9);

    //! Извлечь реальное содержитмое из текста ввода по маске
    static QString extractMaskValue(const QString& text, const QString& mask);

    //! Вызывает для виджета с фокусом сброс изменений, чтобы они сохранились
    static void fixUnsavedData();

    //! Вычислить возраст
    static int getAge(const QDate& date_birth, const QDate& target_date = QDate::currentDate());
    //! Вычислить возраст и вернуть в виде текста
    static QString getAgeText(const QDate& date_birth, const QDate& target_date = QDate::currentDate());

    //! Сформировать иконку из набора файлов или ресурсов
    static QIcon icon(const QStringList& files);

    //! Преобразовать QIcon в строку
    static QString iconToSerialized(const QIcon& icon);
    //! Преобразовать строку в QIcon
    static QIcon iconFromSerialized(const QString& s, Error& error);

    //! Преобразовать QIcon в QByteArray
    static QByteArray iconToByteArray(const QIcon& icon);
    //! Преобразовать QByteArray в QIcon
    static QIcon iconFromByteArray(const QByteArray& s, Error& error);

    //! Выдает векторную иконку нужного размера на основе SVG файла
    static QIcon resizeSvg(const QString& svg_file, const QSize& size,
        //! Создать PNG
        bool use_png = false,
        //! Код типа приложения
        const QString& instance_key = {});
    //! Выдает векторную иконку нужного размера на основе SVG файла
    static QIcon resizeSvg(const QString& svg_file, int size,
        //! Создать PNG
        bool use_png = false,
        //! Код типа приложения
        const QString& instance_key = {});

    //! Создает векторную иконку нужного размера в виде SVG файла и возвращет его имя
    static QString resizeSvgFileName(const QString& svg_file, const QSize& size,
        //! Создать PNG
        bool use_png = false,
        //! Код типа приложения
        const QString& instance_key = {});
    //! Создает векторную иконку нужного размера в виде SVG файла и возвращет его имя
    static QString resizeSvgFileName(const QString& svg_file, int size,
        //! Создать PNG
        bool use_png = false,
        //! Код типа приложения
        const QString& instance_key = {});

    //! Создать иконку для кнопки на основе SVG файла
    static QIcon buttonIcon(const QString& svg_file,
        //! Создать PNG
        bool use_png = false,
        //! Код типа приложения
        const QString& instance_key = {});
    //! Создать иконку для тулбара на основе SVG файла
    static QIcon toolButtonIcon(const QString& svg_file,
        //! Тип тулбара
        ToolbarType toolbar_type = ToolbarType::Internal,
        //! Создать PNG
        bool use_png = false,
        //! Код типа приложения
        const QString& instance_key = {});

    //! Преобразовать Svg в Png нужного размера и вернуть в виде QByteArray
    static QByteArray resizeSvgToByteArray(const QString& svg_file, const QSize& size,
        //! Код типа приложения
        const QString& instance_key = {});

    //! Размер экрана в указанной точке. Если точка не указана, то берется положение курсора
    static QSize screenSize(const QPoint& screen_point = {});
    //! Размер экрана для виджета
    static QSize screenSize(const QWidget* w,
        //! Положение точки для определения экрана
        Qt::Alignment alignment = Qt::AlignRight | Qt::AlignBottom);
    //! Положение экрана в указанной точке. Если точка не указана, то берется положение курсора
    static QRect screenRect(const QPoint& screen_point = {});
    //! Положение экрана для виджета
    static QRect screenRect(const QWidget* w,
        //! Положение точки для определения экрана
        Qt::Alignment alignment = Qt::AlignRight | Qt::AlignBottom);

    //! Подогнать окно под размер текущего экрана
    static void resizeWindowToScreen(QWidget* w,
        //! Желаемый размер
        const QSize& default_size = QSize(),
        //! Центрировать после изменения размера
        bool to_center = true,
        //! Сколько процентов от экрана оставить по краям
        int reserve_percent = 10);
    //! Подогнать окно под размер текущего экрана
    static void resizeWindow(
        //! Виджет который надо подогнать
        QWidget* widget,
        //! Ограничения
        const QRect& restriction,
        //! Желаемый размер
        const QSize& default_size = QSize(),
        //! Центрировать после изменения размера
        bool to_center = true,
        //! Сколько процентов от экрана оставить по краям
        int reserve_percent = 10);
    //! Поместить окно в центр экрана
    static void centerWindow(
        //! Виджет который надо подогнать
        QWidget* widget,
        //! Если истина, то в центр главного окна
        bool to_main_window = false);

    //! Показать окно "О приложении"
    static void showAbout(QWidget* parent, const QString& title, const QString& text);

    //! Установить фокус на виджет с активацией закладки QTabWidget/QToolBox на которой он находится
    static void setFocus(QWidget* widget,
        //! Установить фокус напрямую или через Qt::QueuedConnection
        bool direct = true);
    //! Установить фокус для свойства
    static bool setFocus(const DataProperty& p, HighlightMapping* mapping);

    //! Проверка имеет ли виджет фокус
    static bool hasFocus(QWidget* widget);

    //! Конвертировать QKeyEvent в QKeySequence
    static QKeySequence toKeySequence(QKeyEvent* event);

    //! Принудительно обновить размеры виджета
    //! В отличие от QWidget::updateGeometry обновляет размеры сразу
    static void updateGeometry(QWidget* widget);

    //! Генерация ошибок - служебные методы
    static void fatalErrorHelper(const char* text, const char* file = nullptr, const char* function = nullptr, int line = 0);
    static void fatalErrorHelper(const QString& text, const char* file = nullptr, const char* function = nullptr, int line = 0);
    static void fatalErrorHelper(const Error& error, const char* file = nullptr, const char* function = nullptr, int line = 0);
    //! Приложение было остановлено
    static bool isAppHalted();

    //! Включить/отключить остановку приложения при критической ошибке.
    //! Если isFatalHaltBlocked, то будет генерироваться исключение
    //! Можно использовать вложенные вызовы
    static void blockFatalHalt();
    static void unBlockFatalHalt();
    static bool isFatalHaltBlocked();

    //! Удаление пробелов по бокам строки и перевод первой буквы в заглавную, а остальных в строчные
    static QString name(const QString& s);

    //! ФИО в одну строку
    static QString getFIO(const QString& f, const QString& i, const QString& o);
    //! ФИО с инициалами в одну строку
    static QString getShortFIO(const QString& f, const QString& i, const QString& o);
    //! Извлечь имя, фамилию и отчество из строки с ФИО
    static void extractFIO(const QString& fio, QString& lastName, QString& name, QString& middleName);

    //! Заменяет все потенциальные разделители (',', ';', '|', '/', '\\') на Consts::COMPLEX_SEPARATOR
    static QString prepareSearchString(const QString& s, bool to_lower);

    //! Размер файла в байтах
    static qint64 fileSize(const QString& f, Error& error);

    //! Версия компилятора
    static QString compilerString();
    //! Версия компилятора и Qt
    static QString compilerVersion();

    //! Сменить папку относительно path
    static bool dirCd(const QDir& dir,
        //! Если это относительный путь, то идет смена относительно path, иначе переход на dirName
        const QString& dirName,
        //! Результат
        QDir& newDir,
        //! Проверять на наличие целевой папки
        bool check_exists = false);
    //! Перейти на одну папку вверх
    static bool dirCdUp(const QDir& path,
        //! Результат
        QDir& new_path,
        //! Проверять на наличие целевой папки
        bool check_exists = false);

    //! Создание папки
    static Error makeDir(const QString& path
#ifdef Q_OS_LINUX
        ,
        const QString& root_password = QString()
#endif
    );
    //! Проверяет есть ли такая папка и при необходимости создает ее.
    //! Возвращает созданную папку или default_path если создать не удалось
    //! Потокобезопасно
    static QString location(const QString& path, const QString& default_path = documentsLocation());
    //! Удаление папки со всем содержимым
    static Error removeDir(const QString& path
#ifdef Q_OS_LINUX
        ,
        //! Root пароль. Если задан, то операция производится под root
        const QString& root_password = QString()
#endif
    );
    //! Очистка содержимого папки, без удалеия самой папки
    static Error clearDir(const QString& path,
        //! Если такие папки существуют, то не удалять, а только очищать от файлов
        const QSet<QString>& keep = QSet<QString>()
#ifdef Q_OS_LINUX
            ,
        //! Root пароль. Если задан, то операция производится под root
        const QString& root_password = QString()
#endif
    );
    //! Удалить файл
    static Error removeFile(const QString& fileName
#ifdef Q_OS_LINUX
        ,
        //! Root пароль. Если задан, то операция производится под root
        const QString& root_password = QString()
#endif
    );
    //! Переименовать файл
    static Error renameFile(const QString& oldFileName, const QString& newFileName,
        //! Создать папку для нового файла, если она не существует
        bool createDir = true,
        //! Перезаписать
        bool overwrite = false
#ifdef Q_OS_LINUX
        ,
        //! Root пароль. Если задан, то операция производится под root
        const QString& root_password = QString()
#endif
    );
    //! Копировать файл
    static Error copyFile(const QString& fileName, const QString& newName,
        //! Создать папку для нового файла, если она не существует
        bool createDir = true,
        //! Перезаписать
        bool overwrite = false
#ifdef Q_OS_LINUX
        ,
        //! Root пароль. Если задан, то операция производится под root
        const QString& root_password = QString()
#endif
    );
    //! Переместить файл
    static Error moveFile(const QString& fileName, const QString& newName,
        //! Создать папку для нового файла, если она не существует
        bool createDir = true,
        //! Перезаписать
        bool overwrite = false
#ifdef Q_OS_LINUX
        ,
        //! Root пароль. Если задан, то операция производится под root
        const QString& root_password = QString()
#endif
    );

    struct CopyDirInfo
    {
        CopyDirInfo()
            : size(0)
        {
        }
        CopyDirInfo(const Error& error)
            : error(error)
            , size(0)
        {
        }

        //! Ошибка (если была)
        Error error;
        //! Скопированные файлы (источник)
        QStringList source_files;
        //! Скопированные файлы (цель)
        QStringList dest_files;
        //! Размер скопированных файлов в байтах
        qlonglong size;
    };

    //! Копировать одну папку в другую с поддиректориями
    static CopyDirInfo copyDir(const QString& sourcePath, const QString& destinationPath, bool overWriteDirectory,
        //! Не копировать файлы. Указывается полный путь к файлу
        const QStringList& excludedFiles = QStringList(), ProgressObject* progress = nullptr, const QString& progressText = QString()
#ifdef Q_OS_LINUX
                                                                                                  ,
        //! Root пароль. Если задан, то операция производится под root
        const QString& root_password = QString()
#endif
    );

    //! Получить список всех файлов и папок в указанной папке
    static Error getDirContent(const QString& path, QStringList& files,
        //! С поддиректориями
        bool isRecursive = true,
        //! В files включать не полный путь, а только относительно path
        bool isRelative = false);

    //! Путь к корневому каталогу, уникальному для каждого типа приложений - Core::coreInstanceKey()
    static QString instanceLocation(const QString& instance_key = {});
    //! Путь к корневому каталогу временных файлов, уникальному для каждого типа приложений - Core::coreInstanceKey()
    static QString instanceTempLocation(const QString& instance_key = {});
    //! Путь к корневому каталогу файлов кэша, уникальному для каждого типа приложений - Core::coreInstanceKey()
    static QString instanceCacheLocation(const QString& instance_key = {});

    //! Путь к данным ядра
    static QString dataLocation();
    //! Путь папке с документами
    static QString documentsLocation();
    //! Путь к папке с кэшем
    static QString cacheLocation();
    //! Путь к данным ядра, которые не зависят от приложения
    static QString dataSharedLocation();
    //! Путь к данным модуля
    static QString moduleDataLocation(const EntityCode& entity_code);

    //! Путь к журналам работы
    static QString logLocation();
    //! Задать путь к журналам работы. Вызывать до первого вызова Core::writeToLogStorage
    static void setLogLocation(const QString& path);

    //! Временный каталог драйвера (серверный кэш и т.п.)
    static QString driverCacheLocation();

    //! Путь к папке с временными файлами
    static QString tempLocation();
    //! Генерация имени случайного файла во временной папке. Если папки нет, то создает ее
    static QString generateTempFileName(const QString& extention = "tmp");
    //! Генерация имени случайной папки во временной папке. Если временной папки нет, то создает ее
    static QString generateTempDirName(const QString& sub_folder = QString());
    //! Путь к папке с временными файлами для документов. Очищается при старте
    static QString tempDocsLocation();
    //! Генерация имени случайного файла во временной папке документов. Если папки нет, то создает ее
    static QString generateTempDocFileName(const QString& extention = "tmp");

    //! Стандартные расширения для типа документа
    static QStringList fileExtensions(FileType type, bool halt_if_not_found = true);
    //! Основное стандартное расширение для типа документа
    static QString fileExtension(FileType type,
        //! Вставлять в начало точку если расширение найдено
        bool prepend_point = false);
    //! Тип файла по расширению
    static FileType fileTypeFromExtension(const QString& ext);
    //! Тип файла по mime
    static FileType fileTypeFromMime(const QString& mime);
    //! Автоопределение типа файла по его содержимому
    static FileType fileTypeFromBody(const QByteArray& data,
        //! Ожидаемый тип файла. Алгоритм поиска типа по содержимому использует поиск по стандарту
        //! freedesktop (https://github.com/freedesktop/xdg-shared-mime-info), который может не всегда
        //! корректно определить тип (это прежде всего касается всяких doc,docx,xls,xlsx)
        //! Для картинок и pdf работает хорошо. Если будет надо точно определять тип документов msoffice не зная
        //! заранее что в QByteArray, то придется дописать под них специальный алгоритм анализа
        FileType expected_type = FileType::Undefined);
    //! Автоопределение типа файла по его содержимому
    static FileType fileTypeFromFile(
        //! Имя файла
        const QString& file_name,
        //! Ошибка открытия файла
        Error& error,
        //! Ожидаемый тип файла. Если не задано, берет из расширения файла
        FileType expected_type = FileType::Undefined);

    //! Является ли тип файла картинкой
    static bool isImage(FileType type, bool supported_only = true);

    //! Задан ли прокси на уровне системы
    static bool isProxyExists();
    //! Системный прокси для указанного адреса
    static QNetworkProxy proxyForUrl(const QUrl& url);
    //! Системный прокси для указанного адреса
    static QNetworkProxy proxyForUrl(const QString& url);

    //! Убрать из строки, которую выдает svn_version всякий мусор и привести ее е числу
    static int extractSvnRevision(const QString& svn_revision);
    //! Получить версию приложения из переменных окружения
    static Version getVersion();

#ifdef Q_OS_LINUX
    //! Проверка на работу основного процесса приложения под root
    static bool isRoot();
    //! Проверка пароля на root
    static bool checkRootPassword(
        //! Пароль
        const QString& root_password,
        //! Папка, в которой будет создан временный файл для проверки пароля
        const QString& test_folder = QString("/opt"));
#endif

public:
    //! Открыть меню конфигурации заголовка
    static void headerConfig(
        //! На вход: количество зафиксированных групп (узлов верхнего уровня). Если -1, то фукционал фиксации не
        //! поддерживается На выход: количество зафиксированных групп по итогам работы меню. Если -1, то ничего не
        //! изменилось
        int& frozen_group,
        //! Заголовок
        HeaderView* header_view,
        //! Точка отображения меню
        const QPoint& pos,
        //! Параметры
        const zf::DatasetConfigOptions& options,
        //! Можно ли двигать и скрывать первую колонку
        bool first_column_movable,
        //! Представление
        const View* view,
        //! Набор данных, к которому относится таблица
        const DataProperty& dataset_property);

    //! Отрисовка индикации куда будет вставлен перетаскиваемый заголовок
    static void paintHeaderDragHandle(QAbstractScrollArea* area, HeaderView* header);

    //! Сохранить состояние заголовка
    static Error saveHeader(QIODevice* device, HeaderItem* root_item, int frozen_group_count);
    //! Восстановить состояние заголовка
    static Error loadHeader(QIODevice* device, HeaderItem* root_item, int& frozen_group_count);

    //! Выставить палитру в соответствии с доступностью виджета
    static void updatePalette(bool read_only, bool warning, QWidget* widget);

    //! Отрисовка рамки для виджета вне зависимости от состояния enabled/readOnly
    static void paintBorder(QWidget* widget, const QRect& rect = QRect(), bool top = true, bool bottom = true, bool left = true, bool right = true);
    static void paintBorder(QPainter* painter, const QRect& rect, bool top = true, bool bottom = true, bool left = true, bool right = true);

    //! Выстраивание порядка перехода через виджеты по табуляции на основании их размещения в QLayout
    static void createTabOrder(QLayout* la, QList<QWidget*>& widgets);
    //! Расстановка табуляции последовательно для списка виджетов
    static void setTabOrder(const QList<QWidget*> widgets);

    //! Получить значение на указанном языке
    static QVariant valueToLanguage(const LanguageMap* value, QLocale::Language language);
    static QVariant valueToLanguage(const LanguageMap& value, QLocale::Language language);

private:
    //! Поиск layout в котором находится виджет
    static QLayout* findParentLayoutHelper(QWidget* parent, QWidget* w);

    //! Остановка приложения с ошибкой
    static void haltInternal(const Error& error);

    //! Вернуть интерфейс, реализуемый объектом с указанным идентификатором
    static QObject* getInterfaceHelper(
        //! Идентификатор сущности, реализующей интерфейс
        const Uid& uid);

    //! Получить все индексы нулевой колонки для модели
    static void getAllIndexesHelper(const QAbstractItemModel* m, const QModelIndex& parent, QModelIndexList& indexes);
    //! Получить количество всех строк включая иерархию
    static void getAllRowCountHelper(const QAbstractItemModel* m, const QModelIndex& parent, int& count);

    enum class ConvertToPdfType
    {
        MSWord,
        MSExcel
    };
    //! Преобразовать PDF
    static Error convertToPdf(const QString& source_file_name, const QString& dest_file_name, ConvertToPdfType type);

    //! Выстраивание порядка перехода через виджеты по табуляции на основании их размещения в QLayout
    static void createTabOrderWidgetHelper(QWidget* w, QList<QWidget*>& widgets);

    //! Создать копию itemModel
    static void cloneItemModel_helper(
        const QAbstractItemModel* source, QAbstractItemModel* destination, const QModelIndex& source_index, const QModelIndex& destination_index);

    //! Выгрузка модели в CSV
    static void itemModelToCSV_helper(const QAbstractItemModel* model, QIODevice* device, const QMap<int, QString>& hMap, QTextCodec* codec,
        const I_DatasetVisibleInfo* visible_info, const QModelIndex& parent,
        //! Выгружать идентификаторы как есть, без преобразования в визуальный формат
        bool original_uid, int timeout_ms);
    //! Выгрузка модели в Excel
    static void itemModelToExcel_helper(const QAbstractItemModel* model, QXlsx::Worksheet* workbook, const I_DatasetVisibleInfo* visible_info, int& export_row,
        const QMap<int, QPair<QString, int>>& hMap, const QModelIndex& parent,
        //! Выгружать идентификаторы как есть, без преобразования в визуальный формат
        bool original_uid, int timeout_ms);

    //! Преобразование QVariant в строку для отображения
    static QString variantToStringHelper(const QVariant& value, const QLocale* locale,
        //! Максимальное количество выводимых элементов для списка
        int max_list_count);
    //! Преобразование строку, введенную пользователем в QVariant
    static QVariant variantFromStringHelper(const QString& value, const QLocale* locale);

    //! Служебная функция
    static QStringList splitArgsHelper(const QString& s, int idx);
    //! Метод заимствован из кода QSettings
    static void chopTrailingSpacesHelper(QString& str);

#ifdef Q_OS_LINUX
    //! Служебный метод для запуска команды под sudo
    static bool execute_sudo_comand_helper(const QString& command,
        //! Пароль root
        const QString& root_password);
#endif
    //! Служебный метод удаления файлов
    static bool remove_file_helper(const QString& fileName
#ifdef Q_OS_LINUX
        ,
        //! Пароль root. Если не задан, то команда выполняется под текущим пользователем
        const QString& root_password
#endif
    );
    //! Служебный метод переименования файлов
    static bool rename_file_helper(const QString& oldName, const QString& newName
#ifdef Q_OS_LINUX
        ,
        //! Пароль root. Если не задан, то команда выполняется под текущим пользователем
        const QString& root_password
#endif
    );
    //! Служебный метод копирования файлов
    static bool copy_file_helper(const QString& fileName, const QString& newName
#ifdef Q_OS_LINUX
        ,
        //! Пароль root. Если не задан, то команда выполняется под текущим пользователем
        const QString& root_password
#endif
    );
    //! Служебный метод создания папки
    static bool make_dir_helper(const QString& path
#ifdef Q_OS_LINUX
        ,
        const QString& root_password
#endif
    );
    //! Служебный метод рекурсивного удаления папки
    static bool remove_dir_helper(const QString& path
#ifdef Q_OS_LINUX
        ,
        const QString& root_password
#endif
    );
    static Error getDirContentHelper(const QString& path, QStringList& files, bool isRecursive, bool isRelative, const QString& relativePath);

    //! Попытка привести QVariant к числу
    static QVariant variantToDigitalHelper(const QVariant& v, bool invalid_if_error, const QLocale* locale);

    /*! Сериализация FlatItemModel в QDataStream (без структуры) полная */
    static bool itemModelToStreamHelper(QDataStream& s, const FlatItemModel* model, int column_count, const QModelIndex& parent);
    //! Десериализация FlatItemModel из QDataStream
    static bool itemModelFromStreamHelper(QDataStream& s, FlatItemModel* model, int column_count, const QModelIndex& parent);

    static QChar prepareRussianStringHelper(const QChar& c);
    static bool isExpandingHelper(QLayout* la, Qt::Orientation orientation);

    //! Вывод содержимого модели в qDebug
    static void debugDataset_helper(const QAbstractItemModel* model, int role, const QModelIndex& parent);

    //! Безопасное удаление QObject и его детей. Проверяется наличие наследников I_ObjectExtension,
    //! которые убираются из детей и для них вызывается deleteLater
    static void safeDeleteHelper(QObject* obj, QObjectList& to_safe_destroy);

    //! Шифрование SimpleCrypt
    static SimpleCrypt* getCryptHelper(quint64 key);

    // Приложение было остановлено
    static bool _is_app_halted;
    //! Включить/отключить остановку приложения при критической ошибке.
    //! Если > 0, то будет генерироваться исключение вместо halt
    static int _fatal_halt_block_counter;

    //! Базовый UUID для генерации V5
    static const QUuid _baseUuid;
    //! "соль" для генерации GUID
    static const QString _guid_salt;
    //! Счетчик запуска QApplication::processEvents()
    static int _process_events_counter;
    //! Контроль частоты вызова QApplication::processEvents()
    static QElapsedTimer _time_process_events_counter;
    //! Счетчик блокировки пауз при обработке processEvents
    static QAtomicInt _disable_process_events_pause_counter;

    //! Счетчик курсора ожидания
    static int _wait_count;
    //! Блокировка курсора ожидания
    static bool _is_block_wait_cursor;

    //! Путь к журналам работы
    static QString _log_folder;

    static const QString _translitRus[74];
    static const QString _translitEng[74];
    static const QString _ambiguities;

    //! Возможные значения маски QLineEdit
    static const QSet<QChar> _mask_values_mask;
    //! Возможные значения метасимволов маски QLineEdit
    static const QSet<QChar> _mask_values_meta;

    static QMutex _location_mutex;
    static std::unique_ptr<QHash<QString, QString>> _location_created;

    //! Список классов, которые являются контейнерами для других виджетов
    static const QStringList _container_widget_classes;

    //! Пользовательские варианты разделителей
    static const QList<QChar> USER_SEPARATORS;

    //! Кэш SVG
    static SvgIconCache _svg_cache;

    //! SimpleCrypt Ключевое значение - криптографический ключ
    static QMap<quint64, SimpleCrypt*> _crypt_info;
    //! SimpleCrypt. Префикс
    static const QByteArray _crypt_version;
    static QMutex _crypt_mutex;

    friend class MessagePauseHelper;
};

class MessagePauseHelper_timeout;
//! Класс для автоматической постановки обработки сообщений на паузу и запуска снова после выхода из метода
class ZCORESHARED_EXPORT MessagePauseHelper
{
    Q_DISABLE_COPY(MessagePauseHelper)
public:
    MessagePauseHelper();
    ~MessagePauseHelper();

    //! Ставит на паузу обработку сообщений
    void pause();

private:
    bool _use_timeout;
    bool _paused = false;

    static std::unique_ptr<MessagePauseHelper_timeout> _timeout;
};

//! Класс для управления курсором без отслеживания выхода из функции
class ZCORESHARED_EXPORT WaitCursor
{
public:
    WaitCursor(bool is_show = false);
    void show(bool processEvents = true);
    void hide();
    ~WaitCursor();

private:
    bool _initialized = false;
};

//! Класс для управления SyncWaitWindow без отслеживания выхода из функции
class ZCORESHARED_EXPORT WaitWindow
{
public:
    enum WaitText
    {
        //! Загрузка...
        Loading,
        //! Сохранение...
        Saving,
        //! Подождите...
        Waiting,
    };

    WaitWindow(const QString& text, bool is_show = false);
    WaitWindow(WaitText type, bool is_show = false);
    //! Сообщение Waiting
    WaitWindow(bool is_show = false);

    void show();
    void hide();

    ~WaitWindow();

private:
    static QString waitText(WaitText type);

    SyncWaitWindow* _window = nullptr;
    QString _text;
};

//! Служебный класс для вывода иерархии дочерних элементов в QDebug
class ZCORESHARED_EXPORT ObjectHierarchy
{
    Q_GADGET
public:
    ObjectHierarchy();
    ObjectHierarchy(QObject* obj);
    QObject* object() const;
    void toDebug(QDebug debug) const;

private:
    static void toDebugHelper(QDebug debug, QObject* obj);

    QPointer<QObject> _object;
};

} // namespace zf

Q_DECLARE_METATYPE(zf::ObjectHierarchy);
ZCORESHARED_EXPORT QDebug operator<<(QDebug debug, const zf::ObjectHierarchy& c);
