// SPDX-FileCopyrightText: 2023 Tobias Fella <tobias.fella@kde.org>
// SPDX-License-Identifier: LGPL-2.0-or-later

#pragma once

#include "e2ee_common.h"
#include <expected>

namespace Quotient {

// Common remark: OpenSSL is a private dependency of libQuotient, meaning that
// headers of libQuotient can't include OpenSSL headers. Instead, alias
// the value or type along with a comment (see SslErrorCode e.g.) and add
// static_assert in the .cpp file to check it against the OpenSSL definition.

constexpr auto Aes256KeySize = 32u;
constexpr auto AesBlockSize = 16u; // AES_BLOCK_SIZE
constexpr auto HmacKeySize = 32u;

// NOLINTNEXTLINE(google-runtime-int): the type is copied from OpenSSL
using SslErrorCode = unsigned long; // decltype(ERR_get_error())

enum SslErrorCodes : SslErrorCode {
    SslErrorUserOffset = 128, // ERR_LIB_USER; never use this bare
    WrongDerivedKeyLength = SslErrorUserOffset + 1,
    SslPayloadTooLong = SslErrorUserOffset + 2
};

template <typename T>
using SslExpected = std::expected<T, SslErrorCode>;

inline QByteArray zeroedByteArray(QByteArray::size_type n = 32) { return { n, '\0' }; }

// NOLINTNEXTLINE(google-runtime-int): the type is copied from OpenSSL
using SslErrorCode = unsigned long; // decltype(ERR_get_error())

//! Obtain a std::span<byte_t, N> looking into the passed buffer
template <size_t N = std::dynamic_extent>
inline auto asWritableCBytes(auto& buf)
{
    return _impl::spanFromBytes<byte_span_t<N>>(buf);
}

//! \brief Decrypt data using AES-CTR-256
//!
//! key and iv have a length of 32 bytes
QUOTIENT_API SslExpected<QByteArray> aesCtr256Decrypt(
    const QByteArray& ciphertext, byte_view_t<Aes256KeySize> key,
    byte_view_t<AesBlockSize> iv);

} // namespace Quotient
