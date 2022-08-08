#pragma once

#include "zf_global.h"
#include <QLineEdit>

namespace zf
{
class ZCORESHARED_EXPORT LineEdit : public QLineEdit
{
    Q_OBJECT
    Q_PROPERTY(bool readOnly READ isReadOnly WRITE setReadOnly)

public:
    explicit LineEdit(QWidget* parent = nullptr);
    explicit LineEdit(const QString& text, QWidget* parent = nullptr);

    //! Стандартное поведение readOnly
    bool isStandardReadOnlyBehavior() const;
    void setStandardReadOnlyBehavior(bool b);

    bool isReadOnly() const;
    void setReadOnly(bool b);

    void setText(const QString& s);

protected:
    void changeEvent(QEvent* event) override;
    void paintEvent(QPaintEvent* event) override;
    void focusInEvent(QFocusEvent* e) override;
    void focusOutEvent(QFocusEvent* e) override;
    void contextMenuEvent(QContextMenuEvent* event) override;
    void keyPressEvent(QKeyEvent* kEvent) override;

    virtual void onContextMenuCreated(QMenu* m);

private slots:
    void sl_deleteSelected();

private:
    void updateStyle();
    QMenu* createStandardContextMenu();

    bool _standard_ro_behavior = true;
    bool _read_only = false;
};

} // namespace zf
