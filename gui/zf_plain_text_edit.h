#pragma once

#include "zf_data_structure.h"
#include <QPlainTextEdit>
#include "zf_object_extension.h"

namespace zf
{
//! В дополнение к стандартному QPlainTextEdit,
//! умеет динамически менять высоту в зависимости от содержимого
class ZCORESHARED_EXPORT PlainTextEdit : public QPlainTextEdit, public I_ObjectExtension
{
    Q_OBJECT
    Q_PROPERTY(int minimumLines READ minimumLines WRITE setMinimumLines DESIGNABLE true STORED true)
    Q_PROPERTY(int maximumLines READ maximumLines WRITE setMaximumLines DESIGNABLE true STORED true)
    Q_PROPERTY(int maximumLength READ maximumLength WRITE setMaximumLength DESIGNABLE true STORED true)
    Q_PROPERTY(bool readOnlyBackground READ readOnlyBackground WRITE setReadOnlyBackground DESIGNABLE true STORED true)

public:
    explicit PlainTextEdit(QWidget* parent = nullptr);
    ~PlainTextEdit();

public: // реализация I_ObjectExtension
    //! Удален ли объект
    bool objectExtensionDestroyed() const final;
    //! Безопасно удалить объект
    void objectExtensionDestroy() override;

    //! Получить данные
    QVariant objectExtensionGetData(
        //! Ключ данных
        const QString& data_key) const final;
    //! Записать данные
    //! Возвращает признак - были ли записаны данные
    bool objectExtensionSetData(
        //! Ключ данных
        const QString& data_key, const QVariant& value,
        //! Замещение. Если false, то при наличии такого ключа, данные не замещаются и возвращается false
        bool replace) const final;

    //! Другой объект сообщает этому, что будет хранить указатель на него
    void objectExtensionRegisterUseExternal(I_ObjectExtension* i) final;
    //! Другой объект сообщает этому, что перестает хранить указатель на него
    void objectExtensionUnregisterUseExternal(I_ObjectExtension* i) final;

    //! Другой объект сообщает этому, что будет удален и надо перестать хранить указатель на него
    //! Реализация этого метода, кроме вызова ObjectExtension::objectExtensionDeleteInfoExternal должна
    //! очистить все ссылки на указанный объект
    void objectExtensionDeleteInfoExternal(I_ObjectExtension* i) final;

    //! Этот объект начинает использовать другой объект
    void objectExtensionRegisterUseInternal(I_ObjectExtension* i) final;
    //! Этот объект прекращает использовать другой объект
    void objectExtensionUnregisterUseInternal(I_ObjectExtension* i) final;

public:
    //! Менять ли цвет фона в зависимости от readOnly
    bool readOnlyBackground() const;
    void setReadOnlyBackground(bool b);

    //! Максимальное количество строк текста, после которых начинается скроллинг
    int maximumLines() const;
    void setMaximumLines(int n);
    //! Минимальное количество строк текста
    int minimumLines() const;
    void setMinimumLines(int n);
    //! Максимальное количество строк текста, после которых начинается скроллинг
    int maximumLength() const;
    void setMaximumLength(int length);

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

    void setStyleSheet(const QString& style_sheet);

protected:
    bool event(QEvent* e) override;
    void changeEvent(QEvent* event) override;
    void paintEvent(QPaintEvent* event) override;
    void scrollContentsBy(int dx, int dy) override;
    void keyPressEvent(QKeyEvent* e) override;
    void focusInEvent(QFocusEvent* e) override;
    void focusOutEvent(QFocusEvent* e) override;
    void insertFromMimeData(const QMimeData* source) override;

private slots:
    void sl_textChanged();
    void sl_scroll(int value);
    //! Обратный вызов
    void sl_callback(int key, const QVariant& data);

private:
    bool isSingleLine(int height) const;
    void updateScrollbar();
    void updateStyle();

    //! Менять ли цвет фона в зависимости от readOnly
    bool _ro_background = true;
    //! Максимальное количество строк текста, после которых начинается скроллинг
    int _maximum_lines = 10;
    //! Минимальное количество строк текста
    int _minimum_lines = 1;
    //! максимально допустимое количество символов
    int _max_length = 32767;
    //! Стиль установлен извне
    bool _external_style_sheet = false;

    //! Потокобезопасное расширение возможностей QObject
    ObjectExtension* _object_extension = nullptr;
};

} // namespace zf
