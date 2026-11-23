#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "BattleGroundMatchGameMode.generated.h"

UCLASS()
class BATTLEGROUND_API ABattleGroundMatchGameMode : public AGameModeBase
{
    GENERATED_BODY()

public:
    ABattleGroundMatchGameMode();

    virtual void PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId,
        FString& ErrorMessage) override;
    virtual APlayerController* Login(UPlayer* NewPlayer, ENetRole InRemoteRole, const FString& Portal,
        const FString& Options, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage) override;
    virtual void HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer) override;
    virtual void Logout(AController* Exiting) override;

    bool CanPlayerAttack(const AController* Controller) const;
    void ApplyDuelDamage(AController* Attacker, AController* Target, int32 Damage);

private:
    static constexpr int32 RoomCapacity = 2;

    int32 FindSeat(const AController* Controller) const;
    int32 CountPlayers() const;
    const TCHAR* GetJoinRejection(const FString& Options) const;

    TWeakObjectPtr<AController> Players[RoomCapacity];
};
