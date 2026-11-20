#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "BattleGroundMenuGameMode.generated.h"

class UBattleGroundMenuWidget;

UCLASS()
class BATTLEGROUND_API ABattleGroundMenuGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ABattleGroundMenuGameMode();

	virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;
	virtual void HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void ShowMenu();

	UPROPERTY(Transient)
	TObjectPtr<UBattleGroundMenuWidget> MenuWidget;
};
