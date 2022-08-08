#include "zf_settings_dialog.h"
#include "ui_zf_settings_dialog.h"
#include "zf_framework.h"
#include "zf_translation.h"

#include <QApplication>

namespace zf
{
SettingsDialog::SettingsDialog()
    : Dialog({QDialogButtonBox::Save, QDialogButtonBox::Cancel, QDialogButtonBox::Reset})
    , ui(new Ui::SettingsDialogUI)
{
    Core::registerNonParentWidget(this);

    ui->setupUi(workspace());
    ui->example->setText(ZF_TR(ZFT_SETTINGS_FONT_PANGRAM));

    setWindowTitle(ZF_TR(ZFT_SETTINGS));
    setWindowIcon(QIcon(":/share_icons/blue/setting.svg"));

    ui->language_gb->setTitle(ZF_TR(ZFT_LANGUAGE));
    ui->ui_language_label->setText(ZF_TR(ZFT_UI_LANGUAGE));
    ui->doc_language_label->setText(ZF_TR(ZFT_DOC_LANGUAGE));

    ui->font_gb->setTitle(ZF_TR(ZFT_FONT));
    ui->font_family_label->setText(ZF_TR(ZFT_FONT_FAMILY));
    ui->font_size_label->setText(ZF_TR(ZFT_FONT_SIZE));
    ui->show_text_main_menu->setText(ZF_TR(ZFT_SHOW_TEXT_MAIN_MENU));

    for (auto l : Core::uiLanguages()) {
        ui->ui_language->addItem(QLocale(l).nativeLanguageName(), static_cast<int>(l));
    }
    for (auto l : Core::connectionInformation()->languages().keys()) {
        ui->doc_language->addItem(QLocale(l).nativeLanguageName(), static_cast<int>(l));
    }

    connect(ui->font_family_value, &QFontComboBox::currentFontChanged, this, [&]() { sl_updateExample(); });
    connect(ui->font_size_value, qOverload<int>(&QSpinBox::valueChanged), this, &SettingsDialog::sl_updateExample);

    sl_updateExample();
}

SettingsDialog::~SettingsDialog()
{
    delete ui;
}

void SettingsDialog::doLoad()
{
    QFont font = Core::fr()->localSettings()->savedSettings()->font();
    ui->font_family_value->setCurrentFont(font);
    ui->font_size_value->setValue(font.pointSize());
    ui->show_text_main_menu->setChecked(Core::fr()->localSettings()->savedSettings()->isShowMainMenuText());

    ui->ui_language->setCurrentIndex(
        ui->ui_language->findData(static_cast<int>(Core::fr()->localSettings()->savedSettings()->languageUI())));
    ui->doc_language->setCurrentIndex(
        ui->doc_language->findData(static_cast<int>(Core::fr()->localSettings()->savedSettings()->languageDoc())));
}

Error SettingsDialog::onApply()
{
    if (isChanged()) {
        QFont font = qApp->font();
        font.setFamily(ui->font_family_value->currentFont().family());
        font.setPointSize(ui->font_size_value->value());

        Core::fr()->localSettings()->savedSettings()->setFont(font);
        Core::fr()->localSettings()->savedSettings()->setShowMainMenuText(ui->show_text_main_menu->isChecked());
        Core::fr()->localSettings()->savedSettings()->setLanguage(currentLanguageUi(), currentLanguageDoc());

        Core::fr()->localSettings()->saveConfig();

        Core::info(ZF_TR(ZFT_SETTINGS_CHANGED));
    }

    return Error();
}

void SettingsDialog::onReset()
{
    QFont font = Core::fr()->localSettings()->defaultSettings()->font();
    ui->font_family_value->setCurrentFont(font);
    ui->font_size_value->setValue(font.pointSize());
    ui->show_text_main_menu->setChecked(Core::fr()->localSettings()->defaultSettings()->isShowMainMenuText());

    ui->ui_language->setCurrentIndex(
        ui->ui_language->findData(static_cast<int>(Core::fr()->localSettings()->defaultSettings()->languageUI())));
    ui->doc_language->setCurrentIndex(
        ui->doc_language->findData(static_cast<int>(Core::fr()->localSettings()->defaultSettings()->languageDoc())));
}

void SettingsDialog::sl_updateExample(int)
{
    QFont font = qApp->font();
    font.setFamily(ui->font_family_value->currentFont().family());

    double scale = Utils::screenScaleFactor();
    if (!fuzzyCompare(scale, 1))
        font.setPointSize(zf::round((double)ui->font_size_value->value() * scale));
    else
        font.setPointSize(ui->font_size_value->value());

    ui->example->setFont(font);
}

QLocale::Language SettingsDialog::currentLanguageUi() const
{
    int lang = ui->ui_language->currentIndex() >= 0 ? ui->ui_language->currentData().toInt() : -1;
    return lang >= 0 ? static_cast<QLocale::Language>(lang) : QLocale::system().language();
}

QLocale::Language SettingsDialog::currentLanguageDoc() const
{
    int lang = ui->doc_language->currentIndex() >= 0 ? ui->doc_language->currentData().toInt() : -1;
    return lang >= 0 ? static_cast<QLocale::Language>(lang) : QLocale::system().language();
}

bool SettingsDialog::isChanged() const
{
    return Core::fr()->localSettings()->savedSettings()->font().family() != ui->font_family_value->currentFont().family()
           || Core::fr()->localSettings()->savedSettings()->font().pointSize() != ui->font_size_value->value()
           || Core::fr()->localSettings()->savedSettings()->isShowMainMenuText() != ui->show_text_main_menu->isChecked()
           || Core::fr()->localSettings()->savedSettings()->languageUI() != currentLanguageUi()
           || Core::fr()->localSettings()->savedSettings()->languageDoc() != currentLanguageDoc();
}

} // namespace zf
