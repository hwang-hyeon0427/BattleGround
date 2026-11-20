#include "BattleGroundGameInstance.h"

#include "Engine/Engine.h"
#include "Engine/NetDriver.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/UObjectGlobals.h"

namespace
{
    const FName MenuMap(TEXT("/Engine/Maps/Entry"));
    const FName ArenaMap(TEXT("/Game/Map/BasicMap"));
    constexpr int32 DefaultRoomPort = 7777;

    bool ParseRoomAddress(const FString& Address, FString& Target)
    {
        const FString Input = Address.TrimStartAndEnd();
        FString Host;
        FString PortText;
        if (!Input.Split(TEXT(":"), &Host, &PortText))
        {
            Host = Input;
            PortText = FString::FromInt(DefaultRoomPort);
        }

        if (Host.IsEmpty() || Host.Len() > 253 || PortText.IsEmpty() || PortText.Len() > 5)
        {
            return false;
        }

        TArray<FString> Labels;
        Host.ParseIntoArray(Labels, TEXT("."), false);
        for (const FString& Label : Labels)
        {
            if (Label.IsEmpty() || Label.Len() > 63 || Label.StartsWith(TEXT("-")) || Label.EndsWith(TEXT("-")))
            {
                return false;
            }
            for (const TCHAR Character : Label)
            {
                const bool bAsciiLetter = (Character >= 'a' && Character <= 'z') || (Character >= 'A' && Character <= 'Z');
                if (!bAsciiLetter && !(Character >= '0' && Character <= '9') && Character != '-')
                {
                    return false;
                }
            }
        }

        for (const TCHAR Character : PortText)
        {
            if (Character < '0' || Character > '9')
            {
                return false;
            }
        }
        const int32 Port = FCString::Atoi(*PortText);
        if (Port < 1 || Port > 65535)
        {
            return false;
        }

        Target = FString::Printf(TEXT("unreal://%s:%d"), *Host, Port);
        return true;
    }
}

void UBattleGroundGameInstance::Init()
{
    Super::Init();
    FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this, &ThisClass::OnMapLoaded);
    if (GEngine)
    {
        GEngine->OnNetworkFailure().AddUObject(this, &ThisClass::OnNetworkFailure);
        GEngine->OnTravelFailure().AddUObject(this, &ThisClass::OnTravelFailure);
    }
}

void UBattleGroundGameInstance::Shutdown()
{
    FCoreUObjectDelegates::PostLoadMapWithWorld.RemoveAll(this);
    if (GEngine)
    {
        GEngine->OnNetworkFailure().RemoveAll(this);
        GEngine->OnTravelFailure().RemoveAll(this);
    }
    Super::Shutdown();
}

void UBattleGroundGameInstance::HostRoom()
{
    if (IsConnecting() || bReturningToMenu || !GetWorld())
    {
        return;
    }

    PendingTravel = ERoomTravel::Hosting;
    ConnectionStatus = NSLOCTEXT("BattleGround", "CreatingRoom", "Creating room...");
    UGameplayStatics::OpenLevel(this, ArenaMap, true, TEXT("listen?game=BattleGroundDuel"));
}

void UBattleGroundGameInstance::JoinRoom(const FString& Address)
{
    if (IsConnecting() || bReturningToMenu)
    {
        return;
    }

    FString Target;
    if (!ParseRoomAddress(Address, Target))
    {
        ConnectionStatus = NSLOCTEXT("BattleGround", "InvalidRoomAddress", "Enter a host address, optionally followed by a port.");
        return;
    }

    if (APlayerController* Controller = GetFirstLocalPlayerController())
    {
        PendingTravel = ERoomTravel::Joining;
        ConnectionStatus = NSLOCTEXT("BattleGround", "JoiningRoom", "Connecting to room...");
        Controller->ClientTravel(Target, TRAVEL_Absolute);
    }
}

void UBattleGroundGameInstance::ReturnToMenu()
{
    ConnectionStatus = FText::GetEmpty();
    OpenMenu();
}

void UBattleGroundGameInstance::OpenMenu()
{
    PendingTravel = ERoomTravel::None;
    bReturningToMenu = true;
    UGameplayStatics::OpenLevel(this, MenuMap, true);
}

void UBattleGroundGameInstance::OnMapLoaded(UWorld* World)
{
    if (!World || World->GetGameInstance() != this)
    {
        return;
    }

    bReturningToMenu = false;
    if (PendingTravel == ERoomTravel::Hosting && World->GetNetMode() != NM_ListenServer)
    {
        ConnectionStatus = NSLOCTEXT("BattleGround", "ListenFailed", "Could not start the room server.");
        OpenMenu();
        return;
    }

    if (PendingTravel != ERoomTravel::None)
    {
        ConnectionStatus = FText::GetEmpty();
        PendingTravel = ERoomTravel::None;
    }
}

void UBattleGroundGameInstance::OnNetworkFailure(UWorld* World, UNetDriver* Driver, ENetworkFailure::Type Type, const FString& Error)
{
    UWorld* FailureWorld = World ? World : (Driver ? Driver->GetWorld() : nullptr);
    if ((FailureWorld && FailureWorld != GetWorld()) || bReturningToMenu)
    {
        return;
    }

    // A failed listener can still finish loading the arena. The post-load check returns it to the menu.
    if (PendingTravel == ERoomTravel::Hosting)
    {
        return;
    }

    const bool bClientDriver = Driver && Driver->ServerConnection != nullptr;
    const bool bClientFailure = bClientDriver || (GetWorld() && GetWorld()->GetNetMode() == NM_Client);
    if (!IsConnecting() && !bClientFailure)
    {
        return;
    }

    PendingTravel = ERoomTravel::None;
    ConnectionStatus = Error.IsEmpty()
        ? NSLOCTEXT("BattleGround", "RoomDisconnected", "Connection to the room was lost.")
        : FText::FromString(Error);
}

void UBattleGroundGameInstance::OnTravelFailure(UWorld* World, ETravelFailure::Type Type, const FString& Error)
{
    if ((World && World != GetWorld()) || (!IsConnecting() && !bReturningToMenu))
    {
        return;
    }

    PendingTravel = ERoomTravel::None;
    bReturningToMenu = false;
    ConnectionStatus = Error.IsEmpty()
        ? NSLOCTEXT("BattleGround", "RoomTravelFailed", "Could not load the room.")
        : FText::FromString(Error);
}
