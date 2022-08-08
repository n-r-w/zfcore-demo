#pragma once

#include "zf_dialog.h"

namespace Ui {
class SettingsDialogUI;
}

namespace zf
{
class SettingsDialog : public Dialog
{
    Q_OBJECT

public:
    explicit SettingsDialog();
    ~SettingsDialog();

protected:
    //! Загрузить состояние диалога
    void doLoad() override;
    //! Применить новое состояние диалога
    Error onApply() override;
    //! Активирована кнопка reset
    void onReset() override;

private slots:
    void sl_updateExample(int = 0);

private:
    QLocale::Language currentLanguageUi() const;
    QLocale::Language currentLanguageDoc() const;

    bool isChanged() const;

    Ui::SettingsDialogUI* ui;
};

} // namespace zf
