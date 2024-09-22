// SPDX-FileCopyrightText: 2023 Tobias Fella <tobias.fella@kde.org>
// SPDX-License-Identifier: LGPL-2.0-or-later

#include "cryptoutils.h"
#include "e2ee_common.h"

#include "../logging_categories_p.h"
#include "../util.h"

#include <openssl/aes.h>
#include <openssl/hmac.h>
#include <openssl/err.h>

using namespace Quotient;

// The checks below make sure the definitions in cryptoutils.h match those in
// OpenSSL headers

static_assert(AesBlockSize == AES_BLOCK_SIZE);
static_assert(std::is_same_v<SslErrorCode, decltype(ERR_get_error())>);
static_assert(SslErrorUserOffset == ERR_LIB_USER);

//! \brief A wrapper for `std::unique_ptr` for use with OpenSSL context functions
//!
//! This class and the deduction guide for it are merely to remove
//! the boilerplate necessary to pass custom deleter to `std::unique_ptr`.
//! Usage: `const ContextHolder ctx(CTX_new(), &CTX_free);`, where `CTX_new` and
//! `CTX_free` are the matching allocation and deallocation functions from
//! OpenSSL API. You can pass additional parameters to the allocation function
//! as needed; the deallocation function is assumed to take exactly one
//! parameter of the same type that is returned by the allocation function.
template <class Context>
class ContextHolder : public std::unique_ptr<Context, void (*)(Context*)> {
public:
    using std::unique_ptr<Context, void (*)(Context*)>::unique_ptr;
};
template <class CryptoContext, typename Deleter>
ContextHolder(CryptoContext*, Deleter) -> ContextHolder<CryptoContext>;

template <typename SizeT>
    requires (sizeof(SizeT) >= sizeof(int))
inline std::pair<int, bool> checkedSize(
    SizeT uncheckedSize,
    std::type_identity_t<SizeT> maxSize = std::numeric_limits<int>::max())
// ^ NB: usage of type_identity_t disables type deduction
{
    Q_ASSERT(uncheckedSize >= 0 && maxSize >= 0);
    if (uncheckedSize <= maxSize) [[likely]]
        return { static_cast<int>(uncheckedSize), false };

    qCCritical(E2EE) << "Cryptoutils:" << uncheckedSize
                     << "bytes is too many for OpenSSL, first" << maxSize
                     << "bytes will be taken";
    return { maxSize, true };
}

#define CLAMP_SIZE(SizeVar_, ByteArray_, ...)                                               \
    const auto [SizeVar_, ByteArray_##Clamped] =                                            \
        checkedSize((ByteArray_).size() __VA_OPT__(, ) __VA_ARGS__);                        \
    if (QUO_ALARM_X(ByteArray_##Clamped,                                                    \
                    u"" #ByteArray_                                                         \
                    " is %1 bytes long, too much for OpenSSL and overall suspicious"_s.arg( \
                        (ByteArray_).size())))                                              \
        return std::unexpected<SslErrorCode>(SslPayloadTooLong);                            \
    do {} while (false)                                                                     \
// End of macro

#define CALL_OPENSSL(Call_)                                                    \
    do {                                                                       \
        if ((Call_) <= 0) {                                                    \
            qCWarning(E2EE) << std::source_location::current().function_name() \
                            << "failed to call OpenSSL API:"                   \
                            << ERR_error_string(ERR_get_error(), nullptr);     \
            return std::unexpected(ERR_get_error());                           \
        }                                                                      \
    } while (false)                                                            \
// End of macro

SslExpected<QByteArray> Quotient::aesCtr256Decrypt(const QByteArray& ciphertext,
                                                   byte_view_t<Aes256KeySize> key,
                                                   byte_view_t<AesBlockSize> iv)
{
    CLAMP_SIZE(ciphertextSize, ciphertext);

    const ContextHolder context(EVP_CIPHER_CTX_new(), &EVP_CIPHER_CTX_free);
    if (!context) {
        qCCritical(E2EE)
            << "aesCtr256Decrypt() failed to create cipher context:"
            << ERR_error_string(ERR_get_error(), nullptr);
        Q_ASSERT(context);
        return std::unexpected(ERR_get_error());
    }

    auto decrypted = zeroedByteArray(ciphertextSize);
    int decryptedLength = 0;
    {
        const auto decryptedSpan = asWritableCBytes(decrypted);
        CALL_OPENSSL(EVP_DecryptInit_ex(context.get(), EVP_aes_256_ctr(),
                                        nullptr, key.data(), iv.data()));
        CALL_OPENSSL(EVP_DecryptUpdate(context.get(), decryptedSpan.data(),
                                       &decryptedLength,
                                       asCBytes(ciphertext).data(),
                                       ciphertextSize));
        int tailLength = -1;
        CALL_OPENSSL(
            EVP_DecryptFinal_ex(context.get(),
                                decryptedSpan.subspan(static_cast<size_t>(decryptedLength)).data(),
                                &tailLength));
        Q_ASSERT_X(tailLength == 0, std::source_location::current().function_name(),
                   "Decrypt operation finalizer returned non-zero-size tail - this should not "
                   "happen with AES CTR algorithm.");
        decryptedLength += tailLength;
    }
    decrypted.resize(decryptedLength);
    return decrypted;
}
