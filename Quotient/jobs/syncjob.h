// SPDX-FileCopyrightText: 2016 Kitsune Ral <Kitsune-Ral@users.sf.net>
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "../csapi/definitions/sync_filter.h"
#include "../syncdata.h"
#include "basejob.h"

namespace Quotient {
class QUOTIENT_API SyncJob : public BaseJob {
public:
    static constexpr auto defaultTimeout = std::chrono::seconds(30);
    static constexpr auto defaultTimeoutMillis =
        std::chrono::milliseconds(defaultTimeout).count();

    explicit SyncJob(const QString& since = {}, const QString& filter = {},
                     int timeout = defaultTimeoutMillis, const QString& presence = {});
    explicit SyncJob(const QString& since, const Filter& filter, int timeout = defaultTimeoutMillis,
                     const QString& presence = {});

    SyncData takeData() { return std::move(d); }

protected:
    Status prepareResult() override;

private:
    SyncData d;
};
} // namespace Quotient
