#include <QHBoxLayout>
#include <QListView>
#include <QApplication>

#include "zf_core.h"
#include "zf_button_combo.h"

namespace zf
{
class BCCombo : public QComboBox
{
public:
    BCCombo(QPushButton* button, QWidget* parent = nullptr);

    void showPopup() override;

private:
    QPushButton* _button;
};

const char *ButtonCombo::INIT_PROPERTY = "init_prop";
const char *ButtonCombo::DATA_PROPERTY = "data_prop";

BCCombo::BCCombo(QPushButton* button, QWidget* parent)
    : QComboBox(parent)
    , _button(button)
{
}

void BCCombo::showPopup()
{
    bool oldAnimation = QApplication::isEffectEnabled(Qt::UI_AnimateCombo);
    QApplication::setEffectEnabled(Qt::UI_AnimateCombo, false);

    QComboBox::showPopup();

    int spacingOffset = 1; // пробел между кнопкой и комбобоксом
    int new_width = _button->width() + width() - spacingOffset;
    int popup_x = mapToGlobal(QPoint(_button->pos())).x();
    QWidget *popup = this->findChild<QFrame*>();
    popup->setGeometry(popup_x - _button->width() + spacingOffset, popup->y(), new_width, popup->height());

    QApplication::setEffectEnabled(Qt::UI_AnimateCombo, oldAnimation);
}

ButtonCombo::ButtonCombo(QWidget *parent)
    : QWidget(parent)
{
    createUI();
    connect(_button, &QPushButton::clicked, this, &ButtonCombo::sl_buttonClicked);
    connect(_combo, qOverload<int>(&QComboBox::activated), this, &ButtonCombo::sl_currentButtonChanged);    
}

int ButtonCombo::addButton(const QString& text, const QIcon& icon, const QVariant& data)
{
    if (!_button->property(INIT_PROPERTY).toBool()) {
        _button->setIcon(icon);
        _button->setText(text);
        _button->setProperty(INIT_PROPERTY, QVariant(true));
        _button->setProperty(DATA_PROPERTY, data);
    } else {
        _combo->addItem(icon, text, data);
        _combo->setVisible(true);
    }

    return _combo->count();
}

void ButtonCombo::removeButton(int index)
{
    Z_CHECK(index >= 0);
    if (index == 0) {
        Z_CHECK(_button->property(INIT_PROPERTY).toBool());
        if (_combo->count() > 0) {
            _button->setIcon(_combo->itemIcon(0));
            _button->setText(_combo->itemText(0));
            _button->setProperty(DATA_PROPERTY, _combo->itemData(0));
            _combo->removeItem(0);
        } else {
            _button->setIcon(QIcon());
            _button->setText("");
            _button->setProperty(DATA_PROPERTY, QVariant());
            _button->setProperty(INIT_PROPERTY, QVariant(false));
        }
    } else {
        Z_CHECK(index <= _combo->count());
        _combo->removeItem(index - 1);
    }

    if (!_combo->count())
        _combo->setHidden(true);
}

int ButtonCombo::findData(const QVariant& data) const
{
    if (!_button->property(INIT_PROPERTY).toBool())
        return -1;

    if (Utils::compareVariant(data, _button->property(DATA_PROPERTY)))
        return 0;

    for (int i = 0; i < _combo->count(); i++) {
        if (Utils::compareVariant(data, _combo->itemData(i)))
            return i + 1;
    }
    return -1;
}

void ButtonCombo::setItemText(int index, const QString& text)
{
    Z_CHECK(index >= 0);
    if (!index)
        _button->setText(text);
    else {
        Z_CHECK(index <= _combo->count());
        _combo->setItemText(index - 1, text);
    }
}

void ButtonCombo::setIcon(int index, const QIcon& icon)
{
    Z_CHECK(index >= 0);
    if (!index)
        _button->setIcon(icon);
    else {
        Z_CHECK(index <= _combo->count());
        _combo->setItemIcon(index - 1, icon);
    }
}

void ButtonCombo::setData(int index, const QVariant& data)
{
    Z_CHECK(index >= 0);
    if (!index)
        _button->setProperty(DATA_PROPERTY, data);
    else {
        Z_CHECK(index <= _combo->count());
        _combo->setItemData(index - 1, data);
    }
}

int ButtonCombo::count() const
{
    return _button->property(INIT_PROPERTY).toBool() ? _combo->count() + 1 : 0;
}

QString ButtonCombo::itemText(int index) const
{
    Z_CHECK(index >= 0 && index <= count());
    return count() == 0 ? _button->text() : _combo->itemText(index - 1);
}

QIcon ButtonCombo::itemIcon(int index) const
{
    Z_CHECK(index >= 0 && index <= count());
    return count() == 0 ? _button->icon() : _combo->itemIcon(index - 1);
}

QVariant ButtonCombo::itemData(int index) const
{
    Z_CHECK(index >= 0 && index <= count());
    return count() == 0 ? _button->property(DATA_PROPERTY) : _combo->itemData(index - 1);
}

void ButtonCombo::createUI()
{
    setLayout(new QHBoxLayout);
    layout()->setSpacing(0);
    layout()->setMargin(0);
    _button = new QPushButton(this);
    _button->setProperty(INIT_PROPERTY, QVariant(false));
    layout()->addWidget(_button);
    _combo = new BCCombo(_button, this);
    _combo->setMinimumWidth(20);
    _combo->setMaximumWidth(20);
    QWidget *popup = qobject_cast<QWidget*>(_combo->view());
    popup->setStyleSheet("background-color: lightgray; selection-color: black; selection-background-color: lightgray;");
    _combo->setVisible(false);
    layout()->addWidget(_combo);
}

void ButtonCombo::sl_buttonClicked()
{
    emit sg_buttonClicked(0);
}

void ButtonCombo::sl_currentButtonChanged(int index)
{
    emit sg_buttonClicked(index + 1);
}

void ButtonCombo::setText(const QString& text)
{
    setItemText(0, text);
}

QString ButtonCombo::text() const
{
    return itemText(0);
}

} // namespace zf
