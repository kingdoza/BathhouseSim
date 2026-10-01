#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "BathWaterOperationsAutomationTestSupport.h"
#include "Camera/CameraComponent.h"
#include "Character/FirstPersonCharacter.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "Computer/BathhouseComputerActor.h"
#include "Computer/PlayerComputerUseComponent.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/LocalPlayer.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Interaction/PlayerInteractionComponent.h"
#include "Slate/SceneViewport.h"
#include "UI/ComputerSampleScreenWidget.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SViewport.h"

namespace ComputerKeyboardFocusTest
{
// Game viewport harness. The viewport client is attached to the test world context only, so the engine's
// global game viewport and the Slate application's game viewport stay untouched.
class FScopedGameViewportHarness
{
public:
	FScopedGameViewportHarness(UWorld& InWorld)
		: World(&InWorld)
	{
		Client = NewObject<UGameViewportClient>(GEngine);
		Client->AddToRoot();
		ViewportWidget = SNew(SViewport);
		SceneViewport = MakeShared<FSceneViewport>(ViewportWidget);
		Client->Viewport = SceneViewport.Get();
		if (FWorldContext* Context = GEngine->GetWorldContextFromWorld(World))
		{
			Context->GameViewport = Client;
		}
	}

	~FScopedGameViewportHarness()
	{
		if (FWorldContext* Context = GEngine ? GEngine->GetWorldContextFromWorld(World) : nullptr)
		{
			Context->GameViewport = nullptr;
		}
		if (Client)
		{
			Client->Viewport = nullptr;
		}
		SceneViewport.Reset();
		ViewportWidget.Reset();
		if (Client)
		{
			Client->RemoveFromRoot();
			Client = nullptr;
		}
	}

	UGameViewportClient* GetClient() const { return Client; }
	TSharedPtr<SViewport> GetViewportWidget() const { return ViewportWidget; }

private:
	UWorld* World = nullptr;
	UGameViewportClient* Client = nullptr;
	TSharedPtr<SViewport> ViewportWidget;
	TSharedPtr<FSceneViewport> SceneViewport;
};

void BeginActor(AActor* Actor)
{
	if (Actor && !Actor->HasActorBegunPlay())
	{
		Actor->DispatchBeginPlay();
	}
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FComputerKeyboardFocusTest,
	"BathhouseSim.Computer.Input.ActiveFocusKeepsKeyboardOnGameViewport",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FComputerKeyboardFocusTest::RunTest(const FString& Parameters)
{
	using namespace ComputerKeyboardFocusTest;
	(void)Parameters;
	BathWaterOperationsTestSupport::FScopedBathWaterOperationsWorld TestWorld(TEXT("ComputerKeyboardFocusWorld"));
	UWorld* World = TestWorld.Get();
	if (!TestNotNull(TEXT("Keyboard focus automation world exists"), World)) return false;

	UStaticMesh* CubeMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	TestNotNull(TEXT("The engine cube mesh is available"), CubeMesh);

	APlayerController* PlayerController = World->SpawnActor<APlayerController>();
	AFirstPersonCharacter* Character = World->SpawnActor<AFirstPersonCharacter>();
	BeginActor(PlayerController);
	BeginActor(Character);
	ULocalPlayer* LocalPlayer = NewObject<ULocalPlayer>(GEngine, TEXT("ComputerKeyboardFocusLocalPlayer"));
	PlayerController->SetPlayer(LocalPlayer);
	PlayerController->Possess(Character);
	PlayerController->SetViewTarget(Character);

	ABathhouseComputerActor* Computer = World->SpawnActor<ABathhouseComputerActor>();
	Computer->ComputerMesh->SetStaticMesh(CubeMesh);
	Computer->ComputerMesh->SetWorldScale3D(FVector(0.5f));
	Computer->FocusBlendInSeconds = 0.0f;
	Computer->FocusBlendOutSeconds = 0.0f;
	UUserWidget* ScreenInstance = NewObject<UComputerSampleScreenWidget>(PlayerController, TEXT("ComputerKeyboardFocusScreen"));
	Computer->ScreenWidget->SetWidget(ScreenInstance);
	BeginActor(Computer);
	Computer->SetActorLocation(
		Character->GetFirstPersonCamera()->GetComponentLocation()
		+ Character->GetFirstPersonCamera()->GetForwardVector() * 150.0f);
	Computer->FocusExitPoint->SetWorldLocationAndRotation(
		Computer->GetActorLocation() + FVector(350.0f, 300.0f, 0.0f),
		FRotator(24.0f, 137.0f, 38.0f));
	Computer->ComputerMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Computer->ComputerMesh->SetCollisionResponseToAllChannels(ECR_Ignore);
	Computer->ComputerMesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	Computer->ComputerMesh->UpdateBounds();
	Computer->ComputerMesh->RecreatePhysicsState();
	World->UpdateWorldComponents(true, false);

	FScopedGameViewportHarness Harness(*World);
	UGameViewportClient* ViewportClient = Harness.GetClient();
	const TSharedPtr<SViewport> ViewportWidget = Harness.GetViewportWidget();
	if (!TestTrue(TEXT("Harness viewport client resolves through the test world"),
		World->GetGameViewport() == ViewportClient && ViewportClient != nullptr)) return false;
	if (!TestTrue(TEXT("Harness exposes its viewport widget"),
		ViewportClient->GetGameViewportWidget() == ViewportWidget)) return false;

	UPlayerComputerUseComponent* ComputerUse = Character->GetPlayerComputerUse();
	UPlayerInteractionComponent* Interaction = Character->GetPlayerInteraction();
	FReply& SlateOperations = LocalPlayer->GetSlateOperations();

	// 1. Negative control: the pre-fix input mode (widget focus on the screen) must be observable by this harness.
	{
		const TSharedRef<SBox> StandInScreenWidget = SNew(SBox);
		SlateOperations = FReply::Unhandled();
		FInputModeGameAndUI DefectiveMode;
		DefectiveMode.SetWidgetToFocus(StandInScreenWidget);
		PlayerController->SetInputMode(DefectiveMode);
		TestTrue(TEXT("Negative control: widget focus in the input mode moves user focus to the screen widget"),
			SlateOperations.ShouldSetUserFocus() && SlateOperations.GetUserFocusRecepient() == StandInScreenWidget);
		SlateOperations = FReply::Unhandled();
	}

	// 2. Entry E, never clicking the screen.
	ViewportClient->SetIgnoreInput(true);
	ViewportClient->SetMouseCaptureMode(EMouseCaptureMode::NoCapture);
	SlateOperations = FReply::Unhandled();
	Interaction->RefreshInteractionQuery();
	TestTrue(TEXT("CMP-001 precondition: the empty-handed player can query the computer"),
		Interaction->GetCurrentInteractionQuery().bCanInteract);
	Character->InteractStartInput();
	if (!TestTrue(TEXT("CMP-001: E enters the active computer session"), ComputerUse->IsActive())) return false;
	const TSharedPtr<SWidget> ScreenSlateWidget = ScreenInstance->GetCachedWidget();
	const TSharedPtr<SWidget> EntryRecipient = SlateOperations.GetUserFocusRecepient();
	TestTrue(TEXT("CMP-001: entry gives no user focus to a widget other than the game viewport"),
		!SlateOperations.ShouldSetUserFocus() || EntryRecipient == ViewportWidget);
	TestTrue(TEXT("CMP-001: entry never focuses the world-space screen widget"),
		!ScreenSlateWidget.IsValid() || EntryRecipient != ScreenSlateWidget);
	TestEqual(TEXT("CMP-001: entry applies GameAndUI capture mode"),
		ViewportClient->GetMouseCaptureMode(), EMouseCaptureMode::CaptureDuringMouseDown);
	TestEqual(TEXT("CMP-001: entry does not lock the mouse"),
		ViewportClient->GetMouseLockMode(), EMouseLockMode::DoNotLock);
	TestFalse(TEXT("CMP-001: entry keeps the cursor visible during capture"), ViewportClient->HideCursorDuringCapture());
	TestFalse(TEXT("CMP-001: entry re-enables viewport input"), ViewportClient->IgnoreInput());

	// 3. CMP-006: releasing the entry E keeps the session and the same input state.
	Character->InteractEndInput();
	TestTrue(TEXT("CMP-006: entry E release keeps the session active"), ComputerUse->IsActive());
	TestTrue(TEXT("CMP-006: entry E release leaves no screen focus request"),
		!SlateOperations.ShouldSetUserFocus() || SlateOperations.GetUserFocusRecepient() == ViewportWidget);
	TestEqual(TEXT("CMP-006: entry E release keeps GameAndUI capture mode"),
		ViewportClient->GetMouseCaptureMode(), EMouseCaptureMode::CaptureDuringMouseDown);

	// 4. CMP-001: second E without any click leaves the session and restores game focus.
	SlateOperations = FReply::Unhandled();
	Character->InteractStartInput();
	TestFalse(TEXT("CMP-001: the second E leaves the computer without any click"), ComputerUse->IsCapturingInput());
	TestTrue(TEXT("CMP-001: focus-out returns user focus to the game viewport"),
		SlateOperations.ShouldSetUserFocus() && SlateOperations.GetUserFocusRecepient() == ViewportWidget);
	const EMouseCaptureMode ExitCapture = ViewportClient->GetMouseCaptureMode();
	TestTrue(TEXT("CMP-001: focus-out restores permanent game mouse capture"),
		ExitCapture == EMouseCaptureMode::CapturePermanently
		|| ExitCapture == EMouseCaptureMode::CapturePermanently_IncludingInitialMouseDown);
	Character->InteractEndInput();

	// 5. CMP-004: a screen click first, then E ends the session the same way.
	ViewportClient->SetMouseCaptureMode(EMouseCaptureMode::NoCapture);
	SlateOperations = FReply::Unhandled();
	Character->SetActorLocation(FVector::ZeroVector);
	PlayerController->SetControlRotation(FRotator::ZeroRotator);
	Character->SetActorRotation(FRotator::ZeroRotator);
	Interaction->RefreshInteractionQuery();
	Character->InteractStartInput();
	if (!TestTrue(TEXT("CMP-004: the computer can be entered again"), ComputerUse->IsActive())) return false;
	Character->InteractEndInput();
	TestTrue(TEXT("CMP-004: the screen pointer press starts"), ComputerUse->PressPointer());
	ComputerUse->ReleasePointer();
	TestTrue(TEXT("CMP-004: a screen click leaves the session active"), ComputerUse->IsActive());
	TestTrue(TEXT("CMP-004: a screen click leaves no screen focus request"),
		!SlateOperations.ShouldSetUserFocus() || SlateOperations.GetUserFocusRecepient() == ViewportWidget);
	SlateOperations = FReply::Unhandled();
	Character->InteractStartInput();
	TestFalse(TEXT("CMP-004: E after a click leaves the computer"), ComputerUse->IsCapturingInput());
	TestTrue(TEXT("CMP-004: focus-out after a click returns user focus to the game viewport"),
		SlateOperations.ShouldSetUserFocus() && SlateOperations.GetUserFocusRecepient() == ViewportWidget);
	Character->InteractEndInput();
	return true;
}

#endif
