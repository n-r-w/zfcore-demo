#pragma once

#include "zf_dialog.h"
#include "zf_uid.h"

#include <QLabel>
#include <QVBoxLayout>

namespace Ui
{
class MessageBoxUI;
}

namespace zf
{
class ZCORESHARED_EXPORT MessageBox : public Dialog
{
    Q_OBJECT
public:
    enum class MessageType
    {
        Info, //! Информационное сообщение
        Warning, //! Предупреждение
        Error, //! Ошибка
        Ask, //! Вопрос да/нет
        AskError, //! Вопрос да/нет с иконкой ошибки
        AskWarning, //! Вопрос да/нет с иконкой ошибки
        Choice, //! Выбор да,нет,отменить
        About, //! О программе
        Custom
    };

    MessageBox(MessageType message_type, const QString& text, const QString& detail = "",
        const Uid& entity_uid = Uid());
    MessageBox(MessageType message_type,
        //! Список кнопок (QDialogButtonBox::StandardButton приведенные к int)
        const QList<int>& buttons_list,
        //! Список текстовых названий кнопок
        const QStringList& buttons_text,
        //! Список ролей кнопок (QDialogButtonBox::ButtonRole приведенные к int)
        const QList<int>& buttons_role, const QString& text, const QString& detail = "",
        const Uid& entity_uid = Uid());
    ~MessageBox() override;

    MessageType messageType() const;
    QString text() const;
    QString detail() const;

    //! Основной лайаут
    QVBoxLayout* layout() const;
    //! Иконка
    QLabel* iconLabel() const;
    //! Виджет, в котором находится иконка и спейсер
    QWidget* iconWidget() const;
    //! Главная рамка
    QFrame* mainFrame() const;

    //! Вставить лайаут в нижнюю часть диалога
    void insertLayout(QLayout* la);

protected:
    //! Изменение размера при запуске
    void adjustDialogSize() override;
    //! Была нажата кнопка. Если событие обработано и не требуется стандартная реакция - вернуть true
    bool onButtonClick(QDialogButtonBox::StandardButton button) override;
    void showEvent(QShowEvent* e) override;

protected:
    //! Установить текст в диалог. Функция вынесена в протектед для тех, кто знает толк в извращениях
    void setText(const QString& text, const QString& detail);

private:
    static QList<int> generateStandardButtons(MessageType messageType);
    static QList<int> generateButtons(const QList<int>& buttonsList, const Uid& instanceUid);
    static QStringList generateButtonsText(
        const QList<int>& buttonsList, const QStringList& buttonsText, const Uid& instanceUid);
    static QList<int> generateButtonsRole(const QList<int>& buttonsList, const QList<int>& buttonsRole, const Uid& instanceUid);

    void setType(MessageType messageType);
    void init();

    Ui::MessageBoxUI* _ui;
    MessageType _message_type;
    QString _text;
    QString _detail;
    Uid _entity_uid;
};
} // namespace zf
