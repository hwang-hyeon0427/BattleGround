#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineBaseTypes.h"
#include "Engine/GameInstance.h"
#include "BattleGroundGameInstance.generated.h"

UCLASS()
class BATTLEGROUND_API UBattleGroundGameInstance : public UGameInstance
{
    GENERATED_BODY()

public:
    virtual void Init() override;
    virtual void Shutdown() override;

    void HostRoom();
    void JoinRoom(const FString& Address);
    void ReturnToMenu();
    FText GetConnectionStatus() const { return ConnectionStatus; }
    bool IsConnecting() const { return PendingTravel != ERoomTravel::None; }

private:
    enum class ERoomTravel : uint8 { None, Hosting, Joining };

    void OnMapLoaded(UWorld* World);
    void OnNetworkFailure(UWorld* World, UNetDriver* Driver, ENetworkFailure::Type Type, const FString& Error);
    void OnTravelFailure(UWorld* World, ETravelFailure::Type Type, const FString& Error);
    void OpenMenu();

    ERoomTravel PendingTravel = ERoomTravel::None;
    FText ConnectionStatus;
    bool bReturningToMenu = false;
};
