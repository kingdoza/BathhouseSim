#if WITH_DEV_AUTOMATION_TESTS
#include "Tests/UtilityLaborAutomationTestSupport.h"
#include "Facility/BathWaterUtilityPlacementInstanceData.h"
#include "Placement/FacilityActorConversionTransaction.h"
#include "Utility/BathWaterBoilerFacilityActor.h"
#include "Utility/BathWaterCirculatorFacilityActor.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUtilityLaborRecoveryPayloadTest,
	"BathhouseSim.Utility.Labor.RecoveryPayloadAndRollback",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUtilityLaborRecoveryPayloadTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FScopedUtilityLaborWorld Scope(TEXT("UtilityLaborRecoveryWorld"));
	UWorld* World = Scope.Get();
	if (!TestNotNull(TEXT("Automation world exists"), World))
	{
		return false;
	}
	UStaticMesh* CubeMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (!TestNotNull(TEXT("Engine cube recovery mesh exists"), CubeMesh))
	{
		return false;
	}

	UFacilityPlacementDefinition* Definition = NewObject<UFacilityPlacementDefinition>();
	Definition->StableId = TEXT("UtilityLaborRecoveryAutomation");
	Definition->FacilityTags.AddTag(FGameplayTag::RequestGameplayTag(TEXT("Facility.Placeable")));
	Definition->PlacedFacilityClass = AUtilityLaborBoilerAutomationActor::StaticClass();
	Definition->RecoveryItemClass = APlaceableFacilityItemActor::StaticClass();
	Definition->RecoveryItemMesh = CubeMesh;
	FText FailureReason;
	TestTrue(TEXT("Boiler placement definition validates"), Definition->ValidateRuntime(FailureReason));

	auto SpawnBoiler = [&](const float InitialPoints, const FVector& Location)
	{
		ABathWaterBoilerFacilityActor* Boiler = World->SpawnActorDeferred<AUtilityLaborBoilerAutomationActor>(
			AUtilityLaborBoilerAutomationActor::StaticClass(),
			FTransform(FRotator::ZeroRotator, Location),
			nullptr,
			nullptr,
			ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
		if (!Boiler)
		{
			return static_cast<ABathWaterBoilerFacilityActor*>(nullptr);
		}
		UBathWaterUtilityCapacityComponent* Capacity = Boiler->GetCapacityComponent();
		Capacity->RestoreCapacity(EBathWaterCapacityKind::Heating, 100.0f);
		TestTrue(TEXT("Boiler fixture authors intake and gauge meshes"), SetBoilerTestMeshes(Boiler, CubeMesh));
		FText PreStageAuthoringReason;
		if (!Boiler->HasValidUtilityAuthoring(PreStageAuthoringReason))
		{
			AddError(FString::Printf(TEXT("Boiler authoring is invalid before placement staging: %s"), *PreStageAuthoringReason.ToString()));
		}
		if (!Boiler->GetFacilityPlacementComponent()->PrepareForStagedPlacement(*Definition, FailureReason))
		{
			AddError(FString::Printf(TEXT("Failed to prepare staged boiler: %s"), *FailureReason.ToString()));
			Boiler->Destroy();
			return static_cast<ABathWaterBoilerFacilityActor*>(nullptr);
		}
		if (!Boiler->GetOperation()->ImportOperationState(InitialPoints, FailureReason))
		{
			AddError(FString::Printf(TEXT("Failed to import staged boiler charge: %s"), *FailureReason.ToString()));
			Boiler->Destroy();
			return static_cast<ABathWaterBoilerFacilityActor*>(nullptr);
		}
		Boiler->FinishSpawning(FTransform(FRotator::ZeroRotator, Location));
		if (!Boiler->HasActorBegunPlay())
		{
			Boiler->DispatchBeginPlay();
		}
		if (!Boiler->GetFacilityPlacementComponent()->FinalizeStagedPlacementCollisionSnapshot(FailureReason))
		{
			AddError(FString::Printf(TEXT("Failed to finalize staged boiler collision snapshot: %s"), *FailureReason.ToString()));
			Boiler->Destroy();
			return static_cast<ABathWaterBoilerFacilityActor*>(nullptr);
		}
		FText AuthoringFailureReason;
		if (!Boiler->HasValidUtilityAuthoring(AuthoringFailureReason))
		{
			const UUtilityFuelIntakeVolumeComponent* Intake = Boiler->GetFuelIntakeVolume();
			AddError(FString::Printf(
				TEXT("Staged boiler authoring invalid before registration: %s (extent=%s, scale=%s, collision=%d, visibility=%d, navigation=%d)"),
				*AuthoringFailureReason.ToString(),
				Intake ? *Intake->GetUnscaledBoxExtent().ToString() : TEXT("<none>"),
				Intake ? *Intake->GetRelativeScale3D().ToString() : TEXT("<none>"),
				Intake ? static_cast<int32>(Intake->GetCollisionEnabled()) : -1,
				Intake ? static_cast<int32>(Intake->GetCollisionResponseToChannel(ECC_Visibility)) : -1,
				Intake ? Intake->CanEverAffectNavigation() : false));
		}
		if (!Boiler->StagePlacedDomainRegistration(FailureReason))
		{
			AddError(FString::Printf(TEXT("Failed to stage boiler domain registration: %s"), *FailureReason.ToString()));
			Boiler->RollbackPlacedDomainRegistration();
			Boiler->Destroy();
			return static_cast<ABathWaterBoilerFacilityActor*>(nullptr);
		}
		if (!Boiler->GetFacilityPlacementComponent()->CommitStagedPlacement(FailureReason))
		{
			AddError(FString::Printf(TEXT("Failed to commit staged boiler placement: %s"), *FailureReason.ToString()));
			Boiler->RollbackPlacedDomainRegistration();
			Boiler->Destroy();
			return static_cast<ABathWaterBoilerFacilityActor*>(nullptr);
		}
		Boiler->PublishPlacedDomainRegistration();
		return Boiler;
	};

	ABathWaterBoilerFacilityActor* Boiler = SpawnBoiler(25.0f, FVector(5000.0f, 0.0f, 100.0f));
	if (!TestNotNull(TEXT("Installed boiler is created from staged placement"), Boiler))
	{
		return false;
	}
	UBathWaterOperationsSubsystem* Operations = World->GetSubsystem<UBathWaterOperationsSubsystem>();
	TestTrue(TEXT("Installed boiler starts with its imported charge"),
		Boiler->GetOperation()->IsPlacedClockActive()
		&& FMath::IsNearlyEqual(Boiler->GetOperation()->GetRemainingPoints(), 25.0f));

	TestTrue(TEXT("Recovery hold begins from installed capacity"), Boiler->TryBeginFacilityRecoveryHold(FailureReason));
	TestTrue(TEXT("Recovery hold blocks fuel intake"), Boiler->GetOperation()->IsLaborBlocked());
	TickWorldForDuration(World, 5.0);
	TestTrue(TEXT("Charge continues to decay during recovery hold"),
		FMath::IsNearlyEqual(Boiler->GetOperation()->GetRemainingPoints(), 20.0f, 0.05f));
	Boiler->CancelFacilityRecoveryHold();
	TestTrue(TEXT("Cancel unblocks operation without rewinding charge"),
		!Boiler->GetOperation()->IsLaborBlocked()
		&& FMath::IsNearlyEqual(Boiler->GetOperation()->GetRemainingPoints(), 20.0f, 0.05f));

	TestTrue(TEXT("Second recovery hold begins"), Boiler->TryBeginFacilityRecoveryHold(FailureReason));
	TickWorldForDuration(World, 2.0);
	const float StagePoints = Boiler->GetOperation()->GetRemainingPoints();
	FFacilityActorConversionTransaction::SetTestFault(
		FFacilityActorConversionTransaction::ETestFault::RecoveryActivation);
	APlaceableFacilityItemActor* FailedItem =
		FFacilityActorConversionTransaction::RecoverFacilityToItem(*Boiler, FailureReason);
	FFacilityActorConversionTransaction::ClearTestFault();
	TestNull(TEXT("Injected activation failure leaves no recovery item"), FailedItem);
	TestTrue(TEXT("Rollback restarts at stage-time charge and clears the hold"),
		IsValid(Boiler) && Boiler->GetOperation()->IsPlacedClockActive()
		&& !Boiler->GetOperation()->IsLaborBlocked()
		&& FMath::IsNearlyEqual(Boiler->GetOperation()->GetRemainingPoints(), StagePoints, 0.05f));

	const float ExportedPoints = Boiler->GetOperation()->GetRemainingPoints();
	TestTrue(TEXT("Final recovery hold begins"), Boiler->TryBeginFacilityRecoveryHold(FailureReason));
	int32 RecoveryPublications = 0;
	const FDelegateHandle PublicationHandle = Operations->OnOperationsChanged.AddLambda(
		[&RecoveryPublications]() { ++RecoveryPublications; });
	APlaceableFacilityItemActor* Item =
		FFacilityActorConversionTransaction::RecoverFacilityToItem(*Boiler, FailureReason);
	TestNotNull(TEXT("Final recovery commits an item"), Item);
	TestTrue(TEXT("Recovery publishes the provider removal once"), RecoveryPublications == 1);
	const FFacilityPlacementPayload& RecoveredPayload = Item->GetPlacementPayload();
	const UBathWaterUtilityPlacementInstanceData* RecoveredData =
		Cast<UBathWaterUtilityPlacementInstanceData>(RecoveredPayload.InstanceData);
	TestTrue(TEXT("Recovery payload preserves a present operation state"), RecoveredData
		&& RecoveredData->bHasOperationState
		&& FMath::IsNearlyEqual(RecoveredData->RemainingOperationPoints, ExportedPoints, 0.05f));
	TickWorldForDuration(World, 30.0);
	TestTrue(TEXT("Packaged boiler charge does not decay"), RecoveredData
		&& FMath::IsNearlyEqual(RecoveredData->RemainingOperationPoints, ExportedPoints, 0.05f));
	Operations->OnOperationsChanged.Remove(PublicationHandle);

	ABathWaterBoilerFacilityActor* Staged = World->SpawnActorDeferred<AUtilityLaborBoilerAutomationActor>(
		AUtilityLaborBoilerAutomationActor::StaticClass(),
		FTransform(FRotator::ZeroRotator, FVector(7000.0f, 0.0f, 100.0f)),
		nullptr,
		nullptr,
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	TestTrue(TEXT("Reinstall fixture authors intake and gauge meshes"), SetBoilerTestMeshes(Staged, CubeMesh));
	TestTrue(TEXT("Reinstall fixture prepares staged placement"),
		Staged->GetFacilityPlacementComponent()->PrepareForStagedPlacement(*Definition, FailureReason));
	TestTrue(TEXT("Payload import succeeds before FinishSpawning"),
		Staged->ImportPlacementPayload(*Item, RecoveredPayload, FailureReason));
	Staged->FinishSpawning(FTransform(FRotator::ZeroRotator, FVector(7000.0f, 0.0f, 100.0f)));
	if (!Staged->HasActorBegunPlay())
	{
		Staged->DispatchBeginPlay();
	}
	TestTrue(TEXT("BeginPlay preserves imported staged charge with clock stopped"),
		Staged->GetOperation()->HasImportedOperationState()
		&& !Staged->GetOperation()->IsPlacedClockActive()
		&& FMath::IsNearlyEqual(Staged->GetOperation()->GetRemainingPoints(), ExportedPoints, 0.05f));
	TestTrue(TEXT("Staged placement captures collision state"),
		Staged->GetFacilityPlacementComponent()->FinalizeStagedPlacementCollisionSnapshot(FailureReason));
	TestTrue(TEXT("Staged provider starts only inside final domain registration"),
		Staged->StagePlacedDomainRegistration(FailureReason)
		&& Staged->GetOperation()->IsPlacedClockActive());
	Staged->RollbackPlacedDomainRegistration();
	Staged->Destroy();

	ABathWaterBoilerFacilityActor* LegacyStaged = World->SpawnActorDeferred<AUtilityLaborBoilerAutomationActor>(
		AUtilityLaborBoilerAutomationActor::StaticClass(),
		FTransform(FRotator::ZeroRotator, FVector(9000.0f, 0.0f, 100.0f)),
		nullptr,
		nullptr,
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	TestTrue(TEXT("Legacy fixture authors intake and gauge meshes"), SetBoilerTestMeshes(LegacyStaged, CubeMesh));
	LegacyStaged->GetFacilityPlacementComponent()->PrepareForStagedPlacement(*Definition, FailureReason);
	UBathWaterUtilityPlacementInstanceData* LegacyData =
		NewObject<UBathWaterUtilityPlacementInstanceData>(Item, TEXT("LegacyUtilityPayload"));
	LegacyData->CapacityKind = EBathWaterCapacityKind::Heating;
	LegacyData->CapacityPoints = 100.0f;
	LegacyData->bHasOperationState = false;
	FFacilityPlacementPayload LegacyPayload;
	LegacyPayload.Definition = Definition;
	LegacyPayload.InstanceData = LegacyData;
	TestTrue(TEXT("Payloads without operation state import as empty"),
		LegacyStaged->ImportPlacementPayload(*Item, LegacyPayload, FailureReason)
		&& FMath::IsNearlyZero(LegacyStaged->GetOperation()->GetRemainingPoints()));
	LegacyStaged->Destroy();

	ABathWaterBoilerFacilityActor* InvalidStaged = World->SpawnActorDeferred<AUtilityLaborBoilerAutomationActor>(
		AUtilityLaborBoilerAutomationActor::StaticClass(),
		FTransform(FRotator::ZeroRotator, FVector(11000.0f, 0.0f, 100.0f)),
		nullptr,
		nullptr,
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	TestTrue(TEXT("Invalid-payload fixture authors intake and gauge meshes"), SetBoilerTestMeshes(InvalidStaged, CubeMesh));
	InvalidStaged->GetFacilityPlacementComponent()->PrepareForStagedPlacement(*Definition, FailureReason);
	UBathWaterUtilityPlacementInstanceData* InvalidData =
		NewObject<UBathWaterUtilityPlacementInstanceData>(Item, TEXT("InvalidOperationPayload"));
	InvalidData->CapacityKind = EBathWaterCapacityKind::Heating;
	InvalidData->CapacityPoints = 100.0f;
	InvalidData->bHasOperationState = true;
	InvalidData->RemainingOperationPoints = std::numeric_limits<float>::quiet_NaN();
	FFacilityPlacementPayload InvalidPayload;
	InvalidPayload.Definition = Definition;
	InvalidPayload.InstanceData = InvalidData;
	TestFalse(TEXT("Invalid operation payload is rejected"),
		InvalidStaged->ImportPlacementPayload(*Item, InvalidPayload, FailureReason));
	TestTrue(TEXT("Rejected import preserves the original recovery item"), Item && Item->ValidatePlacementPayload(FailureReason));
	InvalidStaged->Destroy();

	ABathhouseBathFacilityActor* DemandBath = World->SpawnActor<ABathhouseBathFacilityActor>();
	if (!TestNotNull(TEXT("Capacity-deficit bath fixture exists"), DemandBath))
	{
		Item->Destroy();
		return false;
	}
	TestTrue(TEXT("Demand fixture bath registers"), Operations->RegisterBath(DemandBath->GetBathWaterCondition(), false));
	UFacilityPlacementDefinition* CirculatorDefinition = NewObject<UFacilityPlacementDefinition>();
	CirculatorDefinition->StableId = TEXT("UtilityLaborRecoveryCirculatorAutomation");
	CirculatorDefinition->FacilityTags.AddTag(FGameplayTag::RequestGameplayTag(TEXT("Facility.Placeable")));
	CirculatorDefinition->PlacedFacilityClass = ABathWaterCirculatorFacilityActor::StaticClass();
	CirculatorDefinition->RecoveryItemClass = APlaceableFacilityItemActor::StaticClass();
	CirculatorDefinition->RecoveryItemMesh = CubeMesh;
	ABathWaterCirculatorFacilityActor* Circulator = World->SpawnActorDeferred<ABathWaterCirculatorFacilityActor>(
		ABathWaterCirculatorFacilityActor::StaticClass(),
		FTransform(FVector(10000.0f, 0.0f, 100.0f)), nullptr, nullptr,
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (!TestNotNull(TEXT("Native circulator capacity fixture exists"), Circulator)) return false;
	Circulator->GetCapacityComponent()->RestoreCapacity(EBathWaterCapacityKind::Circulation, 100.0f);
	TestTrue(TEXT("Circulator fixture authors its lever and gauge"), SetCirculatorTestMeshes(Circulator, CubeMesh));
	TestTrue(TEXT("Native circulator stages for the thermal integration fixture"),
		Circulator->GetFacilityPlacementComponent()->PrepareForStagedPlacement(*CirculatorDefinition, FailureReason));
	TestTrue(TEXT("Native circulator starts with active operation points"),
		Circulator->GetOperation()->ImportOperationState(100.0f, FailureReason));
	Circulator->FinishSpawning(FTransform(FVector(10000.0f, 0.0f, 100.0f)));
	BeginActorPlayIfNeeded(Circulator);
	TestTrue(TEXT("Native circulator captures its collision snapshot"),
		Circulator->GetFacilityPlacementComponent()->FinalizeStagedPlacementCollisionSnapshot(FailureReason));
	TestTrue(TEXT("Native circulator registers its active circulation capacity"),
		Circulator->StagePlacedDomainRegistration(FailureReason));
	TestTrue(TEXT("Native circulator commits its placed state"),
		Circulator->GetFacilityPlacementComponent()->CommitStagedPlacement(FailureReason));
	Circulator->PublishPlacedDomainRegistration();
	TestTrue(TEXT("Thermal integration fixture reserves full circulation"),
		Operations->RequestCirculationPercent(DemandBath, 100.0f).bSucceeded);
	DemandBath->GetBathWaterState()->SetNormalizedAmount(0.5f);

	ABathWaterBoilerFacilityActor* ActiveBoiler = SpawnBoiler(25.0f, FVector(12000.0f, 0.0f, 100.0f));
	ABathWaterBoilerFacilityActor* EmptyBoilerToRecover = SpawnBoiler(0.0f, FVector(14000.0f, 0.0f, 100.0f));
	ABathWaterBoilerFacilityActor* RemainingEmptyBoiler = SpawnBoiler(0.0f, FVector(16000.0f, 0.0f, 100.0f));
	if (!TestNotNull(TEXT("Active boiler fixture installs"), ActiveBoiler)
		|| !TestNotNull(TEXT("Recovery boiler fixture installs"), EmptyBoilerToRecover)
		|| !TestNotNull(TEXT("Remaining boiler fixture installs"), RemainingEmptyBoiler))
	{
		Item->Destroy();
		return false;
	}
	UBathWaterConditionComponent* CirculatorCondition = DemandBath->GetBathWaterCondition();
	FFloatProperty* CirculatorDecayProperty = FindFProperty<FFloatProperty>(
		UUtilityOperationComponent::StaticClass(), TEXT("DecayPointsPerSecond"));
	FFloatProperty* ActualTemperatureProperty = FindFProperty<FFloatProperty>(
		UBathWaterConditionComponent::StaticClass(), TEXT("ActualTemperatureC"));
	FFloatProperty* ContaminationProperty = FindFProperty<FFloatProperty>(
		UBathWaterConditionComponent::StaticClass(), TEXT("ContaminationPercent"));
	if (!TestNotNull(TEXT("Circulator decay authoring property exists"), CirculatorDecayProperty)
		|| !TestNotNull(TEXT("Bath actual temperature property exists"), ActualTemperatureProperty)
		|| !TestNotNull(TEXT("Bath contamination property exists"), ContaminationProperty))
	{
		Item->Destroy();
		return false;
	}
	TestTrue(TEXT("LAB-035/036 single-circulator bath accepts active temperature demand"),
		Operations->RequestTargetTemperature(DemandBath, 30.0f).bSucceeded);
	ActualTemperatureProperty->SetPropertyValue_InContainer(CirculatorCondition, 20.0f);
	ContaminationProperty->SetPropertyValue_InContainer(CirculatorCondition, 50.0f);
	CirculatorCondition->TickComponent(10.0f, LEVELTICK_All, nullptr);
	TestTrue(TEXT("LAB-035/036 the sole active circulator cleans and heats the bath"),
		CirculatorCondition->GetContaminationPercent() < 50.0f
		&& CirculatorCondition->GetActualTemperatureC() > 20.0f);

	ActualTemperatureProperty->SetPropertyValue_InContainer(CirculatorCondition, 25.0f);
	ContaminationProperty->SetPropertyValue_InContainer(CirculatorCondition, 50.0f);
	CirculatorDecayProperty->SetPropertyValue_InContainer(Circulator->GetOperation(), 100.0f);
	TickWorldForDuration(World, 1.1, 0.05f);
	TestTrue(TEXT("LAB-035/036 exhausting the only circulator removes all active circulation capacity"),
		FMath::IsNearlyZero(Circulator->GetOperation()->GetRemainingPoints())
		&& FMath::IsNearlyZero(Operations->GetCapacitySnapshot(EBathWaterCapacityKind::Circulation).ActivePoints)
		&& !Operations->IsCapacitySatisfied(EBathWaterCapacityKind::Circulation));
	TestTrue(TEXT("LAB-035/036 exhaustion preserves the bath's circulation and temperature settings"),
		FMath::IsNearlyEqual(CirculatorCondition->GetCirculationPercent(), 100.0f)
		&& FMath::IsNearlyEqual(CirculatorCondition->GetTargetTemperatureC(), 30.0f));
	ActualTemperatureProperty->SetPropertyValue_InContainer(CirculatorCondition, 25.0f);
	ContaminationProperty->SetPropertyValue_InContainer(CirculatorCondition, 50.0f);
	CirculatorCondition->TickComponent(10.0f, LEVELTICK_All, nullptr);
	TestTrue(TEXT("LAB-035 purification stops while LAB-036 active heating gives way to natural return"),
		FMath::IsNearlyEqual(CirculatorCondition->GetContaminationPercent(), 50.0f, 0.001f)
		&& CirculatorCondition->GetActualTemperatureC() < 25.0f);

	CirculatorDecayProperty->SetPropertyValue_InContainer(Circulator->GetOperation(), 0.0f);
	TestTrue(TEXT("Labor recovery restores the exhausted circulator"),
		Circulator->GetOperation()->ApplyLaborReward(10.0f, FailureReason)
		&& FMath::IsNearlyEqual(Circulator->GetOperation()->GetRemainingPoints(), 10.0f)
		&& Operations->IsCapacitySatisfied(EBathWaterCapacityKind::Circulation));
	CirculatorCondition->TickComponent(10.0f, LEVELTICK_All, nullptr);
	TestTrue(TEXT("LAB-035/036 labor recovery automatically resumes purification and active heating"),
		CirculatorCondition->GetContaminationPercent() < 50.0f
		&& CirculatorCondition->GetActualTemperatureC() > 24.0f
		&& FMath::IsNearlyEqual(CirculatorCondition->GetCirculationPercent(), 100.0f)
		&& FMath::IsNearlyEqual(CirculatorCondition->GetTargetTemperatureC(), 30.0f));
	ActualTemperatureProperty->SetPropertyValue_InContainer(CirculatorCondition, 20.0f);

	const FBathWaterSettingRequestResult HeatingReservation = Operations->RequestTargetTemperature(DemandBath, 50.0f);
	TestTrue(TEXT("Three installed boilers accept a 150 point reservation"), HeatingReservation.bSucceeded);
	DemandBath->GetBathWaterCondition()->TickComponent(10.0f, LEVELTICK_All, nullptr);
	TestTrue(TEXT("Heating effect stops when only one of two requested boiler capacities is active"),
		FMath::IsNearlyEqual(DemandBath->GetBathWaterCondition()->GetActualTemperatureC(), 20.0f, 0.001f));
	FBathWaterCapacitySnapshot DeficitSnapshot = Operations->GetCapacitySnapshot(EBathWaterCapacityKind::Heating);
	TestTrue(TEXT("Installed 300 and active 100 preserve a 150 point reservation"),
		FMath::IsNearlyEqual(DeficitSnapshot.TotalPoints, 300.0f)
		&& FMath::IsNearlyEqual(DeficitSnapshot.ActivePoints, 100.0f)
		&& FMath::IsNearlyEqual(DeficitSnapshot.UsedPoints, 150.0f)
		&& FMath::IsNearlyEqual(DeficitSnapshot.DeficitPoints, 50.0f)
		&& FMath::IsNearlyZero(DeficitSnapshot.InstalledDeficitPoints));
	TestTrue(TEXT("Recovery is allowed while active capacity is deficient if installed capacity remains sufficient"),
		EmptyBoilerToRecover->TryBeginFacilityRecoveryHold(FailureReason));
	int32 DeficitRecoveryPublications = 0;
	const FDelegateHandle DeficitPublicationHandle = Operations->OnOperationsChanged.AddLambda(
		[&DeficitRecoveryPublications]() { ++DeficitRecoveryPublications; });
	APlaceableFacilityItemActor* DeficitRecoveryItem =
		FFacilityActorConversionTransaction::RecoverFacilityToItem(*EmptyBoilerToRecover, FailureReason);
	TestNotNull(TEXT("Installed-capacity-valid recovery commits"), DeficitRecoveryItem);
	TestEqual(TEXT("Successful deficit recovery publishes exactly once"), DeficitRecoveryPublications, 1);
	const FBathWaterCapacitySnapshot AfterDeficitRecovery = Operations->GetCapacitySnapshot(EBathWaterCapacityKind::Heating);
	TestTrue(TEXT("Successful recovery preserves reservation and active deficit"),
		FMath::IsNearlyEqual(AfterDeficitRecovery.TotalPoints, 200.0f)
		&& FMath::IsNearlyEqual(AfterDeficitRecovery.ActivePoints, 100.0f)
		&& FMath::IsNearlyEqual(AfterDeficitRecovery.UsedPoints, 150.0f)
		&& FMath::IsNearlyEqual(AfterDeficitRecovery.DeficitPoints, 50.0f)
		&& FMath::IsNearlyZero(AfterDeficitRecovery.InstalledDeficitPoints));
	Operations->OnOperationsChanged.Remove(DeficitPublicationHandle);
	if (DeficitRecoveryItem)
	{
		DeficitRecoveryItem->Destroy();
	}
	ABathWaterBoilerFacilityActor* ResumeBoiler = SpawnBoiler(25.0f, FVector(17000.0f, 0.0f, 100.0f));
	if (!TestNotNull(TEXT("Second active boiler installs for automatic effect resume"), ResumeBoiler))
	{
		Item->Destroy();
		return false;
	}
	const FBathWaterCapacitySnapshot ResumedCapacity = Operations->GetCapacitySnapshot(EBathWaterCapacityKind::Heating);
	TestTrue(TEXT("Second boiler restores Active capacity without changing the reservation"),
		FMath::IsNearlyEqual(ResumedCapacity.TotalPoints, 300.0f)
		&& FMath::IsNearlyEqual(ResumedCapacity.ActivePoints, 200.0f)
		&& FMath::IsNearlyEqual(ResumedCapacity.UsedPoints, 150.0f)
		&& ResumedCapacity.IsSatisfied());
	DemandBath->GetBathWaterCondition()->TickComponent(10.0f, LEVELTICK_All, nullptr);
	TestTrue(TEXT("Heating resumes after a second boiler restores active capacity"),
		DemandBath->GetBathWaterCondition()->GetActualTemperatureC() > 20.0f);

	ABathWaterBoilerFacilityActor* LowChargeBoiler = SpawnBoiler(1.0f, FVector(18000.0f, 0.0f, 100.0f));
	if (!TestNotNull(TEXT("Low-charge hold fixture installs"), LowChargeBoiler))
	{
		Item->Destroy();
		return false;
	}
	TestTrue(TEXT("Low-charge boiler begins recovery hold"), LowChargeBoiler->TryBeginFacilityRecoveryHold(FailureReason));
	FUtilityFuelLoad CoalLoad;
	CoalLoad.Kind = EUtilityFuelKind::Coal;
	CoalLoad.Points = 25.0f;
	TestFalse(TEXT("Recovery hold rejects operation fuel input"),
		LowChargeBoiler->GetOperation()->CanAcceptFuel(CoalLoad, FailureReason));
	TickWorldForDuration(World, 1.0);
	TestTrue(TEXT("Charge reaches exactly zero while recovery hold is active"),
		FMath::IsNearlyZero(LowChargeBoiler->GetOperation()->GetRemainingPoints()));
	LowChargeBoiler->CancelFacilityRecoveryHold();
	TestTrue(TEXT("Cancel after zero does not restore the consumed point"),
		FMath::IsNearlyZero(LowChargeBoiler->GetOperation()->GetRemainingPoints()));
	TestTrue(TEXT("Zero edge updates active capacity once while installed capacity remains"),
		FMath::IsNearlyEqual(Operations->GetCapacitySnapshot(EBathWaterCapacityKind::Heating).ActivePoints, 200.0f)
		&& FMath::IsNearlyEqual(Operations->GetCapacitySnapshot(EBathWaterCapacityKind::Heating).TotalPoints, 400.0f));
	TestTrue(TEXT("A zero-charge boiler can still enter recovery"), LowChargeBoiler->TryBeginFacilityRecoveryHold(FailureReason));
	int32 ZeroChargeRecoveryPublications = 0;
	const FDelegateHandle ZeroChargeRecoveryHandle = Operations->OnOperationsChanged.AddLambda(
		[&ZeroChargeRecoveryPublications]() { ++ZeroChargeRecoveryPublications; });
	APlaceableFacilityItemActor* ZeroChargeItem =
		FFacilityActorConversionTransaction::RecoverFacilityToItem(*LowChargeBoiler, FailureReason);
	TestNotNull(TEXT("Successful recovery commits a zero-charge payload"), ZeroChargeItem);
	const UBathWaterUtilityPlacementInstanceData* ZeroChargeData = ZeroChargeItem
		? Cast<UBathWaterUtilityPlacementInstanceData>(ZeroChargeItem->GetPlacementPayload().InstanceData)
		: nullptr;
	TestTrue(TEXT("Zero-charge recovery preserves an explicit empty operation state"),
		ZeroChargeData && ZeroChargeData->bHasOperationState
		&& FMath::IsNearlyZero(ZeroChargeData->RemainingOperationPoints));
	TestEqual(TEXT("Zero-charge recovery publishes provider removal once"), ZeroChargeRecoveryPublications, 1);
	Operations->OnOperationsChanged.Remove(ZeroChargeRecoveryHandle);
	if (ZeroChargeItem)
	{
		ZeroChargeItem->Destroy();
	}
	ActiveBoiler->Destroy();
	RemainingEmptyBoiler->Destroy();
	ResumeBoiler->Destroy();
	if (IsValid(LowChargeBoiler))
	{
		LowChargeBoiler->Destroy();
	}
	Circulator->Destroy();
	Operations->UnregisterBath(DemandBath->GetBathWaterCondition(), false);
	DemandBath->Destroy();

	ABathWaterBoilerFacilityActor* AutomationTemplateProbe = World->SpawnActorDeferred<AUtilityLaborBoilerAutomationActor>(
		AUtilityLaborBoilerAutomationActor::StaticClass(), FTransform(FRotator::ZeroRotator, FVector(22000.0f, 0.0f, 100.0f)),
		nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (TestNotNull(TEXT("Automation boiler template fixture exists"), AutomationTemplateProbe))
	{
		FText ProbeFailureReason;
		const bool bProbeInitiallyValid = AutomationTemplateProbe->HasValidUtilityAuthoring(ProbeFailureReason);
		TestTrue(FString::Printf(TEXT("Automation boiler template is valid before staging (%s)"), *ProbeFailureReason.ToString()),
			bProbeInitiallyValid);
		TestTrue(TEXT("CDO-configured boiler fixture can stage placement"),
			AutomationTemplateProbe->GetFacilityPlacementComponent()->PrepareForStagedPlacement(*Definition, FailureReason));
		AutomationTemplateProbe->FinishSpawning(FTransform(FRotator::ZeroRotator, FVector(22000.0f, 0.0f, 100.0f)));
		BeginActorPlayIfNeeded(AutomationTemplateProbe);
		TestTrue(TEXT("Automation staged boiler preserves authoring"),
			AutomationTemplateProbe->HasValidUtilityAuthoring(ProbeFailureReason));
		TestTrue(TEXT("CDO-configured boiler completes staged collision capture"),
			AutomationTemplateProbe->GetFacilityPlacementComponent()->FinalizeStagedPlacementCollisionSnapshot(FailureReason));
		TestTrue(TEXT("CDO-configured boiler stages its domain registration"),
			AutomationTemplateProbe->StagePlacedDomainRegistration(FailureReason));
		AutomationTemplateProbe->RollbackPlacedDomainRegistration();
		AutomationTemplateProbe->Destroy();
	}
	AFacilityPlacementZoneAutomationActor* PlacementZone = World->SpawnActor<AFacilityPlacementZoneAutomationActor>(
		AFacilityPlacementZoneAutomationActor::StaticClass(),
		FTransform(FRotator::ZeroRotator, FVector(25000.0f, 0.0f, 0.0f)));
	if (!TestNotNull(TEXT("Utility placement zone fixture is created"), PlacementZone))
	{
		Item->Destroy();
		return false;
	}
	PlacementZone->AddAllowedTag(FGameplayTag::RequestGameplayTag(TEXT("Facility.Placeable")));
	PlacementZone->GetZoneBounds()->SetBoxExtent(FVector(2000.0f, 2000.0f, 500.0f));
	AActor* PlacementCarryOwner = World->SpawnActor<AActor>();
	if (!TestNotNull(TEXT("Placement carry owner is created"), PlacementCarryOwner))
	{
		Item->Destroy();
		return false;
	}
	USceneComponent* PlacementHeldAnchor = NewObject<USceneComponent>(PlacementCarryOwner, TEXT("PlacementHeldAnchor"));
	if (!TestNotNull(TEXT("Placement carry anchor is created"), PlacementHeldAnchor))
	{
		Item->Destroy();
		return false;
	}
	PlacementCarryOwner->SetRootComponent(PlacementHeldAnchor);
	PlacementCarryOwner->AddInstanceComponent(PlacementHeldAnchor);
	PlacementHeldAnchor->RegisterComponent();
	UPlayerCarryComponent* PlacementCarry = NewObject<UPlayerCarryComponent>(PlacementCarryOwner, TEXT("PlacementCarry"));
	PlacementCarryOwner->AddInstanceComponent(PlacementCarry);
	PlacementCarry->ConfigureHeldAnchor(PlacementHeldAnchor);
	PlacementCarry->RegisterComponent();
	if (!TestTrue(TEXT("Recovery item enters the public placement transaction held"),
		Item && PlacementCarry->TryTakePhysicalObject(Item, FailureReason)))
	{
		Item->Destroy();
		return false;
	}
	const FTransform PlacementTransform(FRotator::ZeroRotator, FVector(25000.0f, 0.0f, 100.0f));
	auto CountUtilityBoilers = [World](const bool bActiveOnly)
	{
		int32 Count = 0;
		for (TActorIterator<ABathWaterBoilerFacilityActor> It(World); It; ++It)
		{
			ABathWaterBoilerFacilityActor* Candidate = *It;
			if (IsValid(Candidate) && (!bActiveOnly || Candidate->GetOperation()->IsPlacedClockActive()))
			{
				++Count;
			}
		}
		return Count;
	};
	const int32 BoilerCountBeforePlacementFailure = CountUtilityBoilers(false);
	const int32 ActiveBoilerCountBeforePlacementFailure = CountUtilityBoilers(true);
	const FBathWaterCapacitySnapshot BeforePlacementFailure = Operations->GetCapacitySnapshot(EBathWaterCapacityKind::Heating);
	FFacilityActorConversionTransaction::SetTestFault(
		FFacilityActorConversionTransaction::ETestFault::PlacementDomainRegistration);
	AActor* FailedPlacement = FFacilityActorConversionTransaction::PlaceItemAsFacility(
		*Item, PlacementTransform, *PlacementZone, *PlacementCarry, FailureReason);
	FFacilityActorConversionTransaction::ClearTestFault();
	TestNull(TEXT("Injected public placement failure creates no facility"), FailedPlacement);
	TestTrue(TEXT("Placement rollback preserves the original held item and operation payload"),
		IsValid(Item) && PlacementCarry->GetHeldObject() == Item && Item->IsHeldForPlacement()
		&& RecoveredData && RecoveredData->bHasOperationState
		&& FMath::IsNearlyEqual(RecoveredData->RemainingOperationPoints, ExportedPoints, 0.05f));
	const FBathWaterCapacitySnapshot AfterPlacementFailure = Operations->GetCapacitySnapshot(EBathWaterCapacityKind::Heating);
	TestTrue(TEXT("Failed placement leaves Installed and Active capacity unchanged"),
		FMath::IsNearlyEqual(AfterPlacementFailure.TotalPoints, BeforePlacementFailure.TotalPoints)
		&& FMath::IsNearlyEqual(AfterPlacementFailure.ActivePoints, BeforePlacementFailure.ActivePoints));
	TestTrue(TEXT("Failed placement leaves no staged boiler actor or running clock behind"),
		CountUtilityBoilers(false) == BoilerCountBeforePlacementFailure
		&& CountUtilityBoilers(true) == ActiveBoilerCountBeforePlacementFailure);
	int32 PlacementPublications = 0;
	const FDelegateHandle PlacementPublicationHandle = Operations->OnOperationsChanged.AddLambda(
		[&PlacementPublications]() { ++PlacementPublications; });
	AActor* PlacedActor = FFacilityActorConversionTransaction::PlaceItemAsFacility(
		*Item, PlacementTransform, *PlacementZone, *PlacementCarry, FailureReason);
	TestNotNull(FString::Printf(TEXT("Public PlaceItemAsFacility successfully places the utility boiler (%s)"),
		*FailureReason.ToString()), PlacedActor);
	ABathWaterBoilerFacilityActor* PlacedBoiler = Cast<ABathWaterBoilerFacilityActor>(PlacedActor);
	TestTrue(TEXT("Placement consumes the source item and restores its operation state"),
		PlacedBoiler && !IsValid(Item) && !PlacementCarry->GetHeldObject()
		&& PlacedBoiler->GetFacilityPlacementComponent()->IsPlacedDomainActive()
		&& PlacedBoiler->GetOperation()->IsPlacedClockActive()
		&& FMath::IsNearlyEqual(PlacedBoiler->GetOperation()->GetRemainingPoints(), ExportedPoints, 0.05f));
	const FBathWaterCapacitySnapshot AfterPlacement = Operations->GetCapacitySnapshot(EBathWaterCapacityKind::Heating);
	TestTrue(TEXT("Successful public placement publishes one provider registration"),
		PlacementPublications == 1
		&& FMath::IsNearlyEqual(AfterPlacement.TotalPoints, AfterPlacementFailure.TotalPoints + 100.0f)
		&& FMath::IsNearlyEqual(AfterPlacement.ActivePoints, AfterPlacementFailure.ActivePoints + 100.0f)
		&& CountUtilityBoilers(false) == BoilerCountBeforePlacementFailure + 1
		&& CountUtilityBoilers(true) == ActiveBoilerCountBeforePlacementFailure + 1);
	Operations->OnOperationsChanged.Remove(PlacementPublicationHandle);
	if (IsValid(PlacedBoiler))
	{
		const float ExpectedRemainingAfterReinstallTick = FMath::Max(
			0.0f, ExportedPoints - PlacedBoiler->GetOperation()->GetDecayPointsPerSecond());
		TickWorldForDuration(World, 1.0);
		TestTrue(TEXT("Successfully reinstalled charge resumes decaying from its preserved amount"),
			FMath::IsNearlyEqual(
				PlacedBoiler->GetOperation()->GetRemainingPoints(), ExpectedRemainingAfterReinstallTick, 0.05f));
	}
	if (IsValid(PlacedBoiler))
	{
		PlacedBoiler->Destroy();
	}
	if (IsValid(Item))
	{
		Item->Destroy();
	}
	return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUtilityShovelReleaseVelocityTest,
	"BathhouseSim.Utility.Labor.ShovelReleaseVelocityUsesProperty",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUtilityShovelReleaseVelocityTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FScopedUtilityLaborWorld Scope(TEXT("UtilityShovelReleaseVelocityWorld"));
	UWorld* World = Scope.Get();
	if (!TestNotNull(TEXT("Automation world exists"), World))
	{
		return false;
	}
	AUtilityShovelActor* Shovel = World->SpawnActor<AUtilityShovelActor>();
	if (!TestNotNull(TEXT("Shovel spawns"), Shovel))
	{
		return false;
	}
	FFloatProperty* ForwardProperty = FindFProperty<FFloatProperty>(Shovel->GetClass(), TEXT("ThrowImpulseStrength"));
	FFloatProperty* UpwardProperty =
		FindFProperty<FFloatProperty>(Shovel->GetClass(), TEXT("UpwardThrowImpulseStrength"));
	if (!TestNotNull(TEXT("Forward release property exists"), ForwardProperty) ||
		!TestNotNull(TEXT("Upward release property exists"), UpwardProperty))
	{
		return false;
	}
	// Fixture values differ from the class defaults, so a getter that ignores the property fails.
	const float ForwardFixture = Shovel->GetThrowImpulseStrength() + 37.0f;
	const float UpwardFixture = Shovel->GetUpwardThrowImpulseStrength() + 11.0f;
	ForwardProperty->SetPropertyValue_InContainer(Shovel, ForwardFixture);
	UpwardProperty->SetPropertyValue_InContainer(Shovel, UpwardFixture);
	const IPhysicalCarryable* Carryable = Shovel;
	TestEqual(TEXT("Shovel forward release speed follows its property"),
		Carryable->GetThrowImpulseStrength(), ForwardFixture);
	TestEqual(TEXT("Shovel upward release speed follows its property"),
		Carryable->GetUpwardThrowImpulseStrength(), UpwardFixture);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
