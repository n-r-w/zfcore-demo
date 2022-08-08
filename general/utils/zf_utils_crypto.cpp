#include "zf_utils.h"
#include "simple_crypt/zf_simplecrypt.h"

namespace zf
{
// Ключ xor шифрования по умолчанию
#define Z_DEFAULT_XOR_KEY 6386921

QMap<quint64, SimpleCrypt*> Utils::_crypt_info;
const QByteArray Utils::_crypt_version = "zf_core_01";
QMutex Utils::_crypt_mutex;

SimpleCrypt* Utils::getCryptHelper(quint64 key)
{
    QMutexLocker lock(&_crypt_mutex);

    SimpleCrypt* crypt = _crypt_info.value(key, nullptr);
    if (!crypt) {
        crypt = new SimpleCrypt(key);
        crypt->setCompressionMode(SimpleCrypt::CompressionAlways);
        crypt->setIntegrityProtectionMode(SimpleCrypt::ProtectionHash);

        _crypt_info[key] = crypt;
    }
    return crypt;
}

QVariant Utils::decodeVariant(const QVariant& v, quint64 key)
{
    if (key == 0)
        key = Z_DEFAULT_XOR_KEY;

    if (!v.isValid())
        return QVariant();

    if (v.type() != QVariant::ByteArray)
        return QVariant();

    SimpleCrypt* crypt = getCryptHelper(key);

    QByteArray decoded = crypt->decryptToByteArray(v.toByteArray());
    if (decoded.left(_crypt_version.length()) != _crypt_version)
        return QVariant(); // не та версия структуры данных

    // отрезаем часть с версией
    decoded = decoded.right(decoded.length() - _crypt_version.length());

    QDataStream ds(decoded);
    ds.setVersion(QDataStream::Qt_4_0);
    QVariant res;
    ds >> res;

    return res;
}

QVariant Utils::encodeVariant(const QVariant& v, quint64 key)
{
    if (key == 0)
        key = Z_DEFAULT_XOR_KEY;

    SimpleCrypt* crypt = getCryptHelper(key);

    QByteArray b;
    QDataStream ds(&b, QIODevice::WriteOnly);
    ds.setVersion(QDataStream::Qt_4_0);
    ds << v;

    // Добавляем версию
    b = _crypt_version + b;
    return QVariant(crypt->encryptToByteArray(b));
}

} // namespace zf
