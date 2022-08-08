#pragma once

#include <QString>
#include <QSharedData>
#include <QSharedDataPointer>

#include "zf.h"

namespace zf
{
class TextSource_data;
class TextLink;

/*! Представление набора фрагментов строки в виде одной непрерывной строки. Добавляемые фрагменты должны принадлежать
 * одной исходной строке и не пересекаться Доступ к содержимому либо в целом через toString, либо последовательный
 * через nextChar */
class ZCORESHARED_EXPORT TextSource
{
public:
    TextSource();
    TextSource(const TextSource& t);
    TextSource(const QString* source, int position, int length);
    ~TextSource();

    TextSource& operator=(const TextSource& t);

    bool isEmpty() const;
    void clear();

    TextSource left(int n) const;
    TextSource right(int n) const;
    TextSource mid(int position, int length) const;

    TextLink leftLink(int n) const;
    TextLink rightLink(int n) const;
    TextLink midLink(int position, int length) const;

    const QString* source() const;
    QString toString() const;

    int position() const;
    int length() const;

    //! Сбросить текущую позицию перебора в начало
    void prepareBegin() const;
    //! Сбросить текущую позицию перебора в конец
    void prepareEnd() const;
    //! Достигнут конец в переборе
    bool eof() const;
    //! Получить следующий символ. Контроль окончания перебора через QChar::isNull
    QChar nextChar() const;
    //! Получить предыдущий символ. Контроль окончания перебора через QChar::isNull
    QChar previousChar() const;
    //! Текущее положение перебора
    int characterOffset() const;
    //! Получить символ со сдвигом на n элементов относительно текущего положения перебора (без изменения текущего
    //! положения перебора)
    QChar charShift(int shift) const;

    //! Первый символ (без изменения текущего положения перебора)
    QChar firstChar() const;
    //! Последний символ (без изменения текущего положения перебора)
    QChar lastChar() const;

    //! Положение указанного символа в базовой строке
    int basePosition(int position) const;

    //! Список фрагментов
    QList<QStringRef> fragments() const;
    int count() const;
    //! Номер фрагмента для символа в позиции
    int fragmentByPosition(int position) const;

    //! Добавить фррагмент
    QStringRef addFragment(const QStringRef& s);
    QStringRef addFragment(int position, int length);

private:
    //! Данные
    QSharedDataPointer<TextSource_data> _d;
    //! Текущий фрагмент при последовательном переборе
    mutable int _current_fragment;
    //! Позиция в текущем фрагменте при последовательном переборе
    mutable int _current_fragment_pos;
    //! Текущий полный сдвиг
    mutable int _character_offset;
};

//! Cссылка на TextSource
class ZCORESHARED_EXPORT TextLink
{
public:
    TextLink();
    TextLink(const TextLink& t);
    TextLink(const TextSource* source, int position, int length);

    bool isEmpty() const;

    //! Положение в TextSource
    int position() const;
    //! Длина
    int length() const;

    TextLink left(int n) const;
    TextLink right(int n) const;
    TextLink mid(int position, int n) const;

    //! Первый символ
    QChar firstChar() const;
    //! Последний символ
    QChar lastChar() const;

    //! Преобразовать в обычную строку
    QString toString() const;

    //! Источник данных
    const TextSource* source() const;

    //! Базовая строка
    const QString* base() const;
    //! Положение указанного символа в базовой строке
    int basePosition(int position) const;
    //! Сформировать подстроку в базовой строке
    QStringRef baseRef() const;

private:
    const TextSource* _source = nullptr;
    int _position = 0;
    int _length = 0;
};

} // namespace zf
