#if WITH_DEV_AUTOMATION_TESTS
#include "Tests/UtilityLaborAutomationTestSupport.h"

#include "Engine/LocalPlayer.h"
#include "Facility/BathWaterUtilityPlacementInstanceData.h"
#include "GameFramework/PlayerController.h"
#include "Placement/FacilityPlacementDefinition.h"
#include "Utility/BathWaterCoolerFacilityActor.h"
#include "Utility/BathWaterCirculatorFacilityActor.h"
#include "Utility/UtilityFuelSupplyActor.h"
#include "Utility/UtilityFuelDoorComponent.h"
#include "Utility/UtilityLeverLaborComponent.h"
#include "Utility/UtilityLeverOperatingVolumeComponent.h"
#include "Utility/UtilityOperationComponent.h"
#include "UObject/UnrealType.h"
#if WITH_EDITOR
#include "UObject/ObjectSaveContext.h"
#endif

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUtilityCoolerDryIceIntegrationTest,
	"BathhouseSim.Utility.Labor.CoolerDryIceIntegration",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUtilityCoolerDryIceIntegrationTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FScopedUtilityLaborWorld Scope(TEXT("UtilityCoolerDryIceWorld"));
	UWorld* World = Scope.Get();
	if (!TestNotNull(TEXT("Automation world exists"), World)) return false;
	UStaticMesh* CubeMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	UMaterialInterface* DryIceMaterial = LoadObject<UMaterialInterface>(nullptr,
		TEXT("/Engine/EngineMaterials/DefaultMaterial.DefaultMaterial"));
	if (!TestNotNull(TEXT("Engine cube mesh exists"), CubeMesh)
		|| !TestNotNull(TEXT("Engine material exists"), DryIceMaterial)) return false;

	APawn* User = World->SpawnActor<APawn>();
	UCameraComponent* Camera = User ? NewObject<UCameraComponent>(User, TEXT("CoolerTestCamera")) : nullptr;
	UPlayerCarryComponent* Carry = User ? NewObject<UPlayerCarryComponent>(User, TEXT("CoolerTestCarry")) : nullptr;
	UPlayerInteractionComponent* Interaction = User
		? NewObject<UPlayerInteractionComponent>(User, TEXT("CoolerTestInteraction")) : nullptr;
	if (!TestNotNull(TEXT("Interaction pawn exists"), User)
		|| !TestNotNull(TEXT("Interaction camera exists"), Camera)
		|| !TestNotNull(TEXT("Carry component exists"), Carry)
		|| !TestNotNull(TEXT("Interaction component exists"), Interaction)) return false;
	User->SetRootComponent(Camera);
	User->AddInstanceComponent(Camera);
	User->AddInstanceComponent(Carry);
	User->AddInstanceComponent(Interaction);
	Camera->RegisterComponent();
	Carry->RegisterComponent();
	Interaction->RegisterComponent();
	Carry->ConfigureHeldAnchor(Camera);
	Interaction->Configure(Camera, Carry);

	FEnumProperty* FuelKindProperty = FindFProperty<FEnumProperty>(AUtilityFuelSupplyActor::StaticClass(), TEXT("FuelKind"));
	if (!TestNotNull(TEXT("Fuel supply kind property exists"), FuelKindProperty)) return false;
	auto SpawnSupply = [&](const TCHAR* Name, const EUtilityFuelKind Kind, const FVector& Location)
	{
		AUtilityFuelSupplyActor* Supply = World->SpawnActorDeferred<AUtilityFuelSupplyActor>(
			AUtilityFuelSupplyActor::StaticClass(), FTransform(Location), nullptr, nullptr,
			ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
		if (!Supply) return static_cast<AUtilityFuelSupplyActor*>(nullptr);
		void* ValueAddress = FuelKindProperty->ContainerPtrToValuePtr<void>(Supply);
		FuelKindProperty->GetUnderlyingProperty()->SetIntPropertyValue(ValueAddress, static_cast<int64>(Kind));
		Supply->GetSupplyMesh()->SetStaticMesh(CubeMesh);
		Supply->GetSupplyMesh()->SetWorldScale3D(FVector(0.15f));
		Supply->FinishSpawning(FTransform(Location));
		BeginActorPlayIfNeeded(Supply);
		return Supply;
	};
	AUtilityFuelSupplyActor* DryIceSupply = SpawnSupply(TEXT("DryIceSupply"), EUtilityFuelKind::DryIce, FVector(100.0f, 0.0f, 0.0f));
	AUtilityFuelSupplyActor* CoalSupply = SpawnSupply(TEXT("CoalSupply"), EUtilityFuelKind::Coal, FVector(500.0f, 0.0f, 0.0f));
	AUtilityShovelActor* Shovel = World->SpawnActor<AUtilityShovelActor>();
	if (!TestNotNull(TEXT("Dry ice supply exists"), DryIceSupply)
		|| !TestNotNull(TEXT("Coal supply exists"), CoalSupply)
		|| !TestNotNull(TEXT("Shovel exists"), Shovel)) return false;
	Shovel->GetWorldMesh()->SetWorldScale3D(FVector(0.15f));
	TestTrue(TEXT("Shovel fixture assigns distinct coal and dry ice appearances"), SetShovelTestMeshes(Shovel, CubeMesh));
	BeginActorPlayIfNeeded(Shovel);
	FText FailureReason;
	TestTrue(TEXT("Dry ice supply passes authoring validation"), DryIceSupply->HasValidAuthoring(FailureReason));
	TestTrue(TEXT("Coal supply passes authoring validation"), CoalSupply->HasValidAuthoring(FailureReason));
	TestTrue(TEXT("Shovel can be carried"), Carry->TryTakePhysicalObject(Shovel, FailureReason));

	UStaticMeshComponent* LoadVisual = FindNamedMeshComponent(Shovel, TEXT("LoadVisual"));
	TestNotNull(TEXT("Shovel load visual exists"), LoadVisual);
	Interaction->RefreshInteractionQuery();
	TestTrue(TEXT("Dry ice supply advertises its own fuel kind"),
		Interaction->GetCurrentInteractionQuery().bCanInteract
		&& Interaction->GetCurrentInteractionQuery().ActionName.ToString() == TEXT("드라이아이스 퍼담기"));
	TestTrue(TEXT("E scoops dry ice through the interaction trace"), Interaction->BeginPrimaryInteraction().bSucceeded);
	TestTrue(TEXT("Dry ice load preserves its kind and points"),
		Shovel->GetFuelLoad().Kind == EUtilityFuelKind::DryIce
		&& FMath::IsNearlyEqual(Shovel->GetFuelLoad().Points, 25.0f));
	TestTrue(TEXT("Dry ice applies the configured material while retaining the fallback mesh"),
		LoadVisual && LoadVisual->GetStaticMesh() == CubeMesh && LoadVisual->GetMaterial(0) == DryIceMaterial);
	Interaction->RefreshInteractionQuery();
	TestTrue(TEXT("Matching supply advertises dry ice return"),
		Interaction->GetCurrentInteractionQuery().bCanInteract
		&& Interaction->GetCurrentInteractionQuery().ActionName.ToString() == TEXT("드라이아이스 반환"));
	TestTrue(TEXT("E returns the matching dry ice load"), Interaction->BeginPrimaryInteraction().bSucceeded);
	TestTrue(TEXT("Returning fuel restores the authored empty shovel appearance"),
		Shovel->IsLoadEmpty() && LoadVisual && LoadVisual->GetStaticMesh() == CubeMesh);

	UFacilityPlacementDefinition* CoolerDefinition = NewObject<UFacilityPlacementDefinition>();
	CoolerDefinition->StableId = TEXT("CoolerDryIceAutomation");
	CoolerDefinition->FacilityTags.AddTag(FGameplayTag::RequestGameplayTag(TEXT("Facility.Placeable")));
	CoolerDefinition->PlacedFacilityClass = ABathWaterCoolerFacilityActor::StaticClass();
	CoolerDefinition->RecoveryItemClass = APlaceableFacilityItemActor::StaticClass();
	CoolerDefinition->RecoveryItemMesh = CubeMesh;
	TestTrue(TEXT("Cooler placement definition validates"), CoolerDefinition->ValidateRuntime(FailureReason));
	FFloatProperty* DecayProperty = FindFProperty<FFloatProperty>(UUtilityOperationComponent::StaticClass(), TEXT("DecayPointsPerSecond"));
	if (!TestNotNull(TEXT("Operation decay property exists"), DecayProperty)) return false;
	auto SpawnInstalledCooler = [&](const FVector& Location, const float Points, const float Decay, const TCHAR* Id)
	{
		UFacilityPlacementDefinition* Definition = NewObject<UFacilityPlacementDefinition>();
		Definition->StableId = Id;
		Definition->FacilityTags.AddTag(FGameplayTag::RequestGameplayTag(TEXT("Facility.Placeable")));
		Definition->PlacedFacilityClass = ABathWaterCoolerFacilityActor::StaticClass();
		Definition->RecoveryItemClass = APlaceableFacilityItemActor::StaticClass();
		Definition->RecoveryItemMesh = CubeMesh;
		ABathWaterCoolerFacilityActor* Cooler = World->SpawnActorDeferred<ABathWaterCoolerFacilityActor>(
			ABathWaterCoolerFacilityActor::StaticClass(), FTransform(Location), nullptr, nullptr,
			ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
		if (!Cooler || !SetCoolerTestMeshes(Cooler, CubeMesh)) return static_cast<ABathWaterCoolerFacilityActor*>(nullptr);
		Cooler->GetCapacityComponent()->RestoreCapacity(EBathWaterCapacityKind::Cooling, 100.0f);
		DecayProperty->SetPropertyValue_InContainer(Cooler->GetOperation(), Decay);
		if (!Cooler->GetFacilityPlacementComponent()->PrepareForStagedPlacement(*Definition, FailureReason)
			|| !Cooler->GetOperation()->ImportOperationState(Points, FailureReason))
		{
			Cooler->Destroy();
			return static_cast<ABathWaterCoolerFacilityActor*>(nullptr);
		}
		Cooler->FinishSpawning(FTransform(Location));
		BeginActorPlayIfNeeded(Cooler);
		if (!Cooler->GetFacilityPlacementComponent()->FinalizeStagedPlacementCollisionSnapshot(FailureReason)
			|| !Cooler->StagePlacedDomainRegistration(FailureReason)
			|| !Cooler->GetFacilityPlacementComponent()->CommitStagedPlacement(FailureReason))
		{
			Cooler->RollbackPlacedDomainRegistration();
			Cooler->Destroy();
			return static_cast<ABathWaterCoolerFacilityActor*>(nullptr);
		}
		Cooler->PublishPlacedDomainRegistration();
		return Cooler;
	};
	ABathWaterCoolerFacilityActor* Cooler = SpawnInstalledCooler(
		FVector(150.0f, 0.0f, 0.0f), 90.0f, 0.0f, TEXT("CoolerDryIceAutomation"));
	ABathWaterCoolerFacilityActor* OtherCooler = SpawnInstalledCooler(
		FVector(800.0f, 0.0f, 0.0f), 20.0f, 0.0f, TEXT("CoolerIndependentAutomation"));
	if (!TestNotNull(TEXT("Installed cooler exists"), Cooler)
		|| !TestNotNull(TEXT("Second installed cooler exists"), OtherCooler)) return false;
	TestTrue(TEXT("Cooler advertises DryIce and Cooling authoring"),
		Cooler->GetAcceptedFuelKind() == EUtilityFuelKind::DryIce
		&& Cooler->GetCapacityComponent()->GetCapacityKind() == EBathWaterCapacityKind::Cooling);
	TestTrue(TEXT("Two coolers own independent operation state"),
		!FMath::IsNearlyEqual(Cooler->GetOperation()->GetRemainingPoints(), OtherCooler->GetOperation()->GetRemainingPoints()));
	TestNull(TEXT("Cooler has no native GaugeFace component"), FindNamedMeshComponent(Cooler, TEXT("GaugeFace")));

	DryIceSupply->SetActorLocation(FVector(100.0f, 0.0f, 0.0f));
	Interaction->RefreshInteractionQuery();
	TestTrue(TEXT("Initial dry ice batch loads for the 90 plus 25 cap"), Interaction->BeginPrimaryInteraction().bSucceeded);
	DryIceSupply->SetActorLocation(FVector(1000.0f, 0.0f, 0.0f));
	Interaction->RefreshInteractionQuery();
	TestTrue(TEXT("An installed cooler accepts the initial dry ice batch"),
		Interaction->GetCurrentInteractionQuery().bCanInteract
		&& Cooler->GetFuelDoorPresentation()->GetTargetAlpha() == 1.0f);
	TestTrue(TEXT("First insert clamps 90 plus 25 to 100"),
		Interaction->BeginPrimaryInteraction().bSucceeded
		&& Shovel->IsLoadEmpty()
		&& FMath::IsNearlyEqual(Cooler->GetOperation()->GetRemainingPoints(), 100.0f));

	DryIceSupply->SetActorLocation(FVector(100.0f, 0.0f, 0.0f));
	Interaction->RefreshInteractionQuery();
	TestTrue(TEXT("A second dry ice scoop succeeds"), Interaction->BeginPrimaryInteraction().bSucceeded);
	DryIceSupply->SetActorLocation(FVector(1000.0f, 0.0f, 0.0f));
	Interaction->RefreshInteractionQuery();
	const FPlayerInteractionQuery FullLoadQuery = Interaction->GetCurrentInteractionQuery();
	TestTrue(FString::Printf(TEXT("Full cooler refuses loaded shovel (can=%d points=%.3f max=%.3f load=%d/%.3f action=%s failure=%s)"),
		FullLoadQuery.bCanInteract, Cooler->GetOperation()->GetRemainingPoints(),
		Cooler->GetOperation()->GetMaximumPoints(), static_cast<int32>(Shovel->GetFuelLoad().Kind),
		Shovel->GetFuelLoad().Points, *FullLoadQuery.ActionName.ToString(), *FullLoadQuery.FailureReason.ToString()),
		!FullLoadQuery.bCanInteract);
	TestTrue(TEXT("Full refusal preserves the dry ice batch"), Shovel->GetFuelLoad().Kind == EUtilityFuelKind::DryIce);
	TestEqual(TEXT("Full cooler closes its door target"), Cooler->GetFuelDoorPresentation()->GetTargetAlpha(), 0.0f);
	DecayProperty->SetPropertyValue_InContainer(Cooler->GetOperation(), 1.0f);
	for (int32 Index = 0; Index < 200; ++Index)
	{
		++GFrameCounter;
		World->Tick(LEVELTICK_All, 0.05f);
		Interaction->RefreshInteractionQuery();
	}
	TestTrue(TEXT("The same focused, loaded shovel becomes insertable as capacity decays"),
		Cooler->GetOperation()->GetRemainingPoints() < 100.0f
		&& Cooler->GetOperation()->GetRemainingPoints() > 80.0f
		&& Interaction->GetCurrentInteractionQuery().bCanInteract
		&& Cooler->GetFuelDoorPresentation()->GetTargetAlpha() == 1.0f);
	TestTrue(TEXT("The retained 25 points cap a decayed 90 point cooler back at 100"),
		Interaction->BeginPrimaryInteraction().bSucceeded
		&& Shovel->IsLoadEmpty()
		&& FMath::IsNearlyEqual(Cooler->GetOperation()->GetRemainingPoints(), 100.0f));
	TestTrue(TEXT("Second cooler operation remains untouched"),
		FMath::IsNearlyEqual(OtherCooler->GetOperation()->GetRemainingPoints(), 20.0f));

	CoalSupply->SetActorLocation(FVector(100.0f, 0.0f, 0.0f));
	Interaction->RefreshInteractionQuery();
	TestTrue(TEXT("Coal remains scoopable for cross-fuel validation"), Interaction->BeginPrimaryInteraction().bSucceeded);
	TestTrue(TEXT("Coal load clears the dry ice material override"),
		Shovel->GetFuelLoad().Kind == EUtilityFuelKind::Coal
		&& LoadVisual->GetMaterial(0) == CubeMesh->GetMaterial(0));
	CoalSupply->SetActorLocation(FVector(1000.0f, 0.0f, 0.0f));
	Interaction->RefreshInteractionQuery();
	TestTrue(TEXT("Coal cannot insert and does not open the cooler door"),
		!Interaction->GetCurrentInteractionQuery().bCanInteract
		&& Cooler->GetFuelDoorPresentation()->GetTargetAlpha() == 0.0f);
	TestTrue(TEXT("Rejected cross-fuel insertion preserves the coal batch"),
		Shovel->GetFuelLoad().Kind == EUtilityFuelKind::Coal
		&& FMath::IsNearlyEqual(Shovel->GetFuelLoad().Points, 25.0f));
	DryIceSupply->SetActorLocation(FVector(100.0f, 0.0f, 0.0f));
	Interaction->RefreshInteractionQuery();
	TestFalse(TEXT("Cross-fuel return to dry ice supply is rejected"), Interaction->GetCurrentInteractionQuery().bCanInteract);
	TestTrue(TEXT("Rejected cross-fuel return preserves coal load"),
		Shovel->GetFuelLoad().Kind == EUtilityFuelKind::Coal
		&& FMath::IsNearlyEqual(Shovel->GetFuelLoad().Points, 25.0f));
	DryIceSupply->SetActorLocation(FVector(1000.0f, 0.0f, 0.0f));
	CoalSupply->SetActorLocation(FVector(100.0f, 0.0f, 0.0f));
	Interaction->RefreshInteractionQuery();
	TestTrue(FString::Printf(TEXT("Matching coal return succeeds (target=%s can=%d load=%d/%.1f)"),
		*Interaction->GetCurrentInteractionQuery().TargetName.ToString(),
		Interaction->GetCurrentInteractionQuery().bCanInteract,
		static_cast<int32>(Shovel->GetFuelLoad().Kind), Shovel->GetFuelLoad().Points),
		Interaction->BeginPrimaryInteraction().bSucceeded);

	DecayProperty->SetPropertyValue_InContainer(Cooler->GetOperation(), 100.0f);
	DryIceSupply->SetActorLocation(FVector(100.0f, 0.0f, 0.0f));
	CoalSupply->SetActorLocation(FVector(1000.0f, 0.0f, 0.0f));
	Interaction->RefreshInteractionQuery();
	TestTrue(TEXT("Dry ice can be scooped for an exhausted cooler"), Interaction->BeginPrimaryInteraction().bSucceeded);
	DryIceSupply->SetActorLocation(FVector(1000.0f, 0.0f, 0.0f));
	Interaction->RefreshInteractionQuery();
	TestFalse(TEXT("Full cooler initially refuses that retained dry ice"), Interaction->GetCurrentInteractionQuery().bCanInteract);
	for (int32 Index = 0; Index < 20; ++Index)
	{
		++GFrameCounter;
		World->Tick(LEVELTICK_All, 0.05f);
		Interaction->RefreshInteractionQuery();
	}
	TestTrue(TEXT("An exhausted cooler stays installed with zero active capacity"),
		FMath::IsNearlyZero(Cooler->GetOperation()->GetRemainingPoints())
		&& Cooler->GetFacilityPlacementComponent()->IsPlacedDomainActive()
		&& !Cooler->GetOperation()->IsProvidingCapacity());
	TestTrue(TEXT("The still-focused dry ice inserts and restarts exhausted Cooling"),
		Interaction->GetCurrentInteractionQuery().bCanInteract
		&& Interaction->BeginPrimaryInteraction().bSucceeded
		&& FMath::IsNearlyEqual(Cooler->GetOperation()->GetRemainingPoints(), 25.0f)
		&& Cooler->GetOperation()->IsProvidingCapacity());

	APlaceableFacilityItemActor* RecoveryItem = World->SpawnActor<APlaceableFacilityItemActor>();
	FFacilityPlacementPayload Payload;
	TestTrue(TEXT("Cooler recovery exports its typed operation payload"),
		Cooler->ExportPlacementPayload(*RecoveryItem, Payload, FailureReason));
	const UBathWaterUtilityPlacementInstanceData* RecoveredData =
		Cast<UBathWaterUtilityPlacementInstanceData>(Payload.InstanceData);
	TestTrue(TEXT("Cooler payload preserves Cooling capacity and 25 operation points"), RecoveredData
		&& RecoveredData->CapacityKind == EBathWaterCapacityKind::Cooling
		&& RecoveredData->bHasOperationState
		&& FMath::IsNearlyEqual(RecoveredData->RemainingOperationPoints, 25.0f));
	ABathWaterCoolerFacilityActor* NewCooler = World->SpawnActor<ABathWaterCoolerFacilityActor>();
	TestTrue(TEXT("New cooler has no imported charge and begins at zero"), NewCooler
		&& !NewCooler->GetOperation()->HasImportedOperationState()
		&& FMath::IsNearlyZero(NewCooler->GetOperation()->GetRemainingPoints())
		&& FMath::IsNearlyZero(NewCooler->GetCapacityComponent()->GetActiveCapacityPoints()));

	FUtilityShovelLoadAppearance CoalFullMesh;
	CoalFullMesh.Mesh = CubeMesh;
	FUtilityShovelLoadAppearance SameDryIceMesh;
	SameDryIceMesh.Mesh = CubeMesh;
	Shovel->SetLoadAppearance(EUtilityFuelKind::Coal, CoalFullMesh);
	Shovel->SetLoadAppearance(EUtilityFuelKind::DryIce, SameDryIceMesh);
	TestFalse(TEXT("Identical coal and dry ice appearance combinations fail validation"), Shovel->HasValidAuthoring(FailureReason));
	FUtilityShovelLoadAppearance DryIceFullMesh;
	DryIceFullMesh.Mesh = CubeMesh;
	DryIceFullMesh.Material = DryIceMaterial;
	Shovel->SetLoadAppearance(EUtilityFuelKind::DryIce, DryIceFullMesh);
	LoadVisual->SetStaticMesh(nullptr);
	TestTrue(TEXT("Two distinct explicit mesh appearances do not need a fallback mesh"), Shovel->HasValidAuthoring(FailureReason));
	LoadVisual->SetStaticMesh(CubeMesh);
	FUtilityShovelLoadAppearance EmptyAppearance;
	Shovel->SetLoadAppearance(EUtilityFuelKind::DryIce, EmptyAppearance);
	TestFalse(TEXT("A fuel appearance with neither mesh nor material fails validation"), Shovel->HasValidAuthoring(FailureReason));
	FUtilityShovelLoadAppearance DryIceMaterialOnly;
	DryIceMaterialOnly.Material = DryIceMaterial;
	Shovel->SetLoadAppearance(EUtilityFuelKind::DryIce, DryIceMaterialOnly);
	LoadVisual->SetStaticMesh(nullptr);
	TestFalse(TEXT("A material-only appearance requires the authored fallback mesh"), Shovel->HasValidAuthoring(FailureReason));
	LoadVisual->SetStaticMesh(CubeMesh);
	TestTrue(TEXT("Restored fallback mesh and distinct fuel appearances validate"), Shovel->HasValidAuthoring(FailureReason));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUtilityCirculatorLeverLaborIntegrationTest,
	"BathhouseSim.Utility.Labor.CirculatorLeverLaborIntegration",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUtilityCirculatorLeverLaborIntegrationTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FScopedUtilityLaborWorld Scope(TEXT("UtilityCirculatorLeverWorld"));
	UWorld* World = Scope.Get();
	if (!TestNotNull(TEXT("Automation world exists"), World)) return false;
	UStaticMesh* CubeMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (!TestNotNull(TEXT("Engine lever mesh exists"), CubeMesh)) return false;

	APawn* User = World->SpawnActor<APawn>();
	UCameraComponent* Camera = User ? NewObject<UCameraComponent>(User, TEXT("LeverTestCamera")) : nullptr;
	UPlayerCarryComponent* Carry = User ? NewObject<UPlayerCarryComponent>(User, TEXT("LeverTestCarry")) : nullptr;
	UPlayerInteractionComponent* Interaction = User
		? NewObject<UPlayerInteractionComponent>(User, TEXT("LeverTestInteraction")) : nullptr;
	if (!TestNotNull(TEXT("Lever interaction pawn exists"), User)
		|| !TestNotNull(TEXT("Lever camera exists"), Camera)
		|| !TestNotNull(TEXT("Lever carry exists"), Carry)
		|| !TestNotNull(TEXT("Lever interaction exists"), Interaction)) return false;
	User->SetRootComponent(Camera);
	User->AddInstanceComponent(Camera);
	User->AddInstanceComponent(Carry);
	User->AddInstanceComponent(Interaction);
	Camera->RegisterComponent();
	Carry->RegisterComponent();
	Interaction->RegisterComponent();
	Carry->ConfigureHeldAnchor(Camera);
	Interaction->Configure(Camera, Carry);
	APlayerController* PlayerController = World->SpawnActor<APlayerController>();
	ULocalPlayer* LocalPlayer = NewObject<ULocalPlayer>(GEngine, TEXT("LeverTestLocalPlayer"));
	if (PlayerController && LocalPlayer)
	{
		BeginActorPlayIfNeeded(PlayerController);
		PlayerController->SetPlayer(LocalPlayer);
		PlayerController->Possess(User);
	}
	const bool bLocallyControlled = User->IsLocallyControlled();
	if (!bLocallyControlled)
	{
		AddWarning(TEXT("Standalone possession did not produce local control; this test explicitly refreshes the focus query during fixed world ticks."));
		Interaction->SetComponentTickEnabled(false);
	}

	UFacilityPlacementDefinition* Definition = NewObject<UFacilityPlacementDefinition>();
	Definition->StableId = TEXT("CirculatorLeverLaborAutomation");
	Definition->FacilityTags.AddTag(FGameplayTag::RequestGameplayTag(TEXT("Facility.Placeable")));
	Definition->PlacedFacilityClass = ABathWaterCirculatorFacilityActor::StaticClass();
	Definition->RecoveryItemClass = APlaceableFacilityItemActor::StaticClass();
	Definition->RecoveryItemMesh = CubeMesh;
	FText FailureReason;
	TestTrue(TEXT("Circulator placement definition validates"), Definition->ValidateRuntime(FailureReason));
	ABathWaterCirculatorFacilityActor* Circulator = World->SpawnActorDeferred<ABathWaterCirculatorFacilityActor>(
		ABathWaterCirculatorFacilityActor::StaticClass(), FTransform(FVector(200.0f, 0.0f, 0.0f)),
		nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (!TestNotNull(TEXT("Circulator is created"), Circulator)) return false;
	TestTrue(TEXT("Circulator fixture assigns lever and gauge meshes"), SetCirculatorTestMeshes(Circulator, CubeMesh));
	FFloatProperty* DecayProperty = FindFProperty<FFloatProperty>(UUtilityOperationComponent::StaticClass(), TEXT("DecayPointsPerSecond"));
	if (!TestNotNull(TEXT("Operation decay property exists"), DecayProperty)) return false;
	DecayProperty->SetPropertyValue_InContainer(Circulator->GetOperation(), 0.0f);
	TestTrue(TEXT("Circulator passes its complete native authoring contract"),
		Circulator->HasValidUtilityAuthoring(FailureReason));
	FFloatProperty* CancelReturnProperty = FindFProperty<FFloatProperty>(
		UUtilityLeverLaborComponent::StaticClass(), TEXT("CancelReturnSeconds"));
	FFloatProperty* DownAngleProperty = FindFProperty<FFloatProperty>(
		UUtilityLeverLaborComponent::StaticClass(), TEXT("DownAngleDegrees"));
	if (!TestNotNull(TEXT("CancelReturnSeconds reflected property exists"), CancelReturnProperty)
		|| !TestNotNull(TEXT("DownAngleDegrees reflected property exists"), DownAngleProperty)) return false;
	const float AuthoredReturnSeconds = CancelReturnProperty->GetPropertyValue_InContainer(Circulator->GetLeverLabor());
	const float AuthoredDownAngle = DownAngleProperty->GetPropertyValue_InContainer(Circulator->GetLeverLabor());
	CancelReturnProperty->SetPropertyValue_InContainer(Circulator->GetLeverLabor(), 0.0f);
	TestTrue(TEXT("A zero cancel return duration is valid authoring"),
		Circulator->HasValidUtilityAuthoring(FailureReason));
	DownAngleProperty->SetPropertyValue_InContainer(Circulator->GetLeverLabor(), 0.0f);
	TestFalse(TEXT("A zero lever down angle is invalid authoring"),
		Circulator->HasValidUtilityAuthoring(FailureReason));
	DownAngleProperty->SetPropertyValue_InContainer(Circulator->GetLeverLabor(), AuthoredDownAngle);
	CancelReturnProperty->SetPropertyValue_InContainer(Circulator->GetLeverLabor(), AuthoredReturnSeconds);
	TestTrue(TEXT("Restored lever authoring values remain valid"),
		Circulator->HasValidUtilityAuthoring(FailureReason));

	UUtilityLeverLaborComponent* TemplateLever = NewObject<UUtilityLeverLaborComponent>();
	UUtilityOperationComponent* TemplateOperation = NewObject<UUtilityOperationComponent>(TemplateLever);
	USceneComponent* TemplatePivot = NewObject<USceneComponent>(TemplateLever);
	UUtilityLeverOperatingVolumeComponent* TemplateVolume =
		NewObject<UUtilityLeverOperatingVolumeComponent>(TemplateLever);
	if (!TestNotNull(TEXT("Template lever preview fixture exists"), TemplateLever)
		|| !TestNotNull(TEXT("Template lever Operation exists"), TemplateOperation)
		|| !TestNotNull(TEXT("Template lever pivot exists"), TemplatePivot)
		|| !TestNotNull(TEXT("Template lever operating volume exists"), TemplateVolume)) return false;
	TemplatePivot->SetRelativeRotation(FRotator(13.0f, -24.0f, 31.0f));
	const FQuat TemplateBaseline = TemplatePivot->GetRelativeRotation().Quaternion();
	TemplateLever->Configure(TemplateOperation, TemplatePivot);
	TemplateLever->SetOperatingVolume(TemplateVolume);
	TestTrue(TEXT("World-less Blueprint template has valid preview authoring"),
		TemplateLever->HasValidAuthoring(FailureReason));
	TemplateLever->PreviewDownPose();
	TemplateLever->RestoreUpPose();
	TestTrue(TEXT("World-less template preview calls do not mutate its pivot"),
		TemplatePivot->GetRelativeRotation().Quaternion().Equals(TemplateBaseline, 1.0e-4f)
		&& FMath::IsNearlyZero(TemplateLever->GetPoseAlpha()));
#if WITH_EDITOR
	const UFunction* PreviewDownFunction = UUtilityLeverLaborComponent::StaticClass()->FindFunctionByName(TEXT("PreviewDownPose"));
	const UFunction* RestoreUpFunction = UUtilityLeverLaborComponent::StaticClass()->FindFunctionByName(TEXT("RestoreUpPose"));
	TestTrue(TEXT("Lever preview and restore appear as CallInEditor functions"),
		PreviewDownFunction && PreviewDownFunction->HasMetaData(TEXT("CallInEditor"))
		&& RestoreUpFunction && RestoreUpFunction->HasMetaData(TEXT("CallInEditor")));
#endif

	TestTrue(TEXT("Circulator prepares staged placement"),
		Circulator->GetFacilityPlacementComponent()->PrepareForStagedPlacement(*Definition, FailureReason));
	TestTrue(TEXT("Operation imports 20 points before installation"),
		Circulator->GetOperation()->ImportOperationState(20.0f, FailureReason));
	Circulator->FinishSpawning(FTransform(FVector(200.0f, 0.0f, 0.0f)));
	BeginActorPlayIfNeeded(Circulator);
	TestTrue(TEXT("Circulator captures its staged collision snapshot"),
		Circulator->GetFacilityPlacementComponent()->FinalizeStagedPlacementCollisionSnapshot(FailureReason));
	TestTrue(TEXT("Circulator registers its placed domain"), Circulator->StagePlacedDomainRegistration(FailureReason));
	TestTrue(TEXT("Circulator commits installation"),
		Circulator->GetFacilityPlacementComponent()->CommitStagedPlacement(FailureReason));
	Circulator->PublishPlacedDomainRegistration();
	UUtilityLeverLaborComponent* LeverPreview = Circulator->GetLeverLabor();
	const FQuat AuthoredUpPose = Circulator->GetLeverPivot()->GetRelativeRotation().Quaternion();
	LeverPreview->PreviewDownPose();
	LeverPreview->RestoreUpPose();
	TestTrue(TEXT("LAB-Editor game-world preview calls leave the lever unchanged"),
		FMath::IsNearlyZero(LeverPreview->GetPoseAlpha())
		&& Circulator->GetLeverPivot()->GetRelativeRotation().Quaternion().Equals(AuthoredUpPose, 1.0e-4f));
#if WITH_EDITOR
	const EWorldType::Type OriginalWorldType = World->WorldType;
	World->WorldType = EWorldType::EditorPreview;
	LeverPreview->PreviewDownPose();
	TestTrue(TEXT("EditorPreview world applies the authored down pose"),
		FMath::IsNearlyEqual(LeverPreview->GetPoseAlpha(), 1.0f)
		&& !Circulator->GetLeverPivot()->GetRelativeRotation().Quaternion().Equals(AuthoredUpPose, 1.0e-4f));
	FObjectSaveContextData LeverSaveData;
	FObjectPreSaveContext LeverSaveContext(LeverSaveData);
	LeverPreview->PreSave(LeverSaveContext);
	TestTrue(TEXT("PreSave restores the authored up pose after editor preview"),
		FMath::IsNearlyZero(LeverPreview->GetPoseAlpha())
		&& Circulator->GetLeverPivot()->GetRelativeRotation().Quaternion().Equals(AuthoredUpPose, 1.0e-4f));
	World->WorldType = OriginalWorldType;
#endif
	TestFalse(TEXT("Labor reward rejects a nonpositive amount"),
		Circulator->GetOperation()->ApplyLaborReward(0.0f, FailureReason));

	Interaction->RefreshInteractionQuery();
	TestTrue(TEXT("Camera trace sees the lever operating volume"),
		Interaction->GetCurrentInteractionQuery().bCanInteract
		&& Interaction->GetCurrentInteractionQuery().ActionName.ToString() == TEXT("레버 왕복"));
	FHitResult LeverHit;
	TestTrue(TEXT("Trace hits the exact lever volume component"),
		Interaction->GetCurrentFocusHit(LeverHit)
		&& LeverHit.GetActor() == Circulator
		&& LeverHit.GetComponent() == Circulator->GetLeverOperatingVolume());
	APawn* OtherUser = World->SpawnActor<APawn>();
	UCameraComponent* OtherCamera = OtherUser ? NewObject<UCameraComponent>(OtherUser, TEXT("OtherLeverCamera")) : nullptr;
	UPlayerCarryComponent* OtherCarry = OtherUser ? NewObject<UPlayerCarryComponent>(OtherUser, TEXT("OtherLeverCarry")) : nullptr;
	UPlayerInteractionComponent* OtherInteraction = OtherUser
		? NewObject<UPlayerInteractionComponent>(OtherUser, TEXT("OtherLeverInteraction")) : nullptr;
	if (!TestNotNull(TEXT("Second lever user exists"), OtherUser)
		|| !TestNotNull(TEXT("Second user camera exists"), OtherCamera)
		|| !TestNotNull(TEXT("Second user carry exists"), OtherCarry)
		|| !TestNotNull(TEXT("Second user interaction exists"), OtherInteraction)) return false;
	OtherUser->SetRootComponent(OtherCamera);
	OtherUser->SetActorLocation(FVector(-500.0f, 0.0f, 0.0f));
	OtherUser->AddInstanceComponent(OtherCamera);
	OtherUser->AddInstanceComponent(OtherCarry);
	OtherUser->AddInstanceComponent(OtherInteraction);
	OtherCamera->RegisterComponent();
	OtherCarry->RegisterComponent();
	OtherInteraction->RegisterComponent();
	OtherCarry->ConfigureHeldAnchor(OtherCamera);
	OtherInteraction->Configure(OtherCamera, OtherCarry);
	AUtilityShovelActor* OtherShovel = World->SpawnActor<AUtilityShovelActor>();
	if (!TestNotNull(TEXT("Second user's shovel exists"), OtherShovel)) return false;
	OtherShovel->SetActorLocation(FVector(-500.0f, 0.0f, 0.0f));
	OtherShovel->GetWorldMesh()->SetWorldScale3D(FVector(0.15f));
	TestTrue(TEXT("Second user's shovel passes authoring validation"), SetShovelTestMeshes(OtherShovel, CubeMesh));
	BeginActorPlayIfNeeded(OtherShovel);
	TestTrue(TEXT("Second user takes the empty shovel"), OtherCarry->TryTakePhysicalObject(OtherShovel, FailureReason));
	FPlayerInteractionContext OtherContext;
	OtherContext.Interactor = OtherUser;
	OtherContext.CarryComponent = OtherCarry;
	OtherContext.InteractionComponent = OtherInteraction;
	OtherContext.HitActor = Circulator;
	OtherContext.HitComponent = Circulator->GetLeverOperatingVolume();
	const FPlayerInteractionQuery EmptyShovelQuery = Circulator->GetLeverOperatingVolume()->QueryInteraction(OtherContext);
	TestTrue(TEXT("LAB-069 empty carried shovel cannot start a stroke"),
		!EmptyShovelQuery.bCanInteract && EmptyShovelQuery.FailureReason.ToString() == TEXT("손이 비어 있을 때 레버를 조작할 수 있습니다."));
	FStructProperty* FuelLoadProperty = FindFProperty<FStructProperty>(AUtilityShovelActor::StaticClass(), TEXT("FuelLoad"));
	if (!TestNotNull(TEXT("Shovel fuel payload property exists"), FuelLoadProperty)) return false;
	FUtilityFuelLoad LoadedCoal;
	LoadedCoal.Kind = EUtilityFuelKind::Coal;
	LoadedCoal.Points = 25.0f;
	FuelLoadProperty->SetValue_InContainer(OtherShovel, &LoadedCoal);
	const FPlayerInteractionQuery LoadedShovelQuery = Circulator->GetLeverOperatingVolume()->QueryInteraction(OtherContext);
	TestTrue(TEXT("LAB-069 loaded carried shovel also cannot start and its load remains unchanged"),
		!LoadedShovelQuery.bCanInteract
		&& LoadedShovelQuery.FailureReason.ToString() == TEXT("손이 비어 있을 때 레버를 조작할 수 있습니다.")
		&& OtherShovel->GetFuelLoad().Kind == EUtilityFuelKind::Coal
		&& FMath::IsNearlyEqual(OtherShovel->GetFuelLoad().Points, 25.0f));

	int32 OperationPublications = 0;
	bool bReentrantRewardRejected = false;
	const FDelegateHandle OperationHandle = Circulator->GetOperation()->OnOperationChanged.AddLambda(
		[Operation = Circulator->GetOperation(), &OperationPublications, &bReentrantRewardRejected]()
		{
			++OperationPublications;
			FText ReentrantFailure;
			bReentrantRewardRejected = !Operation->ApplyLaborReward(1.0f, ReentrantFailure);
		});
	TestTrue(TEXT("E starts a single stroke through query, trace and execute"), Interaction->BeginPrimaryInteraction().bSucceeded);
	TestTrue(TEXT("The lever enters Stroking"),
		Circulator->GetLeverLabor()->GetState() == EUtilityLeverLaborState::Stroking);
	const FPlayerInteractionQuery BusyQuery = Circulator->GetLeverOperatingVolume()->QueryInteraction(OtherContext);
	TestTrue(TEXT("A different focus source sees the active user's lever lock"),
		!BusyQuery.bCanInteract
		&& BusyQuery.ActionName.ToString() == TEXT("레버 왕복")
		&& BusyQuery.FailureReason.ToString() == TEXT("다른 사용자가 조작 중"));
	TestTrue(TEXT("Same source sees progress without another activation"),
		!Interaction->GetCurrentInteractionQuery().bCanInteract
		&& Interaction->GetCurrentInteractionQuery().bPrimaryProgressVisible
		&& Interaction->GetCurrentInteractionQuery().FailureReason.IsEmpty());
	TestFalse(TEXT("LAB-030 another E press while stroking does not start or queue a second stroke"),
		Interaction->BeginPrimaryInteraction().bSucceeded);
	auto TickFocused = [&](const int32 Count)
	{
		for (int32 Index = 0; Index < Count; ++Index)
		{
			Interaction->RefreshInteractionQuery();
			++GFrameCounter;
			World->Tick(LEVELTICK_All, 0.05f);
		}
	};
	TickFocused(5);
	Interaction->RefreshInteractionQuery();
	TestTrue(TEXT("LAB-029 at 0.25 seconds progress is 0.25 and lever alpha is 0.5"),
		Interaction->GetCurrentInteractionQuery().bPrimaryProgressVisible
		&& FMath::IsNearlyEqual(Interaction->GetCurrentInteractionQuery().HoldProgress, 0.25f, 0.02f)
		&& FMath::IsNearlyEqual(Circulator->GetLeverLabor()->GetPoseAlpha(), 0.5f, 0.02f));
	TickFocused(5);
	TestTrue(TEXT("LAB-029 at the 0.5 second midpoint the lever reaches its down pose"),
		FMath::IsNearlyEqual(Circulator->GetLeverLabor()->GetProgress(), 0.5f, 0.02f)
		&& FMath::IsNearlyEqual(Circulator->GetLeverLabor()->GetPoseAlpha(), 1.0f, 0.02f));
	TickFocused(5);
	TestTrue(TEXT("LAB-029 at 0.75 seconds the lever has returned halfway while progress keeps rising"),
		FMath::IsNearlyEqual(Circulator->GetLeverLabor()->GetProgress(), 0.75f, 0.02f)
		&& FMath::IsNearlyEqual(Circulator->GetLeverLabor()->GetPoseAlpha(), 0.5f, 0.02f));
	TickFocused(4);
	const FQuat LeverRotationBeforeCompletion = Circulator->GetLeverPivot()->GetRelativeRotation().Quaternion();
	TestTrue(TEXT("LAB-029 the last pre-completion frame is already near the up pose"),
		FMath::IsNearlyEqual(Circulator->GetLeverLabor()->GetProgress(), 0.95f, 0.02f)
		&& FMath::IsNearlyEqual(Circulator->GetLeverLabor()->GetPoseAlpha(), 0.1f, 0.02f));
	TickFocused(1);
	const FQuat LeverRotationAtCompletion = Circulator->GetLeverPivot()->GetRelativeRotation().Quaternion();
	const float CompletionFrameRotationDeltaDegrees = FMath::RadiansToDegrees(
		LeverRotationBeforeCompletion.AngularDistance(LeverRotationAtCompletion));
	TestTrue(FString::Printf(TEXT("LAB-029 one complete stroke rewards once without a final-frame snap (state=%d points=%.3f publications=%d reentrantRejected=%d alpha=%.3f delta=%.3f degrees)"),
		static_cast<int32>(Circulator->GetLeverLabor()->GetState()),
		Circulator->GetOperation()->GetRemainingPoints(), OperationPublications,
		bReentrantRewardRejected, Circulator->GetLeverLabor()->GetPoseAlpha(),
		CompletionFrameRotationDeltaDegrees),
		Circulator->GetLeverLabor()->GetState() == EUtilityLeverLaborState::Idle
		&& FMath::IsNearlyEqual(Circulator->GetOperation()->GetRemainingPoints(), 30.0f)
		&& OperationPublications == 1 && bReentrantRewardRejected
		&& FMath::IsNearlyZero(Circulator->GetLeverLabor()->GetPoseAlpha())
		&& CompletionFrameRotationDeltaDegrees <= 6.1f);
	Interaction->RefreshInteractionQuery();
	TestFalse(TEXT("LAB-072 progress row hides immediately after stroke completion"),
		Interaction->GetCurrentInteractionQuery().bPrimaryProgressVisible);
	FText FailureReasonForCap;
	TestTrue(TEXT("Labor rewards clamp at the operation maximum"),
		Circulator->GetOperation()->ApplyLaborReward(1000.0f, FailureReasonForCap)
		&& FMath::IsNearlyEqual(Circulator->GetOperation()->GetRemainingPoints(), 100.0f));

	Interaction->RefreshInteractionQuery();
	TestFalse(TEXT("LAB-066 a full circulator refuses a new stroke"),
		Interaction->GetCurrentInteractionQuery().bCanInteract);
	DecayProperty->SetPropertyValue_InContainer(Circulator->GetOperation(), 100.0f);
	++GFrameCounter;
	World->Tick(LEVELTICK_All, 0.05f);
	Interaction->RefreshInteractionQuery();
	TestTrue(TEXT("LAB-066 decay below maximum re-enables a stroke"),
		Circulator->GetOperation()->GetRemainingPoints() < Circulator->GetOperation()->GetMaximumPoints()
		&& Interaction->GetCurrentInteractionQuery().bCanInteract);
	for (int32 Index = 0; Index < 19; ++Index)
	{
		++GFrameCounter;
		World->Tick(LEVELTICK_All, 0.05f);
	}
	TestTrue(TEXT("Natural operation exhaustion reaches zero without blocking lever authoring"),
		FMath::IsNearlyZero(Circulator->GetOperation()->GetRemainingPoints()));
	DecayProperty->SetPropertyValue_InContainer(Circulator->GetOperation(), 0.0f);
	User->SetActorLocation(FVector::ZeroVector);
	Interaction->RefreshInteractionQuery();
	TestTrue(TEXT("A zero-point circulator can begin a recovery stroke"), Interaction->BeginPrimaryInteraction().bSucceeded);
	TickFocused(20);
	TestTrue(TEXT("Completing a stroke reactivates an exhausted operation"),
		FMath::IsNearlyEqual(Circulator->GetOperation()->GetRemainingPoints(), 10.0f)
		&& Circulator->GetOperation()->IsProvidingCapacity());

	FText RewardFailure;
	DecayProperty->SetPropertyValue_InContainer(Circulator->GetOperation(), 1.0f);
	TestTrue(TEXT("Test fixture prepares 50 operation points"), Circulator->GetOperation()->ApplyLaborReward(40.0f, RewardFailure));
	TestTrue(TEXT("A 50 point circulator begins a stroke"), Interaction->BeginPrimaryInteraction().bSucceeded);
	TickFocused(20);
	TestTrue(TEXT("LAB-033 50 points decay by one during the stroke, then receive ten"),
		FMath::IsNearlyEqual(Circulator->GetOperation()->GetRemainingPoints(), 59.0f));
	Interaction->RefreshInteractionQuery();
	TestTrue(TEXT("Test fixture prepares 95 operation points"), Circulator->GetOperation()->ApplyLaborReward(36.0f, RewardFailure));
	TestTrue(TEXT("A 95 point circulator begins a stroke"), Interaction->BeginPrimaryInteraction().bSucceeded);
	TickFocused(20);
	TestTrue(TEXT("LAB-034 95 points decay then clamp the ten point reward to 100"),
		FMath::IsNearlyEqual(Circulator->GetOperation()->GetRemainingPoints(), 100.0f));

	DecayProperty->SetPropertyValue_InContainer(Circulator->GetOperation(), 100.0f);
	for (int32 Index = 0; Index < 19; ++Index)
	{
		++GFrameCounter;
		World->Tick(LEVELTICK_All, 0.05f);
	}
	Interaction->RefreshInteractionQuery();
	TestTrue(FString::Printf(TEXT("LAB-071 operation reaches five points before the final stroke (points=%.6f time=%.6f decay=%.6f)"),
		Circulator->GetOperation()->GetRemainingPoints(), World->GetTimeSeconds(),
		Circulator->GetOperation()->GetDecayPointsPerSecond()),
		FMath::IsNearlyEqual(Circulator->GetOperation()->GetRemainingPoints(), 5.0f, 0.02f));
	TestTrue(TEXT("Five remaining points still permit a recovery stroke"), Interaction->BeginPrimaryInteraction().bSucceeded);
	TickFocused(1);
	TestTrue(TEXT("LAB-071 natural exhaustion during Stroking does not cancel or hide progress"),
		FMath::IsNearlyZero(Circulator->GetOperation()->GetRemainingPoints())
		&& Circulator->GetLeverLabor()->GetState() == EUtilityLeverLaborState::Stroking
		&& Interaction->GetCurrentInteractionQuery().bPrimaryProgressVisible);
	TickFocused(19);
	TestTrue(TEXT("LAB-071 stroke completion restores ten points after mid-stroke exhaustion"),
		Circulator->GetLeverLabor()->GetState() == EUtilityLeverLaborState::Idle
		&& FMath::IsNearlyEqual(Circulator->GetOperation()->GetRemainingPoints(), 10.0f));
	DecayProperty->SetPropertyValue_InContainer(Circulator->GetOperation(), 0.0f);
	Interaction->RefreshInteractionQuery();

	TestTrue(TEXT("A new stroke starts for LAB-031 range-loss cancellation"), Interaction->BeginPrimaryInteraction().bSucceeded);
	TickFocused(15);
	const float PointsBeforeFocusLoss = Circulator->GetOperation()->GetRemainingPoints();
	const float PoseAlphaAtFocusLoss = Circulator->GetLeverLabor()->GetPoseAlpha();
	TestTrue(TEXT("LAB-031 cancellation starts in the return half of the stroke"),
		FMath::IsNearlyEqual(PoseAlphaAtFocusLoss, 0.5f, 0.02f));
	User->SetActorLocation(FVector(1000.0f, 0.0f, 0.0f));
	Interaction->RefreshInteractionQuery();
	const FPlayerInteractionQuery ReturningQuery =
		Circulator->GetLeverOperatingVolume()->QueryInteraction(OtherContext);
	TestTrue(TEXT("LAB-031 leaving trace range starts return from the current alpha without reward"),
		Circulator->GetLeverLabor()->GetState() == EUtilityLeverLaborState::Returning
		&& FMath::IsNearlyEqual(Circulator->GetLeverLabor()->GetPoseAlpha(), PoseAlphaAtFocusLoss, 0.02f)
		&& FMath::IsNearlyEqual(Circulator->GetOperation()->GetRemainingPoints(), PointsBeforeFocusLoss));
	TestTrue(TEXT("Returning query uses the stable action and failure reason"),
		!ReturningQuery.bCanInteract
		&& ReturningQuery.ActionName.ToString() == TEXT("레버 왕복")
		&& ReturningQuery.FailureReason.ToString() == TEXT("레버가 돌아오는 중"));
	User->SetActorLocation(FVector::ZeroVector);
	Interaction->RefreshInteractionQuery();
	TestFalse(TEXT("LAB-068 E is ignored while the lever is returning"),
		Interaction->BeginPrimaryInteraction().bSucceeded);
	TestTrue(TEXT("LAB-068 ignored E leaves return state and operation points unchanged"),
		Circulator->GetLeverLabor()->GetState() == EUtilityLeverLaborState::Returning
		&& FMath::IsNearlyEqual(Circulator->GetOperation()->GetRemainingPoints(), PointsBeforeFocusLoss));
	TickFocused(1);
	TestTrue(TEXT("LAB-031 return starts from the existing alpha and advances continuously"),
		FMath::IsNearlyEqual(Circulator->GetLeverLabor()->GetPoseAlpha(), PoseAlphaAtFocusLoss * 0.75f, 0.02f));
	TickFocused(3);
	TestTrue(TEXT("LAB-068 cancelled lever returns to idle and the authored up pose"),
		Circulator->GetLeverLabor()->GetState() == EUtilityLeverLaborState::Idle
		&& FMath::IsNearlyZero(Circulator->GetLeverLabor()->GetPoseAlpha()));

	CancelReturnProperty->SetPropertyValue_InContainer(Circulator->GetLeverLabor(), 0.0f);
	Interaction->RefreshInteractionQuery();
	TestTrue(TEXT("Zero-duration cancellation starts a new stroke"), Interaction->BeginPrimaryInteraction().bSucceeded);
	TickFocused(15);
	const float PointsBeforeImmediateReturn = Circulator->GetOperation()->GetRemainingPoints();
	User->SetActorLocation(FVector(1000.0f, 0.0f, 0.0f));
	Interaction->RefreshInteractionQuery();
	TestTrue(TEXT("LAB-031 zero CancelReturnSeconds returns immediately without reward"),
		Circulator->GetLeverLabor()->GetState() == EUtilityLeverLaborState::Idle
		&& FMath::IsNearlyZero(Circulator->GetLeverLabor()->GetPoseAlpha())
		&& FMath::IsNearlyEqual(Circulator->GetOperation()->GetRemainingPoints(), PointsBeforeImmediateReturn));
	User->SetActorLocation(FVector::ZeroVector);
	CancelReturnProperty->SetPropertyValue_InContainer(Circulator->GetLeverLabor(), AuthoredReturnSeconds);

	Interaction->RefreshInteractionQuery();
	TestTrue(TEXT("E starts a fresh stroke after return"), Interaction->BeginPrimaryInteraction().bSucceeded);
	for (int32 Index = 0; Index < 20; ++Index)
	{
		User->SetActorLocation(FVector(static_cast<float>(Index + 1) * 0.5f, 0.0f, 0.0f));
		TickFocused(1);
	}
	TestTrue(TEXT("LAB-070 moving the Pawn while aiming keeps the fixed operating volume focused through completion"),
		Circulator->GetLeverLabor()->GetState() == EUtilityLeverLaborState::Idle
		&& FMath::IsNearlyEqual(Circulator->GetOperation()->GetRemainingPoints(), 20.0f));

	User->SetActorLocation(FVector::ZeroVector);
	Interaction->RefreshInteractionQuery();
	TestTrue(TEXT("A stroke starts before interaction suppression"), Interaction->BeginPrimaryInteraction().bSucceeded);
	TickFocused(4);
	const float PointsBeforeSuppression = Circulator->GetOperation()->GetRemainingPoints();
	Interaction->SetInteractionSuppressed(true);
	TestTrue(TEXT("LAB-032 suppression cancels the active stroke without reward and hides progress"),
		Circulator->GetLeverLabor()->GetState() == EUtilityLeverLaborState::Returning
		&& FMath::IsNearlyEqual(Circulator->GetOperation()->GetRemainingPoints(), PointsBeforeSuppression)
		&& !Interaction->GetCurrentInteractionQuery().bPrimaryProgressVisible);
	Interaction->SetInteractionSuppressed(false);
	for (int32 Index = 0; Index < 4; ++Index)
	{
		Interaction->RefreshInteractionQuery();
		++GFrameCounter;
		World->Tick(LEVELTICK_All, 0.05f);
	}
	TestEqual(TEXT("Suppression cancellation finishes its return"),
		Circulator->GetLeverLabor()->GetState(), EUtilityLeverLaborState::Idle);

	Interaction->RefreshInteractionQuery();
	TestTrue(TEXT("A stroke starts before the actual recovery hold"), Interaction->BeginPrimaryInteraction().bSucceeded);
	TickFocused(4);
	const float PointsBeforeRecovery = Circulator->GetOperation()->GetRemainingPoints();
	TestTrue(FString::Printf(TEXT("Recovery hold starts while lever stroke is active: %s"), *FailureReason.ToString()),
		Circulator->TryBeginFacilityRecoveryHold(FailureReason));
	Interaction->RefreshInteractionQuery();
	++GFrameCounter;
	World->Tick(LEVELTICK_All, 0.05f);
	Interaction->RefreshInteractionQuery();
	TestTrue(FString::Printf(TEXT("LAB-032 recovery hold cancels the active stroke without reward and hides progress (state=%d blocked=%d points=%.3f before=%.3f progress=%d)"),
		static_cast<int32>(Circulator->GetLeverLabor()->GetState()),
		Circulator->GetOperation()->IsLaborBlocked(), Circulator->GetOperation()->GetRemainingPoints(),
		PointsBeforeRecovery, Interaction->GetCurrentInteractionQuery().bPrimaryProgressVisible),
		Circulator->GetLeverLabor()->GetState() == EUtilityLeverLaborState::Returning
		&& FMath::IsNearlyEqual(Circulator->GetOperation()->GetRemainingPoints(), PointsBeforeRecovery)
		&& !Interaction->GetCurrentInteractionQuery().bPrimaryProgressVisible);
	Circulator->CancelFacilityRecoveryHold();
	for (int32 Index = 0; Index < 4; ++Index)
	{
		Interaction->RefreshInteractionQuery();
		++GFrameCounter;
		World->Tick(LEVELTICK_All, 0.05f);
	}
	TestEqual(TEXT("Recovery hold cancellation finishes lever return"),
		Circulator->GetLeverLabor()->GetState(), EUtilityLeverLaborState::Idle);

	UUtilityLeverLaborComponent* LeverLabor = Circulator->GetLeverLabor();
	UUtilityOperationComponent* InjectedOperation = Circulator->GetOperation();
	USceneComponent* InjectedPivot = Circulator->GetLeverPivot();
	LeverLabor->EndPlay(EEndPlayReason::RemovedFromWorld);
	TestTrue(TEXT("Lever EndPlay restores pose and retains injected references"),
		LeverLabor->HasValidAuthoring(FailureReason)
		&& Circulator->GetLeverLabor() == LeverLabor
		&& Circulator->GetOperation() == InjectedOperation
		&& Circulator->GetLeverPivot() == InjectedPivot
		&& FMath::IsNearlyZero(LeverLabor->GetPoseAlpha()));

	Interaction->RefreshInteractionQuery();
	TestTrue(TEXT("A stroke starts before its actor is destroyed"), Interaction->BeginPrimaryInteraction().bSucceeded);
	TickFocused(4);
	TestEqual(TEXT("Destroy scenario has an active lever stroke"),
		LeverLabor->GetState(), EUtilityLeverLaborState::Stroking);
	const int32 PublicationsBeforeDestroy = OperationPublications;
	TWeakObjectPtr<ABathWaterCirculatorFacilityActor> CirculatorWeak(Circulator);
	TestTrue(TEXT("Destroying a stroking circulator succeeds"), Circulator->Destroy());
	Interaction->RefreshInteractionQuery();
	TestFalse(TEXT("LAB-032 destroyed target clears lever progress"),
		Interaction->GetCurrentInteractionQuery().bPrimaryProgressVisible);
	TestTrue(TEXT("LAB-032 actor destruction cancels without publishing a reward"),
		!CirculatorWeak.IsValid() && OperationPublications == PublicationsBeforeDestroy);
	InjectedOperation->OnOperationChanged.Remove(OperationHandle);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
