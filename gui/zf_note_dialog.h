#pragma once

#include "zf_dialog.h"
#include <QLabel>
#include <QVBoxLayout>

class QToolBar;
class QTextEdit;

namespace zf
{
//! Диалог с одним мемо полем
class ZCORESHARED_EXPORT NoteDialog : public Dialog
{
    Q_OBJECT
public:
    NoteDialog(const QString& caption = QString());
    ~NoteDialog() override;

    //! Задать HTML текст
    void setHtml(const QString& text);
    //! Задать обычный текст
    void setPlainText(const QString& text);

protected:
    //! Изменение размера при запуске
    void adjustDialogSize() override;

private:
    QVBoxLayout* _layout = nullptr;
    QTextEdit* _textedit = nullptr;
};

} // namespace zf
