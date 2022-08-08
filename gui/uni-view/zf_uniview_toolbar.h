#pragma once

#include <QWidget>
#include "zf.h"
#include <QIcon>

class QFrame;
class QToolButton;
class QSpacerItem;
class QLayoutItem;
class QToolButton;
class QFrame;

namespace Ui
{
class UniViewToolbarUI;
}

namespace zf
{
class UniViewWidget;

//! Тулбар для компонента UniViewWidget
class ZCORESHARED_EXPORT UniViewToolbar : public QWidget
{
    Q_OBJECT
public:
    UniViewToolbar(UniViewWidget* uniview, QWidget* parent = nullptr);
    ~UniViewToolbar();

    //! Рамка вокруг тулбара
    QFrame* border() const;

    //! Добавить произвольную кнопку. Универсальный метод
    QToolButton* addButton(const QString& translation_id, const QIcon& icon = QIcon(), Qt::ToolButtonStyle style = Qt::ToolButtonIconOnly);
    //! Добавить произвольную без текста
    QToolButton* addButton(const QIcon& icon);

    //! Добавить вертикальую линию
    QFrame* addLine();
    //! Добавить разделитель
    QSpacerItem* addSpacer(int width = SPACER_WIDTH, QSizePolicy::Policy policy = QSizePolicy::Fixed);

    //! Параметры по умолчанию
    UniviewParameters defaultParameters() const;
    void setDefaultParameters(const UniviewParameters& params);

    //! Имеет ли видимые кнопки и прочие элементы
    bool hasVisibleControls() const;

signals:
    void sg_buttonClicked(QToolButton* b);

private:
    void testEnabled();
    void setFitMode(FittingType mode);

    void addWidget(QWidget* w);
    void addItem(QLayoutItem* item);

    //! Обновить состояние текущей и максимальной страницы
    void updatePageNum(int current, int maximum);

    UniViewWidget* _uni_view;
    Ui::UniViewToolbarUI* _ui;
    FittingType _fit_mode = FittingType::Undefined;
    bool _by_user = false;

    //! Параметры по умолчанию
    UniviewParameters _default_parameters;

    //! Рамка
    QFrame* _border;
    //! Дополнительные экшены
    QList<QToolButton*> _buttons;

    //! Задержка перед реакцией на изменение значений пользователем
    inline static const int EDIT_TIMEOUT_MS = 500;
    //! Ширина разделителя между кнопками
    inline static const int SPACER_WIDTH = 10;
};

} // namespace
