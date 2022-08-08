
#pragma once

#include <QWidget>
#include "zf_error.h"
#include "zf_web_engine_injection.h"
#include "zf_webengineview.h"

namespace zf
{
class WebEngineView;

class WebViewWidget : public QWidget
{
    Q_OBJECT
public:
    WebViewWidget(
        //! Объект для перехвата событий веб страницы
        WebEngineViewInjection* injection = nullptr, QWidget* parent = nullptr);

    //! Загрузка файла PDF или HTML
    zf::Error loadFile(const QString& file_name);
    //! Загрузка HTML из строки
    void loadHtmlString(const QString& content);
    //! Загрузка HTML из URL
    void loadHtmlUrl(const QUrl& url);

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

    WebEngineView* view() const;
    WebEngineViewInjection* injection() const;

#ifdef Q_OS_LINUX
    zf::Error loadWord(const QString& file_name);
    zf::Error loadExcel(const QString& file_name);
#endif

signals:
    void sg_load_finished(const zf::Error& error);

    //! Нажата кнопка
    void sg_webKeyPressed(const QString& target_id, const QString& key);
    //! Нажата кнопка
    void sg_webMouseClicked(const QString& target_id, Qt::KeyboardModifiers modifiers, Qt::MouseButton button);
    //! Изменилась текущая станица (только для pdf)
    void sg_currentPageChanged(int current, int maximum);

private slots:
    void sl_load_finished(bool ok);

private:
    WebEngineView* _view = nullptr;
};

} // namespace zf
