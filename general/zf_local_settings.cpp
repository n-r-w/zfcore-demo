#include "zf_local_settings.h"
#include "zf_core.h"
#include "zf_framework.h"
#include <QApplication>

#define FONT_FAMILY "FONT_FAMILY"
#define FONT_SIZE "FONT_SIZE"

#define SHOW_MAIN_MENU_TEXT "SHOW_MAIN_MENU_TEXT"
#define SHOW_MAIN_MENU_TEXT_DEFAULT true

#define LANGUAGE_UI "LANGUAGE_UI"
#define LANGUAGE_DOC "LANGUAGE_DOC"

#define DEFAULT_FONT_SIZE 8

namespace zf
{
LocalSettings::LocalSettings()
{
    if (Core::mode().testFlag(CoreMode::Application)) {
        _font = qApp->font();
    }
}

const QFont& LocalSettings::font() const
{
    return _font;
}

void LocalSettings::setFont(const QFont& font)
{
    _font = font;
}

bool LocalSettings::isShowMainMenuText() const
{
    return _show_main_menu_text;
}

void LocalSettings::setShowMainMenuText(bool b)
{
    _show_main_menu_text = b;
}

QLocale::Language LocalSettings::languageUI() const
{
    return _language_ui;
}

QLocale::Language LocalSettings::languageDoc() const
{
    return _language_doc;
}

void LocalSettings::setLanguage(QLocale::Language ui, QLocale::Language doc)
{
    _language_ui = ui;
    _language_doc = doc;
}

void LocalSettings::copyFrom(const LocalSettings* settings)
{
    Z_CHECK_NULL(settings);
    _font = settings->_font;
    _show_main_menu_text = settings->_show_main_menu_text;
    _language_ui = settings->_language_ui;
    _language_doc = settings->_language_doc;
}

void LocalSettingsManager::apply()
{
    Core::fr()->setLanguage(currentSettings()->languageUI(), currentSettings()->languageDoc());

    if (Core::mode().testFlag(CoreMode::Application)) {
        QFont font = currentSettings()->font();

        double scale = Utils::screenScaleFactor();
        if (!fuzzyCompare(scale, 1)) {
            if (font.pixelSize() > 0)
                font.setPixelSize(zf::round((double)font.pixelSize() * scale));
            else
                font.setPointSize(zf::round((double)font.pointSize() * scale));
        }

        qApp->setFont(font);

        Core::fr()->setToolbarStyle(ToolbarType::Main,
                                    currentSettings()->isShowMainMenuText() ? Qt::ToolButtonTextUnderIcon : Qt::ToolButtonIconOnly,
                                    currentSettings()->isShowMainMenuText() ? Utils::scaleUI(24) : Utils::scaleUI(40));
        Core::fr()->setToolbarStyle(ToolbarType::Window, Qt::ToolButtonIconOnly, Utils::scaleUI(24));
    }
}

void LocalSettingsManager::resetDefault()
{
    currentSettings()->setLanguage(QLocale::system().language(), QLocale::system().language());
    if (Core::mode().testFlag(CoreMode::Application)) {
        currentSettings()->setFont(qApp->font());
        currentSettings()->setShowMainMenuText(SHOW_MAIN_MENU_TEXT_DEFAULT);
    }
}

void LocalSettingsManager::loadDefault()
{
    _default_settings->setLanguage(QLocale::system().language(), QLocale::system().language());
    if (Core::mode().testFlag(CoreMode::Application)) {
        _default_settings->setFont(qApp->font());
        _default_settings->setShowMainMenuText(SHOW_MAIN_MENU_TEXT_DEFAULT);
    }
}

void LocalSettingsManager::saveConfig()
{
    if (Core::mode().testFlag(CoreMode::Application)) {
        if (savedSettings()->font().family() != defaultSettings()->font().family())
            Core::fr()->coreConfiguration()->setValue(FONT_FAMILY, savedSettings()->font().family());
        else
            Core::fr()->coreConfiguration()->remove(FONT_FAMILY);

        if (savedSettings()->font().pointSize() != defaultSettings()->font().pointSize())
            Core::fr()->coreConfiguration()->setValue(FONT_SIZE, savedSettings()->font().pointSize());
        else
            Core::fr()->coreConfiguration()->remove(FONT_SIZE);

        if (savedSettings()->isShowMainMenuText() != defaultSettings()->isShowMainMenuText())
            Core::fr()->coreConfiguration()->setValue(SHOW_MAIN_MENU_TEXT, savedSettings()->isShowMainMenuText());
        else
            Core::fr()->coreConfiguration()->remove(SHOW_MAIN_MENU_TEXT);

        if (savedSettings()->languageUI() != defaultSettings()->languageUI())
            Core::fr()->coreConfiguration()->setValue(LANGUAGE_UI, static_cast<int>(savedSettings()->languageUI()));
        else
            Core::fr()->coreConfiguration()->remove(LANGUAGE_UI);

        if (savedSettings()->languageDoc() != defaultSettings()->languageDoc())
            Core::fr()->coreConfiguration()->setValue(LANGUAGE_DOC, static_cast<int>(savedSettings()->languageDoc()));
        else
            Core::fr()->coreConfiguration()->remove(LANGUAGE_DOC);
    }
}

void LocalSettingsManager::loadConfig()
{
    if (Core::mode().testFlag(CoreMode::Application)) {
        QFont font = qApp->font();
        font.setFamily(Core::fr()->coreConfiguration()->value(FONT_FAMILY, font.family()).toString());
        font.setPointSize(Core::fr()->coreConfiguration()->value(FONT_SIZE, DEFAULT_FONT_SIZE).toInt());
        savedSettings()->setFont(font);

        savedSettings()->setShowMainMenuText(
            Core::fr()->coreConfiguration()->value(SHOW_MAIN_MENU_TEXT, SHOW_MAIN_MENU_TEXT_DEFAULT).toBool());

        savedSettings()->setLanguage(
            static_cast<QLocale::Language>(Core::fr()->coreConfiguration()->value(LANGUAGE_UI, QLocale::system().language()).toInt()),
            static_cast<QLocale::Language>(Core::fr()->coreConfiguration()->value(LANGUAGE_DOC, QLocale::system().language()).toInt()));
    }
}

LocalSettingsManager::LocalSettingsManager()
{
    _default_settings = std::make_unique<LocalSettings>();
    _saved_settings = std::make_unique<LocalSettings>();
    _current_settings = std::make_unique<LocalSettings>();

    loadDefault();
    loadConfig();

    // текущие изначально копируются из файла
    _current_settings->copyFrom(_saved_settings.get());
}

LocalSettings* LocalSettingsManager::currentSettings() const
{
    return _current_settings.get();
}

LocalSettings* LocalSettingsManager::defaultSettings() const
{
    return _default_settings.get();
}

LocalSettings* LocalSettingsManager::savedSettings() const
{
    return _saved_settings.get();
}

} // namespace zf
