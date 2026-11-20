#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "BattleGroundWidgets.generated.h"

class UBorder;
class UButton;
class UEditableTextBox;
class UTextBlock;
class UWidget;

UCLASS()
class BATTLEGROUND_API UBattleGroundMenuWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	UFUNCTION()
	void HostRoom();

	UFUNCTION()
	void JoinRoom();

	UFUNCTION()
	void SubmitAddress(const FText& Address, ETextCommit::Type CommitType);

	UFUNCTION()
	void Quit();

	void RefreshConnectionStatus();

	UPROPERTY(Transient)
	TObjectPtr<UButton> HostButton;

	UPROPERTY(Transient)
	TObjectPtr<UButton> JoinButton;

	UPROPERTY(Transient)
	TObjectPtr<UEditableTextBox> AddressBox;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> ConnectionStatus;

	float RefreshDelay = 0.0f;
};

UCLASS()
class BATTLEGROUND_API UBattleGroundMatchWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetMenuOpen(bool bOpen);

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	UFUNCTION()
	void Resume();

	UFUNCTION()
	void LeaveRoom();

	void RefreshMatchStatus();

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> RoundStatus;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> HealthStatus;

	UPROPERTY(Transient)
	TObjectPtr<UBorder> MenuBackdrop;

	UPROPERTY(Transient)
	TObjectPtr<UWidget> Reticle;

	bool bMenuOpen = false;
	float RefreshDelay = 0.0f;
};
