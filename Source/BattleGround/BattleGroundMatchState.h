#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerState.h"
#include "BattleGroundMatchState.generated.h"

UENUM()
enum class EBattleGroundMatchPhase : uint8
{
    Waiting,
    Playing,
    Finished
};

USTRUCT()
struct FBattleGroundDuelStatus
{
    GENERATED_BODY()

    static constexpr int32 MaxHealth = 100;

    UPROPERTY()
    EBattleGroundMatchPhase Phase = EBattleGroundMatchPhase::Waiting;

    UPROPERTY()
    int32 PlayerCount = 0;

    UPROPERTY()
    int32 SeatOneHealth = MaxHealth;

    UPROPERTY()
    int32 SeatTwoHealth = MaxHealth;

    UPROPERTY()
    int32 WinnerSeat = 0;

    UPROPERTY()
    bool bForfeit = false;
};

UCLASS()
class BATTLEGROUND_API ABattleGroundMatchState : public AGameStateBase
{
    GENERATED_BODY()

public:
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    EBattleGroundMatchPhase GetPhase() const { return Status.Phase; }
    int32 GetPlayerCount() const { return Status.PlayerCount; }
    int32 GetWinnerSeat() const { return Status.WinnerSeat; }
    bool WasForfeit() const { return Status.bForfeit; }
    int32 GetHealth(int32 Seat) const;

private:
    friend class ABattleGroundMatchGameMode;

    UPROPERTY(Replicated)
    FBattleGroundDuelStatus Status;
};

UCLASS()
class BATTLEGROUND_API ABattleGroundMatchPlayerState : public APlayerState
{
    GENERATED_BODY()

public:
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    int32 GetSeat() const { return Seat; }

private:
    friend class ABattleGroundMatchGameMode;

    UPROPERTY(Replicated)
    int32 Seat = 0;
};
