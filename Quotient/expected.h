// SPDX-FileCopyrightText: 2022 Kitsune Ral <Kitsune-Ral@users.sf.net>
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <expected>

namespace Quotient {

template <typename T, typename E>
    requires (!std::is_same_v<T, E>)
class [[deprecated("Use std::expected instead")]] Expected : public std::expected<T, E> {
public:
    using std::expected<T, E>::expected;

    template <std::convertible_to<E> X>
    explicit(false) Expected(X&& x) : std::expected<T, E>(std::unexpect, std::forward<X>(x))
    {}

    T&& move_value_or(T&& fallback)
    {
        if (this->has_value())
            return std::move(this->value());
        return std::move(fallback);
    }
};

} // namespace Quotient
