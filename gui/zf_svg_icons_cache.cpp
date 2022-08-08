#include "zf_svg_icons_cache.h"
#include "zf_core.h"

#include <QSvgGenerator>
#include <QSvgRenderer>
#include <QPainter>
#include <QIconEnginePlugin>
#include <QCache>
#include <QFileInfo>

namespace zf
{
const QString SvgIconCache::FOLDER_NAME = "/icons/";

SvgIconCache::SvgIconCache(int size)
{
    _cache.setMaxCost(size);
}

QIcon SvgIconCache::object(const QString& svg_file, const QSize& size, bool use_png, const QString& instance_key) const
{
    // приходится извратиться с хранением результата конвертации в файлах. иначе видимо никак

    QMutexLocker lock(&_mutex);

    QString key = createKey(svg_file, size, use_png);
    QString temp_file = tempFileName(svg_file, size, use_png, instance_key);

    QIcon* icon = _cache.object(key);

    if (icon == nullptr) {
        if (QFile::exists(temp_file)) {
            icon = new QIcon(temp_file);
            _cache.insert(key, icon);
            _file_exist << key;
            return *icon;
        }
    }

    if (icon != nullptr && !_file_exist.contains(key)) {
        if (!QFile::exists(temp_file)) {
            _cache.remove(key);
            icon = nullptr;
        }
        _file_exist << key;
    }

    if (icon == nullptr) {
        QFileInfo fi(svg_file);
        Z_CHECK(comp(fi.suffix(), QStringLiteral("svg")));

        Utils::removeFile(temp_file);
        Utils::makeDir(QFileInfo(temp_file).absolutePath());

        QSvgRenderer renderer(svg_file);
        if (use_png) {
            QPixmap image(size.width(), size.height());
            image.fill(Qt::transparent);

            QPainter painter;
            painter.setRenderHint(QPainter::Antialiasing);
            painter.setRenderHint(QPainter::SmoothPixmapTransform);
            painter.begin(&image);
            renderer.render(&painter);
            painter.end();

            image.save(temp_file);

        } else {
            QSvgGenerator generator;
            generator.setFileName(temp_file);
            generator.setSize(size);
            generator.setViewBox(QRect(0, 0, size.width(), size.height()));

            QPainter painter;
            painter.begin(&generator);
            renderer.render(&painter);
            painter.end();
        }
        icon = new QIcon(temp_file);
        _cache.insert(key, icon);
    }

    return *icon;
}

QIcon SvgIconCache::object(const QString& svg_file, int size, bool use_png, const QString& instance_key) const
{
    return object(svg_file, QSize {size, size}, use_png, instance_key);
}

QIcon SvgIconCache::objectByStringLines(const QString& svg_file, int size, bool use_png, const QFontMetrics& fm,
                                        const QString& instance_key) const
{
    Z_CHECK(size > 0);
    return object(svg_file, Utils::fontHeight(fm) * size, use_png, instance_key);
}

QString SvgIconCache::fileName(const QString& svg_file, const QSize& size, bool use_png, const QString& instance_key) const
{
    // инициируем создание файла
    object(svg_file, size, use_png);
    return tempFileName(svg_file, size, use_png, instance_key);
}

QString SvgIconCache::fileName(const QString& svg_file, int size, bool use_png, const QString& instance_key) const
{
    return fileName(svg_file, QSize {size, size}, use_png, instance_key);
}

QString SvgIconCache::fileNameByStringLines(const QString& svg_file, int size, bool use_png, const QFontMetrics& fm,
                                            const QString& instance_key) const
{
    Z_CHECK(size > 0);
    return fileName(svg_file, Utils::fontHeight(fm) * size, use_png, instance_key);
}

QString SvgIconCache::createKey(const QString& svg_file, const QSize& size, bool use_png)
{
    return svg_file + Consts::KEY_SEPARATOR + QString::number(size.width()) + Consts::KEY_SEPARATOR + QString::number(size.height())
           + Consts::KEY_SEPARATOR + (use_png ? 'P' : 'S');
}

QString SvgIconCache::tempFileName(const QString& svg_file, const QSize& size, bool use_png, const QString& instance_key)
{
    return Utils::instanceCacheLocation(instance_key) + FOLDER_NAME + Utils::generateGUIDString(createKey(svg_file, size, use_png))
           + (use_png ? QStringLiteral(".png") : QStringLiteral(".svg"));
}
} // namespace zf
