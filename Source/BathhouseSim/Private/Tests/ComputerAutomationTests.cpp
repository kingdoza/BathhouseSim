#if WITH_DEV_AUTOMATION_TESTS

#include "Tests/BathhouseCleaningTowelTestProbe.h"

#include "Blueprint/UserWidget.h"
#include "Camera/CameraComponent.h"
#include "Character/FirstPersonCharacter.h"
#include "Character/FirstPersonMovementComponent.h"
#include "Cleaning/WaterStainActor.h"
#include "Cleaning/WetMopActor.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/Button.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextBlock.h"
#include "Components/WidgetComponent.h"
#include "Components/WidgetInteractionComponent.h"
#include "Computer/BathhouseComputerActor.h"
#include "Computer/ComputerFocusExitPlacement.h"
#include "Computer/PlayerComputerUseComponent.h"
#include "Engine/Engine.h"
#include "Engine/LocalPlayer.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "Interaction/BathhouseKeyActor.h"
#include "Interaction/PlayerCarryComponent.h"
#include "Interaction/PlayerInteractionComponent.h"
#include "Misc/AutomationTest.h"
#include "SceneUtils.h"
#include "Towel/TowelBasketActor.h"
#include "UI/ComputerSampleScreenWidget.h"

namespace
{
class FScopedComputerAutomationWorld
{
public:
	explicit FScopedComputerAutomationWorld(const TCHAR* BaseName)
	{
		if (!GEngine)
		{
			return;
		}
		const FName WorldName = MakeUniqueObjectName(nullptr, UWorld::StaticClass(), BaseName);
		FWorldContext& WorldContext = GEngine->CreateNewWorldContext(EWorldType::Game);
		World = UWorld::CreateWorld(EWorldType::Game, false, WorldName, GetTransientPackage());
		if (!World)
		{
			GEngine->DestroyWorldContext(World);
			return;
		}
		World->AddToRoot();
		WorldContext.SetCurrentWorld(World);
		World->InitializeActorsForPlay(FURL());
	}

	~FScopedComputerAutomationWorld()
	{
		if (World && GEngine)
		{
			World->DestroyWorld(false);
			GEngine->DestroyWorldContext(World);
			World->RemoveFromRoot();
		}
	}

	UWorld* Get() const { return World; }

private:
	UWorld* World = nullptr;
};

void BeginActorForComputerTest(AActor* Actor)
{
	if (Actor && !Actor->HasActorBegunPlay())
	{
		Actor->DispatchBeginPlay();
	}
}

void ConfigureCarryMesh(AActor* Actor, UStaticMesh* Mesh)
{
	if (UStaticMeshComponent* Primitive = Actor ? Actor->FindComponentByClass<UStaticMeshComponent>() : nullptr)
	{
		Primitive->SetStaticMesh(Mesh);
		Primitive->SetWorldScale3D(FVector(0.2f));
		Primitive->UpdateBounds();
	}
}

AActor* SpawnComputerPlacementBlocker(UWorld* World, const FVector& Center, const FVector& Extent)
{
	AActor* BlockerActor = World ? World->SpawnActor<AActor>() : nullptr;
	if (!BlockerActor)
	{
		return nullptr;
	}

	UBoxComponent* Box = NewObject<UBoxComponent>(BlockerActor, TEXT("ComputerPlacementBlocker"));
	BlockerActor->SetRootComponent(Box);
	BlockerActor->AddInstanceComponent(Box);
	Box->SetBoxExtent(Extent);
	Box->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Box->SetCollisionObjectType(ECC_WorldStatic);
	Box->SetCollisionResponseToAllChannels(ECR_Ignore);
	Box->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
	Box->RegisterComponent();
	BlockerActor->SetActorLocation(Center);
	World->UpdateWorldComponents(true, false);
	return BlockerActor;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBathhouseComputerSessionTest,
	"BathhouseSim.Computer.FocusSessionSuppressionAndSampleScreen",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBathhouseComputerSessionTest::RunTest(const FString& Parameters)
{
	FScopedComputerAutomationWorld TestWorld(TEXT("ComputerAutomationWorld"));
	UWorld* World = TestWorld.Get();
	if (!World)
	{
		AddError(TEXT("Failed to create the computer automation world."));
		return false;
	}

	UStaticMesh* CubeMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	TestNotNull(TEXT("The engine cube mesh is available for interaction traces"), CubeMesh);

	APlayerController* PlayerController = World->SpawnActor<APlayerController>();
	AFirstPersonCharacter* Character = World->SpawnActor<AFirstPersonCharacter>();
	BeginActorForComputerTest(PlayerController);
	BeginActorForComputerTest(Character);
	ULocalPlayer* LocalPlayer = NewObject<ULocalPlayer>(GEngine, TEXT("ComputerTestLocalPlayer"));
	PlayerController->SetPlayer(LocalPlayer);
	PlayerController->Possess(Character);
	PlayerController->SetViewTarget(Character);
	TestTrue(TEXT("The automation character is locally controlled"), Character->IsLocallyControlled());

	ABathhouseComputerActor* Computer = World->SpawnActor<ABathhouseComputerActor>();
	Computer->ComputerMesh->SetStaticMesh(CubeMesh);
	Computer->ComputerMesh->SetWorldScale3D(FVector(0.5f));
	Computer->FocusBlendInSeconds = 0.0f;
	Computer->FocusBlendOutSeconds = 0.0f;
	UUserWidget* ScreenInstance = NewObject<UComputerSampleScreenWidget>(PlayerController, TEXT("ComputerScreenInstance"));
	Computer->ScreenWidget->SetWidget(ScreenInstance);
	BeginActorForComputerTest(Computer);

	TestEqual(TEXT("ComputerMesh keeps its reflected subobject name"), Computer->ComputerMesh->GetFName(), FName(TEXT("ComputerMesh")));
	TestEqual(TEXT("ScreenWidget keeps its reflected subobject name"), Computer->ScreenWidget->GetFName(), FName(TEXT("ScreenWidget")));
	TestEqual(TEXT("FocusCamera keeps its reflected subobject name"), Computer->FocusCamera->GetFName(), FName(TEXT("FocusCamera")));
	TestTrue(TEXT("A real screen widget makes the computer ready"), Computer->IsScreenReady());
	TestEqual(TEXT("Computer exit search defaults to 100 cm"), Computer->GetFocusExitSearchRadiusCm(), 100.0f);

	FPlayerInteractionContext Context;
	Context.Interactor = Character;
	Context.CarryComponent = Character->GetPlayerCarry();
	FPlayerInteractionContext MissingCarryContext = Context;
	MissingCarryContext.CarryComponent = nullptr;
	const FPlayerInteractionQuery MissingCarryQuery = Computer->QueryInteraction(MissingCarryContext);
	TestFalse(TEXT("A missing carry context rejects computer use"), MissingCarryQuery.bCanInteract);
	TestEqual(TEXT("Missing carry reports the carry-specific failure"),
		MissingCarryQuery.FailureReason.ToString(), FString(TEXT("소지 상태를 확인할 수 없습니다.")));

	AActor* MissingUseOwner = World->SpawnActor<AActor>();
	UPlayerCarryComponent* MissingUseCarry = NewObject<UPlayerCarryComponent>(MissingUseOwner, TEXT("MissingUseCarry"));
	MissingUseOwner->AddInstanceComponent(MissingUseCarry);
	MissingUseCarry->RegisterComponent();
	FPlayerInteractionContext MissingUseContext;
	MissingUseContext.Interactor = MissingUseOwner;
	MissingUseContext.CarryComponent = MissingUseCarry;
	TestFalse(TEXT("A missing player-computer component rejects computer use"),
		Computer->QueryInteraction(MissingUseContext).bCanInteract);

	ABathhouseKeyActor* Key = World->SpawnActor<ABathhouseKeyActor>();
	BeginActorForComputerTest(Key);
	TestTrue(TEXT("The key can occupy the character hand for the gate test"), Character->GetPlayerCarry()->CommitTakeKey(Key));
	const FPlayerInteractionQuery KeyHeldQuery = Computer->QueryInteraction(Context);
	TestFalse(TEXT("A held key rejects computer use"), KeyHeldQuery.bCanInteract);
	TestEqual(TEXT("Every held object uses the exact empty-hand failure"),
		KeyHeldQuery.FailureReason.ToString(), FString(TEXT("손에 든 물건을 내려놓아야 합니다")));
	Character->GetPlayerCarry()->CommitReleaseKey(Key);

	AWetMopActor* Mop = World->SpawnActor<AWetMopActor>();
	ATowelBasketActor* Basket = World->SpawnActor<ATowelBasketActor>();
	BeginActorForComputerTest(Mop);
	BeginActorForComputerTest(Basket);
	ConfigureCarryMesh(Mop, CubeMesh);
	ConfigureCarryMesh(Basket, CubeMesh);
	FText CarryFailure;
	TestTrue(TEXT("The wet mop can occupy the hand for the gate test"),
		Character->GetPlayerCarry()->TryTakePhysicalObject(Mop, CarryFailure));
	TestFalse(TEXT("A held wet mop rejects computer use"), Computer->QueryInteraction(Context).bCanInteract);
	Character->GetPlayerCarry()->CommitReleasePhysicalObject(Mop);
	TestTrue(TEXT("The towel basket can occupy the hand for the gate test"),
		Character->GetPlayerCarry()->TryTakePhysicalObject(Basket, CarryFailure));
	TestFalse(TEXT("A held towel basket rejects computer use"), Computer->QueryInteraction(Context).bCanInteract);
	Character->GetPlayerCarry()->CommitReleasePhysicalObject(Basket);
	Key->SetActorEnableCollision(false);
	Mop->SetActorEnableCollision(false);
	Basket->SetActorEnableCollision(false);
	Computer->ComputerMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Computer->ComputerMesh->SetCollisionResponseToAllChannels(ECR_Ignore);
	Computer->ComputerMesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	Computer->SetActorLocation(
		Character->GetFirstPersonCamera()->GetComponentLocation()
		+ Character->GetFirstPersonCamera()->GetForwardVector() * 150.0f);
	Computer->FocusExitPoint->SetWorldLocationAndRotation(
		Computer->GetActorLocation() + FVector(350.0f, 300.0f, 0.0f),
		FRotator(24.0f, 137.0f, 38.0f));
	Computer->ComputerMesh->UpdateBounds();
	Computer->ComputerMesh->RecreatePhysicsState();
	World->UpdateWorldComponents(true, false);

	ABathhouseComputerActor* MissingScreenComputer = World->SpawnActor<ABathhouseComputerActor>();
	BeginActorForComputerTest(MissingScreenComputer);
	TestFalse(TEXT("A computer without a user widget is unavailable"),
		MissingScreenComputer->QueryInteraction(Context).bCanInteract);
	MissingScreenComputer->SetActorEnableCollision(false);

	UPlayerComputerUseComponent* OtherUser = NewObject<UPlayerComputerUseComponent>(MissingUseOwner, TEXT("OtherComputerUser"));
	MissingUseOwner->AddInstanceComponent(OtherUser);
	OtherUser->RegisterComponent();
	TestTrue(TEXT("The computer accepts the first reservation"), Computer->TryReserveFor(OtherUser));
	TestFalse(TEXT("An occupied computer rejects another player"), Computer->QueryInteraction(Context).bCanInteract);
	Computer->ReleaseReservation(Character->GetPlayerComputerUse());
	TestTrue(TEXT("A different user's release cannot clear the reservation"), Computer->IsReservedBy(OtherUser));
	Computer->ReleaseReservation(OtherUser);
	Computer->ReleaseReservation(OtherUser);
	TestFalse(TEXT("Expected release is idempotent"), Computer->IsReservedBy(OtherUser));

	AActor* InvalidBeginOwner = World->SpawnActor<AActor>();
	UPlayerCarryComponent* InvalidBeginCarry = NewObject<UPlayerCarryComponent>(InvalidBeginOwner, TEXT("InvalidBeginCarry"));
	UPlayerComputerUseComponent* InvalidOwnerUse = NewObject<UPlayerComputerUseComponent>(InvalidBeginOwner, TEXT("InvalidOwnerUse"));
	InvalidBeginOwner->AddInstanceComponent(InvalidBeginCarry);
	InvalidBeginOwner->AddInstanceComponent(InvalidOwnerUse);
	InvalidBeginCarry->RegisterComponent();
	InvalidOwnerUse->RegisterComponent();
	FPlayerInteractionContext InvalidBeginContext;
	InvalidBeginContext.Interactor = InvalidBeginOwner;
	InvalidBeginContext.CarryComponent = InvalidBeginCarry;
	TestFalse(TEXT("Session start fails for a non-pawn owner"), Computer->ExecuteInteraction(InvalidBeginContext).bSucceeded);
	TestFalse(TEXT("Failed session start rolls its reservation back"), Computer->IsReservedBy(InvalidOwnerUse));

	UPlayerInteractionComponent* Interaction = Character->GetPlayerInteraction();
	UPlayerComputerUseComponent* ComputerUse = Character->GetPlayerComputerUse();
	UFirstPersonMovementComponent* Movement = Character->GetFirstPersonMovement();
	const EMovementMode MovementModeBeforeFocus = Movement->MovementMode;
	const FVector LocationBeforeInactiveCancel = Character->GetActorLocation();
	Character->CancelInput();
	TestFalse(TEXT("Cancel is inert when no computer session captures input"), ComputerUse->IsCapturingInput());
	TestTrue(TEXT("Cancel outside computer use preserves the player location"),
		Character->GetActorLocation().Equals(LocationBeforeInactiveCancel));
	TestEqual(TEXT("Cancel outside computer use preserves movement mode"),
		Movement->MovementMode.GetValue(), MovementModeBeforeFocus);
	TestTrue(TEXT("An empty hand can query the available computer"), Computer->QueryInteraction(Context).bCanInteract);
	FHitResult DirectTraceHit;
	FCollisionQueryParams DirectTraceParams(SCENE_QUERY_STAT(ComputerAutomationTrace), true, Character);
	const FVector DirectTraceStart = Character->GetFirstPersonCamera()->GetComponentLocation();
	const bool bDirectTraceHit = World->LineTraceSingleByChannel(
		DirectTraceHit,
		DirectTraceStart,
		DirectTraceStart + Character->GetFirstPersonCamera()->GetForwardVector() * 300.0f,
		ECC_Visibility,
		DirectTraceParams);
	TestTrue(TEXT("The fixture visibility trace reaches the computer mesh"),
		bDirectTraceHit && DirectTraceHit.GetActor() == Computer);
	Interaction->RefreshInteractionQuery();
	TestTrue(TEXT("The first-person trace sees the computer"), Interaction->GetCurrentInteractionQuery().bCanInteract);

	int32 AttemptBroadcastCount = 0;
	IConsoleVariable* AntiAliasingMethod = IConsoleManager::Get().FindConsoleVariable(TEXT("r.AntiAliasingMethod"));
	TestNotNull(TEXT("The renderer exposes its anti-aliasing method CVar"), AntiAliasingMethod);
	const int32 AntiAliasingMethodBeforeFocus = AntiAliasingMethod ? AntiAliasingMethod->GetInt() : INDEX_NONE;
	Interaction->OnInteractionAttemptFinishedNative.AddLambda(
		[&AttemptBroadcastCount](const FPlayerInteractionResult&) { ++AttemptBroadcastCount; });
	const FTransform ExpectedExitFootTransform = Computer->GetFocusExitFootTransform();
	const FVector ExpectedExitCapsuleCenter = ExpectedExitFootTransform.GetLocation()
		+ FVector::UpVector * Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	Character->InteractStartInput();
	TestTrue(TEXT("Entry E Started owns the press after opening the computer"), Character->bComputerOwnsInteractPress);
	TestTrue(TEXT("Zero blend enters Active immediately"), ComputerUse->IsActive());
	TestTrue(TEXT("The active session suppresses world interaction"), Interaction->IsInteractionSuppressed());
	TestEqual(TEXT("The active session disables movement"), Movement->MovementMode.GetValue(), MOVE_None);
	TestTrue(TEXT("The controller view target becomes the computer"), PlayerController->GetViewTarget() == Computer);
	TestTrue(TEXT("The active session enables widget hit testing"), Character->ComputerWidgetInteraction->bEnableHitTesting);
	TestTrue(TEXT("The active session displays the mouse cursor"), PlayerController->bShowMouseCursor);
	if (AntiAliasingMethod)
	{
		TestEqual(TEXT("The active computer focus uses FXAA"),
			AntiAliasingMethod->GetInt(), static_cast<int32>(AAM_FXAA));
	}
	TestEqual(TEXT("The screen widget instance is retained on focus-in"),
		Computer->ScreenWidget->GetUserWidgetObject(), ScreenInstance);
	TestFalse(TEXT("A duplicate begin request is rejected"), Computer->ExecuteInteraction(Context).bSucceeded);
	TestTrue(TEXT("A rejected duplicate begin keeps the existing reservation"),
		Computer->IsReservedBy(ComputerUse));

	Character->InteractEndInput();
	TestFalse(TEXT("Entry E Completed is consumed without closing the computer"), Character->bComputerOwnsInteractPress);
	TestTrue(TEXT("Entry release leaves the session active"), ComputerUse->IsActive());

	const FRotator ControlRotationBefore = PlayerController->GetControlRotation();
	Character->DoMove(1.0f, 1.0f);
	Character->DoLook(5.0f, 5.0f);
	Character->DoJumpStart();
	Character->SprintStartInput();
	Character->SecondaryInteractInput();
	Character->DropCarryInput();
	TestTrue(TEXT("Capture blocks movement input"), Character->GetPendingMovementInputVector().IsNearlyZero());
	TestTrue(TEXT("Capture blocks look input"), PlayerController->GetControlRotation().Equals(ControlRotationBefore));
	TestFalse(TEXT("Capture blocks jump start"), Character->IsJumpProvidingForce());
	TestFalse(TEXT("Capture blocks sprint start"), Movement->IsSprinting());
	TestEqual(TEXT("Capture blocks F/G before attempt broadcasting"), AttemptBroadcastCount, 1);

	TestTrue(TEXT("Active pointer press begins exactly once"), ComputerUse->PressPointer());
	TestFalse(TEXT("A repeated pointer press is rejected"), ComputerUse->PressPointer());
	TestTrue(TEXT("Pointer state records the owned press"), ComputerUse->bPointerDown);
	Character->InteractStartInput();
	TestTrue(TEXT("Exit E Started owns its press"), Character->bComputerOwnsInteractPress);
	TestFalse(TEXT("Zero blend exits immediately"), ComputerUse->IsCapturingInput());
	TestFalse(TEXT("Focus-out forces a pending pointer release"), ComputerUse->bPointerDown);
	TestFalse(TEXT("Focus-out disables widget hit testing"), Character->ComputerWidgetInteraction->bEnableHitTesting);
	TestFalse(TEXT("Pointer presses are rejected after focus-out"), ComputerUse->PressPointer());
	TestTrue(TEXT("Focus-out blends back to the owner pawn"), PlayerController->GetViewTarget() == Character);
	TestTrue(TEXT("Normal focus-out places the capsule center at the authored exit point"),
		Character->GetActorLocation().Equals(ExpectedExitCapsuleCenter, 0.1f));
	TestTrue(TEXT("Normal focus-out keeps yaw on the pawn actor only"),
		Character->GetActorRotation().Equals(FRotator(0.0f, 137.0f, 0.0f), 0.1f));
	TestTrue(TEXT("Normal focus-out restores authored pitch and yaw on the controller"),
		PlayerController->GetControlRotation().Equals(FRotator(24.0f, 137.0f, 0.0f), 0.1f));
	TestEqual(TEXT("Focus-out restores the movement mode"),
		Movement->MovementMode.GetValue(), MovementModeBeforeFocus);
	TestFalse(TEXT("Focus-out restores interaction availability"), Interaction->IsInteractionSuppressed());
	if (AntiAliasingMethod)
	{
		TestEqual(TEXT("Focus-out restores the previous anti-aliasing method"),
			AntiAliasingMethod->GetInt(), AntiAliasingMethodBeforeFocus);
	}
	TestTrue(TEXT("Focus-out releases the reservation"), !Computer->IsReservedBy(ComputerUse));
	TestEqual(TEXT("Focus-out does not recreate the screen widget"),
		Computer->ScreenWidget->GetUserWidgetObject(), ScreenInstance);
	Character->InteractEndInput();
	TestFalse(TEXT("Exit E Completed is consumed after the session has ended"), Character->bComputerOwnsInteractPress);

	const FVector ExitStartLocations[] = {
		FVector(10.0f, -80.0f, 96.0f),
		FVector(120.0f, 45.0f, 110.0f),
		FVector(-65.0f, 90.0f, 260.0f)
	};
	const FRotator EntryControlRotations[] = {
		FRotator(5.0f, 15.0f, 0.0f),
		FRotator(-12.0f, 195.0f, 0.0f),
		FRotator(30.0f, -80.0f, 0.0f)
	};
	for (int32 StartIndex = 0; StartIndex < UE_ARRAY_COUNT(ExitStartLocations); ++StartIndex)
	{
		Character->SetActorLocation(ExitStartLocations[StartIndex]);
		PlayerController->SetControlRotation(EntryControlRotations[StartIndex]);
		TestTrue(TEXT("Computer can reserve for each fixed-exit repeat"), Computer->TryReserveFor(ComputerUse));
		TestTrue(TEXT("Computer session starts from each different player position"), ComputerUse->BeginComputerUse(Computer));
		Character->CancelInput();
		TestTrue(TEXT("Each position and entry direction exits at the same capsule center"),
			Character->GetActorLocation().Equals(ExpectedExitCapsuleCenter, 0.1f));
		TestTrue(TEXT("Each position and entry direction exits with the same actor yaw"),
			Character->GetActorRotation().Equals(FRotator(0.0f, 137.0f, 0.0f), 0.1f));
		TestTrue(TEXT("Each position and entry direction exits with the same controller pitch and yaw"),
			PlayerController->GetControlRotation().Equals(FRotator(24.0f, 137.0f, 0.0f), 0.1f));
	}

	Computer->FocusBlendInSeconds = 0.2f;
	Computer->FocusBlendOutSeconds = 0.2f;
	Character->SetActorLocation(FVector::ZeroVector);
	PlayerController->SetControlRotation(FRotator::ZeroRotator);
	Character->SetActorRotation(FRotator::ZeroRotator);
	Interaction->RefreshInteractionQuery();
	Character->InteractStartInput();
	TestEqual(TEXT("A non-zero entry blend stays in FocusingIn"),
		ComputerUse->GetPhase(), EPlayerComputerUsePhase::FocusingIn);
	TestFalse(TEXT("Pointer input is rejected during FocusingIn"), ComputerUse->PressPointer());
	Character->InteractEndInput();
	Character->InteractStartInput();
	TestEqual(TEXT("E reverses a session while FocusingIn"),
		ComputerUse->GetPhase(), EPlayerComputerUsePhase::FocusingOut);
	TestTrue(TEXT("Focus-out moves to the authored exit center at transition start"),
		Character->GetActorLocation().Equals(ExpectedExitCapsuleCenter, 0.1f));
	Character->CancelInput();
	TestEqual(TEXT("Cancel during FocusingOut is ignored"),
		ComputerUse->GetPhase(), EPlayerComputerUsePhase::FocusingOut);
	ComputerUse->RequestEndComputerUse();
	TestEqual(TEXT("Repeated end during FocusingOut is idempotent"),
		ComputerUse->GetPhase(), EPlayerComputerUsePhase::FocusingOut);
	ComputerUse->CompleteFocusOut();
	TestFalse(TEXT("Manual blend completion returns to Inactive"), ComputerUse->IsCapturingInput());
	Character->InteractEndInput();

	Computer->FocusBlendInSeconds = 0.0f;
	Computer->FocusBlendOutSeconds = 0.0f;
	Character->SetActorLocation(FVector::ZeroVector);
	PlayerController->SetControlRotation(FRotator::ZeroRotator);
	Character->SetActorRotation(FRotator::ZeroRotator);
	Interaction->RefreshInteractionQuery();
	Character->InteractStartInput();
	TestTrue(TEXT("The session can be re-entered after cleanup"), ComputerUse->IsActive());
	const FVector LocationBeforeControllerLoss = Character->GetActorLocation();
	PlayerController->UnPossess();
	ComputerUse->TickComponent(0.0f, LEVELTICK_All, nullptr);
	TestFalse(TEXT("Controller loss cleans the player session"), ComputerUse->IsCapturingInput());
	TestFalse(TEXT("Controller loss releases the reservation"), Computer->IsReservedBy(ComputerUse));
	TestFalse(TEXT("Controller loss restores interaction"), Interaction->IsInteractionSuppressed());
	TestTrue(TEXT("Controller loss does not teleport the player"),
		Character->GetActorLocation().Equals(LocationBeforeControllerLoss, 0.1f));
	Character->InteractEndInput();
	PlayerController->Possess(Character);
	PlayerController->SetViewTarget(Character);
	Character->SetActorLocation(FVector(123.0f, 45.0f, 96.0f));
	Interaction->RefreshInteractionQuery();
	TestTrue(TEXT("The session can re-enter after controller recovery"), Computer->TryReserveFor(ComputerUse));
	TestTrue(TEXT("The recovered player can begin a computer session"), ComputerUse->BeginComputerUse(Computer));
	const FVector LocationBeforeUnavailable = Character->GetActorLocation();
	ComputerUse->HandleComputerUnavailable(Computer);
	TestFalse(TEXT("Computer unavailable cleans the player session"), ComputerUse->IsCapturingInput());
	TestFalse(TEXT("Computer unavailable restores interaction"), Interaction->IsInteractionSuppressed());
	TestEqual(TEXT("Computer unavailable restores movement"), Movement->MovementMode.GetValue(), MovementModeBeforeFocus);
	TestTrue(TEXT("Computer unavailable does not teleport the player"),
		Character->GetActorLocation().Equals(LocationBeforeUnavailable, 0.1f));
	Computer->ReleaseReservation(ComputerUse);
	TestTrue(TEXT("A computer remains usable after unavailable cleanup"), Computer->TryReserveFor(ComputerUse));
	TestTrue(TEXT("A second session begins before EndPlay"), ComputerUse->BeginComputerUse(Computer));
	const FVector LocationBeforeEndPlay = Character->GetActorLocation();
	Computer->Destroy();
	TestFalse(TEXT("Computer EndPlay cleans the player session"), ComputerUse->IsCapturingInput());
	TestFalse(TEXT("Computer EndPlay restores interaction"), Interaction->IsInteractionSuppressed());
	TestEqual(TEXT("Computer EndPlay restores movement"), Movement->MovementMode.GetValue(), MovementModeBeforeFocus);
	TestTrue(TEXT("Computer EndPlay restores the player view"), PlayerController->GetViewTarget() == Character);
	TestTrue(TEXT("Computer EndPlay does not teleport the player"),
		Character->GetActorLocation().Equals(LocationBeforeEndPlay, 0.1f));

	UComputerSampleScreenWidget* SampleWidget = NewObject<UComputerSampleScreenWidget>();
	SampleWidget->TestButton = NewObject<UButton>(SampleWidget, TEXT("TestButton"));
	SampleWidget->ClickResultText = NewObject<UTextBlock>(SampleWidget, TEXT("ClickResultText"));
	SampleWidget->NativeConstruct();
	TestEqual(TEXT("The native sample screen starts with its prompt"),
		SampleWidget->ClickResultText->GetText().ToString(), FString(TEXT("버튼을 클릭하세요")));
	SampleWidget->TestButton->OnClicked.Broadcast();
	TestEqual(TEXT("The native button click updates its result"),
		SampleWidget->ClickResultText->GetText().ToString(), FString(TEXT("클릭 확인")));
	SampleWidget->NativeDestruct();
	SampleWidget->ClickResultText->SetText(FText::GetEmpty());
	SampleWidget->NativeConstruct();
	TestEqual(TEXT("Reconstructing the same widget instance preserves click state"),
		SampleWidget->ClickResultText->GetText().ToString(), FString(TEXT("클릭 확인")));
	SampleWidget->NativeDestruct();

	APawn* SuppressionPawn = World->SpawnActor<APawn>();
	UCameraComponent* SuppressionCamera = NewObject<UCameraComponent>(SuppressionPawn, TEXT("SuppressionCamera"));
	UPlayerCarryComponent* SuppressionCarry = NewObject<UPlayerCarryComponent>(SuppressionPawn, TEXT("SuppressionCarry"));
	UPlayerInteractionComponent* SuppressionInteraction = NewObject<UPlayerInteractionComponent>(SuppressionPawn, TEXT("SuppressionInteraction"));
	SuppressionPawn->SetRootComponent(SuppressionCamera);
	SuppressionPawn->AddInstanceComponent(SuppressionCamera);
	SuppressionPawn->AddInstanceComponent(SuppressionCarry);
	SuppressionPawn->AddInstanceComponent(SuppressionInteraction);
	SuppressionCamera->RegisterComponent();
	SuppressionCarry->RegisterComponent();
	SuppressionInteraction->RegisterComponent();
	SuppressionCarry->ConfigureHeldAnchor(SuppressionCamera);
	SuppressionInteraction->Configure(SuppressionCamera, SuppressionCarry);
	AWetMopActor* SuppressionMop = World->SpawnActor<AWetMopActor>();
	AWaterStainActor* SuppressionStain = World->SpawnActor<AWaterStainActor>();
	BeginActorForComputerTest(SuppressionMop);
	BeginActorForComputerTest(SuppressionStain);
	ConfigureCarryMesh(SuppressionMop, CubeMesh);
	TestTrue(TEXT("Suppression setup takes a wet mop"),
		SuppressionCarry->TryTakePhysicalObject(SuppressionMop, CarryFailure));
	FPlayerInteractionContext SuppressionContext;
	SuppressionContext.Interactor = SuppressionPawn;
	SuppressionContext.CarryComponent = SuppressionCarry;
	SuppressionInteraction->ActiveHoldTarget = SuppressionStain;
	SuppressionInteraction->ActiveHoldContext = SuppressionContext;
	SuppressionInteraction->ActiveHoldProgress = 0.5f;
	SuppressionInteraction->bPrimaryInputHeld = true;
	SuppressionInteraction->CurrentTarget = SuppressionStain;
	SuppressionInteraction->CurrentQuery = SuppressionStain->QueryInteraction(SuppressionContext);
	UBathhouseInteractionQueryProbe* QueryProbe = NewObject<UBathhouseInteractionQueryProbe>();
	QueryProbe->Bind(SuppressionInteraction);
	int32 SuppressedAttemptCount = 0;
	SuppressionInteraction->OnInteractionAttemptFinishedNative.AddLambda(
		[&SuppressedAttemptCount](const FPlayerInteractionResult&) { ++SuppressedAttemptCount; });
	SuppressionInteraction->SetInteractionSuppressed(true);
	SuppressionInteraction->SetInteractionSuppressed(true);
	TestFalse(TEXT("Suppression clears the active hold"), SuppressionInteraction->IsPrimaryHoldActive());
	TestFalse(TEXT("Suppression commits one empty query"), SuppressionInteraction->GetCurrentInteractionQuery().bVisible);
	TestEqual(TEXT("Suppression broadcasts the empty query exactly once"), QueryProbe->BroadcastCount, 1);
	TestFalse(TEXT("The suppression query broadcast is empty"), QueryProbe->LastQuery.bVisible);
	SuppressionInteraction->BeginPrimaryInteraction();
	SuppressionInteraction->TryInteract();
	SuppressionInteraction->TrySecondaryInteract();
	SuppressionInteraction->TryDropCarry(FVector::ZeroVector, FVector::ForwardVector);
	SuppressionInteraction->EndPrimaryInteraction();
	TestEqual(TEXT("Suppressed direct attempts do not broadcast results"), SuppressedAttemptCount, 0);
	TestTrue(TEXT("Suppressed direct attempts do not mutate the held mop"),
		SuppressionCarry->GetHeldObject() == SuppressionMop);
	SuppressionInteraction->SetInteractionSuppressed(false);
	const int32 QueryBroadcastCountAfterRefresh = QueryProbe->BroadcastCount;
	SuppressionInteraction->SetInteractionSuppressed(false);
	TestFalse(TEXT("Suppression release is idempotent"), SuppressionInteraction->IsInteractionSuppressed());
	TestEqual(TEXT("Repeated suppression release does not refresh again"),
		QueryProbe->BroadcastCount, QueryBroadcastCountAfterRefresh);
	QueryProbe->Unbind();

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FComputerFocusExitPlacementTest,
	"BathhouseSim.Computer.FocusExitPlacement",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FComputerFocusExitPlacementTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FScopedComputerAutomationWorld TestWorld(TEXT("ComputerExitPlacementWorld"));
	UWorld* World = TestWorld.Get();
	if (!World)
	{
		AddError(TEXT("Failed to create the computer exit-placement automation world."));
		return false;
	}

	AFirstPersonCharacter* Character = World->SpawnActor<AFirstPersonCharacter>();
	BeginActorForComputerTest(Character);
	if (!TestNotNull(TEXT("The test character exists"), Character))
	{
		return false;
	}
	UCapsuleComponent* Capsule = Character->GetCapsuleComponent();
	const float HalfHeight = Capsule->GetScaledCapsuleHalfHeight();
	const FVector FootLocation(3.0f, 4.0f, 37.0f);
	const FVector FixedCenter = FootLocation + FVector::UpVector * HalfHeight;
	FComputerFocusExitPlacementResult Result;

	TestTrue(TEXT("An open authored position is accepted"),
		FComputerFocusExitPlacement::Resolve(World, Capsule, Character, FootLocation, 0.0f, 100.0f, Result));
	TestEqual(TEXT("An open authored position uses the fixed path"),
		static_cast<uint8>(Result.Path),
		static_cast<uint8>(FComputerFocusExitPlacementResult::EPath::Fixed));
	TestTrue(TEXT("Capsule center is exactly foot height plus scaled half-height"),
		Result.CapsuleCenter.Equals(FixedCenter, 0.1f));
	TestEqual(TEXT("Fixed placement has zero search distance"), Result.SearchDistanceCm, 0.0f);

	AActor* CenterBlocker = SpawnComputerPlacementBlocker(World, FixedCenter, FVector(1.0f, 1.0f, 1.0f));
	TestNotNull(TEXT("A capsule blocker is available"), CenterBlocker);
	const FVector BlockerLocationBeforeSearch = CenterBlocker->GetActorLocation();
	TestTrue(TEXT("A blocked authored position is searched"),
		FComputerFocusExitPlacement::Resolve(World, Capsule, Character, FootLocation, 0.0f, 100.0f, Result));
	TestEqual(TEXT("An available nearby position uses the searched path"),
		static_cast<uint8>(Result.Path),
		static_cast<uint8>(FComputerFocusExitPlacementResult::EPath::Searched));
	TestTrue(TEXT("The selected free position is within the authored radius"),
		Result.SearchDistanceCm > 0.0f && Result.SearchDistanceCm <= 100.0f);
	TestTrue(TEXT("Search does not move the blocking actor"),
		CenterBlocker->GetActorLocation().Equals(BlockerLocationBeforeSearch));

	CenterBlocker->SetActorEnableCollision(false);
	World->UpdateWorldComponents(true, false);
	AActor* Occupant = SpawnComputerPlacementBlocker(World, FixedCenter, FVector(1.0f, 1.0f, 1.0f));
	AActor* Wall = SpawnComputerPlacementBlocker(
		World,
		FixedCenter + FVector(40.0f, 0.0f, 0.0f),
		FVector(1.0f, 250.0f, 150.0f));
	TestNotNull(TEXT("The center occupant is available"), Occupant);
	TestNotNull(TEXT("The separating wall is available"), Wall);
	TestTrue(TEXT("A wall-separated exit candidate is resolved"),
		FComputerFocusExitPlacement::Resolve(World, Capsule, Character, FootLocation, 0.0f, 120.0f, Result));
	TestEqual(TEXT("The search finds a reachable position"),
		static_cast<uint8>(Result.Path),
		static_cast<uint8>(FComputerFocusExitPlacementResult::EPath::Searched));
	TestTrue(TEXT("The result stays on the original side of the wall"),
		Result.CapsuleCenter.X < FixedCenter.X + 40.0f);

	Occupant->SetActorEnableCollision(false);
	Wall->SetActorEnableCollision(false);
	World->UpdateWorldComponents(true, false);
	AActor* EnclosingBlocker = SpawnComputerPlacementBlocker(
		World,
		FixedCenter,
		FVector(200.0f, 200.0f, 200.0f));
	TestNotNull(TEXT("The enclosing blocker is available"), EnclosingBlocker);
	TestTrue(TEXT("A fully blocked search falls back to the authored position"),
		FComputerFocusExitPlacement::Resolve(World, Capsule, Character, FootLocation, 15.0f, 100.0f, Result));
	TestEqual(TEXT("No reachable candidate uses the forced path"),
		static_cast<uint8>(Result.Path),
		static_cast<uint8>(FComputerFocusExitPlacementResult::EPath::Forced));
	TestTrue(TEXT("Forced placement keeps the original capsule center"),
		Result.CapsuleCenter.Equals(FixedCenter, 0.1f));
	TestTrue(TEXT("Forced placement does not move the enclosing actor"),
		EnclosingBlocker->GetActorLocation().Equals(FixedCenter, 0.1f));

	TestTrue(TEXT("A zero search radius does not search"),
		FComputerFocusExitPlacement::Resolve(World, Capsule, Character, FootLocation, 15.0f, 0.0f, Result));
	TestEqual(TEXT("A blocked zero-radius placement is forced"),
		static_cast<uint8>(Result.Path),
		static_cast<uint8>(FComputerFocusExitPlacementResult::EPath::Forced));
	TestTrue(TEXT("Zero-radius placement preserves the fixed height"),
		Result.CapsuleCenter.Equals(FixedCenter, 0.1f));
	return true;
}

#endif
