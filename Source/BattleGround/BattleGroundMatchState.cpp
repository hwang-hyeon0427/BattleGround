#include "BattleGroundMatchState.h"

#include "Net/UnrealNetwork.h"

void ABattleGroundMatchState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(ABattleGroundMatchState, Status);
}

int32 ABattleGroundMatchState::GetHealth(int32 Seat) const
{
    if (Seat == 1)
    {
        return Status.SeatOneHealth;
    }
    return Seat == 2 ? Status.SeatTwoHealth : 0;
}

void ABattleGroundMatchPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(ABattleGroundMatchPlayerState, Seat);
}
