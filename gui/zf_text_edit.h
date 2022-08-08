#pragma once

#include "zf_global.h"
#include <QTextEdit>

namespace zf
{
class ZCORESHARED_EXPORT TextEdit : public QTextEdit
{
    Q_OBJECT
    Q_PROPERTY(int maximumLength READ maximumLength WRITE setMaximumLength DESIGNABLE true STORED true)
    Q_PROPERTY(bool readOnlyBackground READ readOnlyBackground WRITE setReadOnlyBackground DESIGNABLE true STORED true)

public:
    explicit TextEdit(QWidget* parent = nullptr);

    //! Менять ли цвет фона в зависимости от readOnly
    bool readOnlyBackground() const;
    void setReadOnlyBackground(bool b);
    //! Максимальное количество строк текста, после которых начинается скроллинг
    int maximumLength() const;
    void setMaximumLength(int length);

protected:
    void changeEvent(QEvent* event) override;
    void paintEvent(QPaintEvent* event) override;
    void keyPressEvent(QKeyEvent* e) override;
    void insertFromMimeData(const QMimeData* source) override;

private:
    //! Менять ли цвет фона в зависимости от readOnly
    bool _ro_background = true;
    //! максимально допустимое количество символов
    int _max_length = 32767;
};

} // namespace zf
