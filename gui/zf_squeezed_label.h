#pragma once

#include <QLabel>
#include "zf_global.h"

namespace zf
{
//! QLabel, с отрисовкой многоточия, если он не помещается по ширине
class ZCORESHARED_EXPORT SqueezedTextLabel : public QLabel
{
    Q_OBJECT
    Q_PROPERTY(QString text READ text WRITE setText DESIGNABLE true STORED true)
    Q_PROPERTY(Qt::TextElideMode textElideMode READ textElideMode WRITE setTextElideMode DESIGNABLE true STORED true)
    Q_PROPERTY(int indent READ indent WRITE setIndent DESIGNABLE true STORED true)
    Q_PROPERTY(int margin READ margin WRITE setMargin DESIGNABLE true STORED true)
    Q_PROPERTY(Qt::Alignment alignment READ alignment WRITE setAlignment DESIGNABLE true STORED true)

public:
    explicit SqueezedTextLabel(QWidget* parent = nullptr);
    explicit SqueezedTextLabel(const QString& text, QWidget* parent = nullptr);

    ~SqueezedTextLabel() override;

    QSize minimumSizeHint() const override;
    QSize sizeHint() const override;

    void setIndent(int indent);
    void setMargin(int margin);
    void setAlignment(Qt::Alignment);

    Qt::TextElideMode textElideMode() const;
    void setTextElideMode(Qt::TextElideMode mode);

    QString fullText() const;

    bool isSqueezed() const;
    QRect contentsRect() const;

public Q_SLOTS:
    void setText(const QString &text);
    void clear();

protected:    
    void mouseReleaseEvent(QMouseEvent *) override;
    void resizeEvent(QResizeEvent *) override;
    void contextMenuEvent(QContextMenuEvent*) override;

private:
    void squeezeTextToLabel();

    QString _full_text;
    Qt::TextElideMode _elide_mode = Qt::ElideRight;
};

} // namespace zf
