#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/Engine.h"
#include "GameplayTagContainer.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Interaction/InteractionTypes.h"
#include "Interaction/PlayerCarryComponent.h"
#include "Placement/FacilityActorConversionTransaction.h"
#include "Placement/FacilityPlacementCollisionUtils.h"
#include "Placement/FacilityPlacementComponent.h"
#include "Placement/FacilityPlacementDefinition.h"
#include "Placement/PlaceableFacilityItemActor.h"
#include "Tests/FacilityPlacementAutomationTestProbe.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBathhouseFacilityRecoveryItemAuthoredScaleTest,
	"BathhouseSim.Placement.RecoveryItemAuthoredRootScale",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBathhouseFacilityRecoveryItemAuthoredScaleTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	if (!GEngine)
	{
		AddError(TEXT("GEngine is required for the recovery scale automation test."));
		return false;
	}

	UStaticMesh* CubeMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (!TestNotNull(TEXT("The native Cube collision mesh loads"), CubeMesh))
	{
		return false;
	}

	UClass* FixtureClass = AFacilityRecoveryScaleAutomationItemActor::StaticClass();
	const AFacilityRecoveryScaleAutomationItemActor* FixtureCDO =
		FixtureClass->GetDefaultObject<AFacilityRecoveryScaleAutomationItemActor>();
	if (!TestNotNull(TEXT("The scale fixture CDO has an ItemRoot"), FixtureCDO->GetItemRoot()))
	{
		return false;
	}
	const FVector FixtureCDOActorScale = FixtureCDO->GetActorScale3D();
	const FVector FixtureAuthoredRootScale = FixtureCDO->GetItemRoot()->GetRelativeScale3D();
	const FString FixtureScaleRecord = FString::Printf(
		TEXT("Scale fixture pre-record: CDO GetActorScale3D=%s; ItemRoot relative scale=%s; equal=%s"),
		*FixtureCDOActorScale.ToString(),
		*FixtureAuthoredRootScale.ToString(),
		FixtureCDOActorScale.Equals(FixtureAuthoredRootScale) ? TEXT("true") : TEXT("false"));
	AddInfo(FixtureScaleRecord);
	UE_LOG(LogTemp, Display, TEXT("%s"), *FixtureScaleRecord);
	if (FixtureCDOActorScale.Equals(FixtureAuthoredRootScale))
	{
		AddInfo(TEXT("The fixture CDO scales are equal, so this case does not reproduce the stale component-to-world Blueprint CDO condition."));
	}
	TestFalse(TEXT("The fixture reproduces a stale CDO component-to-world scale"),
		FixtureCDOActorScale.Equals(FixtureAuthoredRootScale));

	const FString AssetClassPath =
		TEXT("/Game/Bathhouse/Blueprints/Placement/BP_PlaceableFacilityItem.BP_PlaceableFacilityItem_C");
	UClass* AuthoredBlueprintClass = LoadObject<UClass>(nullptr, *AssetClassPath);
	if (!TestNotNull(TEXT("The shared authored facility item Blueprint class loads"), AuthoredBlueprintClass))
	{
		return false;
	}
	const APlaceableFacilityItemActor* AuthoredAssetCDO =
		AuthoredBlueprintClass->GetDefaultObject<APlaceableFacilityItemActor>();
	if (!TestNotNull(TEXT("The authored Blueprint CDO has an ItemRoot"), AuthoredAssetCDO->GetItemRoot()))
	{
		return false;
	}
	const FVector AuthoredAssetRootScale = AuthoredAssetCDO->GetItemRoot()->GetRelativeScale3D();

	const FGameplayTag PlacementTag = FGameplayTag::RequestGameplayTag(TEXT("Facility.Placeable"));
	auto CreateDefinition = [&](const FName StableId, UClass* RecoveryItemClass)
	{
		UFacilityPlacementDefinition* Definition = NewObject<UFacilityPlacementDefinition>();
		Definition->StableId = StableId;
		Definition->FacilityTags.AddTag(PlacementTag);
		Definition->PlacedFacilityClass = AFacilityRecoveryScaleAutomationFacilityActor::StaticClass();
		Definition->RecoveryItemClass = RecoveryItemClass;
		Definition->RecoveryItemMesh = CubeMesh;
		return Definition;
	};
	UFacilityPlacementDefinition* FixtureDefinition = CreateDefinition(
		TEXT("RecoveryScaleFixtureAutomation"),
		AFacilityRecoveryScaleAutomationItemActor::StaticClass());
	UFacilityPlacementDefinition* AssetDefinition = CreateDefinition(
		TEXT("RecoveryScaleAssetAutomation"),
		AuthoredBlueprintClass);

	FText FailureReason;
	const bool bFixtureDefinitionValid = FixtureDefinition->ValidateRuntime(FailureReason);
	TestTrue(FString::Printf(TEXT("The fixture recovery Definition validates: %s"), *FailureReason.ToString()),
		bFixtureDefinitionValid);
	FailureReason = FText::GetEmpty();
	const bool bAssetDefinitionValid = AssetDefinition->ValidateRuntime(FailureReason);
	TestTrue(FString::Printf(TEXT("The authored Blueprint recovery Definition validates: %s"), *FailureReason.ToString()),
		bAssetDefinitionValid);
	if (!bFixtureDefinitionValid || !bAssetDefinitionValid)
	{
		return false;
	}

	FVector ResolvedFixtureScale = FVector::ZeroVector;
	FailureReason = FText::GetEmpty();
	const bool bFixtureScaleResolved = APlaceableFacilityItemActor::GetDefinitionItemScale(
		*FixtureDefinition, ResolvedFixtureScale, FailureReason);
	TestTrue(FString::Printf(TEXT("The fixture Definition scale resolves: %s"), *FailureReason.ToString()),
		bFixtureScaleResolved);
	TestTrue(TEXT("The scale helper returns the fixture ItemRoot authored relative scale"),
		ResolvedFixtureScale.Equals(FixtureAuthoredRootScale));

	FVector ResolvedAssetScale = FVector::ZeroVector;
	FailureReason = FText::GetEmpty();
	const bool bAssetScaleResolved = APlaceableFacilityItemActor::GetDefinitionItemScale(
		*AssetDefinition, ResolvedAssetScale, FailureReason);
	TestTrue(FString::Printf(TEXT("The authored Blueprint Definition scale resolves: %s"), *FailureReason.ToString()),
		bAssetScaleResolved);
	TestTrue(TEXT("The scale helper returns the authored Blueprint ItemRoot relative scale"),
		ResolvedAssetScale.Equals(AuthoredAssetRootScale));

	const FName WorldName = MakeUniqueObjectName(
		nullptr,
		UWorld::StaticClass(),
		TEXT("FacilityRecoveryScaleAutomationWorld"));
	FWorldContext& WorldContext = GEngine->CreateNewWorldContext(EWorldType::Game);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, WorldName, GetTransientPackage());
	if (!World)
	{
		GEngine->DestroyWorldContext(World);
		AddError(TEXT("Failed to create the recovery scale automation world."));
		return false;
	}
	World->AddToRoot();
	auto CleanupWorld = [&]()
	{
		World->DestroyWorld(false);
		GEngine->DestroyWorldContext(World);
		World->RemoveFromRoot();
	};
	WorldContext.SetCurrentWorld(World);
	World->InitializeActorsForPlay(FURL());
	World->BeginPlay();

	auto SpawnFacility = [&](UFacilityPlacementDefinition& Definition, const FVector& Location)
	{
		const FTransform SpawnTransform(FRotator::ZeroRotator, Location);
		AFacilityPlacementAutomationActor* Facility =
			World->SpawnActorDeferred<AFacilityPlacementAutomationActor>(
				Definition.PlacedFacilityClass,
				SpawnTransform,
				nullptr,
				nullptr,
				ESpawnActorCollisionHandlingMethod::AlwaysSpawn,
				ESpawnActorScaleMethod::OverrideRootScale);
		if (!Facility)
		{
			return static_cast<AFacilityPlacementAutomationActor*>(nullptr);
		}
		Facility->ConfigureForTest(Definition, EBathhouseFacilityType::Shower);
		Facility->FinishSpawning(
			SpawnTransform,
			false,
			nullptr,
			ESpawnActorScaleMethod::OverrideRootScale);
		if (!Facility->HasActorBegunPlay())
		{
			Facility->DispatchBeginPlay();
		}
		return Facility;
	};

	AFacilityPlacementAutomationActor* FixtureSource = SpawnFacility(*FixtureDefinition, FVector(0.0f, 0.0f, 100.0f));
	TestNotNull(TEXT("The fixture facility is active in the transient world"), FixtureSource);
	if (FixtureSource)
	{
		FTransform DropTransform;
		FailureReason = FText::GetEmpty();
		const bool bDropTransformResolved =
			FixtureSource->GetFacilityPlacementComponent()->GetRecoveryDropTransform(
				DropTransform, FailureReason);
		TestTrue(FString::Printf(TEXT("The facility footprint determines its recovery drop transform: %s"),
			*FailureReason.ToString()), bDropTransformResolved);
		FTransform AuthoredScaleTransform = DropTransform;
		AuthoredScaleTransform.SetScale3D(FixtureAuthoredRootScale);
		FVector AuthoredQueryLocation = FVector::ZeroVector;
		FQuat AuthoredQueryRotation = FQuat::Identity;
		FCollisionShape AuthoredQueryShape;
		const UPrimitiveComponent* FixtureCollisionTemplate = nullptr;
		FailureReason = FText::GetEmpty();
		const bool bAuthoredShapeResolved = APlaceableFacilityItemActor::BuildDefinitionCollisionQuery(
			*FixtureDefinition,
			AuthoredScaleTransform,
			AuthoredQueryLocation,
			AuthoredQueryRotation,
			AuthoredQueryShape,
			FixtureCollisionTemplate,
			FailureReason);
		TestTrue(FString::Printf(TEXT("The authored-scale collision shape resolves: %s"),
			*FailureReason.ToString()), bAuthoredShapeResolved);

		FTransform UnitScaleTransform = DropTransform;
		UnitScaleTransform.SetScale3D(FVector::OneVector);
		FVector UnitQueryLocation = FVector::ZeroVector;
		FQuat UnitQueryRotation = FQuat::Identity;
		FCollisionShape UnitQueryShape;
		const UPrimitiveComponent* UnitCollisionTemplate = nullptr;
		FailureReason = FText::GetEmpty();
		const bool bUnitShapeResolved = APlaceableFacilityItemActor::BuildDefinitionCollisionQuery(
			*FixtureDefinition,
			UnitScaleTransform,
			UnitQueryLocation,
			UnitQueryRotation,
			UnitQueryShape,
			UnitCollisionTemplate,
			FailureReason);
		TestTrue(FString::Printf(TEXT("The unit-scale comparison shape resolves: %s"),
			*FailureReason.ToString()), bUnitShapeResolved);
		if (!bDropTransformResolved || !bAuthoredShapeResolved || !bUnitShapeResolved
			|| !FixtureCollisionTemplate || !UnitCollisionTemplate)
		{
			CleanupWorld();
			return false;
		}

		const float BlockerHalfExtentCm = 2.0f;
		const float ClearanceCm = 1.0f;
		const float AuthoredHalfExtentX = CubeMesh->GetBounds().BoxExtent.X * FixtureAuthoredRootScale.X;
		const FVector BlockerLocation = AuthoredQueryLocation
			+ FVector(AuthoredHalfExtentX + BlockerHalfExtentCm + ClearanceCm, 0.0f, 0.0f);
		AActor* Blocker = World->SpawnActor<AActor>(
			AActor::StaticClass(), BlockerLocation, FRotator::ZeroRotator);
		if (!TestNotNull(TEXT("The recovery scale blocker actor spawns"), Blocker))
		{
			CleanupWorld();
			return false;
		}
		UBoxComponent* BlockerBox = NewObject<UBoxComponent>(Blocker, TEXT("RecoveryScaleBlocker"));
		Blocker->AddInstanceComponent(BlockerBox);
		Blocker->SetRootComponent(BlockerBox);
		BlockerBox->SetBoxExtent(FVector(BlockerHalfExtentCm));
		BlockerBox->SetCollisionObjectType(ECC_WorldDynamic);
		BlockerBox->SetCollisionResponseToAllChannels(ECR_Block);
		BlockerBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		BlockerBox->RegisterComponent();
		Blocker->SetActorLocation(BlockerLocation);
		BlockerBox->UpdateBounds();

		FCollisionQueryParams FixtureOverlapParams(
			SCENE_QUERY_STAT(FacilityRecoveryScaleAutomation), false, FixtureSource);
		TestTrue(TEXT("A unit-scale recovery collision shape is blocked by the fixture obstacle"),
			UnitCollisionTemplate && FacilityPlacementCollision::HasBlockingOverlap(
				*World,
				UnitQueryLocation,
				UnitQueryRotation,
				UnitQueryShape,
				*UnitCollisionTemplate,
				FixtureOverlapParams));
		TestFalse(TEXT("The authored-scale recovery collision shape clears the same obstacle"),
			FixtureCollisionTemplate && FacilityPlacementCollision::HasBlockingOverlap(
				*World,
				AuthoredQueryLocation,
				AuthoredQueryRotation,
				AuthoredQueryShape,
				*FixtureCollisionTemplate,
				FixtureOverlapParams));

		const FFacilityPlacementTransactionResult RecoveryQuery = FixtureSource->QueryFacilityRecovery();
		TestTrue(FString::Printf(TEXT("Recovery query uses authored ItemRoot scale: %s"),
			*RecoveryQuery.FailureReason.ToString()), RecoveryQuery.bSucceeded);
		FailureReason = FText::GetEmpty();
		APlaceableFacilityItemActor* RecoveredFixture =
			FFacilityActorConversionTransaction::RecoverFacilityToItem(*FixtureSource, FailureReason);
		TestNotNull(FString::Printf(TEXT("The blocked-at-unit-scale fixture recovers successfully: %s"),
			*FailureReason.ToString()), RecoveredFixture);
		if (RecoveredFixture)
		{
			TestTrue(TEXT("The recovered fixture world scale equals its authored ItemRoot scale once"),
				RecoveredFixture->GetActorScale3D().Equals(FixtureAuthoredRootScale));
			TestTrue(TEXT("Recovery retains physics, CCD and Pawn Ignore"),
				RecoveredFixture->GetItemRoot()->IsSimulatingPhysics()
				&& RecoveredFixture->GetItemRoot()->BodyInstance.bUseCCD
				&& RecoveredFixture->GetItemRoot()->GetCollisionResponseToChannel(ECC_Pawn) == ECR_Ignore);

			const FVector PlayerLocation(12000.0f, 0.0f, 100.0f);
			AActor* Player = World->SpawnActor<AActor>(
				AActor::StaticClass(), PlayerLocation, FRotator::ZeroRotator);
			USceneComponent* PlayerRoot = NewObject<USceneComponent>(Player, TEXT("RecoveryScalePlayerRoot"));
			Player->AddInstanceComponent(PlayerRoot);
			Player->SetRootComponent(PlayerRoot);
			PlayerRoot->RegisterComponent();
			Player->SetActorLocation(PlayerLocation);

			USceneComponent* HeldAnchor = NewObject<USceneComponent>(Player, TEXT("RecoveryScaleHeldAnchor"));
			Player->AddInstanceComponent(HeldAnchor);
			HeldAnchor->SetupAttachment(PlayerRoot);
			HeldAnchor->RegisterComponent();
			UPlayerCarryComponent* Carry = NewObject<UPlayerCarryComponent>(Player, TEXT("RecoveryScaleCarry"));
			Player->AddInstanceComponent(Carry);
			Carry->RegisterComponent();
			Carry->ConfigureHeldAnchor(HeldAnchor);

			FPlayerInteractionContext PickupContext;
			PickupContext.Interactor = Player;
			PickupContext.CarryComponent = Carry;
			PickupContext.HitActor = RecoveredFixture;
			TestTrue(TEXT("E pickup succeeds for the recovered scale fixture"),
				RecoveredFixture->ExecuteInteraction(PickupContext).bSucceeded);
			TestTrue(TEXT("E pickup preserves the authored world scale"),
				RecoveredFixture->GetActorScale3D().Equals(FixtureAuthoredRootScale));

			const FPlayerInteractionResult DropResult = Carry->TryFreeDropHeldObject(FVector::ForwardVector);
			TestTrue(FString::Printf(TEXT("G drop succeeds after pickup: %s"), *DropResult.FailureReason.ToString()),
				DropResult.bSucceeded);
			TestTrue(TEXT("G drop preserves the authored world scale"),
				RecoveredFixture->GetActorScale3D().Equals(FixtureAuthoredRootScale));
		}
	}

	AFacilityPlacementAutomationActor* AssetSource =
		SpawnFacility(*AssetDefinition, FVector(6000.0f, 0.0f, 100.0f));
	TestNotNull(TEXT("The authored Blueprint scale facility is active in the transient world"), AssetSource);
	if (AssetSource)
	{
		FailureReason = FText::GetEmpty();
		APlaceableFacilityItemActor* RecoveredAsset =
			FFacilityActorConversionTransaction::RecoverFacilityToItem(*AssetSource, FailureReason);
		TestNotNull(FString::Printf(TEXT("The authored Blueprint facility recovers: %s"),
			*FailureReason.ToString()), RecoveredAsset);
		if (RecoveredAsset)
		{
			TestTrue(TEXT("The recovered authored Blueprint item world scale equals ItemRoot relative scale"),
				RecoveredAsset->GetActorScale3D().Equals(AuthoredAssetRootScale));
		}
	}

	CleanupWorld();
	return true;
}

#endif
