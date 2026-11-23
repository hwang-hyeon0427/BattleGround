#include "BattleGroundCombatController.h"

#include "Animation/AnimMontage.h"
#include "BattleGroundMatchGameMode.h"
#include "BattleGroundMatchHUD.h"
#include "BattleGroundMatchState.h"
#include "CollisionQueryParams.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "Engine/HitResult.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/Character.h"
#include "InputAction.h"
#include "InputCoreTypes.h"
#include "InputMappingContext.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
    constexpr int32 DamagePerShot = 25;
    constexpr double TimeBetweenShots = 0.35;
    constexpr double ShotRange = 10000.0;
    constexpr double MinimumAimAlignment = 0.70710678;
}

ABattleGroundCombatController::ABattleGroundCombatController()
{
    static ConstructorHelpers::FObjectFinder<UInputAction> Attack(
        TEXT("/Game/Blueprint/IA_Attack.IA_Attack"));
    static ConstructorHelpers::FObjectFinder<UInputMappingContext> Mapping(
        TEXT("/Game/Blueprint/IMC_Default.IMC_Default"));
    static ConstructorHelpers::FObjectFinder<UAnimMontage> Montage(
        TEXT("/Game/Blueprint/SteveAnimationMontage.SteveAnimationMontage"));
    AttackAction = Attack.Object;
    GameplayMapping = Mapping.Object;
    ShotMontage = Montage.Object;
}

void ABattleGroundCombatController::OnPossess(APawn* InPawn)
{
    Super::OnPossess(InPawn);

    if (ACharacter* PossessedCharacter = Cast<ACharacter>(GetPawn()))
    {
        // Multiplayer health belongs to the match, including for the legacy Blueprint pawn.
        PossessedCharacter->SetCanBeDamaged(false);
        PossessedCharacter->GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
        NextShotTime = 0.0;
    }
}

void ABattleGroundCombatController::SetupInputComponent()
{
    Super::SetupInputComponent();

    if (InputComponent)
    {
        InputComponent->BindKey(EKeys::Escape, IE_Pressed, this, &ThisClass::ToggleMatchMenu);
    }

    if (UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(InputComponent);
        EnhancedInput && AttackAction)
    {
        EnhancedInput->BindAction(AttackAction.Get(), ETriggerEvent::Started, this, &ThisClass::OnAttackPressed);
    }

    if (ULocalPlayer* LocalPlayer = GetLocalPlayer())
    {
        UEnhancedInputLocalPlayerSubsystem* InputSubsystem =
            LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();
        if (InputSubsystem && GameplayMapping && !InputSubsystem->HasMappingContext(GameplayMapping.Get()))
        {
            InputSubsystem->AddMappingContext(GameplayMapping.Get(), 0);
        }
    }
}

void ABattleGroundCombatController::BuildInputStack(TArray<UInputComponent*>& InputStack)
{
    const APawn* ControlledPawn = GetPawn();
    UEnhancedInputComponent* PawnInput = ControlledPawn ? Cast<UEnhancedInputComponent>(ControlledPawn->InputComponent) : nullptr;
    if (AttackAction && PawnInput && UpdatedPawnInput.Get() != PawnInput)
    {
        // The pawn still provides movement, aiming and camera controls from its Blueprint.
        const auto& Bindings = PawnInput->GetActionEventBindings();
        for (int32 Index = Bindings.Num() - 1; Index >= 0; --Index)
        {
            if (Bindings[Index]->GetAction() == AttackAction.Get())
            {
                PawnInput->RemoveActionEventBinding(Index);
            }
        }
        UpdatedPawnInput = PawnInput;
    }

    Super::BuildInputStack(InputStack);
}

void ABattleGroundCombatController::ToggleMatchMenu()
{
    if (ABattleGroundMatchHUD* MatchHUD = GetHUD<ABattleGroundMatchHUD>())
    {
        MatchHUD->ToggleMenu();
    }
}

void ABattleGroundCombatController::OnAttackPressed()
{
    const ABattleGroundMatchHUD* MatchHUD = GetHUD<ABattleGroundMatchHUD>();
    const APawn* ControlledPawn = GetPawn();
    UWorld* World = GetWorld();
    const ABattleGroundMatchState* MatchState = World ? World->GetGameState<ABattleGroundMatchState>() : nullptr;
    if (!ControlledPawn || !MatchState || MatchState->GetPhase() != EBattleGroundMatchPhase::Playing ||
        (MatchHUD && MatchHUD->IsMenuOpen()))
    {
        return;
    }

    FVector CameraLocation;
    FRotator CameraRotation;
    GetPlayerViewPoint(CameraLocation, CameraRotation);

    // Align the intent with the local reticle for both camera views. The server traces again from the pawn.
    const FVector CameraEnd = CameraLocation + CameraRotation.Vector() * ShotRange;
    const FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(DuelReticle), false, ControlledPawn);
    FHitResult Hit;
    const bool bHitGeometry = World->LineTraceSingleByChannel(Hit, CameraLocation, CameraEnd, ECC_Visibility, QueryParams);
    FVector AimPoint = bHitGeometry ? Hit.ImpactPoint : CameraEnd;

    FHitResult PawnHit;
    const FCollisionObjectQueryParams PawnObjects(ECC_Pawn);
    if (World->LineTraceSingleByObjectType(PawnHit, CameraLocation, AimPoint, PawnObjects, QueryParams))
    {
        AimPoint = PawnHit.ImpactPoint;
    }

    const FVector ControlDirection = GetControlRotation().Vector();
    const FVector ReticleDirection = (AimPoint - ControlledPawn->GetPawnViewLocation()).GetSafeNormal();
    const FVector AimDirection = (ReticleDirection | ControlDirection) >= MinimumAimAlignment
        ? ReticleDirection
        : ControlDirection;
    ServerFire(FVector_NetQuantizeNormal(AimDirection));
}

void ABattleGroundCombatController::ServerFire_Implementation(FVector_NetQuantizeNormal AimDirection)
{
    UWorld* World = GetWorld();
    ACharacter* Shooter = Cast<ACharacter>(GetPawn());
    ABattleGroundMatchGameMode* Match = World ? World->GetAuthGameMode<ABattleGroundMatchGameMode>() : nullptr;
    if (!Shooter || !Match || !Match->CanPlayerAttack(this) || !AimDirection.IsNormalized() ||
        (AimDirection | GetControlRotation().Vector()) < MinimumAimAlignment)
    {
        return;
    }

    const double CurrentTime = World->GetTimeSeconds();
    if (CurrentTime < NextShotTime)
    {
        return;
    }
    NextShotTime = CurrentTime + TimeBetweenShots;

    for (FConstPlayerControllerIterator Iterator = World->GetPlayerControllerIterator(); Iterator; ++Iterator)
    {
        if (ABattleGroundCombatController* Viewer = Cast<ABattleGroundCombatController>(Iterator->Get()))
        {
            Viewer->ClientPlayShotAnimation(Shooter);
        }
    }

    const FVector Start = Shooter->GetPawnViewLocation();
    const FVector End = Start + AimDirection * ShotRange;
    const FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(DuelShot), false, Shooter);
    FHitResult Hit;
    if (World->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, QueryParams))
    {
        if (const APawn* Target = Cast<APawn>(Hit.GetActor()))
        {
            Match->ApplyDuelDamage(this, Target->GetController(), DamagePerShot);
        }
    }
}

void ABattleGroundCombatController::ClientPlayShotAnimation_Implementation(ACharacter* Shooter)
{
    if (IsValid(Shooter) && ShotMontage)
    {
        Shooter->PlayAnimMontage(ShotMontage.Get());
    }
}
