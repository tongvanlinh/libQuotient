#pragma once

#include <ranges>

namespace Quotient {

//! \brief An indexOf() alternative for any range
//!
//! Unlike QList::indexOf(), returns `range.size()` if \p value is not found
template <typename RangeT, typename ValT, typename ProjT = std::identity>
    requires std::indirectly_comparable<std::ranges::iterator_t<RangeT>, const ValT*,
                                        std::ranges::equal_to, ProjT>
inline auto findIndex(const RangeT& range, const ValT& value, ProjT proj = {})
{
    using namespace std::ranges;
    return distance(begin(range), find(range, value, std::move(proj)));
}

}
