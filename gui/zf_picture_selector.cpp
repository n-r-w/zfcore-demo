#include "zf_picture_selector.h"
#include "private/zf_picture_selector_p.h"
#include "zf_utils.h"
#include "zf_core.h"
#include "zf_translation.h"
#include "zf_framework.h"
#include "zf_colors_def.h"

#include <QDebug>
#include <QLabel>
#include <QVBoxLayout>
#include <QMouseEvent>
#include <QResizeEvent>
#include <QApplication>
#include <QImageReader>
#include <QClipboard>
#include <QPainter>

#define ACCESSIBLE_OPEN 0
#define ACCESSIBLE_PASTE 1
#define ACCESSIBLE_CLEAR 2
#define ACCESSIBLE_COUNT 3

namespace zf
{
PictureSelector::PictureSelector(QWidget* parent)
    : QWidget(parent)
{
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    setAcceptDrops(true);
    setContentsMargins(0, 0, 0, 0);

    _frame = new QFrame(this);
    _frame->setObjectName("PictureSelector_frame");
    _frame->setStyleSheet(QStringLiteral("QFrame#PictureSelector_frame {"
                                         "border: 2px %1;"
                                         "border-top-style: ridge; "
                                         "border-right-style: ridge; "
                                         "border-bottom-style: ridge; "
                                         "border-left-style: ridge;"
                                         "Padding-right: -1"
                                         "}")
                              .arg(Colors::uiLineColor(true).name()));

    _frame->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    _frame->installEventFilter(this);

    _open_button = new QToolButton;
    _open_button->setObjectName(QStringLiteral("zfob"));
    _open_button->setIcon(QIcon(QStringLiteral(":/share_icons/picture.svg")));
    _open_button->setAutoRaise(false);
    _open_button->setToolTip(ZF_TR_EX(ZFT_OPEN_IMAGE, "Выбрать изображение"));
    connect(_open_button, &QToolButton::clicked, this, &PictureSelector::selectPicture);

    _paste_button = new QToolButton;
    _paste_button->setObjectName(QStringLiteral("zfpb"));
    _paste_button->setIcon(QIcon(QStringLiteral(":/share_icons/paste.svg")));
    _paste_button->setAutoRaise(false);
    _paste_button->setToolTip(ZF_TR_EX(ZFT_PASTE_IMAGE, "Вставить изображение"));
    connect(_paste_button, &QToolButton::clicked, this, &PictureSelector::sl_paste);
    connect(QApplication::clipboard(), &QClipboard::dataChanged, this, &PictureSelector::updateEnabled);

    _clear_button = new QToolButton;
    _clear_button->setObjectName(QStringLiteral("zfcb"));
    _clear_button->setIcon(QIcon(QStringLiteral(":/share_icons/cancel-bw.svg")));
    _clear_button->setAutoRaise(false);
    _clear_button->setToolTip(ZF_TR_EX(ZFT_CLEAR_IMAGE, "Очистить изображение"));
    connect(_clear_button, &QToolButton::clicked, this, [&]() {
        clear();
        emit sg_edited();
    });

    _manage = new QWidget(this);
    _manage->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    _manage->setLayout(new QHBoxLayout);
    _manage->layout()->setObjectName(QStringLiteral("zfla"));
    _manage->layout()->setMargin(1);
    _manage->layout()->addWidget(_open_button);
    _manage->layout()->addWidget(_paste_button);
    _manage->layout()->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Fixed));
    _manage->layout()->addWidget(_clear_button);

    setIconNoImage(QIcon());

    updateInternalGeometry();
    updateEnabled();
}

PictureSelector::~PictureSelector()
{
    delete _manage;
}

Error PictureSelector::loadFile(const QString& file_name)
{
    QFile file(file_name);
    if (!file.open(QFile::ReadOnly)) {
        clear();
        return Error::fileIOError(file_name);
    }

    QByteArray file_data = file.readAll();
    QString file_type = QFileInfo(file_name).suffix().toUpper();
    file.close();
    Error error = setFileData(file_data, file_type);

    if (error.isError()) {
        clear();
        return error;
    }

    return Error();
}

Error PictureSelector::setFileData(const QByteArray& file_data, const QString& file_type)
{
    QPixmap pixmap;
    if (!pixmap.loadFromData(file_data, file_type.isEmpty() ? nullptr : file_type.toUpper().toLatin1().constData())) {
        clear();
        return Error::corruptedDataError();
    }

    setIcon(QIcon(pixmap));

    _file_data = file_data;
    _file_type = file_type;

    return {};
}

Error PictureSelector::setFileData(const QByteArray& file_data, FileType file_type)
{
    Z_CHECK(file_type != FileType::Undefined);
    return setFileData(file_data, Utils::fileExtensions(file_type).constFirst());
}

void PictureSelector::setIcon(const QIcon& icon)
{
    _file_data.clear();
    _file_type.clear();

    setIconHelper(icon, true);
}

QByteArray PictureSelector::fileData() const
{
    return _file_data;
}

QString PictureSelector::fileType() const
{
    return _file_type;
}

void PictureSelector::setIconNoImage(const QIcon& icon)
{
    if (icon.isNull())
        _icon_no_image = QIcon(QStringLiteral(":/share_icons/no-picture.svg"));
    else
        _icon_no_image = icon;

    if (isEmpty())
        setIconToPixmap(_icon_no_image);
}

QIcon PictureSelector::iconNoImage() const
{
    return _icon_no_image;
}

void PictureSelector::setImageSize(const QSize& size)
{
    if (_size == size)
        return;

    _size = size;
    updateGeometry();
}

QSize PictureSelector::imageSize() const
{
    return _size;
}

void PictureSelector::clear()
{
    _file_data.clear();
    _file_type.clear();
    _icon = QIcon();
    setIconToPixmap(_icon_no_image);

    updateEnabled();

    emit sg_changed();
}

bool PictureSelector::isEmpty() const
{
    return _icon.isNull();
}


QIcon PictureSelector::icon() const
{
    return _icon;
}

bool PictureSelector::isReadOnly() const
{
    return _is_read_only;
}

void PictureSelector::setReadOnly(bool b)
{
    if (_is_read_only == b)
        return;
    _is_read_only = b;

    updateEnabled();
}

int PictureSelector::margin() const
{
    return _margin;
}

void PictureSelector::setMargin(int m)
{
    if (_margin == m)
        return;
    _margin = m;

    updateInternalGeometry();
    update();
}

QToolButton* PictureSelector::openButton() const
{
    return _open_button;
}

QToolButton* PictureSelector::clearButton() const
{
    return _clear_button;
}

QToolButton* PictureSelector::pasteButton() const
{
    return _paste_button;
}

QSize PictureSelector::sizeHint() const
{
    return minimumSizeHint();
}

QSize PictureSelector::minimumSizeHint() const
{
    return Utils::grownBy(_size, QMargins(_margin, _margin, _margin, _margin));
}

void PictureSelector::resizeEvent(QResizeEvent* event)
{
    updateInternalGeometry();
    QWidget::resizeEvent(event);
}

bool PictureSelector::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == _frame) {
        switch (event->type()) {
            case QEvent::MouseButtonDblClick: {
                if (!_is_read_only && isEnabled())
                    selectPicture();
                break;
            }

            default:
                break;
        }
    }

    return QWidget::eventFilter(watched, event);
}

void PictureSelector::dragEnterEvent(QDragEnterEvent* event)
{
    if (isReadOnly()) {
        event->ignore();
        return;
    }

    if (!allowMimeFile(event->mimeData()).isEmpty()) {
        event->acceptProposedAction();
        return;
    }

    QWidget::dragEnterEvent(event);
}

void PictureSelector::dragMoveEvent(QDragMoveEvent* event)
{
    if (!allowMimeFile(event->mimeData()).isEmpty()) {
        event->acceptProposedAction();
        return;
    }

    QWidget::dragMoveEvent(event);
}

void PictureSelector::dropEvent(QDropEvent* event)
{
    QString drop_file = allowMimeFile(event->mimeData());
    if (!drop_file.isEmpty()) {
        if (loadFile(drop_file).isOk()) {
            emit sg_edited();
            event->acceptProposedAction();
        } else {
            event->ignore();
        }
        return;
    }

    QWidget::dropEvent(event);
}

bool PictureSelector::event(QEvent* event)
{
    switch (event->type()) {
        case QEvent::LayoutDirectionChange:
        case QEvent::ApplicationLayoutDirectionChange:
            updateInternalGeometry();
            break;
        default:
            break;
    }

    return QWidget::event(event);
}

void PictureSelector::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event)

    QPainter painter(this);

    updateDrawIcon();

    QRect r;
    r.setSize(_draw_pixmap.size());
    r.moveCenter(rect().center());

    painter.drawPixmap(r, _draw_pixmap);
}

void PictureSelector::sl_paste()
{
    const QClipboard* clipboard = QApplication::clipboard();
    const QMimeData* mimeData = clipboard->mimeData();
    if (mimeData->hasImage()) {
        QBuffer buffer;
        buffer.open(QBuffer::WriteOnly);
        if (!mimeData->imageData().value<QPixmap>().save(&buffer, "png")) {
            Core::logError("PictureSelector save to buffer error");
            return;
        }

        auto error = setFileData(buffer.data(), "png");
        if (error.isError()) {
            Core::logError(error);
            return;
        }
        emit sg_edited();

    } else {
        QString file_name = allowMimeFile(mimeData);
        if (file_name.isEmpty())
            return;

        if (loadFile(file_name).isOk())
            emit sg_edited();
    }
}

void PictureSelector::setIconToPixmap(const QIcon& icon)
{
    _need_update_draw = true;
    _draw_icon = icon;
    _draw_pixmap = QPixmap();

    update();
}

void PictureSelector::updateDrawIcon()
{
    if (!_need_update_draw)
        return;
    _need_update_draw = false;

    if (_draw_icon.isNull()) {
        _draw_pixmap = QPixmap();

    } else {
        QSize target_size = Utils::grownBy(this->size(), {-_margin, -_margin, -_margin, -_margin});
        QSize size = Utils::bestPixmapSize(_draw_icon, target_size);
        _draw_pixmap = Utils::scalePicture(_draw_icon.pixmap(size), target_size);
    }
}

void PictureSelector::updateInternalGeometry()
{
    _frame->setGeometry(0, 0, width(), height());
    _manage->setGeometry(_margin, height() - _manage->height() - _margin, width() - _margin * 2, _manage->height());
}

void PictureSelector::updateEnabled()
{
    bool ro = _is_read_only || !isEnabled();

    _manage->setHidden(ro);
    if (!ro)
        _clear_button->setEnabled(!isEmpty());

    _paste_button->setHidden(ro);
    if (!ro)
        _paste_button->setEnabled(QApplication::clipboard()->mimeData()->hasImage()
                                  || !allowMimeFile(QApplication::clipboard()->mimeData()).isEmpty());
}

void PictureSelector::setIconHelper(const QIcon& icon, bool changed)
{
    _icon = icon;

    if (_icon.isNull())
        clear();
    else
        setIconToPixmap(_icon);

    updateEnabled();

    if (changed)
        emit sg_changed();
}

void PictureSelector::selectPicture()
{
    QString file_name = Utils::getOpenFileName(ZF_TR_EX(ZFT_OPEN_IMAGE, "Выбрать изображение"), "",
                                               ZF_TR_EX(ZFT_IMAGE_FILES, "Файлы изображений (*.jpg *.png *.svg *.bmp)"));
    if (file_name.isEmpty())
        return;

    Error error = loadFile(file_name);
    if (error.isError())
        Core::error(ZF_TR_EX(ZFT_WRONG_IMAGE_FORMAT, "Неверный формат изображения") + " " + QDir::toNativeSeparators(file_name));
    else
        emit sg_edited();
}

QString PictureSelector::allowMimeFile(const QMimeData* mime_data)
{
    Z_CHECK_NULL(mime_data);

    if (!mime_data->hasUrls())
        return {};

    QStringList formats;
    for (auto& f : QImageReader::supportedImageFormats()) {
        formats << QString::fromLocal8Bit(f).toLower();
    }

    for (const QUrl& url : mime_data->urls()) {
        QFileInfo fi(url.toLocalFile());
        if (!fi.exists())
            continue;

        if (formats.contains(fi.suffix().toLatin1(), Qt::CaseInsensitive))
            return fi.absoluteFilePath();
    }
    return {};
}

QAccessibleInterface* zf::PictureSelectorAccessibilityInterface::accessibleFactory(const QString& class_name, QObject* object)
{
    if (object == nullptr || !object->isWidgetType())
        return nullptr;

    if (class_name == QLatin1String("zf::PictureSelector"))
        return new PictureSelectorAccessibilityInterface(qobject_cast<QWidget*>(object));

    return nullptr;
}

zf::PictureSelectorAccessibilityInterface::PictureSelectorAccessibilityInterface(QWidget* w)
    : QAccessibleWidget(w, QAccessible::Graphic)
{
}

int zf::PictureSelectorAccessibilityInterface::childCount() const
{
    return (selector()->isReadOnly() || !selector()->isEnabled()) ? 0 : ACCESSIBLE_COUNT;
}

QAccessibleInterface* zf::PictureSelectorAccessibilityInterface::child(int index) const
{
    if (index == ACCESSIBLE_OPEN)
        return QAccessible::queryAccessibleInterface(selector()->openButton());
    if (index == ACCESSIBLE_PASTE)
        return QAccessible::queryAccessibleInterface(selector()->pasteButton());
    if (index == ACCESSIBLE_CLEAR)
        return QAccessible::queryAccessibleInterface(selector()->clearButton());
    return nullptr;
}

int zf::PictureSelectorAccessibilityInterface::indexOfChild(const QAccessibleInterface* child) const
{
    if (selector()->openButton() == child->object())
        return ACCESSIBLE_OPEN;
    if (selector()->pasteButton() == child->object())
        return ACCESSIBLE_PASTE;
    if (selector()->clearButton() == child->object())
        return ACCESSIBLE_CLEAR;

    return -1;
}

QString zf::PictureSelectorAccessibilityInterface::text(QAccessible::Text t) const
{
    QString str;

    switch (t) {
        case QAccessible::Name:
            str = selector()->objectName();
            break;
        default:
            break;
    }
    if (str.isEmpty())
        str = QAccessibleWidget::text(t);
    return str;
}

QStringList zf::PictureSelectorAccessibilityInterface::actionNames() const
{
    return QStringList() << pressAction();
}

QString zf::PictureSelectorAccessibilityInterface::localizedActionDescription(const QString& action_name) const
{
    if (action_name == pressAction())
        return utf("Open the PictureSelector"); // TODO translate
    return QString();
}

void zf::PictureSelectorAccessibilityInterface::doAction(const QString& action_name)
{
    if (action_name == pressAction()) {
        if (selector()->isEnabled() && !selector()->isReadOnly())
            selector()->selectPicture();
    }
}

QStringList zf::PictureSelectorAccessibilityInterface::keyBindingsForAction(const QString&) const
{
    return QStringList();
}

QAccessibleInterface* zf::PictureSelectorAccessibilityInterface::childAt(int x, int y) const
{
    if (selector()->isReadOnly() || !selector()->isEnabled())
        return nullptr;

    QPoint point = selector()->mapToGlobal(QPoint(x, y));
    point = selector()->_manage->mapFromGlobal(point);

    if (selector()->openButton()->rect().contains(point))
        return child(ACCESSIBLE_OPEN);
    if (selector()->pasteButton()->rect().contains(point))
        return child(ACCESSIBLE_PASTE);
    if (selector()->clearButton()->rect().contains(point))
        return child(ACCESSIBLE_CLEAR);

    return nullptr;
}

PictureSelector* PictureSelectorAccessibilityInterface::selector() const
{
    return qobject_cast<PictureSelector*>(widget());
}

} // namespace zf
