#include "BattleGroundMatchHUD.h"

#include "BattleGroundWidgets.h"
#include "Blueprint/GameViewportSubsystem.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Engine/Engine.h"
#include "GameFramework/PlayerController.h"
#include "TimerManager.h"

namespace
{
	const TCHAR* LegacyHUDClassPath = TEXT("/Game/Blueprint/WBP_HUD.WBP_HUD_C");
}

void ABattleGroundMatchHUD::BeginPlay()
{
	Super::BeginPlay();
	GetWorldTimerManager().SetTimerForNextTick(this, &ABattleGroundMatchHUD::ShowMatchHUD);
}

void ABattleGroundMatchHUD::ShowMatchHUD()
{
	APlayerController* Controller = GetOwningPlayerController();
	if (!Controller || !Controller->IsLocalController())
	{
		return;
	}

	// Existing characters still create their single-player health widget.
	if (UGameViewportSubsystem* Viewport = GEngine ? GEngine->GetEngineSubsystem<UGameViewportSubsystem>() : nullptr)
	{
		Viewport->OnWidgetAdded.AddUObject(this, &ABattleGroundMatchHUD::HideLegacyHealthBar);
	}
	if (UClass* HealthBarClass = FindObject<UClass>(nullptr, LegacyHUDClassPath))
	{
		TArray<UUserWidget*> HealthBars;
		UWidgetBlueprintLibrary::GetAllWidgetsOfClass(this, HealthBars, HealthBarClass);
		for (UUserWidget* HealthBar : HealthBars)
		{
			HideLegacyHealthBar(HealthBar, nullptr);
		}
	}

	MatchWidget = CreateWidget<UBattleGroundMatchWidget>(Controller);
	if (MatchWidget && !MatchWidget->AddToPlayerScreen(1))
	{
		MatchWidget = nullptr;
	}

	Controller->bShowMouseCursor = false;
	Controller->SetInputMode(FInputModeGameOnly());
}

void ABattleGroundMatchHUD::HideLegacyHealthBar(UWidget* Widget, ULocalPlayer*)
{
	if (Widget && Widget->GetWorld() == GetWorld() && Widget->GetClass()->GetPathName() == LegacyHUDClassPath)
	{
		Widget->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void ABattleGroundMatchHUD::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UGameViewportSubsystem* Viewport = GEngine ? GEngine->GetEngineSubsystem<UGameViewportSubsystem>() : nullptr)
	{
		Viewport->OnWidgetAdded.RemoveAll(this);
	}

	if (MatchWidget)
	{
		MatchWidget->RemoveFromParent();
	}

	if (bMenuOpen)
	{
		if (APlayerController* Controller = GetOwningPlayerController())
		{
			Controller->SetIgnoreLookInput(false);
			Controller->SetIgnoreMoveInput(false);
		}
	}

	Super::EndPlay(EndPlayReason);
}

void ABattleGroundMatchHUD::ToggleMenu()
{
	APlayerController* Controller = GetOwningPlayerController();
	if (!MatchWidget || !Controller || !Controller->IsLocalController())
	{
		return;
	}

	bMenuOpen = !bMenuOpen;
	MatchWidget->SetMenuOpen(bMenuOpen);
	Controller->SetIgnoreLookInput(bMenuOpen);
	Controller->SetIgnoreMoveInput(bMenuOpen);
	Controller->bShowMouseCursor = bMenuOpen;

	if (bMenuOpen)
	{
		FInputModeGameAndUI InputMode;
		InputMode.SetWidgetToFocus(MatchWidget->TakeWidget());
		InputMode.SetHideCursorDuringCapture(false);
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		Controller->SetInputMode(InputMode);
	}
	else
	{
		Controller->SetInputMode(FInputModeGameOnly());
	}
}
