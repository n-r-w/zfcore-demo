#pragma once

#include <QAccessibleWidget>
#include <QAccessibleTableInterface>
#include <QAccessibleObject>

namespace zf
{
class PictureSelector;

// Регистрация фабрики UI Automation в Framework::bootstrapAccessibility()

//! Интерфейс для UI Automation
class PictureSelectorAccessibilityInterface : public QAccessibleWidget
{
public:
    static QAccessibleInterface* accessibleFactory(const QString& class_name, QObject* object);

    PictureSelectorAccessibilityInterface(QWidget* w);
    int childCount() const override;
    QAccessibleInterface* child(int index) const override;
    int indexOfChild(const QAccessibleInterface* child) const override;
    QString text(QAccessible::Text t) const override;
    QStringList actionNames() const override;
    QString localizedActionDescription(const QString& action_name) const override;
    void doAction(const QString& action_name) override;
    QStringList keyBindingsForAction(const QString&) const override;
    QAccessibleInterface* childAt(int x, int y) const override;

private:
    PictureSelector* selector() const;
};
} // namespace zf
