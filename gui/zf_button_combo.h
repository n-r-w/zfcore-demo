#pragma once

#include <QWidget>
#include <QComboBox>
#include <QPushButton>

#include "zf_global.h"

namespace zf
{
class BCCombo;

//! Комбобокс с кнопками
class ZCORESHARED_EXPORT ButtonCombo : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(QString text READ text WRITE setText DESIGNABLE true STORED true)
public:
    ButtonCombo(QWidget* parent = nullptr);

    //! Добавить кнопку
    int addButton(const QString& text, const QIcon& icon = QIcon(), const QVariant& data = QVariant());
    //! Удалить кнопку
    void removeButton(int index);
    //! Возвращает индекс кнопки, у которой совпадает data
    int findData(const QVariant& data) const;

    //! Задает текст кнопки
    void setItemText(int index, const QString& text);
    //! Задает иконку кнопки
    void setIcon(int index, const QIcon& icon);
    //! Задает itemData кнопки
    void setData(int index, const QVariant& data);

    //! Кол-во кнопок
    int count() const;
    //! Текст кнопки
    QString itemText(int index) const;
    //! Иконка кнопки
    QIcon itemIcon(int index) const;
    //! itemData кнопки
    QVariant itemData(int index) const;

signals:
    //! сигнал нажатия кнопки
    void sg_buttonClicked(int index);

private slots:
    //! слот для подписки на QPushButton::clicked *_button'а
    void sl_buttonClicked();
    //! слот для подписки на QComboBox::activated *_combo
    void sl_currentButtonChanged(int index);

private:
    void setText(const QString& text);
    QString text() const;

    //! наименование свойства главной кнопки, флаг isInit
    static const char *INIT_PROPERTY;
    //! наименование свойства главной кнопки, itemData
    static const char *DATA_PROPERTY;

    //! инициализация контроллов
    void createUI();

    //! главная кнопка
    QPushButton *_button;
    //! комбобокс со списком кнопок
    BCCombo *_combo;
};
} // namespace zf
