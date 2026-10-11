#pragma once

#include <cerridwen/cross_domain_atomic.hpp>

namespace cerridwen {

using AtomicInt32 = CrossDomainAtomic<int32_t>;
using AtomicInt64 = CrossDomainAtomic<int64_t>;
using AtomicUint32 = CrossDomainAtomic<uint32_t>;
using AtomicUint64 = CrossDomainAtomic<uint64_t>;

} // namespace cerridwen
