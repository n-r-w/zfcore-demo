#pragma once

#include "zf_error.h"
#include <QFont>

namespace zf
{
class ZCORESHARED_EXPORT LocalSettings
{
public:
    LocalSettings();
    //! Шрифт
    const QFont& font() const;
    void setFont(const QFont& font);

    //! Показывать текст под иконками главного меню
    bool isShowMainMenuText() const;
    void setShowMainMenuText(bool b);

    //! Язык интерфейса приложения
    QLocale::Language languageUI() const;
    //! Язык документооборота
    QLocale::Language languageDoc() const;
    //! Задать язык
    void setLanguage(QLocale::Language ui, QLocale::Language doc);

    //! Скопировать данные из
    void copyFrom(const LocalSettings* settings);

private:
    QFont _font;
    bool _show_main_menu_text = true;
    QLocale::Language _language_ui = QLocale::AnyLanguage;
    QLocale::Language _language_doc = QLocale::AnyLanguage;
};

/*! Настройки приложения - локальные
 * Любые изменения сохраняют их в конфиге, но не меняют текущие возвращаемые значения до пересоздания объекта */
class ZCORESHARED_EXPORT LocalSettingsManager
{
public:
    LocalSettingsManager();

    //! Текущие настройки
    LocalSettings* currentSettings() const;
    //! Настройки по умолчанию
    LocalSettings* defaultSettings() const;
    //! Настройки, сохраненные на диск
    LocalSettings* savedSettings() const;

    //! Применить настройки к приложению
    void apply();
    //! Сохранить в файл конфигурации
    void saveConfig();
    //! Загрузить из файла конфигурации
    void loadConfig();
    //! Восстановить значения по умолчанию
    void resetDefault();

private:
    //! Иницаилизация значений по умолчанию
    void loadDefault();

    std::unique_ptr<LocalSettings> _current_settings;
    std::unique_ptr<LocalSettings> _default_settings;
    std::unique_ptr<LocalSettings> _saved_settings;
};

} // namespace zf
