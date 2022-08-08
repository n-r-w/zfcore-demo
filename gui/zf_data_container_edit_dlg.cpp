#include "zf_data_container_edit_dlg.h"

#include "zf_view.h"
#include "zf_widget_replacer_ds.h"
#include "zf_utils.h"
#include "zf_translation.h"

namespace zf
{
DataContainerEditDialog::DataContainerEditDialog(DataContainerPtr data_container, const QString& ui_resource_name,
                                                 const QString& window_title)
    : HighlightDialog(data_container, Dialog::Type::Edit)
    , _ui_resource_name(ui_resource_name)
    , _window_title(window_title)
{
    Z_CHECK(_ui_resource_name.isEmpty() == false);
}

bool DataContainerEditDialog::run()
{
    Z_CHECK(_window_title.isEmpty() == false);
    Z_CHECK_ERROR(setUI(_ui_resource_name));

    if (!_window_title.isEmpty())
        setWindowTitle(QApplication::applicationDisplayName());
    else
        setWindowTitle(_window_title);

    return exec() == QDialogButtonBox::Ok;
}

DataContainerEditDialog::~DataContainerEditDialog()
{
}

} // namespace zf
