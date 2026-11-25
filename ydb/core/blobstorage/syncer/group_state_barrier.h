#pragma once

#include "defs.h"

#include <ydb/core/blobstorage/groupinfo/blobstorage_groupinfo_mirror3dc_state.h>
#include <ydb/core/control/lib/immediate_control_board_wrapper.h>

#include <library/cpp/monlib/dynamic_counters/counters.h>

namespace NKikimr::NGroupSyncer {

// Registers Mirror3dc_<groupId>_HoldStateMask and Mirror3dc_<groupId>_AllowGeneration.
// Both default to zero. Controls are node-local, shared by the group's coordinators,
// and survive actor restarts, but not process restarts. Register before arming in NORMAL.
void RegisterGroupStateBarrierControls(ui32 groupId);

class TGroupStateBarrier {
    const TString GroupId;
    const TString CoordinatorId;
    TControlWrapper HoldStateMask;
    TControlWrapper AllowGeneration;

    ::NMonitoring::TDynamicCounterPtr GroupCounters;
    ::NMonitoring::TDynamicCounters::TCounterPtr Held;
    ::NMonitoring::TDynamicCounters::TCounterPtr Generation;
    ::NMonitoring::TDynamicCounters::TCounterPtr FromState;
    ::NMonitoring::TDynamicCounters::TCounterPtr ToState;

    void EnsureCounters();

public:
    TGroupStateBarrier(ui32 groupId, const TActorId& coordinatorId);
    ~TGroupStateBarrier();

    TGroupStateBarrier(const TGroupStateBarrier&) = delete;
    TGroupStateBarrier& operator=(const TGroupStateBarrier&) = delete;

    // Mask bits select the current state; an allowance applies only to this generation.
    bool IsEnabled(ui32 generation, EMirror3dcGroupState from) const;
    // Call only after automatic transition conditions have been checked. Held records
    // that this generation reached the barrier; it does not replace a fresh health check.
    bool ShouldHold(ui32 generation, EMirror3dcGroupState from, EMirror3dcGroupState to);
    void Reset();
};

} // namespace NKikimr::NGroupSyncer
