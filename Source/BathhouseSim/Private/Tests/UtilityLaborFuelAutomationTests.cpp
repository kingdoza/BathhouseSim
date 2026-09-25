#if WITH_DEV_AUTOMATION_TESTS
#include "Tests/UtilityLaborAutomationTestSupport.h"
#include "Tests/UtilityLaborAutomationTestProbe.h"
#include "Engine/StaticMeshActor.h"
#include "Interaction/PhysicalCarryFixedSlotActor.h"
#include "UObject/StrongObjectPtr.h"
#include "Utility/UtilityFuelTransaction.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUtilityLaborFuelInteractionTest,
	"BathhouseSim.Utility.Labor.FuelInteractionAtomicity",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUtilityLaborFuelInteractionTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FScopedUtilityLaborWorld Scope(TEXT("UtilityLaborFuelWorld"));
	UWorld* World = Scope.Get();
	if (!TestNotNull(TEXT("Automation world exists"), World))
	{
		return false;
	}
	UStaticMesh* CubeMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (!TestNotNull(TEXT("Engine cube trace mesh exists"), CubeMesh))
	{
		return false;
	}

	APawn* User = World->SpawnActor<APawn>();
	UCameraComponent* Camera = NewObject<UCameraComponent>(User, TEXT("FuelTestCamera"));
	UPlayerCarryComponent* Carry = NewObject<UPlayerCarryComponent>(User, TEXT("FuelTestCarry"));
	UPlayerInteractionComponent* Interaction = NewObject<UPlayerInteractionComponent>(User, TEXT("FuelTestInteraction"));
	UPlayerEquipmentUseComponent* EquipmentUse = NewObject<UPlayerEquipmentUseComponent>(User, TEXT("FuelTestEquipmentUse"));
	User->SetRootComponent(Camera);
	User->AddInstanceComponent(Camera);
	User->AddInstanceComponent(Carry);
	User->AddInstanceComponent(Interaction);
	User->AddInstanceComponent(EquipmentUse);
	Camera->RegisterComponent();
	Carry->RegisterComponent();
	Interaction->RegisterComponent();
	EquipmentUse->RegisterComponent();
	Carry->ConfigureHeldAnchor(Camera);
	Carry->ConfigureEquipmentUse(EquipmentUse);
	Interaction->Configure(Camera, Carry);
	Interaction->ConfigureEquipmentUse(EquipmentUse);
	EquipmentUse->Configure(Camera, Carry, Interaction, nullptr);

	AUtilityFuelSupplyActor* Supply = World->SpawnActor<AUtilityFuelSupplyActor>(FVector(100.0f, 0.0f, 0.0f), FRotator::ZeroRotator);
	Supply->GetSupplyMesh()->SetStaticMesh(CubeMesh);
	Supply->GetSupplyMesh()->SetWorldScale3D(FVector(0.15f));
	Supply->GetSupplyMesh()->UpdateBounds();
	AUtilityShovelActor* Shovel = World->SpawnActor<AUtilityShovelActor>();
	FText FailureReason;
	TestFalse(TEXT("Shovel authoring rejects missing visual meshes"), Shovel->HasValidAuthoring(FailureReason));
	TestTrue(TEXT("Shovel test fixture assigns both authored meshes"), SetShovelTestMeshes(Shovel, CubeMesh));
	UStaticMeshComponent* ShovelWorldMesh = Shovel->GetWorldMesh();
	UStaticMeshComponent* ShovelLoadVisual = FindNamedMeshComponent(Shovel, TEXT("LoadVisual"));
	if (!TestNotNull(TEXT("Shovel physical root component exists"), ShovelWorldMesh)
		|| !TestNotNull(TEXT("Shovel load visual component exists"), ShovelLoadVisual))
	{
		return false;
	}
	ShovelWorldMesh->SetWorldScale3D(FVector(0.15f));
	ShovelWorldMesh->UpdateBounds();
	TestTrue(TEXT("Shovel authoring accepts valid visual and collision contracts"), Shovel->HasValidAuthoring(FailureReason));
	FStructProperty* HeldTransformProperty = FindFProperty<FStructProperty>(
		AUtilityShovelActor::StaticClass(), TEXT("HeldTransform"));
	FTransform* HeldTransform = HeldTransformProperty
		? HeldTransformProperty->ContainerPtrToValuePtr<FTransform>(Shovel) : nullptr;
	if (TestTrue(TEXT("Held transform authoring property exists"), HeldTransform != nullptr))
	{
		HeldTransform->SetScale3D(FVector(2.0f));
		TestFalse(TEXT("Shovel authoring rejects non-unit held scale"), Shovel->HasValidAuthoring(FailureReason));
		HeldTransform->SetScale3D(FVector::OneVector);
	}
	UStaticMesh* AuthoredWorldMesh = ShovelWorldMesh->GetStaticMesh();
	UStaticMesh* AuthoredLoadMesh = ShovelLoadVisual->GetStaticMesh();
	ShovelWorldMesh->SetStaticMesh(nullptr);
	TestFalse(TEXT("Shovel authoring rejects a missing physical root mesh"), Shovel->HasValidAuthoring(FailureReason));
	ShovelWorldMesh->SetStaticMesh(AuthoredWorldMesh);
	ShovelLoadVisual->SetStaticMesh(nullptr);
	TestFalse(TEXT("Shovel authoring rejects a missing load visual mesh"), Shovel->HasValidAuthoring(FailureReason));
	ShovelLoadVisual->SetStaticMesh(AuthoredLoadMesh);
	ShovelLoadVisual->DetachFromComponent(FDetachmentTransformRules::KeepRelativeTransform);
	TestFalse(TEXT("Shovel authoring rejects a detached load visual"), Shovel->HasValidAuthoring(FailureReason));
	ShovelLoadVisual->AttachToComponent(Shovel->GetWorldMesh(), FAttachmentTransformRules::KeepRelativeTransform);
	TestTrue(TEXT("Restored shovel visual hierarchy passes validation"), Shovel->HasValidAuthoring(FailureReason));
	ShovelWorldMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	TestFalse(TEXT("Unheld shovel authoring rejects a disabled physical root"), Shovel->HasValidAuthoring(FailureReason));
	ShovelWorldMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	ShovelWorldMesh->SetUseCCD(false);
	TestFalse(TEXT("Shovel authoring rejects disabled root CCD"), Shovel->HasValidAuthoring(FailureReason));
	ShovelWorldMesh->SetUseCCD(true);
	ShovelWorldMesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
	TestFalse(TEXT("Shovel authoring rejects a root that blocks Pawns"), Shovel->HasValidAuthoring(FailureReason));
	ShovelWorldMesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
	ShovelWorldMesh->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Ignore);
	TestFalse(TEXT("Shovel authoring rejects a root that ignores world collision"), Shovel->HasValidAuthoring(FailureReason));
	ShovelWorldMesh->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
	ShovelLoadVisual->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	TestFalse(TEXT("Shovel authoring rejects load visuals with collision"), Shovel->HasValidAuthoring(FailureReason));
	ShovelLoadVisual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ShovelLoadVisual->SetCanEverAffectNavigation(true);
	TestFalse(TEXT("Shovel authoring rejects load visuals that affect navigation"), Shovel->HasValidAuthoring(FailureReason));
	ShovelLoadVisual->SetCanEverAffectNavigation(false);
	TestTrue(TEXT("The empty hand takes the shovel"), Carry->TryTakePhysicalObject(Shovel, FailureReason));

	AStaticMeshActor* Blocker = World->SpawnActor<AStaticMeshActor>(
		AStaticMeshActor::StaticClass(), FTransform(FRotator::ZeroRotator, FVector(50.0f, 0.0f, 0.0f)));
	UStaticMeshComponent* BlockerMesh = Blocker ? Blocker->GetStaticMeshComponent() : nullptr;
	if (!TestNotNull(TEXT("Visibility blocker actor is created"), Blocker)
		|| !TestNotNull(TEXT("Visibility blocker has a registered root mesh"), BlockerMesh))
	{
		return false;
	}
	BlockerMesh->SetStaticMesh(CubeMesh);
	BlockerMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	BlockerMesh->SetCollisionResponseToAllChannels(ECR_Ignore);
	BlockerMesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	BlockerMesh->SetWorldScale3D(FVector(0.1f));
	BlockerMesh->UpdateBounds();
	FHitResult FocusHit;
	TestTrue(TEXT("The fresh interaction trace sees the blocker"), Interaction->GetCurrentFocusHit(FocusHit));
	TestTrue(FString::Printf(
		TEXT("The blocker is the first trace hit (actual actor: %s, component: %s, distance: %.1f cm)"),
		*GetNameSafe(FocusHit.GetActor()),
		*GetNameSafe(FocusHit.GetComponent()),
		FocusHit.Distance), FocusHit.GetActor() == Blocker);
	TestFalse(TEXT("An occluded supply cannot load the shovel"), EquipmentUse->BeginEquipmentUse().bSucceeded);
	TestTrue(TEXT("An occluded attempt preserves an empty shovel"), Shovel->IsLoadEmpty());
	BlockerMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Blocker->SetActorLocation(FVector(1000.0f, 0.0f, 0.0f), false, nullptr, ETeleportType::TeleportPhysics);
	Supply->SetActorLocation(FVector(600.0f, 0.0f, 0.0f));
	const bool bOutOfRangeHit = Interaction->GetCurrentFocusHit(FocusHit);
	TestFalse(FString::Printf(
		TEXT("A target beyond the shared interaction range is not traced (actor=%s, component=%s, distance=%.1f cm)"),
		*GetNameSafe(FocusHit.GetActor()), *GetNameSafe(FocusHit.GetComponent()), FocusHit.Distance), bOutOfRangeHit);
	TestFalse(TEXT("An out-of-range supply cannot load the shovel"), EquipmentUse->BeginEquipmentUse().bSucceeded);
	Supply->SetActorLocation(FVector(100.0f, 0.0f, 0.0f));
	TestTrue(TEXT("A visible supply can be scooped"), EquipmentUse->BeginEquipmentUse().bSucceeded);
	EquipmentUse->EndEquipmentUse();
	TestTrue(TEXT("One scoop copies exactly 25 coal points"),
		Shovel->GetFuelLoad().Kind == EUtilityFuelKind::Coal
		&& FMath::IsNearlyEqual(Shovel->GetFuelLoad().Points, 25.0f));
	TestFalse(TEXT("A loaded shovel cannot scoop a second batch"), EquipmentUse->BeginEquipmentUse().bSucceeded);

	ABathWaterBoilerFacilityActor* Boiler = World->SpawnActor<ABathWaterBoilerFacilityActor>(FVector(150.0f, 0.0f, 0.0f), FRotator::ZeroRotator);
	TestFalse(TEXT("Boiler authoring rejects missing gauge meshes"), Boiler->HasValidUtilityAuthoring(FailureReason));
	TestTrue(TEXT("Boiler test fixture assigns intake and gauge meshes"), SetBoilerTestMeshes(Boiler, CubeMesh));
	Boiler->GetFuelIntake()->SetWorldScale3D(FVector(0.15f));
	TestTrue(TEXT("Boiler native components pass labor authoring validation"), Boiler->HasValidUtilityAuthoring(FailureReason));
	UStaticMeshComponent* BoilerGaugeFace = FindNamedMeshComponent(Boiler, TEXT("GaugeFace"));
	UStaticMeshComponent* BoilerGaugeNeedle = FindNamedMeshComponent(Boiler, TEXT("GaugeNeedleMesh"));
	USceneComponent* BoilerGaugePivot = BoilerGaugeNeedle ? BoilerGaugeNeedle->GetAttachParent() : nullptr;
	USceneComponent* BoilerGaugeRoot = BoilerGaugePivot ? BoilerGaugePivot->GetAttachParent() : nullptr;
	if (TestNotNull(TEXT("Boiler gauge face exists"), BoilerGaugeFace)
		&& TestNotNull(TEXT("Boiler gauge needle exists"), BoilerGaugeNeedle)
		&& TestNotNull(TEXT("Boiler gauge pivot exists"), BoilerGaugePivot)
		&& TestNotNull(TEXT("Boiler gauge root exists"), BoilerGaugeRoot))
	{
		BoilerGaugeFace->SetStaticMesh(nullptr);
		TestFalse(TEXT("Boiler authoring rejects a missing gauge face mesh"), Boiler->HasValidUtilityAuthoring(FailureReason));
		BoilerGaugeFace->SetStaticMesh(CubeMesh);
		BoilerGaugeNeedle->SetStaticMesh(nullptr);
		TestFalse(TEXT("Boiler authoring rejects a missing gauge needle mesh"), Boiler->HasValidUtilityAuthoring(FailureReason));
		BoilerGaugeNeedle->SetStaticMesh(CubeMesh);
		BoilerGaugeFace->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		TestFalse(TEXT("Boiler authoring rejects gauge face collision"), Boiler->HasValidUtilityAuthoring(FailureReason));
		BoilerGaugeFace->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		BoilerGaugeFace->SetCanEverAffectNavigation(true);
		TestFalse(TEXT("Boiler authoring rejects gauge face navigation influence"), Boiler->HasValidUtilityAuthoring(FailureReason));
		BoilerGaugeFace->SetCanEverAffectNavigation(false);
		BoilerGaugeFace->DetachFromComponent(FDetachmentTransformRules::KeepRelativeTransform);
		TestFalse(TEXT("Boiler authoring rejects a detached gauge face"), Boiler->HasValidUtilityAuthoring(FailureReason));
		BoilerGaugeFace->AttachToComponent(BoilerGaugeRoot, FAttachmentTransformRules::KeepRelativeTransform);
		BoilerGaugeNeedle->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		TestFalse(TEXT("Boiler authoring rejects gauge needle collision"), Boiler->HasValidUtilityAuthoring(FailureReason));
		BoilerGaugeNeedle->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		BoilerGaugeNeedle->SetCanEverAffectNavigation(true);
		TestFalse(TEXT("Boiler authoring rejects gauge needle navigation influence"), Boiler->HasValidUtilityAuthoring(FailureReason));
		BoilerGaugeNeedle->SetCanEverAffectNavigation(false);
		BoilerGaugeNeedle->DetachFromComponent(FDetachmentTransformRules::KeepRelativeTransform);
		TestFalse(TEXT("Boiler authoring rejects a detached gauge needle"), Boiler->HasValidUtilityAuthoring(FailureReason));
		BoilerGaugeNeedle->AttachToComponent(BoilerGaugePivot, FAttachmentTransformRules::KeepRelativeTransform);
		BoilerGaugePivot->DetachFromComponent(FDetachmentTransformRules::KeepRelativeTransform);
		TestFalse(TEXT("Boiler authoring rejects a detached gauge pivot"), Boiler->HasValidUtilityAuthoring(FailureReason));
		BoilerGaugePivot->AttachToComponent(BoilerGaugeRoot, FAttachmentTransformRules::KeepRelativeTransform);
	}
	TestTrue(TEXT("Restored boiler gauge hierarchy passes validation"), Boiler->HasValidUtilityAuthoring(FailureReason));
	TestTrue(TEXT("Recovered charge imports before operation starts"), Boiler->GetOperation()->ImportOperationState(90.0f, FailureReason));
	Boiler->GetFacilityPlacementComponent()->SetPlacedDomainActive(true);
	UBathWaterOperationsSubsystem* Operations = World->GetSubsystem<UBathWaterOperationsSubsystem>();
	TestTrue(TEXT("Test boiler registers as a provider"), Operations->RegisterProvider(Boiler->GetCapacityComponent(), false));
	TestTrue(TEXT("Installed boiler clock starts"), Boiler->GetOperation()->StartPlacedClock(false));
	Supply->SetActorLocation(FVector(500.0f, 0.0f, 0.0f));
	TestTrue(TEXT("The visible native intake is the exact target hit"),
		Interaction->GetCurrentFocusHit(FocusHit) && FocusHit.GetActor() == Boiler
		&& FocusHit.GetComponent() == Boiler->GetFuelIntake());
	TestTrue(TEXT("A 25 point load enters a 90 point boiler"), EquipmentUse->BeginEquipmentUse().bSucceeded);
	EquipmentUse->EndEquipmentUse();
	TestTrue(TEXT("Partial remaining space is capped at 100 and the whole load is consumed"),
		FMath::IsNearlyEqual(Boiler->GetOperation()->GetRemainingPoints(), 100.0f)
		&& Shovel->IsLoadEmpty());

	Boiler->SetActorLocation(FVector(500.0f, 0.0f, 0.0f));
	Supply->SetActorLocation(FVector(100.0f, 0.0f, 0.0f));
	TestTrue(TEXT("A second coal batch loads after the first was consumed"), EquipmentUse->BeginEquipmentUse().bSucceeded);
	EquipmentUse->EndEquipmentUse();
	Boiler->SetActorLocation(FVector(150.0f, 0.0f, 0.0f));
	Supply->SetActorLocation(FVector(500.0f, 0.0f, 0.0f));
	TestFalse(TEXT("A full boiler rejects insertion"), EquipmentUse->BeginEquipmentUse().bSucceeded);
	TestTrue(TEXT("Full-boiler refusal keeps all shovel fuel"),
		Shovel->GetFuelLoad().Kind == EUtilityFuelKind::Coal
		&& FMath::IsNearlyEqual(Shovel->GetFuelLoad().Points, 25.0f));
	Boiler->SetActorLocation(FVector(500.0f, 0.0f, 0.0f));
	Supply->SetActorLocation(FVector(100.0f, 0.0f, 0.0f));
	TestTrue(TEXT("F returns a matching load through the focused supply"),
		Interaction->TrySecondaryInteract().bSucceeded);
	TestTrue(TEXT("Fuel return clears the same shovel load"), Shovel->IsLoadEmpty());

	UFacilityPlacementDefinition* RaceBoilerDefinition = NewObject<UFacilityPlacementDefinition>();
	RaceBoilerDefinition->StableId = TEXT("UtilityLaborRaceBoiler");
	RaceBoilerDefinition->FacilityTags.AddTag(FGameplayTag::RequestGameplayTag(TEXT("Facility.Placeable")));
	RaceBoilerDefinition->PlacedFacilityClass = ABathWaterBoilerFacilityActor::StaticClass();
	RaceBoilerDefinition->RecoveryItemClass = APlaceableFacilityItemActor::StaticClass();
	RaceBoilerDefinition->RecoveryItemMesh = CubeMesh;
	TestTrue(TEXT("Race boiler placement definition validates"), RaceBoilerDefinition->ValidateRuntime(FailureReason));
	auto SpawnRaceBoiler = [&](const FVector& Location)
	{
		ABathWaterBoilerFacilityActor* RaceBoiler = World->SpawnActorDeferred<ABathWaterBoilerFacilityActor>(
			ABathWaterBoilerFacilityActor::StaticClass(),
			FTransform(FRotator::ZeroRotator, Location),
			nullptr,
			nullptr,
			ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
		if (!RaceBoiler || !SetBoilerTestMeshes(RaceBoiler, CubeMesh))
		{
			return static_cast<ABathWaterBoilerFacilityActor*>(nullptr);
		}
		RaceBoiler->GetFuelIntake()->SetWorldScale3D(FVector(0.15f));
		if (!RaceBoiler->GetFacilityPlacementComponent()->PrepareForStagedPlacement(*RaceBoilerDefinition, FailureReason))
		{
			RaceBoiler->Destroy();
			return static_cast<ABathWaterBoilerFacilityActor*>(nullptr);
		}
		if (!RaceBoiler->GetOperation()->ImportOperationState(5.0f, FailureReason))
		{
			RaceBoiler->Destroy();
			return static_cast<ABathWaterBoilerFacilityActor*>(nullptr);
		}
		RaceBoiler->FinishSpawning(FTransform(FRotator::ZeroRotator, Location));
		BeginActorPlayIfNeeded(RaceBoiler);
		if (!RaceBoiler->GetFacilityPlacementComponent()->FinalizeStagedPlacementCollisionSnapshot(FailureReason)
			|| !RaceBoiler->StagePlacedDomainRegistration(FailureReason))
		{
			RaceBoiler->RollbackPlacedDomainRegistration();
			RaceBoiler->Destroy();
			return static_cast<ABathWaterBoilerFacilityActor*>(nullptr);
		}
		if (!RaceBoiler->GetFacilityPlacementComponent()->CommitStagedPlacement(FailureReason))
		{
			RaceBoiler->RollbackPlacedDomainRegistration();
			RaceBoiler->Destroy();
			return static_cast<ABathWaterBoilerFacilityActor*>(nullptr);
		}
		RaceBoiler->PublishPlacedDomainRegistration();
		return RaceBoiler;
	};

	ABathWaterBoilerFacilityActor* TickFirstBoiler = SpawnRaceBoiler(FVector(150.0f, 0.0f, 0.0f));
	if (!TestNotNull(TEXT("Tick-first race boiler installs"), TickFirstBoiler))
	{
		return false;
	}
	TArray<bool> TickFirstEdges;
	int32 TickFirstOperationsPublications = 0;
	const FDelegateHandle TickFirstOperatingHandle = TickFirstBoiler->GetOperation()->OnOperatingChanged.AddLambda(
		[&TickFirstEdges](const bool bOperating) { TickFirstEdges.Add(bOperating); });
	const FDelegateHandle TickFirstPublicationHandle = Operations->OnOperationsChanged.AddLambda(
		[&TickFirstOperationsPublications]() { ++TickFirstOperationsPublications; });
	TickWorldForDuration(World, 5.0);
	TestTrue(TEXT("Exhaustion Tick first publishes the zero-capacity edge"),
		FMath::IsNearlyZero(TickFirstBoiler->GetOperation()->GetRemainingPoints())
		&& TickFirstEdges.Num() == 1 && !TickFirstEdges[0]
		&& TickFirstOperationsPublications == 1);
	Supply->SetActorLocation(FVector(100.0f, 0.0f, 0.0f));
	TickFirstBoiler->SetActorLocation(FVector(500.0f, 0.0f, 0.0f));
	TestTrue(TEXT("Fuel can be scooped for the exhausted boiler"), EquipmentUse->BeginEquipmentUse().bSucceeded);
	EquipmentUse->EndEquipmentUse();
	Supply->SetActorLocation(FVector(500.0f, 0.0f, 0.0f));
	TickFirstBoiler->SetActorLocation(FVector(150.0f, 0.0f, 0.0f));
	TestTrue(TEXT("Recovery hold starts for the exhausted boiler"), TickFirstBoiler->TryBeginFacilityRecoveryHold(FailureReason));
	TestFalse(TEXT("Actual equipment-use path rejects fuel during recovery Hold"), EquipmentUse->BeginEquipmentUse().bSucceeded);
	TestTrue(TEXT("Rejected Hold insertion preserves shovel coal"),
		Shovel->GetFuelLoad().Kind == EUtilityFuelKind::Coal
		&& FMath::IsNearlyEqual(Shovel->GetFuelLoad().Points, 25.0f));
	TickFirstBoiler->CancelFacilityRecoveryHold();
	TestTrue(TEXT("Insertion after the exhaustion Tick succeeds"), EquipmentUse->BeginEquipmentUse().bSucceeded);
	EquipmentUse->EndEquipmentUse();
	TestTrue(TEXT("Tick-first insertion publishes the positive edge exactly once"),
		TickFirstEdges.Num() == 2 && TickFirstEdges[1]
		&& TickFirstOperationsPublications == 2);
	TickFirstBoiler->GetOperation()->OnOperatingChanged.Remove(TickFirstOperatingHandle);
	Operations->OnOperationsChanged.Remove(TickFirstPublicationHandle);

	ABathWaterBoilerFacilityActor* InsertFirstBoiler = SpawnRaceBoiler(FVector(500.0f, 0.0f, 0.0f));
	if (!TestNotNull(TEXT("Insert-first race boiler installs"), InsertFirstBoiler))
	{
		return false;
	}
	Supply->SetActorLocation(FVector(100.0f, 0.0f, 0.0f));
	TestTrue(TEXT("Fuel can be scooped for the insert-first boiler"), EquipmentUse->BeginEquipmentUse().bSucceeded);
	EquipmentUse->EndEquipmentUse();
	Supply->SetActorLocation(FVector(500.0f, 0.0f, 0.0f));
	InsertFirstBoiler->SetActorLocation(FVector(150.0f, 0.0f, 0.0f));
	InsertFirstBoiler->GetOperation()->SetComponentTickEnabled(false);
	TArray<bool> InsertFirstEdges;
	int32 InsertFirstOperationsPublications = 0;
	int32 InsertFirstPointPublications = 0;
	const FDelegateHandle InsertFirstOperatingHandle = InsertFirstBoiler->GetOperation()->OnOperatingChanged.AddLambda(
		[&InsertFirstEdges](const bool bOperating) { InsertFirstEdges.Add(bOperating); });
	const FDelegateHandle InsertFirstPointHandle = InsertFirstBoiler->GetOperation()->OnOperationChanged.AddLambda(
		[&InsertFirstPointPublications]() { ++InsertFirstPointPublications; });
	const FDelegateHandle InsertFirstPublicationHandle = Operations->OnOperationsChanged.AddLambda(
		[&InsertFirstOperationsPublications]() { ++InsertFirstOperationsPublications; });
	TickWorldForDuration(World, 5.0);
	TestTrue(TEXT("Projected exhaustion is visible before its disabled Tick publishes"),
		FMath::IsNearlyZero(InsertFirstBoiler->GetOperation()->GetRemainingPoints())
		&& !InsertFirstBoiler->GetOperation()->IsProvidingCapacity()
		&& InsertFirstEdges.IsEmpty()
		&& InsertFirstOperationsPublications == 0);
	const float ActiveBeforeInsert = Operations->GetCapacitySnapshot(EBathWaterCapacityKind::Heating).ActivePoints;
	APawn* ReentrantUser = World->SpawnActor<APawn>();
	UCameraComponent* ReentrantCamera = NewObject<UCameraComponent>(ReentrantUser, TEXT("ReentrantFuelCamera"));
	UPlayerCarryComponent* ReentrantCarry = NewObject<UPlayerCarryComponent>(ReentrantUser, TEXT("ReentrantFuelCarry"));
	UPlayerInteractionComponent* ReentrantInteraction = NewObject<UPlayerInteractionComponent>(ReentrantUser, TEXT("ReentrantFuelInteraction"));
	UPlayerEquipmentUseComponent* ReentrantEquipmentUse = NewObject<UPlayerEquipmentUseComponent>(ReentrantUser, TEXT("ReentrantFuelEquipmentUse"));
	ReentrantUser->SetRootComponent(ReentrantCamera);
	ReentrantUser->AddInstanceComponent(ReentrantCamera);
	ReentrantUser->AddInstanceComponent(ReentrantCarry);
	ReentrantUser->AddInstanceComponent(ReentrantInteraction);
	ReentrantUser->AddInstanceComponent(ReentrantEquipmentUse);
	ReentrantCamera->RegisterComponent();
	ReentrantCarry->RegisterComponent();
	ReentrantInteraction->RegisterComponent();
	ReentrantEquipmentUse->RegisterComponent();
	ReentrantCarry->ConfigureHeldAnchor(ReentrantCamera);
	ReentrantCarry->ConfigureEquipmentUse(ReentrantEquipmentUse);
	ReentrantInteraction->Configure(ReentrantCamera, ReentrantCarry);
	ReentrantInteraction->ConfigureEquipmentUse(ReentrantEquipmentUse);
	ReentrantEquipmentUse->Configure(ReentrantCamera, ReentrantCarry, ReentrantInteraction, nullptr);
	AUtilityShovelActor* ReentrantShovel = World->SpawnActor<AUtilityShovelActor>();
	TestTrue(TEXT("Reentrant shovel has valid authoring"), SetShovelTestMeshes(ReentrantShovel, CubeMesh));
	TestTrue(TEXT("Second user takes the reentrant-test shovel"), ReentrantCarry->TryTakePhysicalObject(ReentrantShovel, FailureReason));
	Supply->SetActorLocation(FVector(100.0f, 0.0f, 0.0f));
	TestTrue(TEXT("Second user loads an independent batch for reentrancy"), ReentrantEquipmentUse->BeginEquipmentUse().bSucceeded);
	ReentrantEquipmentUse->EndEquipmentUse();
	Supply->SetActorLocation(FVector(500.0f, 0.0f, 0.0f));
	Boiler->SetActorLocation(FVector(800.0f, 500.0f, 0.0f));
	TickFirstBoiler->SetActorLocation(FVector(800.0f, 0.0f, 0.0f));
	InsertFirstBoiler->SetActorLocation(FVector(150.0f, 0.0f, 0.0f));
	FHeldEquipmentUseContext ReentrantContext;
	ReentrantContext.User = ReentrantUser;
	ReentrantContext.Equipment = ReentrantShovel;
	ReentrantContext.CarryComponent = ReentrantCarry;
	ReentrantContext.InteractionComponent = ReentrantInteraction;
	ReentrantContext.Camera = ReentrantCamera;
	TestTrue(TEXT("Reentrant context focuses the boiler intake"),
		ReentrantInteraction->GetCurrentFocusHit(ReentrantContext.FocusHit)
		&& ReentrantContext.FocusHit.GetActor() == InsertFirstBoiler
		&& ReentrantContext.FocusHit.GetComponent() == InsertFirstBoiler->GetFuelIntake());
	TStrongObjectPtr<UUtilityLaborFuelChangedAutomationProbe> FuelChangedProbe(
		NewObject<UUtilityLaborFuelChangedAutomationProbe>(World));
	if (!TestNotNull(TEXT("Fuel delegate observer is created"), FuelChangedProbe.Get()))
	{
		return false;
	}
	FuelChangedProbe->Bind(
		Shovel,
		ReentrantShovel,
		InsertFirstBoiler->GetOperation(),
		InsertFirstBoiler->GetFuelIntake(),
		ReentrantContext,
		25.0f);
	int32 OperationChangedCallbacks = 0;
	bool bOperationCallbackSawCompleteState = false;
	bool bReentrantInsertRejectedByGuard = false;
	const FDelegateHandle ReentrantOperationHandle = InsertFirstBoiler->GetOperation()->OnOperationChanged.AddLambda([&]()
	{
		++OperationChangedCallbacks;
		bOperationCallbackSawCompleteState = Shovel->IsLoadEmpty()
			&& ReentrantShovel->GetFuelLoad().Points == 25.0f
			&& FMath::IsNearlyEqual(InsertFirstBoiler->GetOperation()->GetRemainingPoints(), 25.0f);
		const FUtilityFuelResult NestedResult = FUtilityFuelTransaction::Insert(
			*ReentrantShovel, *InsertFirstBoiler->GetFuelIntake(), ReentrantContext);
		bReentrantInsertRejectedByGuard = !NestedResult.bSucceeded
			&& NestedResult.Failure == EUtilityFuelFailure::TransactionBusy;
	});
	TestTrue(TEXT("Fuel insertion before the exhaustion Tick succeeds"), EquipmentUse->BeginEquipmentUse().bSucceeded);
	EquipmentUse->EndEquipmentUse();
	FuelChangedProbe->Unbind();
	InsertFirstBoiler->GetOperation()->OnOperationChanged.Remove(ReentrantOperationHandle);
	const float ActiveAfterInsert = Operations->GetCapacitySnapshot(EBathWaterCapacityKind::Heating).ActivePoints;
	TestTrue(TEXT("Projected zero-to-positive insertion publishes one active edge"),
		InsertFirstEdges.Num() == 1 && InsertFirstEdges[0]
		&& InsertFirstOperationsPublications == 1
		&& InsertFirstPointPublications == 1
		&& OperationChangedCallbacks == 1
		&& bOperationCallbackSawCompleteState
		&& bReentrantInsertRejectedByGuard
		&& FuelChangedProbe->CallbackCount == 1
		&& FuelChangedProbe->bSawCompleteCommittedState
		&& FuelChangedProbe->bReentrantInsertRejectedByGuard
		&& FMath::IsNearlyEqual(ReentrantShovel->GetFuelLoad().Points, 25.0f)
		&& FMath::IsNearlyEqual(ActiveAfterInsert - ActiveBeforeInsert, 100.0f)
		&& FMath::IsNearlyEqual(InsertFirstBoiler->GetOperation()->GetRemainingPoints(), 25.0f));
	World->Tick(LEVELTICK_All, 0.1f);
	TestTrue(TEXT("A later normal Tick does not duplicate the active edge"),
		InsertFirstEdges.Num() == 1 && InsertFirstOperationsPublications == 1);
	InsertFirstBoiler->GetOperation()->OnOperatingChanged.Remove(InsertFirstOperatingHandle);
	InsertFirstBoiler->GetOperation()->OnOperationChanged.Remove(InsertFirstPointHandle);
	Operations->OnOperationsChanged.Remove(InsertFirstPublicationHandle);

	Supply->SetActorLocation(FVector(100.0f, 0.0f, 0.0f));
	TickFirstBoiler->SetActorLocation(FVector(800.0f, 0.0f, 0.0f));
	InsertFirstBoiler->SetActorLocation(FVector(800.0f, 500.0f, 0.0f));
	TestTrue(TEXT("Final load is scooped through the equipment-use action"), EquipmentUse->BeginEquipmentUse().bSucceeded);
	EquipmentUse->EndEquipmentUse();
	Supply->SetActorLocation(FVector(500.0f, 0.0f, 0.0f));
	TestTrue(TEXT("G drop action releases the loaded shovel"),
		Interaction->TryDropCarry(FVector::ForwardVector).bSucceeded);
	TestTrue(TEXT("G drop preserves the shovel's loaded points"),
		Shovel->GetFuelLoad().Kind == EUtilityFuelKind::Coal
		&& FMath::IsNearlyEqual(Shovel->GetFuelLoad().Points, 25.0f));
	TestTrue(TEXT("Dropped loaded shovel can be picked up again"), Carry->TryTakePhysicalObject(Shovel, FailureReason));

	APhysicalCarryFixedSlotActor* ShovelSlot = World->SpawnActorDeferred<APhysicalCarryFixedSlotActor>(
		APhysicalCarryFixedSlotActor::StaticClass(),
		FTransform(FRotator::ZeroRotator, FVector(100.0f, 0.0f, 0.0f)),
		nullptr,
		nullptr,
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (!TestNotNull(TEXT("Shovel slot is spawned before runtime assignment"), ShovelSlot))
	{
		return false;
	}
	FObjectPropertyBase* AssignedItemProperty = CastField<FObjectPropertyBase>(
		FindFProperty<FProperty>(APhysicalCarryFixedSlotActor::StaticClass(), TEXT("AssignedItem")));
	FBoolProperty* StartOccupiedProperty = CastField<FBoolProperty>(
		FindFProperty<FProperty>(APhysicalCarryFixedSlotActor::StaticClass(), TEXT("bStartOccupied")));
	if (!TestNotNull(TEXT("Slot assignment property exists"), AssignedItemProperty)
		|| !TestNotNull(TEXT("Slot occupancy property exists"), StartOccupiedProperty))
	{
		return false;
	}
	AssignedItemProperty->SetObjectPropertyValue_InContainer(ShovelSlot, Shovel);
	StartOccupiedProperty->SetPropertyValue_InContainer(ShovelSlot, false);
	ShovelSlot->FinishSpawning(FTransform(FRotator::ZeroRotator, FVector(100.0f, 0.0f, 0.0f)));
	BeginActorPlayIfNeeded(ShovelSlot);
	TestTrue(TEXT("Runtime slot binds the assigned shovel"), Shovel->GetAssignedPhysicalCarryFixedSlot() == ShovelSlot);
	TestTrue(TEXT("Interaction stores the loaded shovel in its assigned slot"), Interaction->TryInteract().bSucceeded);
	TestTrue(TEXT("Slot store preserves the load and occupancy"), ShovelSlot->IsOccupied()
		&& Shovel->GetFuelLoad().Kind == EUtilityFuelKind::Coal
		&& FMath::IsNearlyEqual(Shovel->GetFuelLoad().Points, 25.0f));
	TestTrue(TEXT("Interaction takes the same loaded shovel back from its slot"), Interaction->TryInteract().bSucceeded);
	TestTrue(TEXT("Slot take preserves the load in hand"), Carry->GetHeldObject() == Shovel
		&& Shovel->GetFuelLoad().Kind == EUtilityFuelKind::Coal
		&& FMath::IsNearlyEqual(Shovel->GetFuelLoad().Points, 25.0f));
	return true;
}


#endif // WITH_DEV_AUTOMATION_TESTS
