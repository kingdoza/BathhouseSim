#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Customer/CustomerBathLoopLog.h"
#include "Customer/CustomerRoutineDefinition.h"
#include "Customer/CustomerSessionComponent.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Facility/BathWaterControlComponent.h"
#include "Facility/BathWaterSettings.h"
#include "Facility/BathWaterStateComponent.h"
#include "Facility/BathhouseBathFacilityActor.h"
#include "Facility/BathhouseFacilitySubsystem.h"
#include "Facility/BathhouseFacilitySlotComponent.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Character.h"
#include "Interaction/PlayerInteractable.h"
#include "Misc/DataValidation.h"
#include "NiagaraComponent.h"
#include "Placement/FacilityPlacementComponent.h"

#include <limits>

namespace
{
class FScopedBathWaterAutomationWorld
{
public:
	explicit FScopedBathWaterAutomationWorld(const TCHAR* BaseName)
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

	~FScopedBathWaterAutomationWorld()
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
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBathhouseBathWaterStateTest,
	"BathhouseSim.BathWater.StateThresholdFlowAndFreeze",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBathhouseBathWaterStateTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FScopedBathWaterAutomationWorld Scope(TEXT("BathWaterStateWorld"));
	UWorld* World = Scope.Get();
	if (!TestNotNull(TEXT("Automation world exists"), World))
	{
		return false;
	}
	AActor* ComponentOwner = World->SpawnActor<AActor>();
	UBathWaterSettings* Settings = NewObject<UBathWaterSettings>();
	TestEqual(TEXT("Global bath threshold defaults to eighty percent"), Settings->GetCustomerUsableThresholdPercent(), 80.0f);
	Settings->CustomerUsableThresholdPercent = -1.0f;
	TestEqual(TEXT("Threshold clamps below zero"), Settings->GetCustomerUsableThresholdPercent(), 0.0f);
	Settings->CustomerUsableThresholdPercent = 120.0f;
	TestEqual(TEXT("Threshold clamps above one hundred"), Settings->GetCustomerUsableThresholdPercent(), 100.0f);
	Settings->CustomerUsableThresholdPercent = std::numeric_limits<float>::quiet_NaN();
	TestEqual(TEXT("Non-finite threshold falls back to eighty"), Settings->GetCustomerUsableThresholdPercent(), 80.0f);
	UBathWaterStateComponent* DefaultFlow = NewObject<UBathWaterStateComponent>(ComponentOwner, TEXT("DefaultFlow"));
	DefaultFlow->RegisterComponent();
	DefaultFlow->ResetEmptyForPlacement();
	FText DefaultFlowFailure;
	DefaultFlow->RequestSetControlOpen(
		EBathWaterControlType::FillValve, true, EBathWaterControlChangeReason::PlayerInteraction, DefaultFlowFailure);
	DefaultFlow->TickComponent(15.0f, LEVELTICK_All, nullptr);
	TestEqual(TEXT("Default fill rate reaches full in about fifteen seconds"), DefaultFlow->GetNormalizedAmount(), 1.0f);
	DefaultFlow->RequestSetControlOpen(
		EBathWaterControlType::DrainLever, true, EBathWaterControlChangeReason::PlayerInteraction, DefaultFlowFailure);
	DefaultFlow->TickComponent(1.0f, LEVELTICK_All, nullptr);
	DefaultFlow->RequestSetControlOpen(
		EBathWaterControlType::FillValve, true, EBathWaterControlChangeReason::PlayerInteraction, DefaultFlowFailure);
	const float BeforeNetFlow = DefaultFlow->GetNormalizedAmount();
	DefaultFlow->TickComponent(1.0f, LEVELTICK_All, nullptr);
	TestTrue(TEXT("Default simultaneous flow drains by about 3.3333 percentage points per second"),
		FMath::IsNearlyEqual(BeforeNetFlow - DefaultFlow->GetNormalizedAmount(), 0.03333333f, 0.0001f));
	DefaultFlow->RequestSetControlOpen(
		EBathWaterControlType::FillValve, false, EBathWaterControlChangeReason::PlayerInteraction, DefaultFlowFailure);
	DefaultFlow->TickComponent(20.0f, LEVELTICK_All, nullptr);
	TestEqual(TEXT("Default drain rate reaches empty without crossing below zero"), DefaultFlow->GetNormalizedAmount(), 0.0f);

	UBathWaterStateComponent* Water = NewObject<UBathWaterStateComponent>(ComponentOwner, TEXT("WaterUnderTest"));
	Water->RegisterComponent();
	Water->FillRatePercentPerSecond = 10.0f;
	Water->DrainRatePercentPerSecond = 10.0f;
	Water->ResetEmptyForPlacement();
	int32 AmountTransitions = 0;
	int32 StateTransitions = 0;
	int32 UsabilityTransitions = 0;
	int32 AutoCloseTransitions = 0;
	Water->OnWaterAmountChangedNative.AddLambda([&AmountTransitions](float, float) { ++AmountTransitions; });
	Water->OnWaterStateChangedNative.AddLambda([&StateTransitions](EBathWaterState, EBathWaterState) { ++StateTransitions; });
	Water->OnCustomerUsabilityChangedNative.AddLambda([&UsabilityTransitions](bool) { ++UsabilityTransitions; });
	Water->OnControlChangedNative.AddLambda(
		[&AutoCloseTransitions](EBathWaterControlType, bool, EBathWaterControlChangeReason Reason)
		{
			if (Reason == EBathWaterControlChangeReason::FullAutoClose)
			{
				++AutoCloseTransitions;
			}
		});

	FText FailureReason;
	TestTrue(TEXT("Fill valve opens through the native request API"), Water->RequestSetControlOpen(
		EBathWaterControlType::FillValve, true, EBathWaterControlChangeReason::PlayerInteraction, FailureReason));
	Water->TickComponent(7.999f, LEVELTICK_All, nullptr);
	TestFalse(TEXT("0.7999 remains below the exact global threshold"), Water->IsCustomerUsable());
	Water->TickComponent(0.001f, LEVELTICK_All, nullptr);
	TestTrue(TEXT("0.8 is customer usable"), Water->IsCustomerUsable());
	TestEqual(TEXT("Threshold crossing publishes once"), UsabilityTransitions, 1);
	Water->TickComponent(2.0f, LEVELTICK_All, nullptr);
	TestTrue(TEXT("Fill reaches the exact full endpoint"), Water->GetNormalizedAmount() == 1.0f);
	TestFalse(TEXT("Fill auto closes at full"), Water->IsControlOpen(EBathWaterControlType::FillValve));
	TestEqual(TEXT("Full auto-close publishes once"), AutoCloseTransitions, 1);
	TestTrue(TEXT("Closed fill rejects reopening at full"), !Water->CanSetControlOpen(
		EBathWaterControlType::FillValve, true, FailureReason));

	TestTrue(TEXT("Drain can open"), Water->RequestSetControlOpen(
		EBathWaterControlType::DrainLever, true, EBathWaterControlChangeReason::PlayerInteraction, FailureReason));
	Water->TickComponent(10.0f, LEVELTICK_All, nullptr);
	TestTrue(TEXT("Drain reaches the exact empty endpoint"), Water->GetNormalizedAmount() == 0.0f);
	TestTrue(TEXT("Drain remains logically open at zero"), Water->IsControlOpen(EBathWaterControlType::DrainLever));
	TestTrue(TEXT("Fill can open while drain remains open"), Water->RequestSetControlOpen(
		EBathWaterControlType::FillValve, true, EBathWaterControlChangeReason::PlayerInteraction, FailureReason));
	Water->TickComponent(1.0f, LEVELTICK_All, nullptr);
	TestEqual(TEXT("Equal simultaneous rates commit one zero net delta"), Water->GetNormalizedAmount(), 0.0f);
	TestFalse(TEXT("Zero net flow disables the water tick"), Water->IsComponentTickEnabled());

	TestTrue(TEXT("Recovery freeze snapshots the running logical state"), Water->BeginRecoveryFreeze(FailureReason));
	Water->TickComponent(5.0f, LEVELTICK_All, nullptr);
	TestEqual(TEXT("Frozen water does not advance"), Water->GetNormalizedAmount(), 0.0f);
	Water->PrepareRecoveryCommit();
	TestFalse(TEXT("Recovery commit-pending closes fill"), Water->IsControlOpen(EBathWaterControlType::FillValve));
	TestFalse(TEXT("Recovery commit-pending closes drain"), Water->IsControlOpen(EBathWaterControlType::DrainLever));
	Water->CancelRecoveryFreeze();
	TestTrue(TEXT("Recovery rollback restores fill"), Water->IsControlOpen(EBathWaterControlType::FillValve));
	TestTrue(TEXT("Recovery rollback restores drain"), Water->IsControlOpen(EBathWaterControlType::DrainLever));
	TestEqual(TEXT("Amount delegate fires once for each actual delta"), AmountTransitions, 4);
	TestEqual(TEXT("Derived state delegate fires once for each actual state transition"), StateTransitions, 3);
	TestEqual(TEXT("Usability delegate fires once per threshold direction"), UsabilityTransitions, 2);
	Water->ResetEmptyForPlacement();
	TestTrue(TEXT("Placement reset atomically restores exact empty state"),
		Water->IsEmpty() && Water->GetWaterState() == EBathWaterState::Empty
		&& !Water->IsControlOpen(EBathWaterControlType::FillValve)
		&& !Water->IsControlOpen(EBathWaterControlType::DrainLever)
		&& !Water->IsComponentTickEnabled());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBathhouseBathWaterControlPresentationTest,
	"BathhouseSim.BathWater.ControlAxisMotionAndPlanePresentation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBathhouseBathWaterControlPresentationTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FScopedBathWaterAutomationWorld Scope(TEXT("BathWaterPresentationWorld"));
	UWorld* World = Scope.Get();
	if (!TestNotNull(TEXT("Automation world exists"), World))
	{
		return false;
	}
	AActor* ComponentOwner = World->SpawnActor<AActor>();
	UBathWaterControlComponent* Control = NewObject<UBathWaterControlComponent>(ComponentOwner, TEXT("ControlUnderTest"));
	Control->RegisterComponent();
	Control->SetRelativeRotation(FRotator(10.0f, 20.0f, 30.0f));
	Control->InitializeClosedPose();
	const FQuat Closed = Control->GetRelativeRotation().Quaternion();
	Control->LocalRotationAxis = FVector(1.0f, 0.0f, 0.0f);
	Control->OpenAngleDegrees = 90.0f;
	const FQuat PositiveAxis = Control->CalculateOpenRotation();
	Control->LocalRotationAxis = FVector(-1.0f, 0.0f, 0.0f);
	const FQuat NegativeAxis = Control->CalculateOpenRotation();
	Control->LocalRotationAxis = FVector(1.0f, 0.0f, 0.0f);
	Control->OpenAngleDegrees = -90.0f;
	TestTrue(TEXT("Negative axis plus positive angle equals positive axis plus negative angle"),
		NegativeAxis.Equals(Control->CalculateOpenRotation(), KINDA_SMALL_NUMBER));
	TestFalse(TEXT("Positive and negative local axes rotate in opposite directions"),
		PositiveAxis.Equals(NegativeAxis, KINDA_SMALL_NUMBER));
	TestTrue(TEXT("Control component directly implements player interaction"),
		Control->GetClass()->ImplementsInterface(UPlayerInteractable::StaticClass()));
	TestEqual(TEXT("Control defaults to query-only collision"), Control->GetCollisionEnabled(), ECollisionEnabled::QueryOnly);
	TestEqual(TEXT("Control blocks Visibility"), Control->GetCollisionResponseToChannel(ECC_Visibility), ECR_Block);
	TestFalse(TEXT("Control cannot affect navigation"), Control->CanEverAffectNavigation());
	TestFalse(TEXT("Missing authored mesh fails interaction geometry"), Control->HasValidInteractionGeometry());

	Control->OpenAngleDegrees = 90.0f;
	Control->RotationDurationSeconds = 1.0f;
	Control->ApplyLogicalState(true, EBathWaterControlChangeReason::PlayerInteraction);
	Control->TickComponent(0.3f, LEVELTICK_All, nullptr);
	const FQuat FrozenRotation = Control->GetRelativeRotation().Quaternion();
	Control->BeginRecoveryFreeze();
	Control->TickComponent(0.5f, LEVELTICK_All, nullptr);
	TestTrue(TEXT("Recovery freeze preserves the exact in-flight quaternion"),
		Control->GetRelativeRotation().Quaternion().Equals(FrozenRotation, SMALL_NUMBER));
	Control->BeginRecoveryFreeze();
	Control->PrepareRecoveryCommit();
	TestTrue(TEXT("Recovery commit-pending closes the visual control"),
		Control->GetRelativeRotation().Quaternion().Equals(Closed, KINDA_SMALL_NUMBER));
	Control->CancelRecoveryFreeze();
	TestTrue(TEXT("Recovery rollback restores the in-flight quaternion and motion"),
		Control->GetRelativeRotation().Quaternion().Equals(FrozenRotation, SMALL_NUMBER)
		&& Control->IsMoving() && Control->IsComponentTickEnabled());
	Control->TickComponent(0.7f, LEVELTICK_All, nullptr);
	TestFalse(TEXT("Restored recovery motion completes without extending full duration"), Control->IsMoving());
	Control->ApplyLogicalState(false, EBathWaterControlChangeReason::Reset);
	Control->ApplyLogicalState(true, EBathWaterControlChangeReason::PlayerInteraction);
	Control->TickComponent(0.4f, LEVELTICK_All, nullptr);
	TestTrue(TEXT("Control is moving during partial open"), Control->IsMoving());
	Control->ApplyLogicalState(false, EBathWaterControlChangeReason::FullAutoClose);
	Control->TickComponent(1.0f, LEVELTICK_All, nullptr);
	TestTrue(TEXT("Auto-close reversal returns to the authored closed pose"),
		Control->GetRelativeRotation().Quaternion().Equals(Closed, KINDA_SMALL_NUMBER));
	TestFalse(TEXT("Control disables motion after reaching closed pose"), Control->IsMoving());
	Control->LocalRotationAxis = FVector::ZeroVector;
	Control->RotationDurationSeconds = 0.0f;
	FDataValidationContext InvalidControlContext;
	TestEqual(TEXT("Invalid axis and duration fail native Data Validation"),
		Control->IsDataValid(InvalidControlContext), EDataValidationResult::Invalid);

	ABathhouseBathFacilityActor* Bath = World->SpawnActor<ABathhouseBathFacilityActor>();
	Bath->GetWaterLevelEmptyPoint()->SetRelativeLocation(FVector(1.0f, 2.0f, 3.0f));
	Bath->GetWaterLevelFullPoint()->SetRelativeLocation(FVector(5.0f, 6.0f, 11.0f));
	Bath->GetFacilityPlacementComponent()->SetPlacedDomainActive(true);
	UBathWaterControlComponent* InteractionControl = Bath->GetFillValveControl();
	UStaticMesh* InteractionMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (!TestNotNull(TEXT("Interaction fixture mesh loads"), InteractionMesh)
		|| !TestNotNull(TEXT("Bath fill control exists"), InteractionControl))
	{
		return false;
	}
	InteractionControl->SetStaticMesh(InteractionMesh);
	InteractionControl->LocalRotationAxis = FVector::UpVector;
	InteractionControl->OpenAngleDegrees = 90.0f;
	InteractionControl->RotationDurationSeconds = 0.5f;
	FPlayerInteractionContext InteractionContext;
	InteractionContext.HitActor = Bath;
	InteractionContext.HitComponent = InteractionControl;
	TestTrue(TEXT("Valid control motion authoring and query geometry allow interaction"),
		InteractionControl->QueryInteraction(InteractionContext).bCanInteract);

	UBathWaterStateComponent* InteractionWater = Bath->GetBathWaterState();
	UNiagaraComponent* InteractionFlow = Bath->FindComponentByClass<UNiagaraComponent>();
	int32 InteractionControlChanges = 0;
	InteractionWater->OnControlChangedNative.AddLambda(
		[&InteractionControlChanges](EBathWaterControlType, bool, EBathWaterControlChangeReason)
		{
			++InteractionControlChanges;
		});
	const auto VerifyInvalidMotionAuthoring = [this, InteractionControl, InteractionWater, InteractionFlow, &InteractionContext, &InteractionControlChanges](
		const TCHAR* Label,
		const FVector Axis,
		const float Angle,
		const float Duration)
	{
		InteractionControl->LocalRotationAxis = Axis;
		InteractionControl->OpenAngleDegrees = Angle;
		InteractionControl->RotationDurationSeconds = Duration;
		const FQuat RotationBefore = InteractionControl->GetRelativeRotation().Quaternion();
		const float AmountBefore = InteractionWater->GetNormalizedAmount();
		const EBathWaterState StateBefore = InteractionWater->GetWaterState();
		const bool bWaterTickBefore = InteractionWater->IsComponentTickEnabled();
		const bool bControlTickBefore = InteractionControl->IsComponentTickEnabled();
		const bool bFlowActiveBefore = InteractionFlow && InteractionFlow->IsActive();
		const int32 ControlChangesBefore = InteractionControlChanges;
		const FPlayerInteractionQuery Query = InteractionControl->QueryInteraction(InteractionContext);
		const FPlayerInteractionResult Result = InteractionControl->ExecuteInteraction(InteractionContext);
		InteractionControl->TickComponent(1.0f, LEVELTICK_All, nullptr);
		const FString Prefix(Label);
		TestFalse(*(Prefix + TEXT(" query is fail-closed")), Query.bCanInteract);
		TestTrue(*(Prefix + TEXT(" query reports an explicit reason")), !Query.FailureReason.IsEmpty());
		TestFalse(*(Prefix + TEXT(" execute fails")), Result.bSucceeded);
		TestFalse(*(Prefix + TEXT(" logical fill state remains closed")),
			InteractionWater->IsControlOpen(EBathWaterControlType::FillValve));
		TestEqual(*(Prefix + TEXT(" water amount is unchanged")), InteractionWater->GetNormalizedAmount(), AmountBefore);
		TestEqual(*(Prefix + TEXT(" derived water state is unchanged")), InteractionWater->GetWaterState(), StateBefore);
		TestEqual(*(Prefix + TEXT(" water tick state is unchanged")), InteractionWater->IsComponentTickEnabled(), bWaterTickBefore);
		TestEqual(*(Prefix + TEXT(" control tick state is unchanged")), InteractionControl->IsComponentTickEnabled(), bControlTickBefore);
		TestTrue(*(Prefix + TEXT(" control rotation is unchanged")),
			InteractionControl->GetRelativeRotation().Quaternion().Equals(RotationBefore, SMALL_NUMBER));
		TestFalse(*(Prefix + TEXT(" transform remains finite")), InteractionControl->GetComponentTransform().ContainsNaN());
		TestEqual(*(Prefix + TEXT(" control delegate is not emitted")), InteractionControlChanges, ControlChangesBefore);
		TestEqual(*(Prefix + TEXT(" Niagara state is unchanged")),
			InteractionFlow && InteractionFlow->IsActive(), bFlowActiveBefore);
	};
	const float QuietNaN = std::numeric_limits<float>::quiet_NaN();
	const float Infinity = std::numeric_limits<float>::infinity();
	VerifyInvalidMotionAuthoring(TEXT("Zero axis"), FVector::ZeroVector, 90.0f, 0.5f);
	VerifyInvalidMotionAuthoring(TEXT("NaN axis"), FVector(QuietNaN, 0.0f, 1.0f), 90.0f, 0.5f);
	VerifyInvalidMotionAuthoring(TEXT("Infinite axis"), FVector(Infinity, 0.0f, 1.0f), 90.0f, 0.5f);
	VerifyInvalidMotionAuthoring(TEXT("NaN angle"), FVector::UpVector, QuietNaN, 0.5f);
	VerifyInvalidMotionAuthoring(TEXT("Infinite angle"), FVector::UpVector, Infinity, 0.5f);
	VerifyInvalidMotionAuthoring(TEXT("NaN duration"), FVector::UpVector, 90.0f, QuietNaN);
	VerifyInvalidMotionAuthoring(TEXT("Infinite duration"), FVector::UpVector, 90.0f, Infinity);
	VerifyInvalidMotionAuthoring(TEXT("Zero duration"), FVector::UpVector, 90.0f, 0.0f);
	VerifyInvalidMotionAuthoring(TEXT("Negative duration"), FVector::UpVector, 90.0f, -0.5f);
	InteractionControl->LocalRotationAxis = FVector::UpVector;
	InteractionControl->OpenAngleDegrees = 90.0f;
	InteractionControl->RotationDurationSeconds = 0.5f;
	TestTrue(TEXT("Restored valid authoring executes through the interaction boundary"),
		InteractionControl->ExecuteInteraction(InteractionContext).bSucceeded);
	TestTrue(TEXT("Valid interaction opens the logical fill control and starts motion"),
		InteractionWater->IsControlOpen(EBathWaterControlType::FillValve)
		&& InteractionControl->IsMoving()
		&& InteractionControl->IsComponentTickEnabled());
	InteractionWater->ResetEmptyForPlacement();

	UBathWaterStateComponent* BathWater = Bath->GetBathWaterState();
	BathWater->FillRatePercentPerSecond = 100.0f;
	BathWater->DrainRatePercentPerSecond = 100.0f;
	int32 BathAvailabilityNotifications = 0;
	UBathhouseFacilitySubsystem* FacilitySubsystem = World->GetSubsystem<UBathhouseFacilitySubsystem>();
	if (!TestNotNull(TEXT("Facility subsystem exists"), FacilitySubsystem))
	{
		return false;
	}
	const FDelegateHandle AvailabilityHandle = FacilitySubsystem->OnFacilityAvailabilityChanged.AddLambda(
		[&BathAvailabilityNotifications](const EBathhouseFacilityType FacilityType)
		{
			if (FacilityType == EBathhouseFacilityType::Bath)
			{
				++BathAvailabilityNotifications;
			}
		});
	const FRotator SurfaceRotation = Bath->GetWaterSurfaceMesh()->GetRelativeRotation();
	const FVector SurfaceScale = Bath->GetWaterSurfaceMesh()->GetRelativeScale3D();
	const int32 SurfaceMaterialCount = Bath->GetWaterSurfaceMesh()->GetNumMaterials();
	TestFalse(TEXT("An empty placed Bath is below the global reservation threshold"), Bath->IsAvailableForReservation());
	TestFalse(TEXT("Zero water hides the plane mesh"), Bath->GetWaterSurfaceMesh()->IsVisible());
	TestTrue(TEXT("Zero water also enables hidden in game"),
		Bath->GetWaterSurfaceMesh()->bHiddenInGame);
	FText FailureReason;
	TestTrue(TEXT("Bath fill opens for presentation test"), BathWater->RequestSetControlOpen(
		EBathWaterControlType::FillValve, true, EBathWaterControlChangeReason::PlayerInteraction, FailureReason));
	BathWater->TickComponent(0.25f, LEVELTICK_All, nullptr);
	TestTrue(TEXT("Plane mover uses exact quarter vector lerp"),
		Bath->GetWaterSurfaceMover()->GetRelativeLocation().Equals(FVector(2.0f, 3.0f, 5.0f)));
	BathWater->TickComponent(0.25f, LEVELTICK_All, nullptr);
	TestTrue(TEXT("Plane mover uses exact half vector lerp"),
		Bath->GetWaterSurfaceMover()->GetRelativeLocation().Equals(FVector(3.0f, 4.0f, 7.0f)));
	BathWater->TickComponent(0.3f, LEVELTICK_All, nullptr);
	TestTrue(TEXT("Plane mover uses exact eighty-percent vector lerp"),
		Bath->GetWaterSurfaceMover()->GetRelativeLocation().Equals(FVector(4.2f, 5.2f, 9.4f), KINDA_SMALL_NUMBER));
	TestTrue(TEXT("The exact global threshold enables Bath reservation"), Bath->IsAvailableForReservation());
	TestEqual(TEXT("Rising through the threshold publishes availability once"), BathAvailabilityNotifications, 1);
	BathWater->TickComponent(0.2f, LEVELTICK_All, nullptr);
	TestTrue(TEXT("Full water places the mover at the exact full marker"),
		Bath->GetWaterSurfaceMover()->GetRelativeLocation().Equals(FVector(5.0f, 6.0f, 11.0f)));
	TestTrue(TEXT("Positive water amount shows the plane mesh component"), Bath->GetWaterSurfaceMesh()->IsVisible());
	TestFalse(TEXT("Positive water clears the authored hidden-in-game flag"),
		Bath->GetWaterSurfaceMesh()->bHiddenInGame);
	TestTrue(TEXT("Water updates leave surface rotation, scale and materials unchanged"),
		Bath->GetWaterSurfaceMesh()->GetRelativeRotation().Equals(SurfaceRotation)
		&& Bath->GetWaterSurfaceMesh()->GetRelativeScale3D().Equals(SurfaceScale)
		&& Bath->GetWaterSurfaceMesh()->GetNumMaterials() == SurfaceMaterialCount);
	TestTrue(TEXT("Water plane never participates in collision, overlap or navigation"),
		Bath->GetWaterSurfaceMesh()->GetCollisionEnabled() == ECollisionEnabled::NoCollision
		&& !Bath->GetWaterSurfaceMesh()->GetGenerateOverlapEvents()
		&& !Bath->GetWaterSurfaceMesh()->CanEverAffectNavigation());
	TestTrue(TEXT("Drain can open at full"), BathWater->RequestSetControlOpen(
		EBathWaterControlType::DrainLever, true, EBathWaterControlChangeReason::PlayerInteraction, FailureReason));
	BathWater->TickComponent(1.0f, LEVELTICK_All, nullptr);
	TestTrue(TEXT("Zero water places the mover at the exact empty marker and hides it"),
		Bath->GetWaterSurfaceMover()->GetRelativeLocation().Equals(FVector(1.0f, 2.0f, 3.0f))
		&& !Bath->GetWaterSurfaceMesh()->IsVisible()
		&& Bath->GetWaterSurfaceMesh()->bHiddenInGame);
	TestFalse(TEXT("Falling below threshold disables Bath reservation"), Bath->IsAvailableForReservation());
	TestEqual(TEXT("Each threshold direction publishes exactly once"), BathAvailabilityNotifications, 2);
	FacilitySubsystem->OnFacilityAvailabilityChanged.Remove(AvailabilityHandle);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBathhouseBathWaterSessionTest,
	"BathhouseSim.BathWater.CustomerSearchActualTimeAndInvalidation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBathhouseBathWaterSessionTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FScopedBathWaterAutomationWorld Scope(TEXT("BathWaterSessionWorld"));
	UWorld* World = Scope.Get();
	if (!TestNotNull(TEXT("Automation world exists"), World))
	{
		return false;
	}

	UBathWaterSettings* Settings = GetMutableDefault<UBathWaterSettings>();
	const float OriginalThreshold = Settings->CustomerUsableThresholdPercent;
	Settings->CustomerUsableThresholdPercent = 80.0f;

	ACharacter* Customer = World->SpawnActor<ACharacter>();
	UCustomerSessionComponent* Session = NewObject<UCustomerSessionComponent>(Customer, TEXT("BathSession"));
	Session->RegisterComponent();
	UCustomerRoutineDefinition* Definition = NewObject<UCustomerRoutineDefinition>(Customer);
	Definition->BathStayDurationSeconds = 60.0f;
	Definition->BathSearchTimeoutSeconds = 10.0f;
	Session->InitializeSession(Definition, nullptr);
	TestTrue(TEXT("Total bath stay starts"), Session->StartBathStay());
	TestTrue(TEXT("First bath search window starts"), Session->BeginBathSearchWindow());
	const double FirstEndTime = Session->BathSearchEndTime;
	const uint64 FirstSearchSerial = Session->BathSearchSerial;
	const float FirstResolvedDuration = Session->BathSearchResolvedDurationSeconds;
	const float FirstElapsed = Session->GetBathSearchElapsedSeconds();
	TestTrue(TEXT("Retry does not reset an active search window"), Session->BeginBathSearchWindow());
	TestEqual(TEXT("Active search end time remains stable"), Session->BathSearchEndTime, FirstEndTime);
	TestEqual(TEXT("Retry preserves the resolved search duration"),
		Session->BathSearchResolvedDurationSeconds, FirstResolvedDuration);
	TestEqual(TEXT("Retry preserves the logical search serial"), Session->BathSearchSerial, FirstSearchSerial);
	TestTrue(TEXT("Retry does not reset elapsed search progress"),
		Session->GetBathSearchElapsedSeconds() + 0.01f >= FirstElapsed);
	TestTrue(TEXT("Search duration is clamped to ten seconds"),
		FMath::IsNearlyEqual(Session->GetRemainingBathSearchSeconds(), 10.0f, 0.05f));

	ABathhouseBathFacilityActor* Bath = World->SpawnActor<ABathhouseBathFacilityActor>();
	UBathhouseFacilitySlotComponent* Slot = NewObject<UBathhouseFacilitySlotComponent>(Bath, TEXT("BathWaterTestSlot"));
	Slot->RegisterComponent();
	TestTrue(TEXT("Test customer reserves the bath slot"), Slot->TryReserve(Customer));
	Session->CurrentFacilityActor = Bath;
	Session->CurrentFacilitySlot = Slot;
	Session->CacheCurrentFacilityTransforms();
	Session->BindCurrentBathWater();
	const FString NavigationDiagnostic = Session->BuildBathLoopDiagnosticLine(
		TEXT("Navigation"), TEXT("Failed"), ECustomerBathLoopReason::NavigationFailed, 1.25f);
	const FString EntryDiagnostic = Session->BuildBathLoopDiagnosticLine(
		TEXT("EntryUseValidation"), TEXT("Failed"), ECustomerBathLoopReason::EntryValidationFailed, 1.25f);
	const FString AdvanceDiagnostic = Session->BuildBathLoopDiagnosticLine(
		TEXT("Advance"), TEXT("LeaveBathLoop"), ECustomerBathLoopReason::SearchExpired, 1.25f);
	TestTrue(TEXT("StateTree Bath diagnostics distinguish navigation failure"),
		NavigationDiagnostic.Contains(TEXT("Phase=Navigation"))
		&& NavigationDiagnostic.Contains(TEXT("Reason=NavigationFailed")));
	TestTrue(TEXT("StateTree Bath diagnostics distinguish entry validation failure"),
		EntryDiagnostic.Contains(TEXT("Phase=EntryUseValidation"))
		&& EntryDiagnostic.Contains(TEXT("Reason=EntryValidationFailed")));
	TestTrue(TEXT("StateTree Bath diagnostics distinguish BathLoop advance"),
		AdvanceDiagnostic.Contains(TEXT("Phase=Advance"))
		&& AdvanceDiagnostic.Contains(TEXT("Reason=SearchExpired")));
	const TArray<FString> OrderedFields = {
		TEXT("Customer="), TEXT("Iteration="), TEXT("Phase="), TEXT("BathActor="), TEXT("Slot="),
		TEXT("WaterPercent="), TEXT("ThresholdPercent="), TEXT("BathStayRemaining="),
		TEXT("ActualBathSeconds="), TEXT("SearchElapsed="), TEXT("Result="), TEXT("Reason=") };
	int32 PreviousFieldIndex = INDEX_NONE;
	bool bDiagnosticFieldsOrdered = true;
	for (const FString& Field : OrderedFields)
	{
		const int32 FieldIndex = NavigationDiagnostic.Find(Field);
		bDiagnosticFieldsOrdered &= FieldIndex > PreviousFieldIndex;
		PreviousFieldIndex = FieldIndex;
	}
	TestTrue(TEXT("Bath diagnostics preserve the approved correlation field order"), bDiagnosticFieldsOrdered);
	UBathWaterStateComponent* Water = Bath->GetBathWaterState();
	Water->FillRatePercentPerSecond = 100.0f;
	FText FailureReason;
	TestTrue(TEXT("Bath fills through native control request"), Water->RequestSetControlOpen(
		EBathWaterControlType::FillValve, true, EBathWaterControlChangeReason::PlayerInteraction, FailureReason));
	Water->TickComponent(0.8f, LEVELTICK_All, nullptr);
	TestTrue(TEXT("Exact threshold bath is usable"), Session->IsCurrentBathUsable());
	Session->CompleteBathSearchWindow();
	TestTrue(TEXT("Reservation completion ends the search window"), !Session->bBathSearchActive);
	TestTrue(TEXT("Slot enters occupied state"), Slot->BeginUse(Customer));
	TestFalse(TEXT("Occupancy without an action-point snap cannot start actual bath time"),
		Session->BeginActualBathSegment());
	Session->bSnappedToFacilityActionPoint = true;
	TestTrue(TEXT("Actual bath segment starts only after occupancy"), Session->BeginActualBathSegment());
	Session->ActualBathSegmentStartTime = World->GetTimeSeconds() - 3.0;
	Session->EndActualBathSegment(ECustomerBathLoopReason::DwellCompleted);
	TestTrue(TEXT("Actual bath time accumulates timestamp delta"),
		FMath::IsNearlyEqual(Session->GetActualBathSeconds(), 3.0f, 0.01f));

	TestTrue(TEXT("A second actual segment can start"), Session->BeginActualBathSegment());
	Session->ActualBathSegmentStartTime = World->GetTimeSeconds() - 2.0;
	Session->PauseRoutineTimers();
	TestTrue(TEXT("Knockdown pause commits active time once and stops the segment"),
		!Session->bActualBathSegmentActive
		&& FMath::IsNearlyEqual(Session->GetActualBathSeconds(), 5.0f, 0.01f));
	TestTrue(TEXT("Knockdown suspension retains the reservation"), Session->SuspendCurrentFacilityUseForKnockdown());
	Session->ResumeRoutineTimers();
	TestTrue(TEXT("Valid knockdown recovery revalidates water and resumes Bath use"),
		Session->ResumeCurrentFacilityUseAfterKnockdown());
	TestTrue(TEXT("Valid knockdown recovery starts a new actual segment"), Session->bActualBathSegmentActive);
	Session->ActualBathSegmentStartTime = World->GetTimeSeconds() - 2.0;
	TestTrue(TEXT("Fill closes before threshold-drain verification"), Water->RequestSetControlOpen(
		EBathWaterControlType::FillValve, false, EBathWaterControlChangeReason::PlayerInteraction, FailureReason));
	TestTrue(TEXT("Drain opens while occupied"), Water->RequestSetControlOpen(
		EBathWaterControlType::DrainLever, true, EBathWaterControlChangeReason::PlayerInteraction, FailureReason));
	Water->TickComponent(0.1f, LEVELTICK_All, nullptr);
	TestTrue(TEXT("Threshold fall commits forced-exit pending"), Session->IsCurrentBathExitPending());
	TestTrue(TEXT("Threshold fall stores its forced-exit reason"),
		Session->CurrentBathExitReason.IsSet()
		&& Session->CurrentBathExitReason.GetValue() == ECustomerBathLoopReason::WaterBelowThreshold);
	TestFalse(TEXT("Threshold fall stops actual bath time immediately"), Session->bActualBathSegmentActive);
	TestTrue(TEXT("Actual time sums segments while excluding search, movement and knockdown pause"),
		FMath::IsNearlyEqual(Session->GetActualBathSeconds(), 7.0f, 0.01f));

	Water->RequestSetControlOpen(
		EBathWaterControlType::DrainLever, false, EBathWaterControlChangeReason::PlayerInteraction, FailureReason);
	Water->RequestSetControlOpen(
		EBathWaterControlType::FillValve, true, EBathWaterControlChangeReason::PlayerInteraction, FailureReason);
	Water->TickComponent(0.1f, LEVELTICK_All, nullptr);
	TestTrue(TEXT("Rising water does not cancel an in-progress exit"), Session->IsCurrentBathExitPending());

	Session->SnapCurrentFacility(ECustomerFacilitySnapTarget::ApproachPoint);
	Session->ReleaseCurrentFacility();
	TestFalse(TEXT("Release clears exit pending only after slot cleanup"), Session->IsCurrentBathExitPending());
	TestFalse(TEXT("Release clears the committed exit reason"), Session->CurrentBathExitReason.IsSet());
	Session->BathStayEndTime = World->GetTimeSeconds() + 4.0;
	TestTrue(TEXT("A new Bath need starts a fresh search window"), Session->BeginBathSearchWindow());
	TestTrue(TEXT("A new logical search increments its serial"), Session->BathSearchSerial > FirstSearchSerial);
	TestTrue(TEXT("Search duration clamps to remaining total Bath stay"),
		Session->GetRemainingBathSearchSeconds() <= 4.0f
		&& Session->GetRemainingBathSearchSeconds() > 3.9f);
	Session->BathSearchEndTime = World->GetTimeSeconds();
	TestTrue(TEXT("Clamped search reports approximately four actual seconds at expiry"),
		FMath::IsNearlyEqual(
			Session->GetBathSearchElapsedSeconds(),
			Session->BathSearchResolvedDurationSeconds,
			0.01f)
		&& Session->BathSearchResolvedDurationSeconds > 3.9f
		&& Session->BathSearchResolvedDurationSeconds <= 4.0f);
	const float SatisfactionBeforeExpiry = Session->GetSatisfaction();
	Session->HandleBathSearchExpired();
	Session->HandleBathSearchExpired();
	TestTrue(TEXT("Repeated search expiry remains a single committed state"), Session->IsBathSearchExpired());
	Session->HandleBathStayExpired();
	Session->HandleBathStayExpired();
	TestTrue(TEXT("Repeated total expiry remains a single committed state"), Session->IsBathStayExpired());
	TestEqual(TEXT("Bath search and total expiry do not mutate satisfaction"),
		Session->GetSatisfaction(), SatisfactionBeforeExpiry);

	ACharacter* FullWindowCustomer = World->SpawnActor<ACharacter>();
	UCustomerSessionComponent* FullWindowSession = NewObject<UCustomerSessionComponent>(
		FullWindowCustomer, TEXT("FullBathSearchSession"));
	FullWindowSession->RegisterComponent();
	UCustomerRoutineDefinition* FullWindowDefinition = NewObject<UCustomerRoutineDefinition>(FullWindowCustomer);
	FullWindowDefinition->BathStayDurationSeconds = 60.0f;
	FullWindowDefinition->BathSearchTimeoutSeconds = 10.0f;
	FullWindowSession->InitializeSession(FullWindowDefinition, nullptr);
	TestTrue(TEXT("Full search diagnostic fixture starts Bath stay"), FullWindowSession->StartBathStay());
	TestTrue(TEXT("Full ten-second search diagnostic fixture starts"), FullWindowSession->BeginBathSearchWindow());
	FullWindowSession->BathSearchEndTime = World->GetTimeSeconds();
	TestTrue(TEXT("Full search reports approximately ten actual seconds at expiry"),
		FMath::IsNearlyEqual(FullWindowSession->GetBathSearchElapsedSeconds(), 10.0f, 0.01f));
	FullWindowSession->HandleBathSearchExpired();
	TestTrue(TEXT("Expired full search retains its ten-second diagnostic value"),
		FMath::IsNearlyEqual(FullWindowSession->GetBathSearchElapsedSeconds(), 10.0f, 0.01f));
	Settings->CustomerUsableThresholdPercent = OriginalThreshold;
	return true;
}

#endif
