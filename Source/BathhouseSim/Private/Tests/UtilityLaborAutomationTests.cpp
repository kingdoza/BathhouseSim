#if WITH_DEV_AUTOMATION_TESTS
#include "Tests/UtilityLaborAutomationTestSupport.h"
#include "Utility/UtilityGaugeComponent.h"
#include "Utility/UtilityOperationComponent.h"
#include "Utility/UtilityPivotRotation.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUtilityLaborOperationAndCapacityTest,
	"BathhouseSim.Utility.Labor.OperationAndCapacitySplit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUtilityLaborOperationAndCapacityTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FScopedUtilityLaborWorld Scope(TEXT("UtilityLaborOperationWorld"));
	UWorld* World = Scope.Get();
	if (!TestNotNull(TEXT("Automation world exists"), World))
	{
		return false;
	}
	UBathWaterOperationsSubsystem* Operations = World->GetSubsystem<UBathWaterOperationsSubsystem>();
	if (!TestNotNull(TEXT("Operations subsystem exists"), Operations))
	{
		return false;
	}

	AActor* OperationOwner = World->SpawnActorDeferred<AActor>(
		AActor::StaticClass(), FTransform::Identity, nullptr, nullptr,
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (!TestNotNull(TEXT("Operation owner is created before BeginPlay"), OperationOwner))
	{
		return false;
	}
	USceneComponent* OperationOwnerRoot = NewObject<USceneComponent>(OperationOwner, TEXT("LaborOperationRoot"));
	OperationOwner->SetRootComponent(OperationOwnerRoot);
	OperationOwner->AddInstanceComponent(OperationOwnerRoot);
	OperationOwnerRoot->RegisterComponent();
	UUtilityOperationComponent* Operation = NewObject<UUtilityOperationComponent>(OperationOwner, TEXT("LaborOperation"));
	Operation->RegisterComponent();
	FText FailureReason;
	TestTrue(TEXT("Default operation authoring is valid"), Operation->HasValidAuthoring(FailureReason));
	TestEqual(TEXT("New operation begins empty"), Operation->GetRemainingPoints(), 0.0f);
	TestFalse(TEXT("NaN recovery payload is rejected"),
		Operation->ImportOperationState(std::numeric_limits<float>::quiet_NaN(), FailureReason));
	TestTrue(TEXT("Invalid import leaves operation empty"), FMath::IsNearlyZero(Operation->GetRemainingPoints()));
	TestTrue(TEXT("Valid payload imports before the placed clock starts"), Operation->ImportOperationState(25.0f, FailureReason));
	TestEqual(TEXT("Imported points remain available while packaged"), Operation->GetRemainingPoints(), 25.0f);

	UBathWaterUtilityCapacityComponent* Heating = NewObject<UBathWaterUtilityCapacityComponent>(OperationOwner, TEXT("LaborHeatingCapacity"));
	Heating->RestoreCapacity(EBathWaterCapacityKind::Heating, 100.0f);
	Heating->SetUtilityOperation(Operation);
	Heating->RegisterComponent();
	TestTrue(TEXT("Labor provider registers silently"), Operations->RegisterProvider(Heating, false));
	FBathWaterCapacitySnapshot BeforeStart = Operations->GetCapacitySnapshot(EBathWaterCapacityKind::Heating);
	TestTrue(TEXT("Installed rating is present before operation"), FMath::IsNearlyEqual(BeforeStart.TotalPoints, 100.0f));
	TestTrue(TEXT("Unstarted labor operation contributes no active capacity"), FMath::IsNearlyZero(BeforeStart.ActivePoints));

	UBathWaterUtilityCapacityComponent* Circulation = NewObject<UBathWaterUtilityCapacityComponent>(OperationOwner, TEXT("LaborCirculationCapacity"));
	Circulation->RestoreCapacity(EBathWaterCapacityKind::Circulation, 100.0f);
	Circulation->RegisterComponent();
	TestFalse(TEXT("Capacity without an Operation is rejected"), Operations->RegisterProvider(Circulation, false));
	TestTrue(TEXT("Capacity without an Operation contributes zero active points"),
		FMath::IsNearlyZero(Circulation->GetActiveCapacityPoints()));
	OperationOwner->FinishSpawning(FTransform::Identity);
	BeginActorPlayIfNeeded(OperationOwner);
	ABathhouseBathFacilityActor* Bath = World->SpawnActor<ABathhouseBathFacilityActor>();
	TestTrue(TEXT("Bath registers"), Operations->RegisterBath(Bath->GetBathWaterCondition(), false));
	const FBathWaterSettingRequestResult HeatRequest = Operations->RequestTargetTemperature(Bath, 30.0f);
	TestTrue(TEXT("Installed capacity accepts a heating reservation while the boiler is empty"), HeatRequest.bSucceeded);
	FBathWaterCapacitySnapshot EmptyBoilerSnapshot = Operations->GetCapacitySnapshot(EBathWaterCapacityKind::Heating);
	TestTrue(TEXT("Active deficit is based on empty labor capacity"),
		EmptyBoilerSnapshot.DeficitPoints > 0.0f && !EmptyBoilerSnapshot.IsSatisfied());
	TestTrue(TEXT("Installed capacity remains sufficient for the reservation"),
		FMath::IsNearlyZero(EmptyBoilerSnapshot.InstalledDeficitPoints));
	float RemovalDeficit = 0.0f;
	TestFalse(TEXT("Recovery gate still protects installed reservations"), Operations->CanRemoveProvider(Heating, RemovalDeficit));

	int32 OperatingEdges = 0;
	int32 PointPublications = 0;
	Operation->OnOperatingChanged.AddLambda([&OperatingEdges](const bool bIsOperating)
	{
		++OperatingEdges;
	});
	Operation->OnOperationChanged.AddLambda([&PointPublications]()
	{
		++PointPublications;
	});
	TestTrue(TEXT("Starting the placed clock succeeds"), Operation->StartPlacedClock(true));
	TestTrue(TEXT("Positive labor capacity satisfies the reserved heating demand"), Operations->IsCapacitySatisfied(EBathWaterCapacityKind::Heating));
	TestEqual(TEXT("Start emits one operating edge"), OperatingEdges, 1);
	const double GameTimeBeforeFirstTick = World->GetTimeSeconds();
	TickWorldForDuration(World, 10.0);
	const float RemainingAfterFirstTick = Operation->GetRemainingPoints();
	TestTrue(FString::Printf(
		TEXT("Game time decays points independently of editor time (remaining=%.3f, world delta=%.3f, rate=%.3f)"),
		RemainingAfterFirstTick,
		World->GetTimeSeconds() - GameTimeBeforeFirstTick,
		Operation->GetDecayPointsPerSecond()),
		FMath::IsNearlyEqual(RemainingAfterFirstTick, 15.0f, 0.05f));
	TestTrue(TEXT("A positive remainder retains full rated capacity"),
		FMath::IsNearlyEqual(Operations->GetCapacitySnapshot(EBathWaterCapacityKind::Heating).ActivePoints, 100.0f));
	const double GameTimeBeforeBoundaryTick = World->GetTimeSeconds();
	TickWorldForDuration(World, 15.0);
	const float RemainingAtBoundary = Operation->GetRemainingPoints();
	TestTrue(FString::Printf(
		TEXT("Operation reaches exactly empty at its decay boundary (remaining=%.3f, world delta=%.3f)"),
		RemainingAtBoundary,
		World->GetTimeSeconds() - GameTimeBeforeBoundaryTick),
		FMath::IsNearlyZero(RemainingAtBoundary));
	TestEqual(FString::Printf(TEXT("Exhaustion emits exactly one operating edge (operating=%d, points=%d, tick=%d)"),
		OperatingEdges, PointPublications, Operation->IsComponentTickEnabled()), OperatingEdges, 2);
	TestTrue(TEXT("Empty operation removes active capacity"),
		FMath::IsNearlyZero(Operations->GetCapacitySnapshot(EBathWaterCapacityKind::Heating).ActivePoints));

	TestTrue(TEXT("Zero points display at the zero mark"),
		FMath::IsNearlyZero(UUtilityGaugeComponent::CalculateDisplayFraction(0.0f, 100.0f, 1.0f / 3.0f)));
	TestTrue(TEXT("One quarter charge displays at one half"),
		FMath::IsNearlyEqual(UUtilityGaugeComponent::CalculateDisplayFraction(25.0f, 100.0f, 1.0f / 3.0f), 0.5f));
	const float HalfChargeFraction = UUtilityGaugeComponent::CalculateDisplayFraction(50.0f, 100.0f, 1.0f / 3.0f);
	TestTrue(FString::Printf(TEXT("Half charge displays at two thirds (actual=%.8f, expected=%.8f)"),
		HalfChargeFraction, 2.0f / 3.0f), FMath::IsNearlyEqual(HalfChargeFraction, 2.0f / 3.0f, 1.0e-5f));

	UStaticMesh* GaugeTestMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	AActor* GaugeOwner = World->SpawnActorDeferred<AActor>(
		AActor::StaticClass(),
		FTransform::Identity,
		nullptr,
		nullptr,
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (!TestNotNull(TEXT("Gauge lifecycle owner is created before BeginPlay"), GaugeOwner)
		|| !TestNotNull(TEXT("Engine gauge transform mesh exists"), GaugeTestMesh))
	{
		return false;
	}
	USceneComponent* GaugeRoot = NewObject<USceneComponent>(GaugeOwner, TEXT("GaugeTestRoot"));
	GaugeOwner->SetRootComponent(GaugeRoot);
	GaugeOwner->AddInstanceComponent(GaugeRoot);
	GaugeRoot->RegisterComponent();
	UUtilityOperationComponent* GaugeOperation = NewObject<UUtilityOperationComponent>(GaugeOwner, TEXT("GaugeTestOperation"));
	GaugeOwner->AddInstanceComponent(GaugeOperation);
	GaugeOperation->RegisterComponent();
	USceneComponent* NeedlePivot = NewObject<USceneComponent>(GaugeOwner, TEXT("GaugeTestNeedlePivot"));
	NeedlePivot->SetupAttachment(GaugeRoot);
	NeedlePivot->SetRelativeLocation(FVector(4.0f, -2.0f, 3.0f));
	const FQuat AuthoredPivotRotation = FRotator(17.0f, -23.0f, 31.0f).Quaternion();
	NeedlePivot->SetRelativeRotation(AuthoredPivotRotation);
	NeedlePivot->SetRelativeScale3D(FVector(1.25f, 0.8f, 1.1f));
	GaugeOwner->AddInstanceComponent(NeedlePivot);
	NeedlePivot->RegisterComponent();
	UStaticMeshComponent* NeedleChild = NewObject<UStaticMeshComponent>(GaugeOwner, TEXT("GaugeTestNeedleChild"));
	NeedleChild->SetupAttachment(NeedlePivot);
	NeedleChild->SetStaticMesh(GaugeTestMesh);
	NeedleChild->SetRelativeTransform(FTransform(
		FRotator(-12.0f, 8.0f, 4.0f), FVector(2.0f, 0.0f, 1.0f), FVector(0.75f, 0.8f, 0.9f)));
	GaugeOwner->AddInstanceComponent(NeedleChild);
	NeedleChild->RegisterComponent();
	UUtilityGaugeComponent* Gauge = NewObject<UUtilityGaugeComponent>(GaugeOwner, TEXT("GaugeTestPresentation"));
	GaugeOwner->AddInstanceComponent(Gauge);
	Gauge->Configure(GaugeOperation, NeedlePivot);
	Gauge->RegisterComponent();
	const FVector AuthoredPivotLocation = NeedlePivot->GetRelativeLocation();
	const FVector AuthoredPivotScale = NeedlePivot->GetRelativeScale3D();
	const FTransform AuthoredNeedleChildTransform = NeedleChild->GetRelativeTransform();
	Gauge->ApplyConstructionPreview();
	Gauge->ApplyConstructionPreview();
	const FQuat ExpectedZeroRotation = AuthoredPivotRotation
		* FQuat(FVector::ForwardVector, FMath::DegreesToRadians(-90.0f));
	TestTrue(TEXT("Repeated construction previews keep the authored quaternion baseline"),
		NeedlePivot->GetRelativeRotation().Quaternion().Equals(ExpectedZeroRotation, 1.0e-4f));
	GaugeOwner->FinishSpawning(FTransform::Identity);
	BeginActorPlayIfNeeded(GaugeOwner);
	TestTrue(TEXT("Gauge component begins play before lifecycle reconfiguration"), Gauge->HasBegunPlay());
	TestTrue(TEXT("BeginPlay does not recapture the displayed zero rotation as authored"),
		NeedlePivot->GetRelativeRotation().Quaternion().Equals(ExpectedZeroRotation, 1.0e-4f));
	Gauge->Configure(GaugeOperation, NeedlePivot);
	TestTrue(TEXT("Reconfiguring the same pivot remains idempotent"),
		NeedlePivot->GetRelativeRotation().Quaternion().Equals(ExpectedZeroRotation, 1.0e-4f));
	Gauge->ApplyPoints(0.01f, 100.0f);
	const float MinimumPositiveAngle = FMath::Lerp(-90.0f, 90.0f,
		UUtilityGaugeComponent::CalculateDisplayFraction(0.01f, 100.0f, 1.0f / 3.0f));
	const FQuat ExpectedMinimumPositiveRotation = AuthoredPivotRotation
		* FQuat(FVector::ForwardVector, FMath::DegreesToRadians(MinimumPositiveAngle));
	TestTrue(TEXT("Minimum positive charge uses the approved display quaternion"),
		NeedlePivot->GetRelativeRotation().Quaternion().Equals(ExpectedMinimumPositiveRotation, 1.0e-4f));
	auto TestGaugeQuaternionAt = [&](const float Remaining, const TCHAR* Description)
	{
		Gauge->ApplyPoints(Remaining, 100.0f);
		const float Fraction = UUtilityGaugeComponent::CalculateDisplayFraction(Remaining, 100.0f, 1.0f / 3.0f);
		const FQuat Expected = AuthoredPivotRotation
			* FQuat(FVector::ForwardVector, FMath::DegreesToRadians(FMath::Lerp(-90.0f, 90.0f, Fraction)));
		TestTrue(Description, NeedlePivot->GetRelativeRotation().Quaternion().Equals(Expected, 1.0e-4f));
	};
	TestGaugeQuaternionAt(25.0f, TEXT("25/100 charge uses its approved display quaternion"));
	TestGaugeQuaternionAt(50.0f, TEXT("50/100 charge uses its approved display quaternion"));
	TestGaugeQuaternionAt(100.0f, TEXT("100/100 charge uses its approved display quaternion"));
	TestTrue(TEXT("Gauge updates preserve pivot location, scale, and needle child transform"),
		NeedlePivot->GetRelativeLocation().Equals(AuthoredPivotLocation)
		&& NeedlePivot->GetRelativeScale3D().Equals(AuthoredPivotScale)
		&& NeedleChild->GetRelativeTransform().Equals(AuthoredNeedleChildTransform));

	UUtilityOperationComponent* ReboundOperation = NewObject<UUtilityOperationComponent>(GaugeOwner, TEXT("GaugeReboundOperation"));
	TestTrue(TEXT("Replacement operation imports gauge lifecycle state"),
		ReboundOperation->ImportOperationState(25.0f, FailureReason));
	GaugeOwner->AddInstanceComponent(ReboundOperation);
	ReboundOperation->RegisterComponent();
	Gauge->Configure(ReboundOperation, NeedlePivot);
	const float ReboundFraction = UUtilityGaugeComponent::CalculateDisplayFraction(25.0f, 100.0f, 1.0f / 3.0f);
	const FQuat ReboundExpectedRotation = AuthoredPivotRotation
		* FQuat(FVector::ForwardVector, FMath::DegreesToRadians(FMath::Lerp(-90.0f, 90.0f, ReboundFraction)));
	const FQuat ReboundActualRotation = NeedlePivot->GetRelativeRotation().Quaternion();
	TestTrue(FString::Printf(TEXT("Configure rebinds the gauge to a replacement operation (actual=%s, expected=%s)"),
		*ReboundActualRotation.ToString(), *ReboundExpectedRotation.ToString()),
		ReboundActualRotation.Equals(ReboundExpectedRotation, 1.0e-4f));
	const FQuat ProbeRotation(FRotator(4.0f, 5.0f, 6.0f).Quaternion());
	NeedlePivot->SetRelativeRotation(ProbeRotation);
	GaugeOperation->OnOperationChanged.Broadcast();
	TestTrue(TEXT("The previous operation delegate is removed after reconfiguration"),
		NeedlePivot->GetRelativeRotation().Quaternion().Equals(ProbeRotation, 1.0e-4f));
	ReboundOperation->OnOperationChanged.Broadcast();
	TestTrue(TEXT("The replacement operation delegate remains bound"),
		NeedlePivot->GetRelativeRotation().Quaternion().Equals(ReboundExpectedRotation, 1.0e-4f));
	Gauge->DestroyComponent();
	NeedlePivot->SetRelativeRotation(ProbeRotation);
	ReboundOperation->OnOperationChanged.Broadcast();
	TestTrue(TEXT("Destroying the gauge component removes the operation delegate"),
		NeedlePivot->GetRelativeRotation().Quaternion().Equals(ProbeRotation, 1.0e-4f));
	return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUtilityPivotRotationRotatedParentLocationTest,
	"BathhouseSim.Utility.Labor.PivotRotationKeepsExactLocationUnderRotatedParent",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUtilityPivotRotationRotatedParentLocationTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FScopedUtilityLaborWorld Scope(TEXT("UtilityPivotRotatedParentWorld"));
	UWorld* World = Scope.Get();
	if (!TestNotNull(TEXT("Automation world exists"), World))
	{
		return false;
	}
	AActor* Owner = World->SpawnActorDeferred<AActor>(
		AActor::StaticClass(), FTransform::Identity, nullptr, nullptr,
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (!TestNotNull(TEXT("Rotated-parent pivot owner exists"), Owner))
	{
		return false;
	}

	// Mirrors a utility facility authored with a scaled physical root and a rotated SceneRoot.
	USceneComponent* PhysicalRoot = NewObject<USceneComponent>(Owner, TEXT("PivotDriftPhysicalRoot"));
	PhysicalRoot->SetRelativeScale3D(FVector(0.5));
	Owner->SetRootComponent(PhysicalRoot);
	Owner->AddInstanceComponent(PhysicalRoot);
	PhysicalRoot->RegisterComponent();
	USceneComponent* SceneRoot = NewObject<USceneComponent>(Owner, TEXT("PivotDriftSceneRoot"));
	SceneRoot->SetupAttachment(PhysicalRoot);
	SceneRoot->SetRelativeRotation(FRotator(0.0, -90.0, 0.0));
	Owner->AddInstanceComponent(SceneRoot);
	SceneRoot->RegisterComponent();

	auto CreatePivot = [&](const TCHAR* Name)
	{
		USceneComponent* Pivot = NewObject<USceneComponent>(Owner, Name);
		Pivot->SetupAttachment(SceneRoot);
		Pivot->SetRelativeLocation_Direct(FVector(500.0, 23.12633895875516, 192.53095578010425));
		Owner->AddInstanceComponent(Pivot);
		Pivot->RegisterComponent();
		return Pivot;
	};
	auto DescribeLocation = [](const FVector& Location)
	{
		return FString::Printf(TEXT("(%.17g, %.17g, %.17g)"), Location.X, Location.Y, Location.Z);
	};

	// Control: the raw engine call is the drift source this regression test guards against.
	USceneComponent* ControlPivot = CreatePivot(TEXT("PivotDriftControlPivot"));
	const FVector ControlAuthoredLocation = ControlPivot->GetRelativeLocation();
	ControlPivot->SetRelativeRotation(FQuat(FVector::RightVector, FMath::DegreesToRadians(-30.0)));
	AddInfo(FString::Printf(TEXT("Raw SetRelativeRotation under rotated parent: authored=%s after=%s drifted=%s"),
		*DescribeLocation(ControlAuthoredLocation), *DescribeLocation(ControlPivot->GetRelativeLocation()),
		ControlPivot->GetRelativeLocation() != ControlAuthoredLocation ? TEXT("true") : TEXT("false")));

	USceneComponent* Pivot = CreatePivot(TEXT("PivotDriftPivot"));
	const FVector AuthoredLocation = Pivot->GetRelativeLocation();
	auto TestExactLocation = [&](const TCHAR* Step)
	{
		TestTrue(FString::Printf(TEXT("%s keeps the authored pivot location bit-exact (authored=%s, actual=%s)"),
			Step, *DescribeLocation(AuthoredLocation), *DescribeLocation(Pivot->GetRelativeLocation())),
			Pivot->GetRelativeLocation() == AuthoredLocation);
	};

	FUtilityPivotRotation PivotRotation;
	PivotRotation.CaptureBaseline(Pivot);
	PivotRotation.Apply(Pivot, FVector::RightVector, -30.0f);
	TestExactLocation(TEXT("Apply"));
	PivotRotation.Apply(Pivot, FVector::RightVector, -150.0f);
	TestExactLocation(TEXT("Repeated Apply"));
	PivotRotation.Reset(Pivot);
	TestExactLocation(TEXT("Reset"));
	TestTrue(TEXT("Reset still restores the authored baseline rotation"),
		Pivot->GetRelativeRotation().Quaternion().Equals(FQuat::Identity, 1.0e-4f));
	PivotRotation.Apply(Pivot, FVector::RightVector, -90.0f);
	TestTrue(TEXT("Apply still produces the requested local-axis rotation"),
		Pivot->GetRelativeRotation().Quaternion().Equals(
			FQuat(FVector::RightVector, FMath::DegreesToRadians(-90.0f)), 1.0e-4f));
	TestTrue(TEXT("Pivot world transform matches its preserved relative transform"),
		Pivot->GetComponentTransform().Equals(
			Pivot->GetRelativeTransform() * SceneRoot->GetComponentTransform(), 1.0e-6f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUtilityGaugePreviewBaselineFromTemplateTest,
	"BathhouseSim.Utility.Labor.GaugePreviewBaselineUsesComponentTemplate",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUtilityGaugePreviewBaselineFromTemplateTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FScopedUtilityLaborWorld Scope(TEXT("UtilityGaugeTemplateBaselineWorld"));
	UWorld* World = Scope.Get();
	UStaticMesh* CubeMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (!TestNotNull(TEXT("Automation world exists"), World)
		|| !TestNotNull(TEXT("Engine cube mesh exists"), CubeMesh))
	{
		return false;
	}
	ABathWaterBoilerFacilityActor* Boiler = World->SpawnActorDeferred<ABathWaterBoilerFacilityActor>(
		ABathWaterBoilerFacilityActor::StaticClass(), FTransform::Identity, nullptr, nullptr,
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (!TestNotNull(TEXT("Deferred boiler exists"), Boiler))
	{
		return false;
	}
	TestTrue(TEXT("Boiler fixture assigns door and gauge meshes"), SetBoilerTestMeshes(Boiler, CubeMesh));
	Boiler->FinishSpawning(FTransform::Identity);
	BeginActorPlayIfNeeded(Boiler);

	UUtilityGaugeComponent* Gauge = Boiler->GetGaugePresentation();
	USceneComponent* NeedlePivot = Boiler->GetGaugeNeedlePivot();
	UUtilityOperationComponent* Operation = Boiler->GetOperation();
	const USceneComponent* NeedlePivotTemplate = NeedlePivot ? Cast<USceneComponent>(NeedlePivot->GetArchetype()) : nullptr;
	if (!TestNotNull(TEXT("Gauge presentation exists"), Gauge)
		|| !TestNotNull(TEXT("Gauge needle pivot exists"), NeedlePivot)
		|| !TestNotNull(TEXT("Gauge operation exists"), Operation)
		|| !TestNotNull(TEXT("Gauge needle pivot has a component template"), NeedlePivotTemplate))
	{
		return false;
	}
	// Native defaults: LocalRotationAxis = Forward, ZeroAngleDegrees = -90.
	const FQuat ExpectedZeroPose = NeedlePivotTemplate->GetRelativeRotation().Quaternion()
		* FQuat(FVector::ForwardVector, FMath::DegreesToRadians(-90.0f));
	auto DescribePose = [NeedlePivot]()
	{
		return NeedlePivot->GetRelativeRotation().ToString();
	};

	Gauge->ApplyConstructionPreview();
	TestTrue(FString::Printf(TEXT("Construction preview shows the template zero pose (actual=%s)"), *DescribePose()),
		NeedlePivot->GetRelativeRotation().Quaternion().Equals(ExpectedZeroPose, 1.0e-4f));

	// Blueprint compile reinstancing keeps the displayed pivot pose but loses the gauge's baseline state.
	// Re-pointing the gauge at another pivot and back forgets the baseline the same way.
	USceneComponent* ScratchPivot = NewObject<USceneComponent>(Boiler, TEXT("GaugeTemplateBaselineScratchPivot"));
	auto ForgetBaseline = [&]()
	{
		Gauge->Configure(Operation, ScratchPivot);
		Gauge->Configure(Operation, NeedlePivot);
	};
	for (int32 Cycle = 1; Cycle <= 3; ++Cycle)
	{
		ForgetBaseline();
		Gauge->ApplyConstructionPreview();
		TestTrue(FString::Printf(TEXT("Reinstancing cycle %d does not accumulate the zero angle (actual=%s)"),
			Cycle, *DescribePose()),
			NeedlePivot->GetRelativeRotation().Quaternion().Equals(ExpectedZeroPose, 1.0e-4f));
	}

	// A level instance saved with a displayed pose (e.g. Pitch 30) must not treat it as authored on reload.
	NeedlePivot->SetRelativeRotation(FRotator(30.0f, 0.0f, 0.0f));
	ForgetBaseline();
	Gauge->ApplyConstructionPreview();
	TestTrue(FString::Printf(TEXT("A stale saved instance pose is ignored in favor of the template (actual=%s)"),
		*DescribePose()),
		NeedlePivot->GetRelativeRotation().Quaternion().Equals(ExpectedZeroPose, 1.0e-4f));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
