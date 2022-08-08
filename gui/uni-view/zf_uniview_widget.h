
#pragma once

#include <QWidget>
#include <QWebEngineView>
#include "zf_error.h"
#include "zf_web_engine_injection.h"
#include "zf_webengineview.h"

class QVBoxLayout;
class QTextEdit;

#include <QPdfDocument>
#include "zf_pdf_view.h"

namespace zf
{
class PictureViewer;
class WebViewWidget;

#ifdef Q_OS_WINDOWS
class MSWidget;
#endif

//! Виджет для отображения MS Word, MS Excel, PDF, HTML, Text
class ZCORESHARED_EXPORT UniViewWidget : public QWidget
{
    Q_OBJECT
public:
    explicit UniViewWidget(
        //! Объект для перехвата событий веб страницы
        WebEngineViewInjection* web_injection = nullptr, QWidget* parent = nullptr);
    ~UniViewWidget();

    //! Загружено ли что-то
    bool hasData() const;

public:
    //! Загрузка HTML из URL
    Error loadHtmlUrl(const QUrl& url);

public: // Загрузка данных из файла
    //! Отобразить файл. Универсальный метод
    Error loadFile(const QString& file_name, FileType type = FileType::Undefined,
                   UniViewWidgetOptions options = UniViewWidgetOption::MsOfficeClone);

    //! Загрузить текстовый файл
    Error loadTextFile(const QString& file_name, FileType type = FileType::Undefined, bool editable = false);
    //! Загрузить файл ms office
    Error loadMsOfficeFile(const QString& file_name, FileType type = FileType::Undefined,
                           UniViewWidgetOptions options = UniViewWidgetOption::MsOfficeClone);
    //! Загрузить файл pdf
    Error loadPdfFile(const QString& file_name);
    //! Загрузить файл изображения
    Error loadIconFile(const QString& file_name, FileType type = FileType::Undefined);

public: // Загрузка данных из памяти
    //! Отобразить данные из памяти. Универсальный метод
    Error loadData(const QByteArray& data, FileType type, UniViewWidgetOptions options = {});

    //! Загрузить из строки
    Error loadTextData(const QString& string,
                       //! Тип файла: поддерживается HTML, Plain, Json, Xml
                       FileType type, bool editable = false);

    //! Загрузить документ ms office
    Error loadMsOfficeData(const QByteArray& data, FileType type
#ifdef Q_OS_WINDOWS
                           ,
                           bool to_pdf = false
#endif
    );

    //! Отобразить pdf файл из QByteArray
    Error loadPdfData(const QByteArray& data);

    //! Загрузить изображение из QIcon
    Error loadIconData(const QIcon& icon);
    //! Загрузить изображение из QByteArray
    Error loadIconData(const QByteArray& data,
                       //! Расширение файлов, относящихся к этим данным
                       const QString& file_type = QString());
    Error loadIconData(const QByteArray& data,
                       //! Расширение файлов, относящихся к этим данным
                       FileType file_type);

public:
    //! Сохранить в файл. Доступно для Text и MSWord/MSExcel (под windows)
    Error saveAs(const QString& file_name, bool override = true) const;

    //! Поддерживаемые парамеры (какие надо показывать на экране)
    UniviewParameters supportedParameters() const;
    //! Доступные парамеры (какие должны быть активны на экране)
    UniviewParameters availableParameters() const;

    //! Задать масштабирование
    Error setFittingType(FittingType type);

    //! Увеличить масштаб
    Error zoomIn();
    //! Уменьшить масштаб
    Error zoomOut();
    //! Задать масштаб (1.0 - 100%)
    Error setScale(double scale);
    //! Текущий масштаб (если -1, то не поддерживается)
    double scaleFactor() const;

    //! Задать минимальный масштаб
    Error setMinimumScale(double scale);
    //! Минимальный масштаб (-1 если не поддерживается)
    double minimumScale() const;

    //! Задать максимальный масштаб
    Error setMaximumScale(double scale);
    //! Максимальный масштаб
    double maximumScale() const;

    //! Повернуть на 90 градусов по часовой стрелке
    Error rotateRight();
    //! Повернуть на 90 градусов против часовой стрелки
    Error rotateLeft();

    //! Установить текущую страницу (начиная с 1)
    Error setCurrentPage(int page);
    int currentPage() const;

    //! Максимальное количество страниц
    int maximumPage() const;

    //! В режиме редактирования
    bool isEditable() const;

    void clear();
    bool isValid() const;

    //! Загружено
    bool isLoading() const;

    //! Отображается ли в режиме QWebEngineView
    bool isWebView() const;
    //! Прямой доступ к QWebEngineView. Если отображается не веб страница, то nullptr
    WebEngineView* view() const;

    //! Находимся ли в режиме просмотра pdf
    bool isChromiumPdfView() const;

    //! Скрыт ли верхний тулбар при просмотре pdf
    bool isChromiumHidePdfToolbar() const;
    //! Скрывать ли верхний тулбар при просмотре pdf
    Error setChromiumHidePdfToolbar(bool b);

    //! Скрыта ли панель масштаба при просмотре pdf
    bool isChromiumHidePdfZoom() const;
    //! Скрывать ли панель масштаба при просмотре pdf
    Error setChromiumHidePdfZoom(bool b);

signals:
    //! Загрузка начата
    void sg_loadStarted();
    //! Загрузка закончена
    void sg_loadFinished(const zf::Error& error);
    //! Изменились поддерживаемые или доступные парамеры
    void sg_parametersChanged();
    //! Нажата кнопка
    void sg_webKeyPressed(const QString& target_id, const QString& key);
    //! Нажата кнопка
    void sg_webMouseClicked(const QString& target_id, Qt::KeyboardModifiers modifiers, Qt::MouseButton button);
    //! Изменилась текущая станица (только для pdf)
    void sg_currentPageChanged(int current, int maximum);
    //! Изменилось значение масштаба
    void sg_zoomScaleChanged(double scale);
    //! Активирована подгонка размера
    void sg_fittingTypeChaged(zf::FittingType type);
    //! Поворот
    void sg_roteated(int old_degrees, int new_degrees);

protected:
    void resizeEvent(QResizeEvent* event) override;
    bool event(QEvent* event) override;

private slots:
    void sl_web_load_finished(const zf::Error& error);

    void sl_pdf_statusChanged(QPdfDocument::Status status);
    void sl_pdf_zoomModeChanged(PdfView::ZoomMode zoomMode);

private:
    //! Загрузка HTML из URL
    Error loadHtmlUrl_helper(const QUrl& url);

    // Загрузка данных из файла
    //! Отобразить файл. Универсальный метод
    Error loadFile_helper(const QString& file_name, FileType type, UniViewWidgetOptions options);

    //! Загрузить текстовый
    Error loadTextFile_helper(const QString& file_name, FileType type, bool editable);
    //! Загрузить файл ms office
    Error loadMsOfficeFile_helper(const QString& file_name, FileType type, UniViewWidgetOptions options);
    //! Загрузить файл pdf
    Error loadPdfFile_helper(const QString& file_name);
    //! Загрузить файл изображения
    Error loadIconFile_helper(const QString& file_name, FileType type);

    // Загрузка данных из памяти
    //! Отобразить данные из памяти. Универсальный метод
    Error loadData_helper(const QByteArray& data, FileType type, UniViewWidgetOptions options);

    //! Загрузить из строки
    Error loadTextData_helper(const QString& string,
                              //! Тип файла: поддерживается HTML, Plain, Json, Xml
                              FileType type, bool editable = false);

    //! Загрузить документ ms office
    Error loadMsOfficeData_helper(const QByteArray& data, FileType type
#ifdef Q_OS_WINDOWS
                                  ,
                                  bool to_pdf = false
#endif
    );

    //! Отобразить pdf файл из QByteArray
    Error loadPdfData_helper(const QByteArray& data);

    //! Загрузить изображение из QIcon
    Error loadIconData_helper(const QIcon& icon);
    //! Загрузить изображение из QByteArray
    Error loadIconData_helper(const QByteArray& data,
                              //! Расширение файлов, относящихся к этим данным
                              const QString& file_type = QString());
    Error loadIconData_helper(const QByteArray& data,
                              //! Расширение файлов, относящихся к этим данным
                              FileType file_type);

private:
    void startLoad();
    void finishLoad(const Error& error);
    void updateInternalGeometry();

    void clearHelper();
    void setParameters(UniviewParameters supported_params, UniviewParameters available_params);
    void setParameters(UniviewParameters params);
    //! Удалить все виджеты кроме указанного. Если null, то удалить все
    void prepare(QWidget* keep);

    //! Создать PictureViewer
    void createPictureViewer();
    //! Создать WebViewWidget
    void createWebViewWidget();
    //! Создать PdfView
    void createPdfView();
    //! Создание QTextEdit
    void createTextView();
#ifdef Q_OS_WINDOWS
    //! Создать MSWidget
    void createMSWidget();
#endif

    //! Задать масштабирование
    Error setFittingTypeHelper(FittingType type);

    //! Параметры pdf при просмотре внутри chromium
    static UniviewParameters pdfChromiumParameters();

    //! Удалять файл, который просматривается по окончании просмотра
    QStringList _to_remove_files;
    //! Удалять папку, в которой находится файл просмотра
    QStringList _to_remove_dirs;

    //! Параметры просмиотра
    UniViewWidgetOptions _options;

    //! Основной layout просмотра
    QVBoxLayout* _layout = nullptr;

    //! Отображение HTML, PDF, (MS Word/Excel - для Linux)
    WebViewWidget* _web_widget = nullptr;
    //! Отображение текста
    QTextEdit* _text_widget = nullptr;
#ifdef Q_OS_WINDOWS
    //! Отображение MS Word/Excel
    MSWidget* _ms_widget = nullptr;
#endif
    //! Отображение картинок
    PictureViewer* _picture_widget = nullptr;

    //! Отображение PDF
    PdfView* _pdf_view = nullptr;    
    QPdfDocument* _pdf_doc = nullptr;
    QByteArray* _pdf_data = nullptr;
    QBuffer* _pdf_data_read_bufer = nullptr;

    //! Шаг зума
    const double _zoom_step = 0.2; // 20%

    //! В процессе загрузки
    bool _is_loading = false;

    //! Объект для перехвата событий веб страницы
    WebEngineViewInjection* _web_injection = nullptr;

    //! Поддерживаемые парамеры
    UniviewParameters _supported_params;
    //! Доступные парамеры
    UniviewParameters _available_params;
    //! Тип подгонки размера
    FittingType _fitting_type = FittingType::Undefined;

    //! В процессе удаления
    bool _destructing = false;    
};

} // namespace zf
