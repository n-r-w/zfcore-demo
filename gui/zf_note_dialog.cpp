#include "zf_note_dialog.h"
#include "zf_core.h"
#include "zf_html_tools.h"

#include <QDebug>
#include <QTextEdit>
#include <QToolBar>
#include <QVBoxLayout>

namespace zf
{
NoteDialog::NoteDialog(const QString& caption)
    : Dialog({QDialogButtonBox::Ok})
    , _layout(new QVBoxLayout)
{
    Core::registerNonParentWidget(this);

    _layout->setMargin(6);
    _layout->setSpacing(0);
    _layout->setObjectName(QStringLiteral("zfla"));
    workspace()->setLayout(_layout);

    _textedit = new QTextEdit;
    _textedit->setReadOnly(true);

    _layout->addWidget(_textedit);

    setBottomLineVisible(false);
    setModal(true);

    if (!caption.isEmpty())
        setWindowTitle(caption);

    setWindowFlags(
        Qt::Window | Qt::CustomizeWindowHint | Qt::WindowTitleHint | Qt::WindowMaximizeButtonHint | Qt::WindowCloseButtonHint | Qt::WindowSystemMenuHint);
}

NoteDialog::~NoteDialog()
{
}

void NoteDialog::setHtml(const QString& text)
{
    _textedit->setHtml(text);
}

void NoteDialog::setPlainText(const QString& text)
{
    _textedit->setPlainText(text);
}

void NoteDialog::adjustDialogSize()
{
    Utils::resizeWindowToScreen(this, QSize(800, 600));
}

} // namespace zf
