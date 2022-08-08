#pragma once

#include "zf_error.h"
#include <QIntValidator>
#include <QDoubleValidator>
#include <QMutex>

namespace zf
{
/*! Разные проверки
 *  Алгоритмы проверки контрольных сумм взяты из https://github.com/Kholenkov/js-data-validation/blob/master/data-validation.js */
class ZCORESHARED_EXPORT Validators
{
public:
    //! Состояние валидатора для виджета. Если валидатора нет, то Acceptable
    static QValidator::State validatorState(const QWidget* w);
    //! Проверка текста на валидатор
    static QValidator::State validatorState(const QValidator* v, const QString& text);
    //! Проверка текста на валидатор
    static QValidator::State validatorState(const QScopedPointer<QValidator>& v, const QString& text);
    //! Проверка QVariant на валидатор
    static QValidator::State validatorState(const QValidator* v, const QVariant& value);
    //! Проверка QVariant на валидатор
    static QValidator::State validatorState(const QScopedPointer<QValidator>& v, const QVariant& value);

    //! Проверка состояния валидатора для виджета на Acceptable. Если валидатора нет, то true
    static bool validatorCheck(const QWidget* w);
    //! Проверка текста на Acceptabl
    static bool validatorCheck(const QValidator* v, const QString& text);
    //! Проверка текста на Acceptabl
    static bool validatorCheck(const QScopedPointer<QValidator>& v, const QString& text);
    //! Проверка QVariant на Acceptabl
    static bool validatorCheck(const QValidator* v, const QVariant& value);
    //! Проверка QVariant на Acceptabl
    static bool validatorCheck(const QScopedPointer<QValidator>& v, const QVariant& value);

    //! Проверка текста на регулярное выражение
    static bool reqExpCheck(const QString& regexp, const QString& text);
    //! Проверка текста на регулярное выражение
    static bool reqExpCheck(const QRegularExpression& regexp, const QString& text);
    //! Проверка QVariant на регулярное выражение
    static bool reqExpCheck(const QString& regexp, const QVariant& value);
    //! Проверка QVariant на регулярное выражение
    static bool reqExpCheck(const QRegularExpression& regexp, const QVariant& value);

    //! Проверка email на корректность
    static Error checkEmail(const QString& s);
    //! Создать валидатор email
    static QValidator* createEmail(QObject* parent = nullptr);

    //! Создать валидатор телефона
    static QValidator* createPhone(QLocale::Country country = QLocale::AnyCountry, QObject* parent = nullptr);
    //! Проверка телефона на корректность
    static Error checkPhone(const QString& s, QLocale::Country country = QLocale::AnyCountry);

    //! Проверка int
    static QIntValidator* createInt(QObject* parent = nullptr);
    static QIntValidator* createInt(int bottom, int top, QObject* parent = nullptr);
    //! Проверка double
    static QDoubleValidator* createDouble(QObject* parent = nullptr);
    static QDoubleValidator* createDouble(double bottom, double top, int decimals, QObject* parent = nullptr);

    //! Паспорт серия
    static QValidator* createPassportSerialRus(QObject* parent = nullptr);
    //! Проверка Паспорт серия
    static Error checkPassportSerialRus(const QString& v);

    //! Паспорт номер
    static QValidator* createPassportNumberRus(QObject* parent = nullptr);
    //! Проверка Паспорт номер
    static Error checkPassportNumberRus(const QString& v);

    //! Паспорт подразделение
    static QValidator* createPassportDivisionRus(QObject* parent = nullptr);
    //! Проверка Паспорт подразделение
    static Error checkPassportDivisionRus(const QString& v);

    //! IBAN
    static QValidator* createIBAN(QObject* parent = nullptr);
    //! Проверка IBAN
    static Error checkIBAN(const QString& v);

    //! SWIFT
    static QValidator* createSWIFT(QObject* parent = nullptr);
    //! Проверка SWIFT
    static Error checkSWIFT(const QString& v);

    //! Иностранный счет
    static QValidator* createForeignAccount(QObject* parent = nullptr);
    //! Проверка иностранного счет
    static Error checkForeignAccount(const QString& v);

    //^\w{5,20}$

    //! БИК РФ
    static QValidator* createBIKRus(QObject* parent = nullptr);
    //! Проверка БИК РФ
    static Error checkBIKRus(const QString& v);

    //! Рассчетный счет РФ
    static QValidator* createRSRus(QObject* parent = nullptr);
    //! Проверка рассчетного счета РФ c учетом контрольной суммы
    static Error checkRSRus(const QString& bik, const QString& v);

    //! Коррсчет РФ
    static QValidator* createKSRus(QObject* parent = nullptr);
    //! Проверка коррсчета счета РФ c учетом контрольной суммы
    static Error checkKSRus(const QString& bik, const QString& ks);

    //! Вид ИНН
    enum class INN_type
    {
        Phisical = 1,
        Juridical = 2,
        Any = Phisical | Juridical
    };

    //! ИНН РФ
    static QValidator* createINNRus(
        //! Юридическое или физическое лицо
        INN_type type, QObject* parent = nullptr);
    //! Проверка ИНН РФ c учетом контрольной суммы
    static Error checkINNRus(
        //! Юридическое или физическое лицо
        INN_type type, const QString& v);

    //! КПП РФ
    static QValidator* createKPPRus(QObject* parent = nullptr);
    //! Проверка КПП РФ
    static Error checkKPPRus(const QString& v);

    //! Снилс РФ
    static QValidator* createSNILSRus(QObject* parent = nullptr);
    //! Проверка СНИЛС РФ c учетом контрольной суммы
    static Error checkSNILSRus(const QString& v);

private:
    //! Регулярка для проверки эл.почты
    static const QString _EMAIL_REGEXP;
    static std::unique_ptr<QValidator> _email_validator;
    static QMutex _email_validator_mutex;

    //! Регулярка для проверки телефона
    static const QMap<QLocale::Country, QString> _PHONE_REGEXP;
    static std::unique_ptr<QMap<QLocale::Country, std::shared_ptr<QValidator>>> _phone_validator;
    static QMutex _phone_validator_mutex;
};

} // namespace zf
