#include "BattleGroundMenuGameMode.h"

#include "BattleGroundWidgets.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/WorldSettings.h"
#include "TimerManager.h"

ABattleGroundMenuGameMode::ABattleGroundMenuGameMode()
{
	DefaultPawnClass = nullptr;
}

AActor* ABattleGroundMenuGameMode::ChoosePlayerStart_Implementation(AController*)
{
	// Login still needs a start actor even though the empty menu map has no PlayerStart.
	return GetWorld()->GetWorldSettings();
}

void ABattleGroundMenuGameMode::HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer)
{
	// The menu uses only a local player controller and does not spawn a pawn.
	if (NewPlayer->IsLocalPlayerController())
	{
		GetWorldTimerManager().SetTimerForNextTick(this, &ABattleGroundMenuGameMode::ShowMenu);
	}
}

void ABattleGroundMenuGameMode::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (MenuWidget)
	{
		MenuWidget->RemoveFromParent();
	}

	Super::EndPlay(EndPlayReason);
}

void ABattleGroundMenuGameMode::ShowMenu()
{
	APlayerController* Controller = GetWorld()->GetFirstPlayerController();
	if (!Controller || !Controller->IsLocalController() || MenuWidget)
	{
		return;
	}

	MenuWidget = CreateWidget<UBattleGroundMenuWidget>(Controller);
	if (MenuWidget && MenuWidget->AddToPlayerScreen())
	{
		Controller->bShowMouseCursor = true;
		FInputModeUIOnly InputMode;
		InputMode.SetWidgetToFocus(MenuWidget->TakeWidget());
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		Controller->SetInputMode(InputMode);
	}
}
