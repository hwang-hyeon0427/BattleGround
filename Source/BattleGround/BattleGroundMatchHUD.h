#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "BattleGroundMatchHUD.generated.h"

class UBattleGroundMatchWidget;
class ULocalPlayer;
class UWidget;

UCLASS()
class BATTLEGROUND_API ABattleGroundMatchHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	void ToggleMenu();
	bool IsMenuOpen() const { return bMenuOpen; }

private:
	void ShowMatchHUD();
	void HideLegacyHealthBar(UWidget* Widget, ULocalPlayer* Player);

	UPROPERTY(Transient)
	TObjectPtr<UBattleGroundMatchWidget> MatchWidget;

	bool bMenuOpen = false;
};
