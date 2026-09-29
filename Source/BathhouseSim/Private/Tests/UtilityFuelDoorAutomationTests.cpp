#if WITH_DEV_AUTOMATION_TESTS
#include "Tests/UtilityLaborAutomationTestSupport.h"

#include "Camera/CameraComponent.h"
#include "Combat/MonkeyWrenchActor.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"
#include "Interaction/PhysicalCarryFixedSlotActor.h"
#include "Interaction/PlayerCarryComponent.h"
#include "Interaction/PlayerInteractionComponent.h"
#include "Placement/FacilityActorConversionTransaction.h"
#include "Placement/FacilityPlacementComponent.h"
#include "Placement/FacilityPlacementDefinition.h"
#include "Placement/PlaceableFacilityItemActor.h"
#include "Utility/UtilityFuelDoorComponent.h"
#include "Utility/UtilityFuelSupplyActor.h"
#include "Utility/UtilityShovelActor.h"
#include "Utility/UtilityOperationComponent.h"
#include "UObject/UnrealType.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUtilityFuelDoorPresentationTest,
	"BathhouseSim.Utility.Labor.FuelDoorPresentation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUtilityFuelDoorPresentationTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FScopedUtilityLaborWorld Scope(TEXT("UtilityFuelDoorWorld"));
	UWorld* World = Scope.Get();
	if (!TestNotNull(TEXT("Automation world exists"), World))
	{
		return false;
	}
	UStaticMesh* CubeMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (!TestNotNull(TEXT("Engine cube door mesh exists"), CubeMesh))
	{
		return false;
	}

	ABathWaterBoilerFacilityActor* Boiler = World->SpawnActorDeferred<ABathWaterBoilerFacilityActor>(
		ABathWaterBoilerFacilityActor::StaticClass(),
		FTransform::Identity,
		nullptr,
		nullptr,
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (!TestNotNull(TEXT("Deferred boiler exists"), Boiler))
	{
		return false;
	}
	USceneComponent* Pivot = Boiler->GetFuelDoorMesh()->GetAttachParent();
	if (!TestNotNull(TEXT("Door mesh pivot exists"), Pivot))
	{
		return false;
	}
	// The authored pose is the component template's. A stale instance rotation (e.g. a saved preview pose)
	// must not become the baseline.
	const USceneComponent* PivotTemplate = Cast<USceneComponent>(Pivot->GetArchetype());
	if (!TestNotNull(TEXT("Door pivot has a component template"), PivotTemplate))
	{
		return false;
	}
	const FQuat AuthoredClosedRotation = PivotTemplate->GetRelativeRotation().Quaternion();
	Pivot->SetRelativeRotation(FRotator(13.0f, -21.0f, 7.0f));
	UStaticMeshComponent* NeedleMesh = FindNamedMeshComponent(Boiler, TEXT("GaugeNeedleMesh"));
	USceneComponent* NeedlePivot = NeedleMesh ? NeedleMesh->GetAttachParent() : nullptr;
	if (!TestNotNull(TEXT("Gauge needle pivot exists before BeginPlay"), NeedlePivot))
	{
		return false;
	}
	const USceneComponent* NeedlePivotTemplate = Cast<USceneComponent>(NeedlePivot->GetArchetype());
	if (!TestNotNull(TEXT("Gauge needle pivot has a component template"), NeedlePivotTemplate))
	{
		return false;
	}
	const FQuat AuthoredNeedleRotation = NeedlePivotTemplate->GetRelativeRotation().Quaternion();
	NeedlePivot->SetRelativeRotation(FRotator(-8.0f, 17.0f, 4.0f));
	TestTrue(TEXT("Boiler test fixture assigns door and gauge meshes"), SetBoilerTestMeshes(Boiler, CubeMesh));
	Boiler->FinishSpawning(FTransform::Identity);
	BeginActorPlayIfNeeded(Boiler);

	ABathWaterBoilerFacilityActor* OtherBoiler = World->SpawnActor<ABathWaterBoilerFacilityActor>(
		FVector(500.0f, 0.0f, 0.0f), FRotator::ZeroRotator);
	TestTrue(TEXT("Second boiler fixture assigns door and gauge meshes"), SetBoilerTestMeshes(OtherBoiler, CubeMesh));
	BeginActorPlayIfNeeded(OtherBoiler);
	UUtilityFuelDoorComponent* Door = Boiler->GetFuelDoorPresentation();
	UUtilityFuelDoorComponent* OtherDoor = OtherBoiler->GetFuelDoorPresentation();
	if (!TestNotNull(TEXT("Boiler door presentation exists"), Door)
		|| !TestNotNull(TEXT("Second boiler door presentation exists"), OtherDoor))
	{
		return false;
	}
	FText FailureReason;
	TestTrue(TEXT("Door presentation authoring is valid"), Door->HasValidAuthoring(FailureReason));
	TestTrue(TEXT("Boiler utility authoring includes the required collision-free door mesh"), Boiler->HasValidUtilityAuthoring(FailureReason));
	TestTrue(TEXT("Door mesh has no collision or navigation influence"),
		Boiler->GetFuelDoorMesh()->GetCollisionEnabled() == ECollisionEnabled::NoCollision
		&& !Boiler->GetFuelDoorMesh()->CanEverAffectNavigation());
	TestTrue(TEXT("FuelIntakeVolume is a fixed unit-scale Box target"),
		Boiler->GetFuelIntakeVolume()->GetCollisionEnabled() == ECollisionEnabled::QueryOnly
		&& Boiler->GetFuelIntakeVolume()->GetRelativeScale3D().Equals(FVector::OneVector)
		&& Boiler->GetFuelIntakeVolume()->GetUnscaledBoxExtent().Equals(FVector(15.0f)));

	AActor* SourceA = World->SpawnActor<AActor>();
	AActor* SourceB = World->SpawnActor<AActor>();
	TestNotNull(TEXT("First focus source exists"), SourceA);
	TestNotNull(TEXT("Second focus source exists"), SourceB);
	Door->SetSourceInsertable(SourceA, true);
	TestEqual(TEXT("An insertable source targets the door open"), Door->GetTargetAlpha(), 1.0f);
	TestEqual(TEXT("A different boiler keeps an independent closed target"), OtherDoor->GetTargetAlpha(), 0.0f);
	TickWorldForDuration(World, 0.1, 0.1f);
	TestTrue(TEXT("The door reaches half alpha in half of the 0.2 second opening time"),
		FMath::IsNearlyEqual(Door->GetOpenAlpha(), 0.5f, 1.0e-4f));
	const FQuat ExpectedHalfOpen = AuthoredClosedRotation
		* FQuat(FVector::UpVector, FMath::DegreesToRadians(45.0f));
	TestTrue(TEXT("Door rotation uses authored baseline followed by local axis-angle"),
		Pivot->GetRelativeRotation().Quaternion().Equals(ExpectedHalfOpen, 1.0e-4f));
	TestTrue(TEXT("The second boiler remains closed"), FMath::IsNearlyZero(OtherDoor->GetOpenAlpha()));

	Door->SetSourceInsertable(SourceB, true);
	Door->SetSourceInsertable(SourceA, false);
	TestEqual(TEXT("One of multiple valid focus sources keeps the door open"), Door->GetTargetAlpha(), 1.0f);
	Door->SetSourceInsertable(SourceB, false);
	TestEqual(TEXT("All non-insertable sources target closed"), Door->GetTargetAlpha(), 0.0f);
	TickWorldForDuration(World, 0.05, 0.05f);
	TestTrue(TEXT("Closing advances linearly from the current alpha"),
		FMath::IsNearlyEqual(Door->GetOpenAlpha(), 0.25f, 1.0e-4f));
	Door->SetSourceInsertable(SourceA, true);
	TestEqual(TEXT("A mid-close state reverses toward open without resetting alpha"), Door->GetTargetAlpha(), 1.0f);
	TickWorldForDuration(World, 0.05, 0.05f);
	TestTrue(TEXT("Reversal continues from the current door pose"),
		FMath::IsNearlyEqual(Door->GetOpenAlpha(), 0.5f, 1.0e-4f));

	Door->RemoveSource(SourceA);
	TickWorldForDuration(World, 0.1, 0.1f);
	TestTrue(TEXT("Removing the last insertable source closes to the authored baseline"),
		FMath::IsNearlyZero(Door->GetOpenAlpha())
		&& Pivot->GetRelativeRotation().Quaternion().Equals(AuthoredClosedRotation, 1.0e-4f));
	Door->PreviewOpenPose();
	TestTrue(TEXT("Editor preview is a no-op in a game world"), FMath::IsNearlyZero(Door->GetOpenAlpha()));
	Door->RestoreClosedPose();

	Door->SetSourceInsertable(SourceA, true);
	Door->ApplyClosedImmediately();
	TestTrue(TEXT("Immediate close clears source state and restores the closed pose"),
		Door->GetTargetAlpha() == 0.0f && Door->GetOpenAlpha() == 0.0f
		&& Pivot->GetRelativeRotation().Quaternion().Equals(AuthoredClosedRotation, 1.0e-4f));

	UUtilityGaugeComponent* Gauge = Boiler->FindComponentByClass<UUtilityGaugeComponent>();
	if (!TestNotNull(TEXT("Gauge presentation exists for lifecycle assertions"), Gauge))
	{
		return false;
	}
	const EWorldType::Type PreviousWorldType = World->WorldType;
	World->WorldType = EWorldType::EditorPreview;
	Door->PreviewOpenPose();
	TestTrue(TEXT("Door editor preview opens at its authored baseline"),
		Door->GetOpenAlpha() == 1.0f
		&& Pivot->GetRelativeRotation().Quaternion().Equals(
			AuthoredClosedRotation * FQuat(FVector::UpVector, FMath::DegreesToRadians(90.0f)), 1.0e-4f));
	Door->EndPlay(EEndPlayReason::RemovedFromWorld);
	TestTrue(TEXT("Door EndPlay closes pose and preserves the injected pivot and baseline"),
		Door->HasValidAuthoring(FailureReason)
		&& Door->GetOpenAlpha() == 0.0f
		&& Pivot->GetRelativeRotation().Quaternion().Equals(AuthoredClosedRotation, 1.0e-4f));
	Door->EndPlay(EEndPlayReason::RemovedFromWorld);
	Door->PreviewOpenPose();
	TestTrue(TEXT("Door can preview again after repeated EndPlay without cumulative rotation"),
		Door->GetOpenAlpha() == 1.0f
		&& Pivot->GetRelativeRotation().Quaternion().Equals(
			AuthoredClosedRotation * FQuat(FVector::UpVector, FMath::DegreesToRadians(90.0f)), 1.0e-4f));
	Door->RestoreClosedPose();
	TestTrue(TEXT("Door preview restoration returns to the original authored rotation"),
		Pivot->GetRelativeRotation().Quaternion().Equals(AuthoredClosedRotation, 1.0e-4f));

	const FQuat AuthoredNeedleZeroPose = AuthoredNeedleRotation
		* FQuat(FVector::ForwardVector, FMath::DegreesToRadians(-90.0f));
	Gauge->EndPlay(EEndPlayReason::RemovedFromWorld);
	TestTrue(TEXT("Gauge EndPlay retains its operation, pivot, and authoring contract"),
		Gauge->HasValidAuthoring(FailureReason));
	Gauge->ApplyConstructionPreview();
	const FQuat FirstPostEndPlayPreview = NeedlePivot->GetRelativeRotation().Quaternion();
	Gauge->ApplyConstructionPreview();
	Gauge->ApplyPoints(0.0f, 100.0f);
	TestTrue(TEXT("Gauge can restore preview after EndPlay without cumulative rotation"),
		FirstPostEndPlayPreview.Equals(AuthoredNeedleZeroPose, 1.0e-4f)
		&& NeedlePivot->GetRelativeRotation().Quaternion().Equals(FirstPostEndPlayPreview, 1.0e-4f));
	Gauge->EndPlay(EEndPlayReason::RemovedFromWorld);
	TestTrue(TEXT("Repeated gauge EndPlay keeps injected references valid"), Gauge->HasValidAuthoring(FailureReason));
	World->WorldType = PreviousWorldType;
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUtilityFuelDoorFocusIntegrationTest,
	"BathhouseSim.Utility.Labor.FuelDoorFocusIntegration",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUtilityFuelDoorFocusIntegrationTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FScopedUtilityLaborWorld Scope(TEXT("UtilityFuelDoorFocusWorld"));
	UWorld* World = Scope.Get();
	if (!TestNotNull(TEXT("Automation world exists"), World))
	{
		return false;
	}
	UStaticMesh* CubeMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (!TestNotNull(TEXT("Engine cube mesh exists"), CubeMesh))
	{
		return false;
	}

	APawn* User = World->SpawnActor<APawn>();
	UCameraComponent* Camera = User ? NewObject<UCameraComponent>(User, TEXT("FuelDoorTestCamera")) : nullptr;
	UPlayerCarryComponent* Carry = User ? NewObject<UPlayerCarryComponent>(User, TEXT("FuelDoorTestCarry")) : nullptr;
	UPlayerInteractionComponent* Interaction = User
		? NewObject<UPlayerInteractionComponent>(User, TEXT("FuelDoorTestInteraction")) : nullptr;
	if (!TestNotNull(TEXT("Interaction pawn exists"), User)
		|| !TestNotNull(TEXT("Interaction camera exists"), Camera)
		|| !TestNotNull(TEXT("Carry component exists"), Carry)
		|| !TestNotNull(TEXT("Interaction component exists"), Interaction))
	{
		return false;
	}
	User->SetRootComponent(Camera);
	User->AddInstanceComponent(Camera);
	User->AddInstanceComponent(Carry);
	User->AddInstanceComponent(Interaction);
	Camera->RegisterComponent();
	Carry->RegisterComponent();
	Interaction->RegisterComponent();
	Carry->ConfigureHeldAnchor(Camera);
	Interaction->Configure(Camera, Carry);
	BeginActorPlayIfNeeded(User);

	APlayerController* PlayerController = World->SpawnActor<APlayerController>();
	ULocalPlayer* LocalPlayer = NewObject<ULocalPlayer>(GEngine, TEXT("FuelDoorTestLocalPlayer"));
	if (PlayerController && LocalPlayer)
	{
		BeginActorPlayIfNeeded(PlayerController);
		PlayerController->SetPlayer(LocalPlayer);
		PlayerController->Possess(User);
	}
	const bool bLocallyControlled = User->IsLocallyControlled();
	if (!bLocallyControlled)
	{
		AddWarning(FString::Printf(
			TEXT("Standalone possession did not produce local control (controller=%s, pawn controller=%s); using explicit focus refresh with interaction Tick disabled."),
			*GetNameSafe(PlayerController), *GetNameSafe(User->GetController())));
		Interaction->SetComponentTickEnabled(false);
	}

	const FTransform BoilerTransform(FRotator::ZeroRotator, FVector(200.0f, 0.0f, 0.0f));
	UFacilityPlacementDefinition* BoilerDefinition = NewObject<UFacilityPlacementDefinition>();
	BoilerDefinition->StableId = TEXT("UtilityFuelDoorFocusBoiler");
	BoilerDefinition->FacilityTags.AddTag(FGameplayTag::RequestGameplayTag(TEXT("Facility.Placeable")));
	BoilerDefinition->PlacedFacilityClass = ABathWaterBoilerFacilityActor::StaticClass();
	BoilerDefinition->RecoveryItemClass = APlaceableFacilityItemActor::StaticClass();
	BoilerDefinition->RecoveryItemMesh = CubeMesh;
	FText FailureReason;
	TestTrue(TEXT("Boiler recovery definition validates"), BoilerDefinition->ValidateRuntime(FailureReason));
	ABathWaterBoilerFacilityActor* Boiler = World->SpawnActorDeferred<ABathWaterBoilerFacilityActor>(
		ABathWaterBoilerFacilityActor::StaticClass(),
		BoilerTransform,
		nullptr,
		nullptr,
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	AUtilityFuelSupplyActor* Supply = World->SpawnActor<AUtilityFuelSupplyActor>(
		FVector(100.0f, 0.0f, 0.0f), FRotator::ZeroRotator);
	AUtilityShovelActor* Shovel = World->SpawnActor<AUtilityShovelActor>();
	if (!TestNotNull(TEXT("Boiler exists"), Boiler)
		|| !TestNotNull(TEXT("Fuel supply exists"), Supply)
		|| !TestNotNull(TEXT("Shovel exists"), Shovel))
	{
		return false;
	}
	TestTrue(TEXT("Boiler fixtures assign door and gauge meshes"), SetBoilerTestMeshes(Boiler, CubeMesh));
	Supply->GetSupplyMesh()->SetStaticMesh(CubeMesh);
	Supply->GetSupplyMesh()->SetWorldScale3D(FVector(0.1f));
	Supply->GetSupplyMesh()->UpdateBounds();
	TestTrue(TEXT("Shovel fixture assigns both meshes"), SetShovelTestMeshes(Shovel, CubeMesh));
	BeginActorPlayIfNeeded(Supply);
	BeginActorPlayIfNeeded(Shovel);
	TestTrue(TEXT("Shovel can be carried"), Carry->TryTakePhysicalObject(Shovel, FailureReason));
	TestTrue(TEXT("Boiler authoring is valid"), Boiler->HasValidUtilityAuthoring(FailureReason));
	TestTrue(TEXT("Boiler prepares its staged placement contract"),
		Boiler->GetFacilityPlacementComponent()->PrepareForStagedPlacement(*BoilerDefinition, FailureReason));

	FFloatProperty* MaximumPointsProperty = FindFProperty<FFloatProperty>(
		UUtilityOperationComponent::StaticClass(), TEXT("MaxOperationPoints"));
	FFloatProperty* DecayPointsProperty = FindFProperty<FFloatProperty>(
		UUtilityOperationComponent::StaticClass(), TEXT("DecayPointsPerSecond"));
	if (!TestNotNull(TEXT("Operation capacity authoring property exists"), MaximumPointsProperty))
	{
		return false;
	}
	if (!TestNotNull(TEXT("Operation decay authoring property exists"), DecayPointsProperty))
	{
		return false;
	}
	MaximumPointsProperty->SetPropertyValue_InContainer(Boiler->GetOperation(), 100.0f);
	DecayPointsProperty->SetPropertyValue_InContainer(Boiler->GetOperation(), 0.0f);
	TestTrue(TEXT("Boiler starts with 50 operation points"), Boiler->GetOperation()->ImportOperationState(50.0f, FailureReason));
	Boiler->FinishSpawning(BoilerTransform);
	BeginActorPlayIfNeeded(Boiler);
	TestTrue(TEXT("Boiler finalizes its staged collision snapshot"),
		Boiler->GetFacilityPlacementComponent()->FinalizeStagedPlacementCollisionSnapshot(FailureReason));
	TestTrue(TEXT("Boiler stages its provider registration"), Boiler->StagePlacedDomainRegistration(FailureReason));
	TestTrue(TEXT("Boiler commits staged placement"),
		Boiler->GetFacilityPlacementComponent()->CommitStagedPlacement(FailureReason));
	Boiler->PublishPlacedDomainRegistration();
	TestTrue(TEXT("Installed boiler starts its operation clock"), Boiler->GetOperation()->IsPlacedClockActive());

	UFacilityPlacementDefinition* SecondBoilerDefinition = NewObject<UFacilityPlacementDefinition>();
	SecondBoilerDefinition->StableId = TEXT("UtilityFuelDoorFocusSecondBoiler");
	SecondBoilerDefinition->FacilityTags.AddTag(FGameplayTag::RequestGameplayTag(TEXT("Facility.Placeable")));
	SecondBoilerDefinition->PlacedFacilityClass = ABathWaterBoilerFacilityActor::StaticClass();
	SecondBoilerDefinition->RecoveryItemClass = APlaceableFacilityItemActor::StaticClass();
	SecondBoilerDefinition->RecoveryItemMesh = CubeMesh;
	TestTrue(TEXT("Second boiler recovery definition validates"), SecondBoilerDefinition->ValidateRuntime(FailureReason));
	const FTransform SecondBoilerTransform(FRotator::ZeroRotator, FVector(500.0f, 0.0f, 0.0f));
	ABathWaterBoilerFacilityActor* SecondBoiler = World->SpawnActorDeferred<ABathWaterBoilerFacilityActor>(
		ABathWaterBoilerFacilityActor::StaticClass(),
		SecondBoilerTransform,
		nullptr,
		nullptr,
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (!TestNotNull(TEXT("Second boiler exists"), SecondBoiler))
	{
		return false;
	}
	TestTrue(TEXT("Second boiler fixture assigns its presentation meshes"), SetBoilerTestMeshes(SecondBoiler, CubeMesh));
	TestTrue(TEXT("Second boiler prepares staged placement"),
		SecondBoiler->GetFacilityPlacementComponent()->PrepareForStagedPlacement(*SecondBoilerDefinition, FailureReason));
	MaximumPointsProperty->SetPropertyValue_InContainer(SecondBoiler->GetOperation(), 100.0f);
	DecayPointsProperty->SetPropertyValue_InContainer(SecondBoiler->GetOperation(), 0.0f);
	TestTrue(TEXT("Second boiler starts with 100 operation points"),
		SecondBoiler->GetOperation()->ImportOperationState(100.0f, FailureReason));
	SecondBoiler->FinishSpawning(SecondBoilerTransform);
	BeginActorPlayIfNeeded(SecondBoiler);
	TestTrue(TEXT("Second boiler finalizes its staged collision snapshot"),
		SecondBoiler->GetFacilityPlacementComponent()->FinalizeStagedPlacementCollisionSnapshot(FailureReason));
	TestTrue(TEXT("Second boiler stages its provider registration"), SecondBoiler->StagePlacedDomainRegistration(FailureReason));
	TestTrue(TEXT("Second boiler commits staged placement"),
		SecondBoiler->GetFacilityPlacementComponent()->CommitStagedPlacement(FailureReason));
	SecondBoiler->PublishPlacedDomainRegistration();
	TestTrue(TEXT("Second boiler starts its operation clock"), SecondBoiler->GetOperation()->IsPlacedClockActive());

	Interaction->RefreshInteractionQuery();
	TestTrue(TEXT("Camera trace focuses the supply before scooping"), Interaction->GetCurrentInteractionQuery().bCanHeldApply);
	TestTrue(TEXT("One focused LMB held Apply scoops a coal load"), ExecuteFocusedHeldTargetUse(Interaction, EPlayerHeldTargetUseDirection::Apply).bSucceeded);
	TestTrue(TEXT("Scoop fills the held shovel with 25 coal points"),
		Shovel->GetFuelLoad().Kind == EUtilityFuelKind::Coal
		&& FMath::IsNearlyEqual(Shovel->GetFuelLoad().Points, 25.0f));
	const float FuelBeforeLegacyInputs = Shovel->GetFuelLoad().Points;
	TestFalse(TEXT("E no longer scoops or returns fuel"), Interaction->BeginPrimaryInteraction().bSucceeded);
	TestFalse(TEXT("F has no shovel fuel action"), Interaction->TrySecondaryInteract().bSucceeded);
	TestFalse(TEXT("RMB has no fuel Take action"), ExecuteFocusedHeldTargetUse(Interaction, EPlayerHeldTargetUseDirection::Take).bSucceeded);
	TestTrue(TEXT("Legacy E, F, and RMB preserve the loaded shovel"), FMath::IsNearlyEqual(
		Shovel->GetFuelLoad().Points, FuelBeforeLegacyInputs));

	Supply->SetActorLocation(FVector(1000.0f, 0.0f, 0.0f));
	Interaction->RefreshInteractionQuery();
	TestTrue(TEXT("A camera trace reaches the fixed FuelIntakeVolume"),
		Interaction->GetCurrentInteractionQuery().bCanHeldApply
		&& Interaction->GetCurrentInteractionQuery().HeldApplyActionName.ToString() == TEXT("석탄 투입"));
	FHitResult Hit;
	TestTrue(TEXT("The real camera trace hits the query volume component"),
		Interaction->GetCurrentFocusHit(Hit)
		&& Hit.GetActor() == Boiler
		&& Hit.GetComponent() == Boiler->GetFuelIntakeVolume());
	TestEqual(TEXT("Focus notification opens the door target"),
		Boiler->GetFuelDoorPresentation()->GetTargetAlpha(), 1.0f);

	auto TickFixed = [World, Interaction, bLocallyControlled](
		const int32 StepCount,
		const UUtilityFuelIntakeVolumeComponent* ExpectedFocus = nullptr)
	{
		bool bFocusRemainedOnExpectedVolume = true;
		for (int32 Step = 0; Step < StepCount; ++Step)
		{
			if (!bLocallyControlled)
			{
				Interaction->RefreshInteractionQuery();
			}
			++GFrameCounter;
			World->Tick(LEVELTICK_All, 0.05f);
			if (ExpectedFocus)
			{
				FHitResult StepHit;
				bFocusRemainedOnExpectedVolume &= Interaction->GetCurrentFocusHit(StepHit)
					&& StepHit.GetComponent() == ExpectedFocus;
			}
		}
		return bFocusRemainedOnExpectedVolume;
	};
	TickFixed(4);
	TestTrue(TEXT("Door reaches fully open after four 0.05 second steps"),
		FMath::IsNearlyEqual(Boiler->GetFuelDoorPresentation()->GetOpenAlpha(), 1.0f, 1.0e-4f));
	TestTrue(TEXT("Opening leaves 50 operating points and the shovel load unchanged"),
		FMath::IsNearlyEqual(Shovel->GetFuelLoad().Points, 25.0f)
		&& FMath::IsNearlyEqual(Boiler->GetOperation()->GetRemainingPoints(), 50.0f, 0.25f));

	TestTrue(TEXT("E inserts through query, execution, and the fresh camera trace"),
		ExecuteFocusedHeldTargetUse(Interaction, EPlayerHeldTargetUseDirection::Apply).bSucceeded);
	TestTrue(TEXT("A single 석탄 투입 action empties the shovel and raises operation to 75"),
		Shovel->IsLoadEmpty()
		&& FMath::IsNearlyEqual(Boiler->GetOperation()->GetRemainingPoints(), 75.0f, 0.25f));
	Interaction->RefreshInteractionQuery();
	TestFalse(TEXT("Next focus query rejects insertion with an empty shovel"),
		Interaction->GetCurrentInteractionQuery().bCanHeldApply);
	TestTrue(TEXT("Empty shovel query exposes its failure reason"),
		Interaction->GetCurrentInteractionQuery().HeldApplyFailureReason.ToString() == TEXT("삽에 연료가 없습니다."));
	TestEqual(TEXT("Empty shovel focus targets the door closed"),
		Boiler->GetFuelDoorPresentation()->GetTargetAlpha(), 0.0f);
	const float AlphaBeforeCloseStep = Boiler->GetFuelDoorPresentation()->GetOpenAlpha();
	TickFixed(1);
	TestTrue(TEXT("Door starts closing from its current animated alpha"),
		Boiler->GetFuelDoorPresentation()->GetOpenAlpha() < AlphaBeforeCloseStep);

	DecayPointsProperty->SetPropertyValue_InContainer(Boiler->GetOperation(), 1.0f);
	TickFixed(1);
	Interaction->RefreshInteractionQuery();
	TestTrue(TEXT("Empty shovel reason remains visible below boiler capacity"),
		Boiler->GetOperation()->GetRemainingPoints() < 100.0f
		&& !Interaction->GetCurrentInteractionQuery().bCanHeldApply
		&& Interaction->GetCurrentInteractionQuery().HeldApplyFailureReason.ToString() == TEXT("삽에 연료가 없습니다."));
	TestEqual(TEXT("Empty shovel keeps the door closed"), Boiler->GetFuelDoorPresentation()->GetTargetAlpha(), 0.0f);

	const FPlayerInteractionResult EmptyShovelAttempt = ExecuteFocusedHeldTargetUse(Interaction, EPlayerHeldTargetUseDirection::Apply);
	TestFalse(TEXT("E is rejected while the held shovel is empty"), EmptyShovelAttempt.bSucceeded);
	TestTrue(TEXT("Rejected empty-shovel E reports the same reason"),
		EmptyShovelAttempt.FailureReason.ToString() == TEXT("삽에 연료가 없습니다."));
	TestTrue(TEXT("Dropping the empty shovel releases the hand"),
		Interaction->TryDropCarry(FVector::ForwardVector).bSucceeded);
	Interaction->RefreshInteractionQuery();
	TestTrue(TEXT("Empty hand reports that a shovel is required"),
		!Interaction->GetCurrentInteractionQuery().bCanHeldApply
		&& Interaction->GetCurrentInteractionQuery().HeldApplyFailureReason.ToString() == TEXT("삽을 들고 있어야 합니다."));
	TestEqual(TEXT("Empty hand keeps the door closed"), Boiler->GetFuelDoorPresentation()->GetTargetAlpha(), 0.0f);

	AMonkeyWrenchActor* Wrench = World->SpawnActor<AMonkeyWrenchActor>();
	if (!TestNotNull(TEXT("Other carry fixture exists"), Wrench))
	{
		return false;
	}
	BeginActorPlayIfNeeded(Wrench);
	TestTrue(TEXT("A wrench can replace the empty hand"), Carry->TryTakePhysicalObject(Wrench, FailureReason));
	Interaction->RefreshInteractionQuery();
	TestTrue(TEXT("Other carry cannot enable coal insertion"),
		!Interaction->GetCurrentInteractionQuery().bCanHeldApply
		&& Interaction->GetCurrentInteractionQuery().HeldApplyFailureReason.ToString() == TEXT("삽을 들고 있어야 합니다."));
	TestEqual(TEXT("Other carry keeps the door closed"), Boiler->GetFuelDoorPresentation()->GetTargetAlpha(), 0.0f);
	TestTrue(TEXT("Wrench can be safely released after the negative query"),
		Carry->RecoverHeldPhysicalObject(Wrench));
	TestTrue(TEXT("Wrench release leaves the hand empty"), Carry->IsHandEmpty());
	Wrench->Destroy();
	TestTrue(TEXT("The empty shovel can be picked up again"), Carry->TryTakePhysicalObject(Shovel, FailureReason));

	Supply->SetActorLocation(FVector(100.0f, 0.0f, 0.0f));
	Interaction->RefreshInteractionQuery();
	TestTrue(TEXT("E scoops coal again before focus-loss cases"),
		Interaction->GetCurrentInteractionQuery().bCanHeldApply
		&& ExecuteFocusedHeldTargetUse(Interaction, EPlayerHeldTargetUseDirection::Apply).bSucceeded);
	Supply->SetActorLocation(FVector(1000.0f, 0.0f, 0.0f));
	TickFixed(1);
	Interaction->RefreshInteractionQuery();
	TestTrue(TEXT("Capacity decay restores an insertion-ready focus"),
		Boiler->GetOperation()->GetRemainingPoints() < 100.0f
		&& Interaction->GetCurrentInteractionQuery().bCanHeldApply
		&& Boiler->GetFuelDoorPresentation()->GetTargetAlpha() == 1.0f);
	TestTrue(TEXT("Opening progress retains the same intake focus before E"),
		TickFixed(1, Boiler->GetFuelIntakeVolume()));
	const float AlphaBeforeMidTransitionInsert = Boiler->GetFuelDoorPresentation()->GetOpenAlpha();
	TestTrue(TEXT("E succeeds during the open transition"), ExecuteFocusedHeldTargetUse(Interaction, EPlayerHeldTargetUseDirection::Apply).bSucceeded);
	Interaction->RefreshInteractionQuery();
	TestTrue(TEXT("Insert empties the shovel and targets close during animation"),
		Shovel->IsLoadEmpty()
		&& Boiler->GetFuelDoorPresentation()->GetTargetAlpha() == 0.0f);
	TestTrue(TEXT("The focus hit remains the exact FuelIntakeVolume during transition"),
		Interaction->GetCurrentFocusHit(Hit) && Hit.GetComponent() == Boiler->GetFuelIntakeVolume());
	TestTrue(TEXT("Mid-transition close continues from the current alpha"),
		TickFixed(1, Boiler->GetFuelIntakeVolume())
		&& Boiler->GetFuelDoorPresentation()->GetOpenAlpha() < AlphaBeforeMidTransitionInsert);

	// The second boiler starts full at 100; operation decay alone makes its focused query insertable.
	Supply->SetActorLocation(FVector(100.0f, 0.0f, 0.0f));
	Interaction->RefreshInteractionQuery();
	TestTrue(TEXT("The shovel is loaded for the full-capacity case"),
		Interaction->GetCurrentInteractionQuery().bCanHeldApply
		&& ExecuteFocusedHeldTargetUse(Interaction, EPlayerHeldTargetUseDirection::Apply).bSucceeded);
	Supply->SetActorLocation(FVector(1000.0f, 0.0f, 0.0f));
	User->SetActorLocation(FVector(300.0f, 0.0f, 0.0f));
	Camera->SetWorldRotation(FRotator::ZeroRotator);
	Interaction->RefreshInteractionQuery();
	TestTrue(TEXT("Camera targets the second boiler's fixed FuelIntakeVolume"),
		Interaction->GetCurrentFocusHit(Hit)
		&& Hit.GetActor() == SecondBoiler
		&& Hit.GetComponent() == SecondBoiler->GetFuelIntakeVolume());
	TestFalse(TEXT("Loaded shovel cannot interact with the full 100 point boiler"),
		Interaction->GetCurrentInteractionQuery().bCanHeldApply);
	TestEqual(TEXT("The full second boiler targets closed"), SecondBoiler->GetFuelDoorPresentation()->GetTargetAlpha(), 0.0f);
	TestEqual(TEXT("The untargeted first boiler stays closed"), Boiler->GetFuelDoorPresentation()->GetTargetAlpha(), 0.0f);
	DecayPointsProperty->SetPropertyValue_InContainer(SecondBoiler->GetOperation(), 1.0f);
	TickFixed(1, SecondBoiler->GetFuelIntakeVolume());
	Interaction->RefreshInteractionQuery();
	TestTrue(TEXT("Decay below 100 enables insertion without re-aiming"),
		SecondBoiler->GetOperation()->GetRemainingPoints() < 100.0f
		&& Interaction->GetCurrentInteractionQuery().bCanHeldApply
		&& SecondBoiler->GetFuelDoorPresentation()->GetTargetAlpha() == 1.0f);
	TestEqual(TEXT("Only the focused boiler door opens"), Boiler->GetFuelDoorPresentation()->GetTargetAlpha(), 0.0f);
	SecondBoiler->GetOperation()->StopPlacedClock(false);
	DecayPointsProperty->SetPropertyValue_InContainer(SecondBoiler->GetOperation(), 0.0f);
	TestTrue(TEXT("Second boiler restarts its settled sub-capacity clock"),
		SecondBoiler->GetOperation()->StartPlacedClock(false));
	TestTrue(TEXT("Focus remains on the exact volume throughout opening"),
		TickFixed(2, SecondBoiler->GetFuelIntakeVolume()));

	const float AlphaBeforeSecondInsert = SecondBoiler->GetFuelDoorPresentation()->GetOpenAlpha();
	const float PointsBeforeSecondInsert = SecondBoiler->GetOperation()->GetRemainingPoints();
	TestTrue(FString::Printf(TEXT("Second boiler has load and headroom before E (points=%.3f max=%.3f load=%.3f)"),
		PointsBeforeSecondInsert,
		SecondBoiler->GetOperation()->GetMaximumPoints(),
		Shovel->GetFuelLoad().Points),
		PointsBeforeSecondInsert < SecondBoiler->GetOperation()->GetMaximumPoints()
		&& Shovel->GetFuelLoad().Points > 0.0f);
	TestTrue(TEXT("E begins with the exact second volume still under the camera trace"),
		Interaction->GetCurrentFocusHit(Hit)
		&& Hit.GetComponent() == SecondBoiler->GetFuelIntakeVolume());
	const FPlayerInteractionResult SecondBoilerInsertResult = ExecuteFocusedHeldTargetUse(Interaction, EPlayerHeldTargetUseDirection::Apply);
	TestTrue(FString::Printf(TEXT("E executes for the second boiler (reason=%s)"),
		*SecondBoilerInsertResult.FailureReason.ToString()), SecondBoilerInsertResult.bSucceeded);
	Interaction->RefreshInteractionQuery();
	TestTrue(TEXT("Fuel insertion to capacity targets the second door closed"),
		!Interaction->GetCurrentInteractionQuery().bCanHeldApply
		&& SecondBoiler->GetFuelDoorPresentation()->GetTargetAlpha() == 0.0f);
	TestTrue(TEXT("Focus stays on the second volume while the door closes"),
		TickFixed(1, SecondBoiler->GetFuelIntakeVolume()));
	const float AlphaAfterCloseStep = SecondBoiler->GetFuelDoorPresentation()->GetOpenAlpha();
	TestTrue(TEXT("Closing advances from the existing alpha"), AlphaAfterCloseStep < AlphaBeforeSecondInsert);
	if (Shovel->IsLoadEmpty())
	{
		User->SetActorLocation(FVector::ZeroVector);
		Supply->SetActorLocation(FVector(100.0f, 0.0f, 0.0f));
		Camera->SetWorldRotation(FRotator::ZeroRotator);
		Interaction->RefreshInteractionQuery();
		TestTrue(TEXT("An empty shovel refills before the door-reversal case"),
			Interaction->GetCurrentInteractionQuery().bCanHeldApply
			&& ExecuteFocusedHeldTargetUse(Interaction, EPlayerHeldTargetUseDirection::Apply).bSucceeded);
		Supply->SetActorLocation(FVector(1000.0f, 0.0f, 0.0f));
		User->SetActorLocation(FVector(300.0f, 0.0f, 0.0f));
		Camera->SetWorldRotation(FRotator::ZeroRotator);
	}
	MaximumPointsProperty->SetPropertyValue_InContainer(SecondBoiler->GetOperation(), 101.0f);
	Interaction->RefreshInteractionQuery();
	const FPlayerInteractionQuery SecondReadyQuery = Interaction->GetCurrentInteractionQuery();
	TestTrue(FString::Printf(
		TEXT("Insertion-ready focus reverses the closing target (query=%d, reason=%s, points=%.3f, max=%.3f, load=%.3f, clock=%d, blocked=%d)"),
		SecondReadyQuery.bCanHeldApply,
		*SecondReadyQuery.HeldApplyFailureReason.ToString(),
		SecondBoiler->GetOperation()->GetRemainingPoints(),
		SecondBoiler->GetOperation()->GetMaximumPoints(),
		Shovel->GetFuelLoad().Points,
		SecondBoiler->GetOperation()->IsPlacedClockActive(),
		SecondBoiler->GetOperation()->IsLaborBlocked()),
		SecondReadyQuery.bCanHeldApply
		&& SecondBoiler->GetFuelDoorPresentation()->GetTargetAlpha() == 1.0f);
	TestTrue(TEXT("Reopening rises from its current alpha and preserves the same focus hit"),
		TickFixed(1, SecondBoiler->GetFuelIntakeVolume())
		&& SecondBoiler->GetFuelDoorPresentation()->GetOpenAlpha() > AlphaAfterCloseStep);
	TestEqual(TEXT("The first boiler remains closed while the second is focused"),
		Boiler->GetFuelDoorPresentation()->GetTargetAlpha(), 0.0f);

	// LAB-045: each transition below begins with the primary boiler door fully open.
	User->SetActorLocation(FVector::ZeroVector);
	Camera->SetWorldRotation(FRotator::ZeroRotator);
	MaximumPointsProperty->SetPropertyValue_InContainer(SecondBoiler->GetOperation(), 100.0f);
	DecayPointsProperty->SetPropertyValue_InContainer(SecondBoiler->GetOperation(), 0.0f);
	DecayPointsProperty->SetPropertyValue_InContainer(Boiler->GetOperation(), 0.0f);
	Interaction->RefreshInteractionQuery();

	auto OpenPrimaryDoor = [this, &TickFixed, Interaction, Carry, Boiler]()
	{
		Interaction->RefreshInteractionQuery();
		const FPlayerInteractionQuery PrimaryQuery = Interaction->GetCurrentInteractionQuery();
		FHitResult PrimaryFocusHit;
		const bool bHasPrimaryFocusHit = Interaction->GetCurrentFocusHit(PrimaryFocusHit);
		TestTrue(FString::Printf(
			TEXT("Primary boiler is actionable before opening (query=%d, reason=%s, points=%.3f, max=%.3f, held=%s, action=%s, doorTarget=%.2f, hit=%s.%s, hitOK=%d)"),
			PrimaryQuery.bCanHeldApply,
			*PrimaryQuery.HeldApplyFailureReason.ToString(),
			Boiler->GetOperation()->GetRemainingPoints(),
			Boiler->GetOperation()->GetMaximumPoints(),
			*GetNameSafe(Carry->GetHeldObject()),
			*PrimaryQuery.HeldApplyActionName.ToString(),
			Boiler->GetFuelDoorPresentation()->GetTargetAlpha(),
			*GetNameSafe(PrimaryFocusHit.GetActor()),
			*GetNameSafe(PrimaryFocusHit.GetComponent()),
			bHasPrimaryFocusHit),
			Interaction->GetCurrentInteractionQuery().bCanHeldApply
			&& Boiler->GetFuelDoorPresentation()->GetTargetAlpha() == 1.0f);
		TestTrue(TEXT("Primary door opens in fixed 0.05 second steps"),
			TickFixed(4, Boiler->GetFuelIntakeVolume()));
		TestTrue(TEXT("Primary door reaches fully open alpha"),
			FMath::IsNearlyEqual(Boiler->GetFuelDoorPresentation()->GetOpenAlpha(), 1.0f, 1.0e-4f));
	};
	auto VerifyClosedWithoutFuelMutation = [this, &TickFixed, Boiler]()
	{
		const float PointsBefore = Boiler->GetOperation()->GetRemainingPoints();
		TestEqual(TEXT("Focus-ending case targets the primary door closed"),
			Boiler->GetFuelDoorPresentation()->GetTargetAlpha(), 0.0f);
		TickFixed(4);
		TestTrue(TEXT("Focus-ending case does not change operation points"),
			FMath::IsNearlyEqual(Boiler->GetOperation()->GetRemainingPoints(), PointsBefore, 1.0e-3f));
		TestTrue(TEXT("Primary door reaches fully closed alpha"),
			FMath::IsNearlyZero(Boiler->GetFuelDoorPresentation()->GetOpenAlpha(), 1.0e-4f));
	};

	OpenPrimaryDoor();
	Camera->SetWorldRotation(FRotator(0.0f, 90.0f, 0.0f));
	Interaction->RefreshInteractionQuery();
	TestFalse(TEXT("Looking away removes actionable intake focus"), Interaction->GetCurrentInteractionQuery().bCanHeldApply);
	VerifyClosedWithoutFuelMutation();
	Camera->SetWorldRotation(FRotator::ZeroRotator);
	OpenPrimaryDoor();
	User->SetActorLocation(FVector(1000.0f, 0.0f, 0.0f));
	Interaction->RefreshInteractionQuery();
	TestFalse(TEXT("Leaving trace range removes actionable intake focus"), Interaction->GetCurrentInteractionQuery().bCanHeldApply);
	VerifyClosedWithoutFuelMutation();
	User->SetActorLocation(FVector::ZeroVector);
	OpenPrimaryDoor();
	TestTrue(TEXT("TryDropCarry succeeds while the primary door is open"),
		Interaction->TryDropCarry(FVector::ForwardVector).bSucceeded);
	VerifyClosedWithoutFuelMutation();
	TestTrue(TEXT("Dropped shovel can be picked up for the fixed-slot case"),
		Carry->TryTakePhysicalObject(Shovel, FailureReason));
	OpenPrimaryDoor();

	APhysicalCarryFixedSlotActor* ShovelSlot = World->SpawnActorDeferred<APhysicalCarryFixedSlotActor>(
		APhysicalCarryFixedSlotActor::StaticClass(),
		FTransform(FRotator::ZeroRotator, FVector(100.0f, 0.0f, 0.0f)),
		nullptr,
		nullptr,
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (!TestNotNull(TEXT("Shovel fixed slot exists"), ShovelSlot))
	{
		return false;
	}
	FObjectPropertyBase* AssignedItemProperty = CastField<FObjectPropertyBase>(
		FindFProperty<FProperty>(APhysicalCarryFixedSlotActor::StaticClass(), TEXT("AssignedItem")));
	FBoolProperty* StartOccupiedProperty = CastField<FBoolProperty>(
		FindFProperty<FProperty>(APhysicalCarryFixedSlotActor::StaticClass(), TEXT("bStartOccupied")));
	if (!TestNotNull(TEXT("Fixed slot assignment property exists"), AssignedItemProperty)
		|| !TestNotNull(TEXT("Fixed slot occupancy property exists"), StartOccupiedProperty))
	{
		return false;
	}
	AssignedItemProperty->SetObjectPropertyValue_InContainer(ShovelSlot, Shovel);
	StartOccupiedProperty->SetPropertyValue_InContainer(ShovelSlot, false);
	ShovelSlot->FinishSpawning(FTransform(FRotator::ZeroRotator, FVector(100.0f, 0.0f, 0.0f)));
	BeginActorPlayIfNeeded(ShovelSlot);
	TestTrue(TEXT("Fixed slot binds its assigned shovel"), Shovel->GetAssignedPhysicalCarryFixedSlot() == ShovelSlot);
	Interaction->RefreshInteractionQuery();
	TestTrue(TEXT("The assigned slot becomes the actual camera focus"),
		Interaction->GetCurrentFocusHit(Hit) && Hit.GetActor() == ShovelSlot);
	TestTrue(TEXT("E stores the shovel in its fixed slot"), Interaction->BeginPrimaryInteraction().bSucceeded);
	TestTrue(TEXT("Fixed-slot E preserves the load and occupies the slot"),
		ShovelSlot->IsOccupied() && Shovel->GetFuelLoad().Points > 0.0f);
	VerifyClosedWithoutFuelMutation();
	TestTrue(TEXT("The shovel is taken back from the slot for the recovery case"),
		Carry->TryTakeFromFixedSlot(ShovelSlot).bSucceeded);
	ShovelSlot->SetActorEnableCollision(false);
	ShovelSlot->Destroy();
	TickFixed(1);
	User->SetActorLocation(FVector(300.0f, 0.0f, 0.0f));
	Camera->SetWorldRotation(FRotator(0.0f, 180.0f, 0.0f));
	Interaction->RefreshInteractionQuery();

	OpenPrimaryDoor();
	const float PointsBeforeRecoveryHold = Boiler->GetOperation()->GetRemainingPoints();
	TestTrue(TEXT("Installed boiler accepts the existing recovery-hold API"),
		Boiler->TryBeginFacilityRecoveryHold(FailureReason));
	Interaction->RefreshInteractionQuery();
	TestFalse(TEXT("Recovery hold blocks the actual intake query"), Interaction->GetCurrentInteractionQuery().bCanHeldApply);
	VerifyClosedWithoutFuelMutation();
	Boiler->CancelFacilityRecoveryHold();
	TestTrue(TEXT("Recovery hold preserves boiler operation points"),
		FMath::IsNearlyEqual(Boiler->GetOperation()->GetRemainingPoints(), PointsBeforeRecoveryHold, 1.0e-3f));

	OpenPrimaryDoor();
	Interaction->SetInteractionSuppressed(true);
	TestEqual(TEXT("Interaction suppression clears the focused door target"),
		Boiler->GetFuelDoorPresentation()->GetTargetAlpha(), 0.0f);
	VerifyClosedWithoutFuelMutation();
	Interaction->SetInteractionSuppressed(false);
	Interaction->RefreshInteractionQuery();
	return true;
}
#endif
