#pragma once

#include <QPushButton>
#include <QDialog>
#include <QDialogButtonBox>
#include <QMap>
#include <QPointer>
#include <QHBoxLayout>

#include "zf_global.h"
#include "zf_defs.h"
#include "zf_error.h"
#include "zf_utils.h"
#include "zf_object_extension.h"

namespace Ui
{
class ZFDialog;
}

namespace zf
{
class ObjectsFinder;

//! Базовый класс всех модальных диалогов
class ZCORESHARED_EXPORT Dialog : public QDialog, public I_ObjectExtension
{
    Q_OBJECT
public:
    //! Тип диалога
    enum class Type
    {
        //! Просмотр (кнопка ОК)
        View,
        //! Редактирование (кнопки Сохранить/Отменить)
        Edit,
    };

    //! Если все параметры не заданы, то генерируется кнопка OK
    explicit Dialog(
        //! Список кнопок (QDialogButtonBox::StandardButton приведенные к int)
        const QList<int>& buttons_list = QList<int>(),
        //! Список текстовых названий кнопок
        const QStringList& buttons_text = QStringList(),
        //! Список ролей кнопок (QDialogButtonBox::ButtonRole приведенные к int)
        const QList<int>& buttons_role = QList<int>());
    explicit Dialog(Type type);
    ~Dialog() override;

public: // реализация I_ObjectExtension
    //! Удален ли объект
    bool objectExtensionDestroyed() const final;
    ;
    //! Безопасно удалить объект
    void objectExtensionDestroy() override;

    //! Получить данные
    QVariant objectExtensionGetData(
        //! Ключ данных
        const QString& data_key) const final;
    //! Записать данные
    //! Возвращает признак - были ли записаны данные
    bool objectExtensionSetData(
        //! Ключ данных
        const QString& data_key, const QVariant& value,
        //! Замещение. Если false, то при наличии такого ключа, данные не замещаются и возвращается false
        bool replace) const final;

    //! Другой объект сообщает этому, что будет хранить указатель на него
    void objectExtensionRegisterUseExternal(I_ObjectExtension* i) final;
    //! Другой объект сообщает этому, что перестает хранить указатель на него
    void objectExtensionUnregisterUseExternal(I_ObjectExtension* i) final;

    //! Другой объект сообщает этому, что будет удален и надо перестать хранить указатель на него
    //! Реализация этого метода, кроме вызова ObjectExtension::objectExtensionDeleteInfoExternal должна
    //! очистить все ссылки на указанный объект
    void objectExtensionDeleteInfoExternal(I_ObjectExtension* i) override;

    //! Этот объект начинает использовать другой объект
    void objectExtensionRegisterUseInternal(I_ObjectExtension* i) final;
    //! Этот объект прекращает использовать другой объект
    void objectExtensionUnregisterUseInternal(I_ObjectExtension* i) final;

public:
    void open() override;
    int exec() override;

    /* ВАЖНО. Чтобы была единая логика поведения, закрывать диалог лучше через метод
     * activateButton(QDialogButtonBox::Ok/Cancel), т.е. имитировать нажатие на кнопки.
     * В таком случае можно прописать общую логику в методы onApply/onCancel и т.п.
     * Закрытие через accept/reject/done просто закроет диалог без всякой обработки */
    //! Активировать нажатие на кнопку
    bool activateButton(QDialogButtonBox::StandardButton buttonType);

    //! Имеется ли активный модальный диалог (запущенный через exec)
    static bool isPureModalExists();

    //! Диалог в процессе открытия
    bool isLoading() const;

    //! Переопределение методов с контролем превышения размера экрана
    void resize(int w, int h);
    void resize(const QSize& size);

    //! Автоматическое сохранение и восстановление размера и т.п.
    bool isAutoSaveConfiguration() const;
    void setAutoSaveConfiguration(bool b);

    //! Вернуть текущую конфигурацию
    virtual QMap<QString, QVariant> getConfiguration() const;
    //! Применить конфигурацию
    virtual Error applyConfiguration(const QMap<QString, QVariant>& config);
    //! Вызывается после загрузки конфигурацию (пользовательские настройки окна).
    //! Когда вызван этот метод, окно уже восстановило положения сплиттеров, размер и т.п.
    virtual void afterLoadConfiguration();

    //! Уникальный код диалога для сохранения/восстановления его конфигурации (положение на экране и т.п.)
    QString dialogCode() const;
    void setDialogCode(const QString& code);

    //! Размер диалога по умолчанию
    void setDefaultSize(const QSize& size);
    void setDefaultSize(int x, int y);

    //! Рабочая область диалога
    QWidget* workspace() const;

    //! Линия разделения кнопок и рабочей области
    bool isBottomLineVisible() const;
    void setBottomLineVisible(bool b);

    //! Запретить закрытие диалога по нажатию ENTER, если фокус не на кнопке OK
    void disableDefaultAction();
    bool isDisableDefaultAction() const;
    //! Запретить закрытие диалога по нажатию ESCAPE
    void disableEscapeClose();
    bool isDisableEscapeClose() const;

    //! Запретить закрытие диалога по нажатию на "крестик"
    void disableCrossClose(bool b);
    bool isDisableCrossClose() const;

    //! Изменяемый размер
    bool isResizable() const;
    void setResizable(bool b);

    //! Задать порядок виджетов на форме
    virtual void setTabOrder(const QList<QWidget*>& tab_order);
    //! Порядок виджетов на форме
    const QList<QWidget*>& tabOrder() const;

    //! Центрировать диалог относительно текущего экрана
    void toScreenCenter();

    //! Управляющий блок кнопок
    QDialogButtonBox* buttonBox() const;
    //! Набор управляющих кнопок
    QDialogButtonBox::StandardButtons buttonRoles() const;
    //! Все кнопки, для которых заданы роли
    QList<QPushButton*> buttons() const;
    //! Все кнопки, включая просто добавленные на форму (не в QDialogButtonBox)
    QList<QPushButton*> allButtons() const;

    //! Кнопка по типу
    QPushButton* button(QDialogButtonBox::StandardButton buttonType) const;
    //! Роль кнопки
    QDialogButtonBox::StandardButton role(QPushButton* button) const;

    //! Layout в котором находится QDialogButtonBox
    QHBoxLayout* buttonsLayout() const;

    //! Принудительно задать порядок кнопок
    void setButtonLayoutMode(QDialogButtonBox::ButtonLayout layout);

    //! Загрузить форму (ui файл) из ресурсов и установить ее в workspace.
    //! При необходимости загрузки ui по частям, надо использовать Utils::openUI
    Error setUI(const QString& resource_name);
    //! Загрузить форму (ui файл) из ресурсов и установить ее в workspace.
    //! При необходимости загрузки ui по частям, надо использовать Utils::openUI
    Error setUI(const QString& resource_name, QWidget* target);

    /*! Найти на форме объект класса T по имени */
    template <class T>
    T* object(
        //! Путь к объекту. Последний элемент - имя самого объекта, далее его родитель и т.д.
        //! Родителей можно пропускать. Главное чтобы на форме не было двух объектов, подходящих по критерию поиска
        const QString& name) const
    {
        return object<T>(QStringList(name));
    }
    /*! Найти на форме объект класса T по пути(списку имен самого объекта и его родителей) к объекту */
    template <class T>
    T* object(
        //! Путь к объекту. Последний элемент - имя самого объекта, далее его родитель и т.д.
        //! Родителей можно пропускать. Главное чтобы на форме не было двух объектов, подходящих по критерию поиска
        const QStringList& path) const
    {
        QList<QObject*> objects = findObjectsHelper(path, T::staticMetaObject.className());
        Z_CHECK_X(!objects.isEmpty(),
                  QString("Object %1:%2 not found").arg(T::staticMetaObject.className()).arg(path.join("\\")));
        Z_CHECK_X(objects.count() == 1, QString("Object %1:%2 found more then once (%4)")
                                            .arg(T::staticMetaObject.className())
                                            .arg(path.join("\\"))
                                            .arg(objects.count()));
        T* res = qobject_cast<T*>(objects.first());
        Z_CHECK_X(res != nullptr,
                  QString("Casting error %1:%2").arg(T::staticMetaObject.className()).arg(path.join("\\")));

        return res;
    }

    /*! Существует ли на форме объект класса T по имени */
    template <class T> bool objectExists(const QString& name) const { return objectExists<T>(QStringList(name)); }
    /*! Существует ли на форме объект класса T по пути(списку имен самого объекта и его родителей) к объекту */
    template <class T> bool objectExists(const QStringList& path) const
    {
        return !findObjectsHelper(path, reinterpret_cast<T>(0)->staticMetaObject.className()).isEmpty();
    }

    //! Доступно ли закрытие по Ок
    bool isOkButtonEnabled() const;
    void setOkButtonEnabled(bool b);

    //! Подсветить кнопку цветом
    void setButtonHighlightFrame(QDialogButtonBox::StandardButton button, InformationType info_type);
    //! Цвет подсветки кнопки
    InformationType buttonHighlightFrame(QDialogButtonBox::StandardButton button) const;
    //! Убрать подсветку кнопки цветом
    void removeButtonHighlightFrame(QDialogButtonBox::StandardButton button);

    //! Кнопка ОК, Сохранить, Применить
    QPushButton* okButton() const;

    /*! Завершить работу диалога с признаком
     *     либо QDialogButtonBox::Ok (если такая кнопка есть)
     *     либо QDialog::Accepted (если кнопки QDialogButtonBox::Ok нет)
     * Результат возвращает метод exec */
    void accept() override;
    /*! Завершить работу диалога с признаком
     *     либо QDialogButtonBox::Cancel (если такая кнопка есть)
     *     либо QDialog::Rejected (если кнопки QDialogButtonBox::Ok нет)
     * Результат возвращает метод exec */
    void reject() override;
    //! Завершить работу диалога с произвольным кодом возврата
    void done(int code) override;

    //! Инициировать нажатие на кнопку
    void click(QDialogButtonBox::StandardButton button);

    //! Заменить автосоздаваемые виджеты в workspace
    void replaceWidgets(DataWidgets* data_widgets);
    void replaceWidgets(DataWidgetsPtr data_widgets);

protected:
    //! Формирование списка кнопок Ok и Cancel
    QList<int> buttonsOkCancel() const;

    /*! Создать пользовательский интерфейс путем формирования главного виджета body() */
    virtual void createUI();

    //! Изменение размера при запуске
    virtual void adjustDialogSize();

    //! Загрузить состояние диалога. Не вызывать
    virtual void doLoad();
    //! Очистить данные при закрытии диалога. Не вызывать
    virtual void doClear();

    //! Применить новое состояние диалога. Не вызывать
    virtual Error onApply();
    bool isApplying() const;

    //! Отменить изменения диалога. Не вызывать
    virtual void onCancel();
    bool isCanceling() const;

    //! Активирована кнопка reset. Не вызывать
    virtual void onReset();
    bool isReseting() const;

    //! Была нажата кнопка. Если событие обработано и не требуется стандартная реакция - вернуть true
    virtual bool onButtonClick(QDialogButtonBox::StandardButton button);
    //! Закрытие через крестик. Если возвращает false, то закрытие отменяется
    virtual bool onCloseByCross();
    //! Текущая обрабатываемая кнопка
    QDialogButtonBox::StandardButton processingButton() const;
    //! Текущая обрабатываемая роль
    QDialogButtonBox::ButtonRole processingButtonRole() const;

    //! Перед отображением диалога. Не вызывать
    virtual void beforePopup();
    //! После отображения диалога. Не вызывать
    virtual void afterPopup();
    //! Перед скрытием диалога. Не вызывать
    virtual void beforeHide();
    //! После скрытия диалога. Не вызывать
    virtual void afterHide();
    //! Выполняется после инициализации всех конструкторов, в момент когда Qt переходит в event loop. Не вызывать
    virtual void afterCreateDialog();

    //! Запросить проверку на доступность кнопок.
    //! При множественном вызове этого метода, checkButtonsEnabled вызовется только один раз
    void requestCheckButtonsEnabled();
    //! Проверить на доступность кнопки диалога. Не вызывать
    virtual void checkButtonsEnabled();

    //! Служебный обработчик менеджера обратных вызовов (не использовать)
    virtual void onCallbackManagerInternal(int key, const QVariant& data);

    //! Задать нестандартную реакцию на кнопку
    void setCustomRole(QDialogButtonBox::StandardButton button, QDialogButtonBox::ButtonRole role);
    QDialogButtonBox::ButtonRole customRole(QDialogButtonBox::StandardButton button) const;

    //! Переопределение метода QDialog
    void setVisible(bool visible) override;
    bool eventFilter(QObject* obj, QEvent* e) override;
    void moveEvent(QMoveEvent* e) override;
    void resizeEvent(QResizeEvent* e) override;
    bool event(QEvent* event) override;
    void showEvent(QShowEvent* e) override;
    void closeEvent(QCloseEvent* e) override;
    void keyPressEvent(QKeyEvent* e) override;
    bool nativeEvent(const QByteArray& eventType, void* message, long* result) override;

    static void defaultButtonOptions(QDialogButtonBox::StandardButton button, QString& text,
                                     QDialogButtonBox::ButtonRole& role, QIcon& icon);
    static void getDefaultButtonOptions(
        //! Список управляющих кнопок
        const QList<int>& buttonsList,
        //! Список подписей кнопок
        QStringList& buttonsTexts,
        //! Список ролей кнопок
        QList<int>& buttonsRoles);

    //! Запрос автоматической расстановки табуляции
    void requestInitTabOrder();

    //! Надо ли сохранять конфигурацию для сплиттера. По умолчанию все сплитеры сохраняют свое положение
    virtual bool isSaveSplitterConfiguration(const QSplitter* splitter) const;

    //! Подключено встроенное представление
    virtual void embedViewAttached(View* view);
    //! Отключено встроенное представление
    virtual void embedViewDetached(View* view);
    //! Вызывается когда произошла ошибка во встроенном представлении при загрузке, сохранении или удалении объекта
    virtual void onEmbedViewError(View* view, zf::ObjectActionType type, const Error& error);

    //! Вызывается когда добавлен новый объект
    virtual void onObjectAdded(QObject* obj);
    //! Вызывается когда объект удален из контейнера (изъят из контроля или удален из памяти)
    //! ВАЖНО: при удалении объекта из памяти тут будет только указатель на QObject, т.к. деструкторы наследника уже сработают, поэтому нельзя делать qobject_cast
    virtual void onObjectRemoved(QObject* obj);

    //! Сменилась видимость нижней линии
    virtual void onChangeBottomLineVisible();

protected:
    //! Создание ui при необходимости. Не переопределять и не вызывыать
    virtual void createUI_helper() const;

signals:
    //! Отображение
    void sg_show();
    //! Скрытие
    void sg_hide();

private slots:
    void sl_clicked(QAbstractButton* b);
    //! Менеджер обратных вызовов
    void sl_callbackManager(int key, const QVariant& data);

    //! Вызывается когда добавлен новый объект
    void sl_containerObjectAdded(QObject* obj);
    //! Вызывается когда объект удален из контейнера (изъят из контроля или удален из памяти)
    //! ВАЖНО: при удалении объекта из памяти тут будет только указатель на QObject, т.к. деструкторы наследника уже сработают, поэтому нельзя делать qobject_cast
    void sl_containerObjectRemoved(QObject* obj);   

private:
    //! Подготовить виджет к работе
    void prepareWidget(QWidget* w);
    //! Создать кнопку
    void createButton(QDialogButtonBox::StandardButton button, const QString& text, QDialogButtonBox::ButtonRole role);
    //! Поиск элемента по имени
    QList<QObject*> findObjectsHelper(
        //! Путь к объекту. Последний элемент - имя самого объекта, далее его родитель и т.д.
        const QStringList& path, const char* class_name) const;
    //! Вызывается после создания диалога
    void afterCreatedInternal();

    //! Вернуть текущую конфигурацию
    QMap<QString, QVariant> getConfigurationInternal() const;
    //! Применить конфигурацию
    Error applyConfigurationInternal(const QMap<QString, QVariant>& config);

    //! Список кнопок (QDialogButtonBox::StandardButton приведенные к int)
    static QList<int> buttonsListForType(Type t);
    //! Список текстовых названий кнопок
    static QStringList buttonsTextForType(Type t);
    //! Список ролей кнопок (QDialogButtonBox::ButtonRole приведенные к int)
    static QList<int> buttonsRoleForType(Type t);

    //! Подключено встроенное представление
    void embedViewAttachedHelper(View* view);
    //! Отключено встроенное представление
    void embedViewDetachedHelper(View* view);

    //! Автоматическая расстановка табуляции
    void initTabOrder();

    //! Набор управляющих кнопок
    QDialogButtonBox::StandardButtons _button_roles;
    QList<QPushButton*> _buttons;

    //! Список управляющих кнопок
    QList<int> _buttons_list;
    //! Список подписей кнопок
    QStringList _buttons_text;
    //! Список ролей кнопок
    QList<int> _buttons_role;

    //! Связь между кнопкой и ее кодом
    QHash<QDialogButtonBox::StandardButton, QPushButton*> _button_codes;
    //! Нестандартные роли для кнопок
    QMap<QDialogButtonBox::StandardButton, QDialogButtonBox::ButtonRole> _role_mapping;
    //! Цвет подсветки кнопки
    QMap<QDialogButtonBox::StandardButton, InformationType> _color_frames;

    Ui::ZFDialog* _ui;
    QPointer<QWidget> _workspace;

    //! Был ли подгон размера окна при старте
    bool _is_adjusted;

#ifndef Q_WS_WIN
    bool _is_resizable;
#endif
    //! Запущен через exec
    bool _pure_modal;

    //! Инициализирован
    bool _initialized;
    //! В процессе удаления
    bool _destroying = false;
    //! Автоматическое сохранение и восстановление размера и т.п.
    bool _auto_save_config = true;
    //! В процессе загрузки
    bool _is_loading = false;

    //! Уникальный код диалога для сохранения/восстановления его конфигурации (положение на экране и т.п.)
    QString* _dialog_code = nullptr;
    //! Размер диалога по умолчанию
    QSize _default_size;

    //! Запретить закрытие диалога по нажатию ESCAPE
    bool _disable_escape_close = false;
    //! Запретить закрытие диалога по нажатию ENTER, если фокус не на кнопке OK
    bool _disable_default_action = false;
    //! Запретить закрытие диалога по нажатию на "крестик"
    bool _disable_cross_close = false;

    //! Поиск объектов по имени
    ObjectsFinder* _objects_finder;

    //! Текущая обрабатываемая кнопка
    QDialogButtonBox::StandardButton _processing_button = QDialogButtonBox::NoButton;
    //! Текущая обрабатываемая роль
    QDialogButtonBox::ButtonRole _processing_button_role = QDialogButtonBox::NoRole;

    //! Порядок виджетов на форме
    QList<QWidget*> _tab_order;

    //! Потокобезопасное расширение возможностей QObject
    ObjectExtension* _object_extension = nullptr;

    //! Интерфейс был создан
    mutable bool _ui_created = false;

    //! Количество активных диалогов, запущенных через exec
    static int _execCount;

    friend class DialogConfigurationData;
    friend class View;
};

} // namespace zf
