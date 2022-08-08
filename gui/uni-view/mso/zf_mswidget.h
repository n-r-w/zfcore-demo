#pragma once

#include <QWidget>
#include "zf_error.h"

class MSOControl;

namespace zf
{
/*! Виджет для отображения MS Word, MS Excel
 * Обертка вокруг MSOControl (взят из experium), который в свою очередь основан на DSOFramer от microsoft */
class MSWidget : public QWidget
{    
public:
    enum class ViewType
    {
        Undefined,
        PrintView,
        WebView,
        PrintPreview
    };

    //! Инициализация данных, необходимых для работы
    static void initEngine();
    //! Освобождение ресурсов
    static void clearEngine();

    explicit MSWidget(QWidget* parent = nullptr);
    ~MSWidget() override;

    Error loadWord(const QString& file_name, bool edit = false);
    Error loadExcel(const QString& file_name, bool edit = false);

    void clear();

    //! В режиме редактирования
    bool isEdit() const;

    //! Режим отображения
    ViewType viewType() const;
    bool setViewType(ViewType t);

    //! Вывод на печать
    bool print();

    //! Имеются несохраненные изменения
    bool isDirty() const;
    //! Сохранить под новым именем
    Error saveAs(const QString& file_name);

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

protected:
    bool nativeEvent(const QByteArray& eventType, void* message, long* result) override;
    bool event(QEvent* event) override;
    bool eventFilter(QObject* watched, QEvent* event) override;
    void hideEvent(QHideEvent* e) override;
    void changeEvent(QEvent* e) override;
    void resizeEvent(QResizeEvent* e) override;
    void paintEvent(QPaintEvent* e) override;
    void showEvent(QShowEvent* e) override;
    void focusInEvent(QFocusEvent* e) override;
    void focusOutEvent(QFocusEvent* e) override;
    QPaintEngine* paintEngine() const override;

private:
    MSOControl* _container = nullptr;
    bool _read_only = false;
};

} // namespace zf
