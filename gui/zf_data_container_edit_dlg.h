#pragma once

#include "zf_dialog.h"
#include "zf_data_container.h"
#include "zf_highlight_dialog.h"

namespace zf
{
class ZCORESHARED_EXPORT DataContainerEditDialog : public HighlightDialog
{
    Q_OBJECT
public:
    DataContainerEditDialog(DataContainerPtr data_container, const QString& _ui_resource_name, const QString& _window_title = QString());
    virtual ~DataContainerEditDialog();

    bool run();

private:
    //! Ресурс для создания UI
    QString _ui_resource_name;
    //! Заголовок окна
    QString _window_title;
};

} // namespace zf
