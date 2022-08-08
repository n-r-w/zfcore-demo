#pragma once

#include "zf.h"
#include <QCache>
#include <QSet>
#include <QApplication>

namespace zf
{
//! Кэш для конвертации размера SVG
class ZCORESHARED_EXPORT SvgIconCache
{
public:
    SvgIconCache(int size);

    //! Получить иконку из SVG файла с переводом в нужный размер
    QIcon object(const QString& svg_file, const QSize& size,
                 //! Создать PNG
                 bool use_png = false,
                 //! Код типа приложения
                 const QString& instance_key = {}) const;
    //! Получить иконку из SVG файла с переводом в нужный размер (равносторонняя)
    QIcon object(const QString& svg_file, int size,
                 //! Создать PNG
                 bool use_png = false,
                 //! Код типа приложения
                 const QString& instance_key = {}) const;
    //! Получить иконку из SVG файла с переводом в нужный размер на основании количества строк текста
    QIcon objectByStringLines(const QString& svg_file, int size = 1,
                              //! Создать PNG
                              bool use_png = false, const QFontMetrics& fm = qApp->fontMetrics(),
                              //! Код типа приложения
                              const QString& instance_key = {}) const;

    //! Получить имя файла SVG иконки с переводом в нужный размер
    QString fileName(const QString& svg_file, const QSize& size,
                     //! Создать PNG
                     bool use_png = false,
                     //! Код типа приложения
                     const QString& instance_key = {}) const;
    //! Получить имя файла SVG иконки с переводом в нужный размер (равносторонняя)
    QString fileName(const QString& svg_file, int size,
                     //! Создать PNG
                     bool use_png = false,
                     //! Код типа приложения
                     const QString& instance_key = {}) const;
    //! Получить имя файла SVG иконки с переводом в нужный размер на основании количества строк текста
    QString fileNameByStringLines(const QString& svg_file, int size = 1,
                                  //! Создать PNG
                                  bool use_png = false, const QFontMetrics& fm = qApp->fontMetrics(),
                                  //! Код типа приложения
                                  const QString& instance_key = {}) const;

private:
    static QString createKey(const QString& svg_file, const QSize& size, bool use_png);
    static QString tempFileName(const QString& svg_file, const QSize& size, bool use_png, const QString& instance_key);

    mutable QCache<QString, QIcon> _cache;
    mutable QSet<QString> _file_exist;
    mutable QMutex _mutex;

    static const QString FOLDER_NAME;
};

} // namespace zf
