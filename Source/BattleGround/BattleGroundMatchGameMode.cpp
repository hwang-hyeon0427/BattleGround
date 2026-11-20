#include "BattleGroundMatchGameMode.h"

#include "BattleGroundCombatController.h"
#include "BattleGroundMatchHUD.h"
#include "BattleGroundMatchState.h"
#include "Engine/PlayerStartPIE.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerStart.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"

ABattleGroundMatchGameMode::ABattleGroundMatchGameMode()
{
    PlayerControllerClass = ABattleGroundCombatController::StaticClass();
    PlayerStateClass = ABattleGroundMatchPlayerState::StaticClass();
    GameStateClass = ABattleGroundMatchState::StaticClass();
    HUDClass = ABattleGroundMatchHUD::StaticClass();

    static const ConstructorHelpers::FClassFinder<APawn> Character(TEXT("/Game/Blueprint/BP_Character"));
    if (Character.Succeeded())
    {
        DefaultPawnClass = Character.Class;
    }
}

void ABattleGroundMatchGameMode::PreLogin(const FString& Options, const FString& Address,
    const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage)
{
    if (const TCHAR* Rejection = GetJoinRejection(Options))
    {
        ErrorMessage = Rejection;
        FGameModeEvents::GameModePreLoginEvent.Broadcast(this, UniqueId, ErrorMessage);
        return;
    }
    Super::PreLogin(Options, Address, UniqueId, ErrorMessage);
}

APlayerController* ABattleGroundMatchGameMode::Login(UPlayer* NewPlayer, ENetRole RemoteRole,
    const FString& Portal, const FString& Options, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage)
{
    // Multiple handshakes can pass PreLogin before any of them claims the final seat.
    if (const TCHAR* Rejection = GetJoinRejection(Options))
    {
        ErrorMessage = Rejection;
        return nullptr;
    }

    APlayerController* Controller = Super::Login(NewPlayer, RemoteRole, Portal, Options, UniqueId, ErrorMessage);
    if (!Controller)
    {
        return nullptr;
    }

    for (int32 Index = 0; Index < RoomCapacity; ++Index)
    {
        if (!Players[Index].IsValid())
        {
            Players[Index] = Controller;
            if (ABattleGroundMatchPlayerState* Player = Controller->GetPlayerState<ABattleGroundMatchPlayerState>())
            {
                Player->Seat = Index + 1;
            }
            break;
        }
    }
    return Controller;
}

void ABattleGroundMatchGameMode::HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer)
{
    const int32 Seat = FindSeat(NewPlayer);
    ABattleGroundMatchState* Match = GetGameState<ABattleGroundMatchState>();
    if (Seat == 0 || !Match)
    {
        return;
    }

    Match->Status.PlayerCount = CountPlayers();
    Match->ForceNetUpdate();
    if (Match->Status.PlayerCount != RoomCapacity || Match->GetPhase() != EBattleGroundMatchPhase::Waiting)
    {
        return;
    }

    TArray<APlayerStart*> Starts;
    for (TActorIterator<APlayerStart> Start(GetWorld()); Start; ++Start)
    {
        if (!Start->IsA<APlayerStartPIE>())
        {
            Starts.Add(*Start);
        }
    }
    Starts.Sort([](const APlayerStart& Left, const APlayerStart& Right)
    {
        return Left.GetFName().LexicalLess(Right.GetFName());
    });

    for (int32 Index = 0; Index < RoomCapacity; ++Index)
    {
        APlayerController* Player = Cast<APlayerController>(Players[Index].Get());
        if (!Player || Player->GetPawn())
        {
            continue;
        }
        if (Starts.IsValidIndex(Index))
        {
            RestartPlayerAtPlayerStart(Player, Starts[Index]);
        }
        else
        {
            Super::HandleStartingNewPlayer_Implementation(Player);
        }
    }

    const AController* First = Players[0].Get();
    const AController* Second = Players[1].Get();
    if (First && First->GetPawn() && Second && Second->GetPawn())
    {
        Match->Status.Phase = EBattleGroundMatchPhase::Playing;
    }
    Match->ForceNetUpdate();
}

void ABattleGroundMatchGameMode::Logout(AController* Exiting)
{
    const int32 Seat = FindSeat(Exiting);
    if (Seat != 0)
    {
        Players[Seat - 1].Reset();
        if (ABattleGroundMatchState* Match = GetGameState<ABattleGroundMatchState>())
        {
            Match->Status.PlayerCount = CountPlayers();
            if (Match->GetPhase() == EBattleGroundMatchPhase::Playing)
            {
                Match->Status.Phase = EBattleGroundMatchPhase::Finished;
                Match->Status.WinnerSeat = Match->Status.PlayerCount > 0 ? 3 - Seat : 0;
                Match->Status.bForfeit = true;
            }
            Match->ForceNetUpdate();
        }
    }
    Super::Logout(Exiting);
}

bool ABattleGroundMatchGameMode::CanPlayerAttack(const AController* Controller) const
{
    const ABattleGroundMatchState* Match = GetGameState<ABattleGroundMatchState>();
    return Match && Match->GetPhase() == EBattleGroundMatchPhase::Playing
        && FindSeat(Controller) != 0 && IsValid(Controller->GetPawn());
}

void ABattleGroundMatchGameMode::ApplyDuelDamage(AController* Attacker, AController* Target, int32 Damage)
{
    if (Attacker == Target || Damage <= 0 || !CanPlayerAttack(Attacker) || !CanPlayerAttack(Target))
    {
        return;
    }

    ABattleGroundMatchState* Match = GetGameState<ABattleGroundMatchState>();
    int32& Health = FindSeat(Target) == 1 ? Match->Status.SeatOneHealth : Match->Status.SeatTwoHealth;
    Health = FMath::Max(0, Health - Damage);
    if (Health == 0)
    {
        Match->Status.Phase = EBattleGroundMatchPhase::Finished;
        Match->Status.WinnerSeat = FindSeat(Attacker);
    }
    Match->ForceNetUpdate();
}

int32 ABattleGroundMatchGameMode::FindSeat(const AController* Controller) const
{
    if (Controller)
    {
        for (int32 Index = 0; Index < RoomCapacity; ++Index)
        {
            // Logout can occur while the controller is already being destroyed.
            if (Players[Index].Get(true) == Controller)
            {
                return Index + 1;
            }
        }
    }
    return 0;
}

int32 ABattleGroundMatchGameMode::CountPlayers() const
{
    return static_cast<int32>(Players[0].IsValid()) + static_cast<int32>(Players[1].IsValid());
}

const TCHAR* ABattleGroundMatchGameMode::GetJoinRejection(const FString& Options) const
{
    if (UGameplayStatics::HasOption(Options, TEXT("SpectatorOnly")))
    {
        return TEXT("This room does not allow spectators.");
    }
    if (CountPlayers() >= RoomCapacity)
    {
        return TEXT("This room is full (2/2).");
    }
    if (const ABattleGroundMatchState* Match = GetGameState<ABattleGroundMatchState>())
    {
        if (Match->GetPhase() == EBattleGroundMatchPhase::Finished)
        {
            return TEXT("This match has already finished. Join a new room.");
        }
    }
    return nullptr;
}
