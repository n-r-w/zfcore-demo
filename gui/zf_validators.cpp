#include "zf_core.h"
#include "zf_validators.h"
#include "zf_translation.h"

#include <QLineEdit>
#include <QComboBox>

namespace zf
{
//! Регулярка для проверки эл.почты
const QString Validators::_EMAIL_REGEXP = R"(.+@.+\..+)";
std::unique_ptr<QValidator> Validators::_email_validator;
QMutex Validators::_email_validator_mutex;

//! Регулярка для проверки телефона
const QMap<QLocale::Country, QString> Validators::_PHONE_REGEXP = {}; // пока без проверки
std::unique_ptr<QMap<QLocale::Country, std::shared_ptr<QValidator>>> Validators::_phone_validator;
QMutex Validators::_phone_validator_mutex;

// Проверка int
class IntValidator : public QIntValidator
{
public:
    explicit IntValidator(QObject* parent = nullptr);
    IntValidator(int bottom, int top, QObject* parent = nullptr);

    QValidator::State validate(QString& input, int& pos) const override;
    void fixup(QString& input) const override;
};

// Проверка double
class DoubleValidator : public QDoubleValidator
{
public:
    explicit DoubleValidator(QObject* parent = nullptr);
    DoubleValidator(double bottom, double top, int decimals, QObject* parent = nullptr);

    QValidator::State validate(QString& input, int& pos) const override;
    void fixup(QString& input) const override;

private:
    void prepare(QString& s) const;
};

IntValidator::IntValidator(QObject* parent)
    : QIntValidator(parent)
{
    setLocale(*Core::locale(LocaleType::UserInterface));
}

IntValidator::IntValidator(int bottom, int top, QObject* parent)
    : QIntValidator(bottom, top, parent)
{
    setLocale(*Core::locale(LocaleType::UserInterface));
}

QValidator::State IntValidator::validate(QString& input, int& pos) const
{
    if (input.trimmed().simplified().isEmpty())
        return Acceptable;

    bool ok;
    auto v = locale().toLongLong(input, &ok);
    if (ok)
        return v >= bottom() && v <= top() ? QValidator::Acceptable : QValidator::Invalid;

    return QIntValidator::validate(input, pos);
}

void IntValidator::fixup(QString& input) const
{
    input = input.trimmed().simplified();
    QIntValidator::fixup(input);
}

DoubleValidator::DoubleValidator(QObject* parent)
    : QDoubleValidator(parent)
{
    setLocale(*Core::locale(LocaleType::UserInterface));
}

DoubleValidator::DoubleValidator(double bottom, double top, int decimals, QObject* parent)
    : QDoubleValidator(bottom, top, decimals, parent)
{
    setLocale(*Core::locale(LocaleType::UserInterface));
}

QValidator::State DoubleValidator::validate(QString& input, int& pos) const
{
    QString prepared = input.trimmed().simplified();

    if (prepared.isEmpty())
        return Acceptable;

    prepare(input);

    int point = prepared.indexOf(locale().decimalPoint());
    if (point >= 0 && prepared.length() - point - 1 > decimals())
        return QValidator::Invalid;

    bool ok;
    auto v = locale().toDouble(input, &ok);
    if (ok)
        return (v > bottom() || fuzzyCompare(v, bottom())) && (v < top() || fuzzyCompare(v, top())) ? QValidator::Acceptable : QValidator::Invalid;

    return QDoubleValidator::validate(input, pos);
}

void DoubleValidator::fixup(QString& input) const
{
    input = input.trimmed().simplified();

    prepare(input);

    QDoubleValidator::fixup(input);
}

void DoubleValidator::prepare(QString& s) const
{
    if (locale().decimalPoint() == ',')
        s.replace('.', ',');
    if (locale().decimalPoint() == '.')
        s.replace(',', '.');
}

// Валидатор телефона по умолчанию
class DefaultPhoneValidator : public QValidator
{
public:
    DefaultPhoneValidator(QObject* parent = nullptr)
        : QValidator(parent)
    {
    }
    State validate(QString& text, int&) const override
    {
        QString s = text.simplified();
        for (int i = 0; i < s.length(); i++) {
            if (s.at(i) < '0' || s.at(i) > '9')
                return Invalid;
        }

        if (s.length() < 11 || s.length() > 13)
            return Intermediate;

        return Acceptable;
    }
};

// Исправление бага QLineEdit, который запихивает маску в проверку на регулярку
class MaskedRegularExpressionValidator : public QRegularExpressionValidator
{
public:
    MaskedRegularExpressionValidator(QObject* parent = nullptr)
        : QRegularExpressionValidator(parent)
    {
    }
    MaskedRegularExpressionValidator(const QRegularExpression& re, QObject* parent = nullptr)
        : QRegularExpressionValidator(re, parent)
    {
    }

    QValidator::State validate(QString& input, int& pos) const override
    {
        // выпиливаем маску перед проверкой
        QString fixed = input;
        fixed.replace('_', "");
        return QRegularExpressionValidator::validate(fixed, pos);
    }
};

QValidator* Validators::createEmail(QObject* parent)
{
    return new QRegularExpressionValidator(QRegularExpression(_EMAIL_REGEXP), parent);
}

QValidator* Validators::createPhone(QLocale::Country country, QObject* parent)
{
    auto r = _PHONE_REGEXP.constFind(country);
    if (r == _PHONE_REGEXP.constEnd())
        return new DefaultPhoneValidator(parent);

    return new MaskedRegularExpressionValidator(QRegularExpression(r.value()), parent);
}

QValidator::State Validators::validatorState(const QWidget* w)
{
    Z_CHECK_NULL(w);
    const QValidator* validator = nullptr;
    if (auto x = qobject_cast<const QLineEdit*>(w)) {
        validator = x->validator();
        if (validator == nullptr)
            return QValidator::Acceptable;

        return validatorState(validator, x->text());

    } else if (auto x = qobject_cast<const QComboBox*>(w)) {
        if (!x->isEditable())
            return QValidator::Acceptable;

        validator = x->validator();
        if (validator == nullptr)
            return QValidator::Acceptable;

        return validatorState(validator, x->lineEdit()->text());
    }

    return QValidator::Acceptable;
}

bool Validators::validatorCheck(const QWidget* w)
{
    return validatorState(w) == QValidator::Acceptable;
}

bool Validators::validatorCheck(const QValidator* v, const QString& text)
{
    return validatorState(v, text) == QValidator::Acceptable;
}

bool Validators::validatorCheck(const QScopedPointer<QValidator>& v, const QString& text)
{
    return validatorCheck(v.data(), text);
}

bool Validators::validatorCheck(const QValidator* v, const QVariant& value)
{
    return validatorCheck(v, Utils::variantToString(value));
}

bool Validators::validatorCheck(const QScopedPointer<QValidator>& v, const QVariant& value)
{
    return validatorCheck(v, Utils::variantToString(value));
}

bool Validators::reqExpCheck(const QString& regexp, const QString& text)
{
    return reqExpCheck(QRegularExpression(regexp), text);
}

bool Validators::reqExpCheck(const QString& regexp, const QVariant& value)
{
    return reqExpCheck(regexp, Utils::variantToString(value));
}

bool Validators::reqExpCheck(const QRegularExpression& regexp, const QString& text)
{
    return regexp.match(text, 0, QRegularExpression::NormalMatch).hasMatch();
}

bool Validators::reqExpCheck(const QRegularExpression& regexp, const QVariant& value)
{
    return reqExpCheck(regexp, Utils::variantToString(value));
}

Error Validators::checkEmail(const QString& s)
{
    QMutexLocker lock(&_email_validator_mutex);

    if (_email_validator == nullptr)
        _email_validator.reset(createEmail());

    QString text = s;
    int pos = 0;
    auto res = _email_validator->validate(text, pos);
    return res == QValidator::Acceptable ? Error() : Error(translate(TR::ZFT_BAD_VALUE));
}

Error Validators::checkPhone(const QString& s, QLocale::Country country)
{
    QMutexLocker lock(&_phone_validator_mutex);

    if (_phone_validator == nullptr)
        _phone_validator = std::make_unique<QMap<QLocale::Country, std::shared_ptr<QValidator>>>();

    auto v = _phone_validator->constFind(country);
    std::shared_ptr<QValidator> validator;
    if (v == _phone_validator->constEnd()) {
        validator.reset(createPhone(country));
        _phone_validator->insert(country, validator);

    } else {
        validator = v.value();
    }

    QString text = s;
    int pos = 0;
    return validator->validate(text, pos) == QValidator::Acceptable ? Error() : Error(translate(TR::ZFT_BAD_VALUE));
}

QIntValidator* Validators::createInt(QObject* parent)
{
    return new IntValidator(parent);
}

QIntValidator* Validators::createInt(int bottom, int top, QObject* parent)
{
    return new IntValidator(bottom, top, parent);
}

QDoubleValidator* Validators::createDouble(QObject* parent)
{
    return new DoubleValidator(parent);
}

QDoubleValidator* Validators::createDouble(double bottom, double top, int decimals, QObject* parent)
{
    return new DoubleValidator(bottom, top, decimals, parent);
}

QValidator* Validators::createPassportSerialRus(QObject* parent)
{
    return new MaskedRegularExpressionValidator(QRegularExpression(QStringLiteral(R"(^[0-9]{4}$)")), parent);
}

Error Validators::checkPassportSerialRus(const QString& v)
{
    QScopedPointer<QValidator> validator(createPassportSerialRus());
    if (validatorState(validator, v) != QValidator::Acceptable)
        return Error(translate(TR::ZFT_BAD_VALUE));
    return {};
}

QValidator* Validators::createPassportNumberRus(QObject* parent)
{
    return new MaskedRegularExpressionValidator(QRegularExpression(QStringLiteral(R"(^[0-9]{6}$)")), parent);
}

Error Validators::checkPassportNumberRus(const QString& v)
{
    QScopedPointer<QValidator> validator(createPassportNumberRus());
    if (validatorState(validator, v) != QValidator::Acceptable)
        return Error(translate(TR::ZFT_BAD_VALUE));
    return {};
}

QValidator* Validators::createPassportDivisionRus(QObject* parent)
{
    return new MaskedRegularExpressionValidator(QRegularExpression(QStringLiteral(R"(^[0-9]{6}$)")), parent);
}

Error Validators::checkPassportDivisionRus(const QString& v)
{
    QScopedPointer<QValidator> validator(createPassportDivisionRus());
    if (validatorState(validator, v) != QValidator::Acceptable)
        return Error(translate(TR::ZFT_BAD_VALUE));
    return {};
}

QValidator* Validators::createIBAN(QObject* parent)
{
    return new QRegularExpressionValidator(
        QRegularExpression(QStringLiteral(
            R"(^([A-Z]{2}[ \-]?[0-9]{2})(?=(?:[ \-]?[A-Z0-9]){9,30}$)((?:[ \-]?[A-Z0-9]{3,5}){2,7})([ \-]?[A-Z0-9]{1,3})?$)")),
        parent);
}

Error Validators::checkIBAN(const QString& v)
{
    QScopedPointer<QValidator> validator(createIBAN());
    if (validatorState(validator, v) != QValidator::Acceptable)
        return Error(translate(TR::ZFT_BAD_VALUE));
    return {};
}

QValidator* Validators::createSWIFT(QObject* parent)
{
    return new QRegularExpressionValidator(QRegularExpression(QStringLiteral(R"(^[A-Z]{6}[A-Z0-9]{2}([A-Z0-9]{3})?$)")), parent);
}

Error Validators::checkSWIFT(const QString& v)
{
    QScopedPointer<QValidator> validator(createSWIFT());
    if (validatorState(validator, v) != QValidator::Acceptable)
        return Error(translate(TR::ZFT_BAD_VALUE));
    return {};
}

QValidator* Validators::createForeignAccount(QObject* parent)
{
    return new QRegularExpressionValidator(QRegularExpression(QStringLiteral(R"(^\w{5,20}$)")), parent);
}

Error Validators::checkForeignAccount(const QString& v)
{
    QScopedPointer<QValidator> validator(createForeignAccount());
    if (validatorState(validator, v) != QValidator::Acceptable)
        return Error(translate(TR::ZFT_BAD_VALUE));
    return {};
}

QValidator* Validators::createBIKRus(QObject* parent)
{
    return new MaskedRegularExpressionValidator(QRegularExpression(QStringLiteral(R"(^[0-9]{9}$)")), parent);
}

Error Validators::checkBIKRus(const QString& v)
{
    QScopedPointer<QValidator> validator(createBIKRus());
    if (validatorState(validator, v) != QValidator::Acceptable)
        return Error(translate(TR::ZFT_BAD_VALUE));
    return {};
}

QValidator* Validators::createRSRus(QObject* parent)
{
    return new MaskedRegularExpressionValidator(QRegularExpression(QStringLiteral(R"(^(?:[\. ]*\d){20}$)")), parent);
}

Error Validators::checkRSRus(const QString& bik, const QString& rs)
{
    if (checkBIKRus(bik).isError())
        return Error(translate(TR::ZFT_BAD_VALUE));

    QScopedPointer<QValidator> validator(createRSRus());
    if (validatorState(validator, rs) != QValidator::Acceptable)
        return Error(translate(TR::ZFT_BAD_VALUE));

    // проверка контрольной суммы
    /*
var bikRs = bik.toString().slice(-3) + rs;
var checksum = 0;
var coefficients = [7, 1, 3, 7, 1, 3, 7, 1, 3, 7, 1, 3, 7, 1, 3, 7, 1, 3, 7, 1, 3, 7, 1];
for (var i in coefficients) {
    checksum += coefficients[i] * (bikRs[i] % 10);
}
if (checksum % 10 === 0) {
    result = true;
} else {
    error.code = 4;
    error.message = 'Неправильное контрольное число';
}
    */

    QString bik_rs = bik.right(3) + rs;
    int checksum = 0;
    static const QList<int> coefficients = {7, 1, 3, 7, 1, 3, 7, 1, 3, 7, 1, 3, 7, 1, 3, 7, 1, 3, 7, 1, 3, 7, 1};
    for (int i = 0; i < coefficients.count(); i++) {
        checksum += coefficients.at(i) * (bik_rs.at(i).digitValue() % 10);
    }

    if ((checksum % 10) == 0)
        return Error();

    return Error(translate(TR::ZFT_BAD_CHECKSUM));
}

QValidator* Validators::createKSRus(QObject* parent)
{
    return new MaskedRegularExpressionValidator(QRegularExpression(QStringLiteral(R"(^(?:[\. ]*\d){20}$)")), parent);
}

Error Validators::checkKSRus(const QString& bik, const QString& ks)
{
    if (checkBIKRus(bik).isError())
        return Error(translate(TR::ZFT_BAD_VALUE));

    QScopedPointer<QValidator> validator(createKSRus());
    if (validatorState(validator, ks) != QValidator::Acceptable)
        return Error(translate(TR::ZFT_BAD_VALUE));

    /*
var bikKs = '0' + bik.toString().slice(4, 6) + ks;
var checksum = 0;
var coefficients = [7, 1, 3, 7, 1, 3, 7, 1, 3, 7, 1, 3, 7, 1, 3, 7, 1, 3, 7, 1, 3, 7, 1];
for (var i in coefficients) {
    checksum += coefficients[i] * (bikKs[i] % 10);
}
if (checksum % 10 === 0) {
    result = true;
} else {
    error.code = 4;
    error.message = 'Неправильное контрольное число';
}
    */

    QString bik_ks = "0" + bik.mid(4, 2) + ks;
    int checksum = 0;
    static const QList<int> coefficients = {7, 1, 3, 7, 1, 3, 7, 1, 3, 7, 1, 3, 7, 1, 3, 7, 1, 3, 7, 1, 3, 7, 1};
    for (int i = 0; i < coefficients.count(); i++) {
        checksum += coefficients.at(i) * (bik_ks.at(i).digitValue() % 10);
    }

    if ((checksum % 10) == 0)
        return Error();

    return Error(translate(TR::ZFT_BAD_CHECKSUM));
}

QValidator* Validators::createINNRus(INN_type type, QObject* parent)
{
    QString regexp;
    if ((static_cast<int>(type) & static_cast<int>(INN_type::Phisical)) == static_cast<int>(INN_type::Phisical))
        regexp = QStringLiteral(R"(^([0-9]{12})$)");
    else if ((static_cast<int>(type) & static_cast<int>(INN_type::Juridical)) == static_cast<int>(INN_type::Juridical))
        regexp = QStringLiteral(R"(^([0-9]{10})$)");
    else
        regexp = QStringLiteral(R"(^([0-9]{10}|[0-9]{12})$)");

    return new MaskedRegularExpressionValidator(QRegularExpression(regexp), parent);
}

Error Validators::checkINNRus(INN_type type, const QString& inn)
{
    QScopedPointer<QValidator> validator(createINNRus(type));
    if (validatorState(validator, inn) != QValidator::Acceptable)
        return Error(translate(TR::ZFT_BAD_VALUE));

    /*
var checkDigit = function (inn, coefficients) {
    var n = 0;
    for (var i in coefficients) {
        n += coefficients[i] * inn[i];
    }
    return parseInt(n % 11 % 10);
};
switch (inn.length) {
    case 10:
        var n10 = checkDigit(inn, [2, 4, 10, 3, 5, 9, 4, 6, 8]);
        if (n10 === parseInt(inn[9])) {
            result = true;
        }
        break;
    case 12:
        var n11 = checkDigit(inn, [7, 2, 4, 10, 3, 5, 9, 4, 6, 8]);
        var n12 = checkDigit(inn, [3, 7, 2, 4, 10, 3, 5, 9, 4, 6, 8]);
        if ((n11 === parseInt(inn[10])) && (n12 === parseInt(inn[11]))) {
            result = true;
        }
        break;
}
if (!result) {
    error.code = 4;
    error.message = 'Неправильное контрольное число';
}
    */

    auto checkDigit = [](const QString& inn, const QList<int>& coefficients) -> int {
        int n = 0;
        for (int i = 0; i < coefficients.count(); i++) {
            n += coefficients.at(i) * inn.at(i).digitValue();
        }
        return (n % 11) % 10;
    };

    if (inn.length() == 10) {
        int n = checkDigit(inn, {2, 4, 10, 3, 5, 9, 4, 6, 8});
        if (n == inn.at(9).digitValue())
            return Error();

    } else if (inn.length() == 12) {
        int n = checkDigit(inn, {7, 2, 4, 10, 3, 5, 9, 4, 6, 8});
        if (n == inn.at(10).digitValue()) {
            n = checkDigit(inn, {3, 7, 2, 4, 10, 3, 5, 9, 4, 6, 8});
            if (n == inn.at(11).digitValue())
                return Error();
        }

    } else {
        return Error(translate(TR::ZFT_BAD_VALUE));
    }

    return Error(translate(TR::ZFT_BAD_CHECKSUM));
}

QValidator* Validators::createKPPRus(QObject* parent)
{
    return new MaskedRegularExpressionValidator(QRegularExpression(QStringLiteral(R"(^[0-9]{9}$)")), parent);
}

Error Validators::checkKPPRus(const QString& v)
{
    QScopedPointer<QValidator> validator(createKPPRus());
    if (validatorState(validator, v) != QValidator::Acceptable)
        return Error(translate(TR::ZFT_BAD_VALUE));
    return {};
}

QValidator* Validators::createSNILSRus(QObject* parent)
{
    return new MaskedRegularExpressionValidator(QRegularExpression(QStringLiteral(R"(^(?:[- ]*\d){11}$)")), parent);
}

Error Validators::checkSNILSRus(const QString& snils)
{
    QScopedPointer<QValidator> validator(createSNILSRus());
    if (validatorState(validator, snils) != QValidator::Acceptable)
        return Error(translate(TR::ZFT_BAD_VALUE));

    /*
var sum = 0;
for (var i = 0; i < 9; i++) {
    sum += parseInt(snils[i]) * (9 - i);
}
var checkDigit = 0;
if (sum < 100) {
    checkDigit = sum;
} else if (sum > 101) {
    checkDigit = parseInt(sum % 101);
    if (checkDigit === 100) {
        checkDigit = 0;
    }
}
if (checkDigit === parseInt(snils.slice(-2))) {
    result = true;
} else {
    error.code = 4;
    error.message = 'Неправильное контрольное число';
}
*/
    // проверка только для номеров больше 001001998
    static const QString min_num = "001001998";
    static const int min_num_int = 1001998;

    bool ok;
    int snils_num_calc = snils.leftRef(min_num.length()).toInt(&ok);
    if (!ok)
        return Error(translate(TR::ZFT_BAD_VALUE));

    if (snils_num_calc < min_num_int)
        return Error();

    int sum = 0;
    for (int i = 0; i < 9; i++) {
        sum += snils.at(i).digitValue() * (9 - i);
    }

    int check_digit = 0;
    if (sum < 100) {
        check_digit = sum;

    } else if (sum > 101) {
        check_digit = sum % 101;
        if (check_digit == 100)
            check_digit = 0;
    }

    int ch = snils.rightRef(2).toInt(&ok);
    if (ok && check_digit == ch)
        return Error();

    return Error(translate(TR::ZFT_BAD_CHECKSUM));
}

QValidator::State Validators::validatorState(const QValidator* v, const QString& text)
{
    Z_CHECK_NULL(v);
    QString s = text;
    int pos = 0;
    return v->validate(s, pos);
}

QValidator::State Validators::validatorState(const QScopedPointer<QValidator>& v, const QString& text)
{
    return validatorState(v.data(), text);
}

QValidator::State Validators::validatorState(const QValidator* v, const QVariant& value)
{
    return validatorState(v, Utils::variantToString(value));
}

QValidator::State Validators::validatorState(const QScopedPointer<QValidator>& v, const QVariant& value)
{
    return validatorState(v, Utils::variantToString(value));
}

} // namespace zf
