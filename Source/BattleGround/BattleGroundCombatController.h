#pragma once

#include "CoreMinimal.h"
#include "Engine/NetSerialization.h"
#include "GameFramework/PlayerController.h"
#include "BattleGroundCombatController.generated.h"

class ACharacter;
class UAnimMontage;
class UEnhancedInputComponent;
class UInputAction;
class UInputMappingContext;

UCLASS()
class BATTLEGROUND_API ABattleGroundCombatController : public APlayerController
{
    GENERATED_BODY()

public:
    ABattleGroundCombatController();

protected:
    void OnPossess(APawn* InPawn) override;
    void SetupInputComponent() override;
    void BuildInputStack(TArray<UInputComponent*>& InputStack) override;

private:
    void OnAttackPressed();
    void ToggleMatchMenu();

    UFUNCTION(Server, Unreliable)
    void ServerFire(FVector_NetQuantizeNormal AimDirection);

    UFUNCTION(Client, Unreliable)
    void ClientPlayShotAnimation(ACharacter* Shooter);

    UPROPERTY()
    TObjectPtr<UInputAction> AttackAction;

    UPROPERTY()
    TObjectPtr<UInputMappingContext> GameplayMapping;

    UPROPERTY()
    TObjectPtr<UAnimMontage> ShotMontage;

    TWeakObjectPtr<UEnhancedInputComponent> UpdatedPawnInput;
    double NextShotTime = 0.0;
};
