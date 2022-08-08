#pragma once

#include <QString>
#include "zf.h"

/*! Алгоритм нечеткого поиска
 * Для сравнения двух строк на "похожесть" используется модифициованный метод "Расстояние Дамерау-Левенштейна" с
 * нормализацией результата. В метод вычисления "расстояния" между строками добавлен учет похожести звучания отдельных
 * букв т.е. стоимость замены 'о' на 'я' будет выше чем замена 'о' на 'а' */
class ZCORESHARED_EXPORT FuzzySearch
{
public:
    //! Схожесть строки 0-100%
    //! Изменяет схожесть по отдельным словам и затем вычисляет среднее с учетом "веса"(длины) каждой строки
    //! Потокобезопасно
    static double compareText(const QString& s1, const QString& s2);
    //! Схожесть отдельного слова 0-100%. Потокобезопасно
    static double compareWord(const QString& s1, const QString& s2);

private:
    //! Схожесть отдельного слова 0-100%
    static double compareWord_helper(const QString& s1, const QString& s2, bool to_lower);

    //! Не нормализованная "стоимость" замены символа для алгоритма вычисления схожести строк
    static int cost_helper(const QChar& c1, const QChar& c2);
    //! Сформировать всевозможные комбинации пар символов на основе фонетических групп
    static void preparePhoneticGroups();

    //! Фонетические группы
    static const QMultiMap<QChar, int> _phonetic_groups;
    //! _phonetic_groups_prepared был заполнен данными
    static bool _is_phonetic_groups_prepared;
    //! Фонетические группы, подготовленные для быстрого поиска
    static QSet<QString> _phonetic_groups_prepared;

    //! Макс. кол-во слов в строке
    static const int _max_word_count = 1024;
    //! Макс. длина слова
    static const int _max_word_lenght = 256;

    //! Рейтинг по каждому слову. Выделение памяти для оптимизации
    static QVector<double>* _words_rate;
    //! Макс. длина из каждой пары строк. Выделение памяти для оптимизации
    static QVector<int>* _words_lenght;

    //! Память для работы compareWord. Выделение памяти для оптимизации
    static QVector<QVector<int>>* _matrix;

    static QMutex _phonetic_groups_mutex;
    static QMutex _words_rate_mutex;
    static QMutex _matrix_mutex;
};

