#pragma once

#include "zf_global.h"
#include <QDateTimeEdit>

class QToolButton;

namespace zf
{
class ZCORESHARED_EXPORT DateTimeEdit : public QDateTimeEdit
{
    Q_OBJECT

    Q_PROPERTY(bool nullable READ isNullable WRITE setNullable DESIGNABLE true STORED true)
    Q_PROPERTY(bool null READ isNull)

public:
    explicit DateTimeEdit(QWidget* parent = nullptr);
    ~DateTimeEdit();

    QDateTime dateTime() const;
    QDate date() const;
    QTime time() const;

    //! Выбран NULL
    bool isNull() const;

    //! Выбрано пустое значение
    bool isNullable() const;
    //! Допустимо пустое значение
    void setNullable(bool enable);

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

    void clear() override;

    //! Задать текст, который будет гореть вместо реального текста. Нужно например чтобы отобразить надпись "Загрузка.."
    void setOverrideText(const QString& text);

protected:
    bool eventFilter(QObject* obj, QEvent* event) override;
    QString textFromDateTime(const QDateTime& dt) const override;
    bool event(QEvent* event) override;
    void changeEvent(QEvent* e) override;
    void showEvent(QShowEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void paintEvent(QPaintEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    bool focusNextPrevChild(bool next) override;
    void focusInEvent(QFocusEvent* e) override;
    QValidator::State validate(QString& input, int& pos) const override;
    void wheelEvent(QWheelEvent* event) override;

public Q_SLOTS:
    void setDateTime(const QDateTime& dateTime);
    void setDate(const QDate& date);
    void setDate(const QVariant& date);
    void setTime(const QTime& time);

private slots:
    void sl_clearButtonClicked();
    void sl_textEdited(const QString& s);

private:
    int minimumWidth() const;
    //! Режим редактирования
    bool isReadOnlyHelper() const;

    void setNull(bool b);
    //! Обновить положение дополнительных контролов
    void updateControlsGeometry();
    //! Область кнопки очистки
    QRect clearButtonRect() const;
    //! Область кнопки выпадающего календаря или спинбокса
    QRect buttonRect() const;

    void updateClearButton();
    //! Показ календаря
    bool showPopup();

    QLineEdit* _internal_editor = nullptr;
    QToolButton* _clear_button = nullptr;
    bool _is_null = false;
    bool _is_nullable = false;
    //! Текст, который будет гореть вместо реального текста. Нужно например чтобы отобразить надпись "Загрузка.."
    QString _override_text;
};

} // namespace zf
