#include "zf_fuzzy_search.h"
#include <QSet>
#include <QMultiMap>
#include <QVector>

//! Фонетические группы
const QMultiMap<QChar, int> FuzzySearch::_phonetic_groups {
    {'a', 0},   {'e', 0},   {'i', 0},   {'o', 0},   {'u', 0},   {'y', 0},   {'b', 1},   {'p', 1},   {'c', 2},   {'k', 2},
    {'q', 2},   {'d', 3},   {'t', 3},   {'l', 4},   {'r', 4},   {'m', 5},   {'n', 5},   {'g', 6},   {'j', 6},   {'f', 7},
    {'p', 7},   {'v', 7},   {'s', 8},   {'x', 8},   {'z', 8},   {'c', 9},   {'s', 9},   {'z', 9},

    {L'а', 10}, {L'о', 10}, {L'у', 10}, {L'е', 11}, {L'и', 11}, {L'ю', 12}, {L'у', 12}, {L'э', 13}, {L'е', 13}, {L'е', 14},
    {L'и', 14}, {L'я', 15}, {L'а', 15}, {L'ё', 16}, {L'о', 16}, {L'ы', 17}, {L'и', 17},
};
bool FuzzySearch::_is_phonetic_groups_prepared = false;
QSet<QString> FuzzySearch::_phonetic_groups_prepared;

QVector<double>* FuzzySearch::_words_rate = nullptr;
QVector<int>* FuzzySearch::_words_lenght = nullptr;
QVector<QVector<int>>* FuzzySearch::_matrix = nullptr;

QMutex FuzzySearch::_phonetic_groups_mutex;
QMutex FuzzySearch::_words_rate_mutex;
QMutex FuzzySearch::_matrix_mutex;

void FuzzySearch::preparePhoneticGroups()
{
    _is_phonetic_groups_prepared = true;

    for (auto it1 = _phonetic_groups.constBegin(); it1 != _phonetic_groups.constEnd(); ++it1) {
        for (auto it2 = _phonetic_groups.constBegin(); it2 != _phonetic_groups.constEnd(); ++it2) {
            if (it1.key() == it2.key())
                continue;

            _phonetic_groups_prepared << QString(it1.key()) + QString(it2.key());
        }
    }
}

int FuzzySearch::cost_helper(const QChar& c1, const QChar& c2)
{
    if (c1 == c2)
        return 0;

    if (!c1.isLetter() || !c2.isLetter())
        return 9999; // отличия не в буквах принципиальны

    QMutexLocker lock(&_phonetic_groups_mutex);
    if (!_is_phonetic_groups_prepared) {
        preparePhoneticGroups();
    }

    return _phonetic_groups_prepared.contains(QString(c1) + QString(c2)) ? 1 : 2;
}

double FuzzySearch::compareText(const QString& s1, const QString& s2)
{
    QString str1 = s1.simplified().toLower();
    QString str2 = s2.simplified().toLower();

    if (str1.isEmpty() || str2.isEmpty())
        return 0;

    // Разбиваем на слова
    QStringList s1_words = str1.split(' ', QString::SkipEmptyParts);
    int s1_n = s1_words.count();

    QStringList s2_words = str2.split(' ', QString::SkipEmptyParts);
    int s2_n = s2_words.count();

    QStringList* s1_words_prep;
    QStringList* s2_words_prep;

    if (s1_n != s2_n) {
        if (abs(s1_n - s2_n) > 1)
            return 0; // Если разница больше чем в слово, то это разные строки

        // Количество слов отличается на 1
        if (s1_n > s2_n) {
            s1_words_prep = &s1_words;
            s2_words_prep = &s2_words;
        } else {
            s1_words_prep = &s2_words;
            s2_words_prep = &s1_words;
        }

        // 1 строка содержит больше слов. Надо решить к какому слову первой строки надо
        // прицепить лишнее слово для сравнения со второй строкой
        int move_pos = -1;
        for (int i = 1; i < s2_words_prep->count(); i++) {
            if (s1_words_prep->at(i).length() == s2_words_prep->at(i).length())
                continue;

            // строки отличаются по длине, прицепляем
            move_pos = i;
            break;
        }
        if (move_pos < 0)
            move_pos = s1_words_prep->count() - 1; // последнее слово

        (*s1_words_prep)[move_pos - 1]
                = s1_words_prep->at(move_pos - 1) + QStringLiteral(" ") + s1_words_prep->at(move_pos);
        s1_words_prep->removeAt(move_pos);
    } else {
        // Количество слов совпадает
        s1_words_prep = &s1_words;
        s2_words_prep = &s2_words;
    }

    // Ищем средний рейтинг, с учетом "веса" каждой строки. Чем длиннее, тем "весомее"
    int count = s1_words_prep->count();
    int max_length = 0; // макс. длина строк

    QVector<double>* words_rate; // рейтинг по каждому слову
    QVector<int>* words_lenght; // макс. длина из каждой пары строк

    _words_rate_mutex.lock();
    bool words_rate_allocated;
    if (count <= _max_word_count) {
        if (!_words_rate) {
            _words_rate = new QVector<double>(_max_word_count);
            _words_lenght = new QVector<int>(_max_word_count);
        }

        words_rate = _words_rate;
        words_lenght = _words_lenght;
        words_rate_allocated = false;

    } else {
        words_rate = new QVector<double>(count);
        words_lenght = new QVector<int>(count);
        words_rate_allocated = true;
    }
    _words_rate_mutex.unlock();

    for (int i = 0; i < count; i++) {
        int length = qMax(s1_words_prep->at(i).length(), s2_words_prep->at(i).length());
        max_length = qMax(max_length, length);
        (*words_lenght)[i] = length;
        (*words_rate)[i] = compareWord_helper(s1_words_prep->at(i), s2_words_prep->at(i), false);
    }

    double rate = 0;
    for (int i = 0; i < count; i++) {
        rate += words_rate->at(i) * words_lenght->at(i) / static_cast<double>(max_length);
    }

    if (words_rate_allocated) {
        delete words_rate;
        delete words_lenght;
    }

    return rate / static_cast<double>(count); //-V773
}

double FuzzySearch::compareWord(const QString& s1, const QString& s2)
{
    return compareWord_helper(s1, s2, true);
}

double FuzzySearch::compareWord_helper(const QString& s1, const QString& s2, bool to_lower)
{
    const QString* str1;
    const QString* str2;
    if (to_lower) {
        str1 = new QString(s1.toLower().simplified());
        str2 = new QString(s2.toLower().simplified());
    } else {
        str1 = &s1;
        str2 = &s2;
    }

    int m = str1->size();
    int n = str2->size();
    int max_size = qMax(m, n);

    QVector<QVector<int>>* matrix;

    bool matrix_allocated;
    if (_max_word_lenght >= max_size) {
        _matrix_mutex.lock();
        if (!_matrix)
            _matrix = new QVector<QVector<int>>(_max_word_lenght + 1, QVector<int>(_max_word_lenght + 1));

        matrix = _matrix;
        _matrix_mutex.unlock();
        matrix_allocated = false;

    } else {
        matrix = new QVector<QVector<int>>(m + 1, QVector<int>(n + 1));
        matrix_allocated = true;
    }

    bool dig1 = false;
    bool dig2 = false;

    for (int i = 0; i <= m; ++i) {
        (*matrix)[i][0] = i << 1;

        if (i < m) {
            if (str1->at(i).isDigit())
                dig1 = true;
        }
    }

    for (int j = 0; j <= n; ++j) {
        if (j > 0)
            (*matrix)[0][j] = j << 1;

        if (j < n) {
            if (str2->at(j).isDigit())
                dig2 = true;
        }
    }

    // Если в любом слове есть цифры, то проводить полное сравнение
    if (dig1 || dig2) {
        if (m != n) {
            if (matrix_allocated)
                delete matrix;

            if (to_lower) {
                delete str1;
                delete str2;
            }
            return 0; //-V773
        }

        for (int i = 0; i < n; ++i) {
            if (str1->at(i) != str2->at(i)) {
                if (matrix_allocated)
                    delete matrix;

                if (to_lower) {
                    delete str1;
                    delete str2;
                }
                return 0;
            }
        }
        if (matrix_allocated)
            delete matrix;

        if (to_lower) {
            delete str1;
            delete str2;
        }
        return 100;
    }

    for (int j = 1; j <= n; ++j) {
        for (int i = 1; i <= m; ++i) {
            // Учтем вставки, удаления и замены
            int rcost = cost_helper(str1->at(i - 1), str2->at(j - 1));
            int dist0 = (*matrix)[i - 1][j] + 2;
            int dist1 = (*matrix)[i][j - 1] + 2;
            int dist2 = (*matrix)[i - 1][j - 1] + rcost;
            (*matrix)[i][j] = qMin(dist0, std::min(dist1, dist2));
            // Учтем обмен
            if (i > 1 && j > 1 && str1->at(i - 1) == str2->at(j - 2) && str1->at(i - 2) == str2->at(j - 1)) {
                (*matrix)[i][j] = qMin((*matrix)[i][j], (*matrix)[i - 2][j - 2] + 1);
            }
        }
    }

    if (to_lower) {
        delete str1;
        delete str2;
    }

    // нормализация
    int abs_rate = (*matrix)[m][n];
    int max_rate = 2 * max_size;

    if (matrix_allocated)
        delete matrix;

    if (abs_rate == 0)
        return 100;
    else if (abs_rate >= max_rate)
        return 0;

    // Если не точное совпадение, то нельзя выдавать 100%
    double res = 100.0 - 100.0 * static_cast<double>(abs_rate) / static_cast<double>(max_rate);

    return res >= 100.0 ? 99.9 : res;
}
