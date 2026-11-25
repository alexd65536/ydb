#include "group_state_barrier.h"

#include <ydb/core/base/appdata.h>
#include <ydb/core/base/counters.h>
#include <ydb/core/control/lib/dynamic_control_board_impl.h>

#include <util/string/builder.h>
#include <util/string/cast.h>

#include <limits>

namespace NKikimr::NGroupSyncer {

namespace {

static_assert(NKikimrBlobStorage::TGroupState::EState_MAX < 63);
constexpr i64 MaxHoldStateMask = (i64{1} << (NKikimrBlobStorage::TGroupState::EState_MAX + 1)) - 1;
constexpr ui32 MaxGeneration = std::numeric_limits<ui32>::max();

void RegisterControls(ui32 groupId, TControlWrapper& holdStateMask, TControlWrapper& allowGeneration) {
    if (const auto& dcb = AppData()->Dcb) {
        const TString prefix = TStringBuilder() << "Mirror3dc_" << groupId;
        dcb->RegisterSharedControl(holdStateMask, prefix + "_HoldStateMask");
        dcb->RegisterSharedControl(allowGeneration, prefix + "_AllowGeneration");
    }
}

} // namespace

void RegisterGroupStateBarrierControls(ui32 groupId) {
    TControlWrapper holdStateMask(0, 0, MaxHoldStateMask);
    TControlWrapper allowGeneration(0, 0, MaxGeneration);
    RegisterControls(groupId, holdStateMask, allowGeneration);
}

TGroupStateBarrier::TGroupStateBarrier(ui32 groupId, const TActorId& coordinatorId)
    : GroupId(ToString(groupId))
    , CoordinatorId(coordinatorId.ToString())
    , HoldStateMask(0, 0, MaxHoldStateMask)
    , AllowGeneration(0, 0, MaxGeneration)
{
    RegisterControls(groupId, HoldStateMask, AllowGeneration);
}

TGroupStateBarrier::~TGroupStateBarrier() {
    Reset();
    if (GroupCounters) {
        GroupCounters->RemoveSubgroup("coordinator", CoordinatorId);
    }
}

void TGroupStateBarrier::EnsureCounters() {
    if (Held || !AppData()->Counters) {
        return;
    }
    GroupCounters = GetServiceCounters(AppData()->Counters, "bscontroller")
        ->GetSubgroup("component", "mirror3dc_transition_barrier")
        ->GetSubgroup("group", GroupId);
    const auto counters = GroupCounters->GetSubgroup("coordinator", CoordinatorId);
    Held = counters->GetCounter("Held");
    Generation = counters->GetCounter("Generation");
    FromState = counters->GetCounter("FromState");
    ToState = counters->GetCounter("ToState");
}

bool TGroupStateBarrier::IsEnabled(ui32 generation, EMirror3dcGroupState from) const {
    const auto state = static_cast<int>(from);
    return state >= 0 && state <= NKikimrBlobStorage::TGroupState::EState_MAX
        && (static_cast<ui64>(HoldStateMask) & (ui64{1} << state))
        && static_cast<ui32>(AllowGeneration) != generation;
}

bool TGroupStateBarrier::ShouldHold(ui32 generation, EMirror3dcGroupState from, EMirror3dcGroupState to) {
    if (!IsEnabled(generation, from)) {
        Reset();
        return false;
    }

    EnsureCounters();
    if (Held) {
        Held->Set(0);
        Generation->Set(generation);
        FromState->Set(static_cast<ui32>(from));
        ToState->Set(static_cast<ui32>(to));
        Held->Set(1);
    }
    return true;
}

void TGroupStateBarrier::Reset() {
    if (Held) {
        Held->Set(0);
        Generation->Set(0);
        FromState->Set(0);
        ToState->Set(0);
    }
}

} // namespace NKikimr::NGroupSyncer
