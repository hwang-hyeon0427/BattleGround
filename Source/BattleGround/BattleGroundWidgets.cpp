#include "BattleGroundWidgets.h"

#include "BattleGroundGameInstance.h"
#include "BattleGroundMatchHUD.h"
#include "BattleGroundMatchState.h"
#include "Blueprint/WidgetTree.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/ButtonSlot.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/EditableTextBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/NetDriver.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "IPAddress.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Styling/CoreStyle.h"
#include "Styling/SlateTypes.h"

#define LOCTEXT_NAMESPACE "BattleGroundWidgets"

namespace
{
	const FLinearColor Background = FLinearColor::FromSRGBColor(FColor(12, 15, 20));
	const FLinearColor Panel = FLinearColor::FromSRGBColor(FColor(25, 30, 38));
	const FLinearColor Text = FLinearColor::FromSRGBColor(FColor(245, 247, 251));
	const FLinearColor Muted = FLinearColor::FromSRGBColor(FColor(172, 184, 198));
	const FLinearColor Accent = FLinearColor::FromSRGBColor(FColor(255, 190, 93));
	const FLinearColor Secondary = FLinearColor::FromSRGBColor(FColor(58, 70, 86));
	constexpr float RefreshInterval = 0.125f;

	int32 GetRoomPort(const UWorld& World)
	{
		if (UNetDriver* Driver = World.GetNetDriver())
		{
			if (const TSharedPtr<const FInternetAddr> Address = Driver->GetLocalAddr())
			{
				return Address->GetPort();
			}
		}
		return World.URL.Port;
	}

	UTextBlock* MakeText(UWidgetTree& Tree, const FText& Value, int32 Size,
		const FLinearColor& Color, bool bBold = false)
	{
		UTextBlock* Label = Tree.ConstructWidget<UTextBlock>();
		Label->SetText(Value);
		Label->SetColorAndOpacity(FSlateColor(Color));
		const FName Typeface(bBold ? TEXT("Bold") : TEXT("Regular"));
		Label->SetFont(FCoreStyle::GetDefaultFontStyle(Typeface, Size));
		Label->SetAutoWrapText(true);
		return Label;
	}

	UBorder* MakeBorder(UWidgetTree& Tree, const FLinearColor& Color, float Radius = 0.0f)
	{
		UBorder* Border = Tree.ConstructWidget<UBorder>();
		Border->SetBrush(FSlateRoundedBoxBrush(Color, Radius, FVector2D::ZeroVector));
		return Border;
	}

	UButton* MakeButton(UWidgetTree& Tree, const FText& Label, bool bPrimary)
	{
		const FLinearColor Fill = bPrimary ? Accent : Secondary;
		FButtonStyle Style;
		Style.SetNormal(FSlateRoundedBoxBrush(Fill, 7.0f, FVector2D::ZeroVector));
		Style.SetHovered(FSlateRoundedBoxBrush(FMath::Lerp(Fill, FLinearColor::White, 0.14f), 7.0f, FVector2D::ZeroVector));
		Style.SetPressed(FSlateRoundedBoxBrush(FMath::Lerp(Fill, Background, 0.2f), 7.0f, FVector2D::ZeroVector));
		Style.SetDisabled(FSlateRoundedBoxBrush(Panel, 7.0f, FVector2D::ZeroVector));
		Style.SetNormalPadding(FMargin(0.0f));
		Style.SetPressedPadding(FMargin(0.0f));

		UButton* Button = Tree.ConstructWidget<UButton>();
		Button->SetStyle(Style);
		UTextBlock* Caption = MakeText(Tree, Label, 17, bPrimary ? Background : Text, true);
		Caption->SetJustification(ETextJustify::Center);
		UButtonSlot* Slot = CastChecked<UButtonSlot>(Button->SetContent(Caption));
		Slot->SetHorizontalAlignment(HAlign_Fill);
		Slot->SetPadding(FMargin(12.0f, 10.0f));
		return Button;
	}

	void AddRow(UVerticalBox& Box, UWidget* Widget, float TopPadding = 0.0f)
	{
		Box.AddChildToVerticalBox(Widget)->SetPadding(FMargin(0.0f, TopPadding, 0.0f, 0.0f));
	}

	void FillCanvas(UCanvasPanel& Canvas, UWidget* Child)
	{
		UCanvasPanelSlot* Slot = Canvas.AddChildToCanvas(Child);
		Slot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
		Slot->SetOffsets(FMargin(0.0f));
	}

	void CenterOnCanvas(UCanvasPanel& Canvas, UWidget* Child)
	{
		UCanvasPanelSlot* Slot = Canvas.AddChildToCanvas(Child);
		Slot->SetAnchors(FAnchors(0.5f, 0.5f));
		Slot->SetAlignment(FVector2D(0.5f, 0.5f));
		Slot->SetPosition(FVector2D::ZeroVector);
		Slot->SetAutoSize(true);
	}

	UVerticalBox* MakeCard(UWidgetTree& Tree, float Width, const FLinearColor& Color, float Padding,
		USizeBox*& Card)
	{
		Card = Tree.ConstructWidget<USizeBox>();
		Card->SetWidthOverride(Width);

		UBorder* Border = MakeBorder(Tree, Color, 10.0f);
		Border->SetPadding(FMargin(Padding));
		Card->SetContent(Border);

		UVerticalBox* Rows = Tree.ConstructWidget<UVerticalBox>();
		Border->SetContent(Rows);
		return Rows;
	}
}

void UBattleGroundMenuWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	SetIsFocusable(true);
	UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>();
	WidgetTree->RootWidget = Canvas;
	FillCanvas(*Canvas, MakeBorder(*WidgetTree, Background));

	USizeBox* Card;
	UVerticalBox* Rows = MakeCard(*WidgetTree, 460.0f, Panel, 30.0f, Card);
	CenterOnCanvas(*Canvas, Card);

	AddRow(*Rows, MakeText(*WidgetTree, LOCTEXT("Title", "BattleGround"), 38, Text, true));
	AddRow(*Rows, MakeText(*WidgetTree, LOCTEXT("Subtitle", "1v1 multiplayer"), 18, Muted), 2.0f);

	HostButton = MakeButton(*WidgetTree, LOCTEXT("CreateRoom", "Create Room"), true);
	HostButton->OnClicked.AddDynamic(this, &UBattleGroundMenuWidget::HostRoom);
	AddRow(*Rows, HostButton, 24.0f);

	AddRow(*Rows, MakeText(*WidgetTree, LOCTEXT("RoomAddress", "Host address"), 15, Text, true), 20.0f);
	AddressBox = WidgetTree->ConstructWidget<UEditableTextBox>();
	AddressBox->SetHintText(LOCTEXT("AddressHint", "e.g. 192.168.1.10:7777"));
	AddressBox->SetSelectAllTextWhenFocused(true);
	AddressBox->OnTextCommitted.AddDynamic(this, &UBattleGroundMenuWidget::SubmitAddress);
	USizeBox* AddressField = WidgetTree->ConstructWidget<USizeBox>();
	AddressField->SetMinDesiredHeight(38.0f);
	AddressField->SetContent(AddressBox);
	AddRow(*Rows, AddressField, 7.0f);

	JoinButton = MakeButton(*WidgetTree, LOCTEXT("JoinRoom", "Join Room"), false);
	JoinButton->OnClicked.AddDynamic(this, &UBattleGroundMenuWidget::JoinRoom);
	AddRow(*Rows, JoinButton, 8.0f);

	ConnectionStatus = MakeText(*WidgetTree, FText::GetEmpty(), 14, Accent);
	AddRow(*Rows, ConnectionStatus, 8.0f);

	UButton* QuitButton = MakeButton(*WidgetTree, LOCTEXT("Quit", "Quit Game"), false);
	QuitButton->OnClicked.AddDynamic(this, &UBattleGroundMenuWidget::Quit);
	AddRow(*Rows, QuitButton, 14.0f);

	RefreshConnectionStatus();
}

void UBattleGroundMenuWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	RefreshDelay -= InDeltaTime;
	if (RefreshDelay <= 0.0f)
	{
		RefreshDelay = RefreshInterval;
		RefreshConnectionStatus();
	}
}

void UBattleGroundMenuWidget::HostRoom()
{
	if (UBattleGroundGameInstance* Game = Cast<UBattleGroundGameInstance>(GetGameInstance()))
	{
		Game->HostRoom();
		RefreshConnectionStatus();
	}
}

void UBattleGroundMenuWidget::JoinRoom()
{
	if (UBattleGroundGameInstance* Game = Cast<UBattleGroundGameInstance>(GetGameInstance()))
	{
		Game->JoinRoom(AddressBox->GetText().ToString());
		RefreshConnectionStatus();
	}
}

void UBattleGroundMenuWidget::SubmitAddress(const FText&, ETextCommit::Type CommitType)
{
	if (CommitType == ETextCommit::OnEnter)
	{
		JoinRoom();
	}
}

void UBattleGroundMenuWidget::Quit()
{
	UKismetSystemLibrary::QuitGame(this, GetOwningPlayer(), EQuitPreference::Quit, false);
}

void UBattleGroundMenuWidget::RefreshConnectionStatus()
{
	if (const UBattleGroundGameInstance* Game = Cast<UBattleGroundGameInstance>(GetGameInstance()))
	{
		const FText Status = Game->GetConnectionStatus();
		const bool bReady = !Game->IsConnecting();
		ConnectionStatus->SetText(Status);
		ConnectionStatus->SetVisibility(Status.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
		HostButton->SetIsEnabled(bReady);
		JoinButton->SetIsEnabled(bReady);
		AddressBox->SetIsEnabled(bReady);
	}
}

void UBattleGroundMatchWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>();
	WidgetTree->RootWidget = Canvas;
	SetVisibility(ESlateVisibility::HitTestInvisible);

	USizeBox* StatusCard;
	UVerticalBox* StatusRows = MakeCard(*WidgetTree, 365.0f, Panel.CopyWithNewOpacity(0.92f), 14.0f, StatusCard);
	UCanvasPanelSlot* StatusSlot = Canvas->AddChildToCanvas(StatusCard);
	StatusSlot->SetAutoSize(true);
	StatusSlot->SetPosition(FVector2D(24.0f, 24.0f));

	RoundStatus = MakeText(*WidgetTree, FText::GetEmpty(), 18, Text, true);
	HealthStatus = MakeText(*WidgetTree, FText::GetEmpty(), 15, Muted);
	AddRow(*StatusRows, RoundStatus);
	AddRow(*StatusRows, HealthStatus, 5.0f);
	AddRow(*StatusRows, MakeText(*WidgetTree, LOCTEXT("MenuShortcut", "Esc  ·  Match menu"), 14, Muted), 6.0f);

	USizeBox* ReticleSize = WidgetTree->ConstructWidget<USizeBox>();
	ReticleSize->SetHeightOverride(5.0f);
	ReticleSize->SetWidthOverride(5.0f);
	ReticleSize->SetContent(MakeBorder(*WidgetTree, Text, 2.5f));
	CenterOnCanvas(*Canvas, ReticleSize);
	Reticle = ReticleSize;

	MenuBackdrop = MakeBorder(*WidgetTree, Background.CopyWithNewOpacity(0.75f));
	MenuBackdrop->SetPadding(FMargin(0.0f));
	FillCanvas(*Canvas, MenuBackdrop);
	UCanvasPanel* MenuCanvas = WidgetTree->ConstructWidget<UCanvasPanel>();
	MenuBackdrop->SetContent(MenuCanvas);

	USizeBox* MenuCard;
	UVerticalBox* MenuRows = MakeCard(*WidgetTree, 340.0f, Panel, 24.0f, MenuCard);
	CenterOnCanvas(*MenuCanvas, MenuCard);
	AddRow(*MenuRows, MakeText(*WidgetTree, LOCTEXT("MatchMenu", "Match menu"), 27, Text, true));
	UButton* ResumeButton = MakeButton(*WidgetTree, LOCTEXT("Resume", "Back to Match"), true);
	ResumeButton->OnClicked.AddDynamic(this, &UBattleGroundMatchWidget::Resume);
	AddRow(*MenuRows, ResumeButton, 16.0f);
	UButton* LeaveButton = MakeButton(*WidgetTree, LOCTEXT("Leave", "Return to Main Menu"), false);
	LeaveButton->OnClicked.AddDynamic(this, &UBattleGroundMatchWidget::LeaveRoom);
	AddRow(*MenuRows, LeaveButton, 8.0f);
	SetMenuOpen(false);
}

void UBattleGroundMatchWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	RefreshDelay -= InDeltaTime;
	if (RefreshDelay <= 0.0f)
	{
		RefreshDelay = RefreshInterval;
		RefreshMatchStatus();
	}
}

void UBattleGroundMatchWidget::SetMenuOpen(bool bOpen)
{
	bMenuOpen = bOpen;
	SetIsFocusable(bOpen);
	SetVisibility(bOpen ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::HitTestInvisible);
	MenuBackdrop->SetVisibility(bOpen ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	RefreshMatchStatus();
}

void UBattleGroundMatchWidget::Resume()
{
	if (APlayerController* Controller = GetOwningPlayer())
	{
		if (ABattleGroundMatchHUD* HUD = Cast<ABattleGroundMatchHUD>(Controller->GetHUD()))
		{
			HUD->ToggleMenu();
		}
	}
}

void UBattleGroundMatchWidget::LeaveRoom()
{
	if (UBattleGroundGameInstance* Game = Cast<UBattleGroundGameInstance>(GetGameInstance()))
	{
		Game->ReturnToMenu();
	}
}

void UBattleGroundMatchWidget::RefreshMatchStatus()
{
	const UWorld* World = GetWorld();
	const ABattleGroundMatchState* Match = World ? World->GetGameState<ABattleGroundMatchState>() : nullptr;
	const APlayerController* Controller = GetOwningPlayer();
	const ABattleGroundMatchPlayerState* Player = Controller ? Controller->GetPlayerState<ABattleGroundMatchPlayerState>() : nullptr;
	const int32 Seat = Player ? Player->GetSeat() : 0;

	FText Message = LOCTEXT("LoadingMatch", "Loading match…");
	FText Details;
	bool bPlaying = false;

	if (Match)
	{
		switch (Match->GetPhase())
		{
		case EBattleGroundMatchPhase::Waiting:
			Message = FText::Format(LOCTEXT("Waiting", "Waiting for opponent  ·  {0}/2"), FText::AsNumber(Match->GetPlayerCount()));
			if (World->GetNetMode() == NM_ListenServer)
			{
				Details = FText::Format(LOCTEXT("ShareAddress", "Share your IP address and UDP port {0}."),
					FText::AsNumber(GetRoomPort(*World), &FNumberFormattingOptions::DefaultNoGrouping()));
			}
			break;
		case EBattleGroundMatchPhase::Playing:
			Message = LOCTEXT("Playing", "Match in progress");
			bPlaying = true;
			break;
		case EBattleGroundMatchPhase::Finished:
			if (Match->GetWinnerSeat() == 0 || Seat == 0)
			{
				Message = LOCTEXT("MatchEnded", "Match ended");
			}
			else
			{
				Message = Match->GetWinnerSeat() == Seat ? LOCTEXT("Victory", "You won") : LOCTEXT("Defeat", "You lost");
			}
			if (Match->WasForfeit())
			{
				Details = LOCTEXT("Forfeit", "Opponent disconnected.");
			}
			break;
		}
	}

	if (bPlaying && Seat > 0)
	{
		Details = FText::Format(LOCTEXT("Health", "You  {0} HP      Opponent  {1} HP"),
			FText::AsNumber(Match->GetHealth(Seat)), FText::AsNumber(Match->GetHealth(3 - Seat)));
	}

	RoundStatus->SetText(Message);
	HealthStatus->SetText(Details);
	HealthStatus->SetVisibility(Details.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	Reticle->SetVisibility(bPlaying && !bMenuOpen ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
}

#undef LOCTEXT_NAMESPACE
