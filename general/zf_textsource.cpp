#include "zf_textsource.h"

namespace zf
{
//! Разделяемые данные для TextSource
class TextSource_data : public QSharedData
{
public:
    TextSource_data();
    TextSource_data(const TextSource_data& d);
    ~TextSource_data();

    const QString* source = nullptr;
    QList<QStringRef> fragments;
    int length = 0;
};

TextSource_data::TextSource_data()
    : source(nullptr)
    , length(0)
{
}

TextSource_data::TextSource_data(const TextSource_data& d)
    : QSharedData(d)
    , source(d.source)
    , length(d.length)
{
}

TextSource_data::~TextSource_data()
{
}

TextSource::TextSource()
    : _d(new TextSource_data)
{
    prepareBegin();
}

TextSource::TextSource(const TextSource& t)
    : _d(t._d)
{
    prepareBegin();
}

TextSource::TextSource(const QString* source, int position, int length)
    : _d(new TextSource_data)
{
    Q_ASSERT(source);
    prepareBegin();
    addFragment(source->midRef(position, length));
}

TextSource::~TextSource()
{
}

TextSource& TextSource::operator=(const TextSource& t)
{
    if (this != &t)
        _d = t._d;
    return *this;
}

bool TextSource::isEmpty() const
{
    return _d->fragments.isEmpty();
}

void TextSource::clear()
{
    *this = TextSource();
}

TextSource TextSource::left(int n) const
{
    return mid(0, n);
}

TextSource TextSource::right(int n) const
{
    return mid(_d->length - n, n);
}

TextSource TextSource::mid(int position, int length) const
{
    int processed = 0;
    int added = 0;
    TextSource t;
    foreach (QStringRef f, _d->fragments) {
        if (f.length() > position - processed) {
            int n1 = qMax(0, position - processed);
            int n2 = qMin(f.length() - n1, length - added);
            QStringRef s = f.mid(n1, n2);
            Q_ASSERT(!s.isEmpty());
            t.addFragment(s);
            added += s.length();
        }

        processed += f.length();
        if (processed >= position + length)
            break;
    }
    return t;
}

TextLink TextSource::leftLink(int n) const
{
    return midLink(0, n);
}

TextLink TextSource::rightLink(int n) const
{
    return midLink(length() - n, n);
}

TextLink TextSource::midLink(int position, int length) const
{
    return TextLink(this, position, length);
}

const QString* TextSource::source() const
{
    return _d->source;
}

QString TextSource::toString() const
{
    QString s;
    foreach (QStringRef f, _d->fragments) {
        s += f.toString();
    }
    return s;
}

int TextSource::position() const
{
    if (_d->source && !_d->fragments.isEmpty())
        return _d->fragments.first().position();
    else
        return -1;
}

int TextSource::length() const
{
    return _d->length;
}

void TextSource::prepareEnd() const
{
    if (_d->fragments.isEmpty())
        return;

    _current_fragment = _d->fragments.count() - 1;
    _current_fragment_pos = _d->fragments.last().length() - 1;
    _character_offset = _d->length - 1;
}

void TextSource::prepareBegin() const
{
    _current_fragment = 0;
    _current_fragment_pos = 0;
    _character_offset = 0;
}

bool TextSource::eof() const
{
    return _d->fragments.isEmpty() || _current_fragment < 0 || _current_fragment_pos < 0
            || _d->fragments.count() <= _current_fragment
            || _d->fragments.at(_current_fragment).length() <= _current_fragment_pos;
}

QChar TextSource::nextChar() const
{
    if (eof())
        return QChar();

    QChar c = _d->fragments.at(_current_fragment).at(_current_fragment_pos);

    if (_d->fragments.at(_current_fragment).length() == _current_fragment_pos + 1) {
        _current_fragment++;
        _current_fragment_pos = 0;
    } else
        _current_fragment_pos++;

    _character_offset++;
    return c;
}

QChar TextSource::previousChar() const
{
    if (eof())
        return QChar();

    QChar c = _d->fragments.at(_current_fragment).at(_current_fragment_pos);

    if (_current_fragment_pos == 0) {
        _current_fragment--;
        if (_current_fragment >= 0)
            _current_fragment_pos = _d->fragments.at(_current_fragment).length() - 1;
    } else
        _current_fragment_pos--;

    _character_offset--;
    return c;
}

int TextSource::characterOffset() const
{
    return _character_offset;
}

QChar TextSource::firstChar() const
{
    if (_d->fragments.isEmpty())
        return QChar();
    else
        return _d->fragments.first().at(0);
}

QChar TextSource::lastChar() const
{
    if (_d->fragments.isEmpty())
        return QChar();
    else
        return _d->fragments.last().at(_d->fragments.last().length() - 1);
}

int TextSource::basePosition(int position) const
{
    if (isEmpty())
        return -1;

    int processed = 0;
    foreach (QStringRef f, _d->fragments) {
        if (f.length() > position - processed) {
            int n1 = position - processed;
            return f.position() + n1;
        }
        processed += f.length();
    }

    return _d->fragments.last().position() + _d->fragments.last().length() + position - _d->length;
}

QChar TextSource::charShift(int shift) const
{
    if (eof())
        return QChar();

    int current_fragment_saved = _current_fragment;
    int current_fragment_pos_saved = _current_fragment_pos;
    int character_offset_saved = _character_offset;

    QChar c;
    int n = 0;
    while (qAbs(n) < qAbs(shift) && !eof()) {
        if (shift > 0)
            c = nextChar();
        else
            c = previousChar();
        n++;
    }

    _current_fragment = current_fragment_saved;
    _current_fragment_pos = current_fragment_pos_saved;
    _character_offset = character_offset_saved;

    return c;
}

QList<QStringRef> TextSource::fragments() const
{
    return _d->fragments;
}

int TextSource::count() const
{
    return _d->fragments.count();
}

int TextSource::fragmentByPosition(int position) const
{
    int processed = 0;
    int i = 0;
    foreach (QStringRef f, _d->fragments) {
        if (f.length() > position - processed)
            return i;

        processed += f.length();
        if (processed >= position) {
            i = -1;
            break;
        }

        i++;
    }
    return i;
}

QStringRef TextSource::addFragment(const QStringRef& s)
{
    if (s.isEmpty())
        return QStringRef();

    if (_d->source) {
        Q_ASSERT(_d->source == s.string());
        if (!_d->fragments.isEmpty())
            Q_ASSERT(_d->fragments.last().position() + _d->fragments.last().length() <= s.position());
    } else {
        _d->source = s.string();
    }
    _d->fragments.append(s);
    _d->length += s.length();

    return s;
}

QStringRef TextSource::addFragment(int position, int length)
{
    Q_ASSERT(_d->source);
    return addFragment(_d->source->midRef(position, length));
}

TextLink::TextLink()
{
}

TextLink::TextLink(const TextLink& t)
    : _source(t._source)
    , _position(t._position)
    , _length(t._length)
{
}

TextLink::TextLink(const TextSource* source, int position, int length)
    : _source(source)
    , _position(position)
    , _length(length)
{
    Q_ASSERT(source);
    Q_ASSERT(position >= 0 && length >= 0);
}

bool TextLink::isEmpty() const
{
    return !_source || _length == 0 || _source->mid(_position, _length).isEmpty();
}

int TextLink::position() const
{
    return _position;
}

int TextLink::length() const
{
    return _length;
}

TextLink TextLink::left(int n) const
{
    return mid(0, n);
}

TextLink TextLink::right(int n) const
{
    return mid(_length - n, n);
}

TextLink TextLink::mid(int position, int n) const
{
    if (!_source || _length == 0 || n <= 0)
        return TextLink();

    if (position >= _source->length())
        return TextLink();

    if (position < 0)
        n -= qAbs(position);
    else if (position + n > _source->length())
        n = _source->length() - position;

    if (!_source || _length == 0 || n <= 0)
        return TextLink();

    return _source->midLink(_position + position, n);
}

QChar TextLink::firstChar() const
{
    if (!_source || _length == 0)
        return QChar();

    TextSource t = _source->mid(_position, 1);
    return t.isEmpty() ? QChar() : t.fragments().constFirst().at(0);
}

QChar TextLink::lastChar() const
{
    if (!_source || _length == 0)
        return QChar();

    TextSource t = _source->mid(_position + _length - 1, 1);
    return t.isEmpty() ? QChar() : t.fragments().constFirst().at(0);
}

QString TextLink::toString() const
{
    if (!_source || _length == 0)
        return QString();

    return _source->mid(_position, _length).toString();
}

const TextSource* TextLink::source() const
{
    return _source;
}

const QString* TextLink::base() const
{
    return _source ? _source->source() : nullptr;
}

int TextLink::basePosition(int position) const
{
    return _source ? _source->basePosition(_position + position) : -1;
}

QStringRef TextLink::baseRef() const
{
    if (!_source || _length == 0)
        return QStringRef();

    int n1 = basePosition(0);
    int n2 = basePosition(_length);

    return QStringRef(_source->source(), n1, n2 - n1);
}

} // namespace zf
