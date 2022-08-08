#pragma once

#include <QWebEngineView>
#include "zf_error.h"
#include "zf_web_engine_injection.h"

namespace zf
{
//! Объект для встраивания в веб страницу
class WebEngineViewInjectionObject : public QObject
{
    Q_OBJECT
public:
    WebEngineViewInjectionObject(
        //! Интерфейс для перехвата событий на веб странице
        WebEngineViewInjection* injection, QObject* parent);

    //! Находимся ли в режиме просмотра pdf
    Q_INVOKABLE bool isPdfView() const;
    //! Скрывать ли верхний тулбар при просмотре pdf
    Q_INVOKABLE bool isHidePdfToolbar() const;
    //! Скрывать ли панель масштаба при просмотре pdf
    Q_INVOKABLE bool isHidePdfZoom() const;

    //! Нажата кнопка
    Q_INVOKABLE void onKeyPressed(const QString& target_id, const QString& key);
    //! Нажата кнопка
    Q_INVOKABLE void onMouseClicked(const QString& target_id, bool altKey, bool ctrlKey, bool shiftKey, int button);

    //! Изменилась текущая станица (только для pdf)
    Q_INVOKABLE void onCurrentPageChanged(int current, int maximum);

    //! Задать масштабирование
    Q_SIGNAL void setFittingType(
        /*! enum WebEngineView::FittingType         
         * Page = 0,
         * Width = 1,
         * Height = 2 */
        int type);
    //! Увеличить масштаб
    Q_SIGNAL void zoomIn();
    //! Уменьшить масштаб
    Q_SIGNAL void zoomOut();

    //! Повернуть на 90 градусов по часовой стрелке
    Q_SIGNAL void rotateRight();
    //! Повернуть на 90 градусов против часовой стрелки
    Q_SIGNAL void rotateLeft();

    //! Установить текущую страницу
    Q_SIGNAL void setCurrentPage(int page);

    //! Изменились параметры
    Q_SIGNAL void optionsChanged();

private:
    //! Интерфейс для перехвата событий на веб странице
    WebEngineViewInjection* _injection;
};

//! Виджет для отображения веб страниц
class ZCORESHARED_EXPORT WebEngineView : public QWebEngineView
{
    Q_OBJECT
public:
    explicit WebEngineView(WebEngineViewInjection* injection = nullptr, QWidget* parent = nullptr);
    ~WebEngineView();

    //! Задать масштабирование
    Error setFittingType(FittingType type);

    //! Увеличить масштаб
    Error zoomIn();
    //! Уменьшить масштаб
    Error zoomOut();

    //! Повернуть на 90 градусов по часовой стрелке
    Error rotateRight();
    //! Повернуть на 90 градусов против часовой стрелки
    Error rotateLeft();

    //! Установить текущую страницу
    Error setCurrentPage(int page);

    WebEngineViewInjection* injection() const;

protected:
    void contextMenuEvent(QContextMenuEvent* event) override;

private slots:
    void sl_load_started();
    void sl_load_finished(bool ok);

signals:
    //! Нажата кнопка
    void sg_webKeyPressed(const QString& target_id, const QString& key);
    //! Нажата кнопка
    void sg_webMouseClicked(const QString& target_id, Qt::KeyboardModifiers modifiers, Qt::MouseButton button);
    //! Изменилась текущая станица (только для pdf)
    void sg_currentPageChanged(int current, int maximum);
    //! Начата загрузка страницы
    void sg_load_started();
    //! Завершена загрузка страницы
    void sg_load_finished(bool ok);

private:
    using QWebEngineView::loadFinished;
    using QWebEngineView::loadStarted;

    //! Перехват событий на веб странице
    QPointer<WebEngineViewInjection> _injection;
    QPointer<QWebChannel> _channel;
    QPointer<WebEngineViewInjectionObject> _injection_object;
    //! Собственный объект для перехвата, если не был передан внешний
    QPointer<WebEngineViewInjection> _injection_private;

    //! JS для встраивания в веб страницы
    static std::unique_ptr<QString> _web_channel_js;
};

} // namespace zf
