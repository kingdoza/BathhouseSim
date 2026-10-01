#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include <limits>

#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Placement/FacilityPlacementCollisionUtils.h"
#include "Placement/FacilityPlacementDefinition.h"
#include "GameFramework/Character.h"
#include "Interaction/PlayerCarryComponent.h"
#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif
#include "Placement/PlaceableFacilityItemActor.h"
#include "Shop/ShopDeliveryBoxActor.h"
#include "Shop/ShopSettings.h"
#include "Shop/ShopUnboxingCluster.h"
#include "Shop/ShopUnboxingPlacement.h"
#include "Tests/ShopUnboxShapeTestSupport.h"
#include "Tests/ShopAutomationTestProbe.h"

namespace
{
const TCHAR* const ShopDefinitionPaths[] =
{
	TEXT("/Game/Bathhouse/Data/Placement/DA_FacilityPlacement_Bath.DA_FacilityPlacement_Bath"),
	TEXT("/Game/Bathhouse/Data/Placement/DA_FacilityPlacement_Shower.DA_FacilityPlacement_Shower"),
	TEXT("/Game/Bathhouse/Data/Placement/DA_FacilityPlacement_Washer.DA_FacilityPlacement_Washer"),
	TEXT("/Game/Bathhouse/Data/Placement/DA_FacilityPlacement_Dryer.DA_FacilityPlacement_Dryer"),
	TEXT("/Game/Bathhouse/Data/Placement/DA_FacilityPlacement_Boiler.DA_FacilityPlacement_Boiler"),
	TEXT("/Game/Bathhouse/Data/Placement/DA_FacilityPlacement_Cooler.DA_FacilityPlacement_Cooler"),
	TEXT("/Game/Bathhouse/Data/Placement/DA_FacilityPlacement_Circulator.DA_FacilityPlacement_Circulator")
};

UFacilityPlacementDefinition* LoadShopDefinition(FAutomationTestBase& Test, const int32 Index)
{
	UFacilityPlacementDefinition* Definition = LoadObject<UFacilityPlacementDefinition>(
		nullptr, ShopDefinitionPaths[Index]);
	Test.TestNotNull(FString::Printf(TEXT("Shop definition %d loads"), Index), Definition);
	return Definition;
}

// Template item scale = the Root component's default relative scale. A Blueprint CDO never
// computes its component-to-world transform, so CDO->GetActorScale3D() returns 1.0 even when
// the Blueprint authors a non-unit ItemRoot scale (same rule as FacilityPlacementGeometry).
bool GetShopItemTemplateScale(
	FAutomationTestBase& Test,
	const UFacilityPlacementDefinition& Definition,
	FVector& OutScale)
{
	FText FailureReason;
	if (!APlaceableFacilityItemActor::GetDefinitionItemScale(Definition, OutScale, FailureReason))
	{
		Test.AddError(FString::Printf(TEXT("Shop item default root scale is invalid: %s"), *FailureReason.ToString()));
		return false;
	}
	return true;
}

bool GetDefinitionHalfExtent(
	FAutomationTestBase& Test,
	UFacilityPlacementDefinition& Definition,
	FVector& OutHalfExtent)
{
	const APlaceableFacilityItemActor* ItemCDO = Definition.RecoveryItemClass
		? Definition.RecoveryItemClass->GetDefaultObject<APlaceableFacilityItemActor>()
		: nullptr;
		Test.TestNotNull(TEXT("Recovery item CDO exists for collision sizing"), ItemCDO);
	if (!ItemCDO)
	{
		return false;
	}
	FVector TemplateScale = FVector::OneVector;
	if (!GetShopItemTemplateScale(Test, Definition, TemplateScale))
	{
		return false;
	}
	const FTransform ProbeTransform(FRotator::ZeroRotator, FVector::ZeroVector, TemplateScale);
	FVector ShapeCenter = FVector::ZeroVector;
	FQuat ShapeRotation = FQuat::Identity;
	FCollisionShape Shape;
	const UPrimitiveComponent* CollisionTemplate = nullptr;
	FText FailureReason;
	if (!APlaceableFacilityItemActor::BuildDefinitionCollisionQuery(
		Definition, ProbeTransform, ShapeCenter, ShapeRotation, Shape, CollisionTemplate, FailureReason))
	{
		Test.AddError(FString::Printf(TEXT("Definition collision query failed: %s"), *FailureReason.ToString()));
		return false;
	}
	OutHalfExtent = Shape.GetExtent().GetAbs();
	return true;
}

struct FShopScatterAutomationWorld
{
	FWorldContext* Context = nullptr;
	UWorld* World = nullptr;

	bool Initialize(FAutomationTestBase& Test, const TCHAR* Name, const bool bSimulatePhysics = false)
	{
		if (!GEngine)
		{
			Test.AddError(TEXT("GEngine is required for Shop scatter automation."));
			return false;
		}
		const FName WorldName = MakeUniqueObjectName(nullptr, UWorld::StaticClass(), Name);
		Context = &GEngine->CreateNewWorldContext(EWorldType::Game);
		World = UWorld::CreateWorld(EWorldType::Game, false, WorldName, GetTransientPackage());
		if (!World)
		{
			GEngine->DestroyWorldContext(World);
			Context = nullptr;
			Test.AddError(TEXT("Failed to create a Shop scatter test world."));
			return false;
		}
		World->AddToRoot();
		Context->SetCurrentWorld(World);
		if (bSimulatePhysics)
		{
			// Physics tick functions are only registered for worlds that simulate physics.
			World->bShouldSimulatePhysics = true;
		}
		World->InitializeActorsForPlay(FURL());
		if (bSimulatePhysics)
		{
			World->BeginPlay();
		}
		return true;
	}

	void Destroy()
	{
		if (World)
		{
			World->DestroyWorld(false);
			if (GEngine)
			{
				GEngine->DestroyWorldContext(World);
			}
			World->RemoveFromRoot();
			World = nullptr;
		}
		Context = nullptr;
	}

	~FShopScatterAutomationWorld()
	{
		Destroy();
	}
};

bool BuildCollisionQuery(
	UFacilityPlacementDefinition& Definition,
	const FTransform& Transform,
	FVector& OutCenter,
	FQuat& OutRotation,
	FCollisionShape& OutShape)
{
	const UPrimitiveComponent* CollisionTemplate = nullptr;
	FText FailureReason;
	return APlaceableFacilityItemActor::BuildDefinitionCollisionQuery(
		Definition, Transform, OutCenter, OutRotation, OutShape, CollisionTemplate, FailureReason);
}

FVector MakeUnboxingClearanceCenter(const FVector& ShapeCenter, const float ClearanceCm)
{
	return ShapeCenter + FVector(0.0f, 0.0f, FMath::Clamp(ClearanceCm, 0.0f, 50.0f) * 0.5f);
}

FCollisionShape MakeUnboxingClearanceShape(const FCollisionShape& Shape, const float ClearanceCm)
{
	const float Clearance = FMath::Clamp(ClearanceCm, 0.0f, 50.0f);
	const FVector HalfExtent = Shape.GetExtent().GetAbs();
	return FCollisionShape::MakeBox(FVector(
		HalfExtent.X + Clearance,
		HalfExtent.Y + Clearance,
		HalfExtent.Z + Clearance * 0.5f));
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShopUnboxingClusterLayoutAutomationTest,
	"BathhouseSim.Shop.UnboxingClusterLayout",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShopUnboxingClusterLayoutAutomationTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	const FShopUnboxingTuning Tuning = ShopUnboxTest::MakeTuning();
	TArray<FVector> RealHalfExtents;
	RealHalfExtents.Reserve(UE_ARRAY_COUNT(ShopDefinitionPaths));
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(ShopDefinitionPaths); ++Index)
	{
		UFacilityPlacementDefinition* Definition = LoadShopDefinition(*this, Index);
		FVector HalfExtent = FVector::ZeroVector;
		if (!Definition || !GetDefinitionHalfExtent(*this, *Definition, HalfExtent))
		{
			return false;
		}
		RealHalfExtents.Add(HalfExtent);
	}

	for (const int32 Count : { 1, 2, 4, 10 })
	{
		TArray<FVector> HalfExtents;
		for (int32 Index = 0; Index < Count; ++Index)
		{
			HalfExtents.Add(RealHalfExtents[Index % RealHalfExtents.Num()]);
		}
		for (const int32 Seed : { 1, 42, 20260928 })
		{
			FRandomStream Stream(Seed);
			TArray<FShopUnboxClusterItem> Layout;
			TestTrue(FString::Printf(TEXT("%d-item layout builds at seed %d"), Count, Seed),
				FShopUnboxingCluster::BuildLayout(HalfExtents, Tuning.OverlapDepthCm, Tuning.Cluster, Stream, Layout));
			TestEqual(TEXT("Layout restores source item order and count"), Layout.Num(), Count);
			if (Layout.Num() != Count)
			{
				continue;
			}

			float LowestZ = TNumericLimits<float>::Max();
			float HighestZ = -TNumericLimits<float>::Max();
			for (int32 ItemIndex = 0; ItemIndex < Count; ++ItemIndex)
			{
				const FShopUnboxClusterItem& Item = Layout[ItemIndex];
				TestFalse(TEXT("Cluster center is finite"), Item.Center.ContainsNaN());
				TestTrue(TEXT("Yaw is normalized to one revolution"),
					Item.YawDegrees >= 0.0f && Item.YawDegrees < 360.0f);
				LowestZ = FMath::Min(LowestZ, Item.Center.Z);
				HighestZ = FMath::Max(HighestZ, Item.Center.Z);

				float ClosestTargetError = TNumericLimits<float>::Max();
				bool bHasPenetratingPeer = false;
				for (int32 OtherIndex = 0; OtherIndex < Count; ++OtherIndex)
				{
					if (OtherIndex == ItemIndex)
					{
						continue;
					}
					const float TargetDepth = FMath::Min(
						Tuning.OverlapDepthCm,
						Tuning.Cluster.PairDepthExtentRatio * FMath::Min(HalfExtents[ItemIndex].GetMin(), HalfExtents[OtherIndex].GetMin()));
					const float Depth = FShopUnboxingCluster::ComputePenetrationDepth(
						Item.Center, Item.YawDegrees, HalfExtents[ItemIndex],
						Layout[OtherIndex].Center, Layout[OtherIndex].YawDegrees, HalfExtents[OtherIndex]);
					ClosestTargetError = FMath::Min(ClosestTargetError, FMath::Abs(Depth - TargetDepth));
					bHasPenetratingPeer |= Depth > 0.0f;
					TestTrue(TEXT("Non-anchor pair stays within target depth plus the tolerance setting"),
						Depth <= TargetDepth + Tuning.Cluster.DepthToleranceCm + 0.01f);
				}
				if (Count > 1)
				{
					TestTrue(TEXT("Each item has an anchor pair at the requested depth"), ClosestTargetError <= 0.1f);
					TestTrue(TEXT("Every item overlaps at least one peer when D is positive"), bHasPenetratingPeer);
				}
			}
			if (Count > 1)
			{
				TestTrue(TEXT("Layout includes distinct item heights"), HighestZ - LowestZ > 0.1f);
			}
		}
	}

	TArray<FVector> TwoExtents = { RealHalfExtents[1], RealHalfExtents[1] };
	TArray<FShopUnboxClusterItem> FirstLayout;
	TArray<FShopUnboxClusterItem> RepeatLayout;
	TArray<FShopUnboxClusterItem> DifferentSeedLayout;
	FRandomStream FirstStream(711);
	FRandomStream RepeatStream(711);
	FRandomStream DifferentStream(712);
	TestTrue(TEXT("D=0 layout builds"), FShopUnboxingCluster::BuildLayout(TwoExtents, 0.0f, Tuning.Cluster, FirstStream, FirstLayout));
	TestTrue(TEXT("Same-seed layout builds"), FShopUnboxingCluster::BuildLayout(TwoExtents, 0.0f, Tuning.Cluster, RepeatStream, RepeatLayout));
	TestTrue(TEXT("Different-seed layout builds"), FShopUnboxingCluster::BuildLayout(TwoExtents, 0.0f, Tuning.Cluster, DifferentStream, DifferentSeedLayout));
	if (FirstLayout.Num() == 2 && RepeatLayout.Num() == 2 && DifferentSeedLayout.Num() == 2)
	{
		TestTrue(TEXT("Same seed reproduces the complete layout"),
			FirstLayout[1].Center.Equals(RepeatLayout[1].Center, 0.001f)
			&& FMath::IsNearlyEqual(FirstLayout[1].YawDegrees, RepeatLayout[1].YawDegrees, 0.001f));
		TestFalse(TEXT("Different seed changes the generated layout"),
			FirstLayout[1].Center.Equals(DifferentSeedLayout[1].Center, 0.001f)
			&& FMath::IsNearlyEqual(FirstLayout[1].YawDegrees, DifferentSeedLayout[1].YawDegrees, 0.001f));
		TestTrue(TEXT("D=0 items only touch"), FMath::Abs(FShopUnboxingCluster::ComputePenetrationDepth(
			FirstLayout[0].Center, FirstLayout[0].YawDegrees, TwoExtents[0],
			FirstLayout[1].Center, FirstLayout[1].YawDegrees, TwoExtents[1])) <= 0.01f);
	}

	UShopSettings* Settings = GetMutableDefault<UShopSettings>();
	const float PreviousDepth = Settings->UnboxOverlapDepthCm;
	Settings->UnboxOverlapDepthCm = -2.0f;
	TestEqual(TEXT("Overlap setting clamps below zero"), Settings->GetUnboxOverlapDepthCm(), 0.0f);
	Settings->UnboxOverlapDepthCm = UShopSettings::MaxUnboxOverlapDepthCm + 30.0f;
	TestEqual(TEXT("Overlap setting clamps above its maximum"), Settings->GetUnboxOverlapDepthCm(),
		UShopSettings::MaxUnboxOverlapDepthCm);
	Settings->UnboxOverlapDepthCm = std::numeric_limits<float>::quiet_NaN();
	TestEqual(TEXT("Non-finite overlap setting falls back to the header default"), Settings->GetUnboxOverlapDepthCm(),
		UShopSettings::DefaultUnboxOverlapDepthCm);
	Settings->UnboxOverlapDepthCm = PreviousDepth;
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShopUnboxingPawnAvoidanceAutomationTest,
	"BathhouseSim.Shop.UnboxingPawnAvoidance",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShopUnboxingPawnAvoidanceAutomationTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	UFacilityPlacementDefinition* Definition = LoadShopDefinition(*this, 1);
	if (!Definition)
	{
		return false;
	}
	FShopScatterAutomationWorld WorldFixture;
	if (!WorldFixture.Initialize(*this, TEXT("ShopPawnAvoidanceWorld")))
	{
		return false;
	}
	UWorld& World = *WorldFixture.World;
	ACharacter* Player = World.SpawnActor<ACharacter>(ACharacter::StaticClass(), FVector(0.0f, 0.0f, 1000.0f), FRotator::ZeroRotator);
	AShopDeliveryBoxActor* Box = World.SpawnActor<AShopDeliveryBoxActor>(
		AShopDeliveryBoxActor::StaticClass(), FVector(5000.0f, 0.0f, 1000.0f), FRotator::ZeroRotator);
	TestNotNull(TEXT("Player exists for the unboxing query"), Player);
	TestNotNull(TEXT("Delivery box exists for the unboxing query"), Box);
	if (!Player || !Box || !Player->GetCapsuleComponent())
	{
		return false;
	}

	const FVector FootLocation = Player->GetActorLocation()
		- FVector::UpVector * Player->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	TArray<UFacilityPlacementDefinition*> Definitions = { Definition };
	TArray<FTransform> BaselineTransforms;
	FRandomStream BaselineStream(31415);
	FText FailureReason;
	TestTrue(TEXT("Clear front area generates a baseline candidate"), FShopUnboxingPlacement::FindSpawnTransforms(
		World,
		*Player,
		*Player->GetCapsuleComponent(),
		*Box,
		ShopUnboxTest::MakeShapes(Definitions),
		ShopUnboxTest::MakeRequest(*Player),
		ShopUnboxTest::MakeFloorStageTuning(500.0f, 8.0f),
		BaselineStream,
		BaselineTransforms,
		FailureReason));
	if (BaselineTransforms.Num() != 1)
	{
		return false;
	}

	FVector GuestCenter = FVector::ZeroVector;
	FQuat GuestRotation = FQuat::Identity;
	FCollisionShape GuestShape;
	if (!BuildCollisionQuery(*Definition, BaselineTransforms[0], GuestCenter, GuestRotation, GuestShape))
	{
		AddError(TEXT("Failed to resolve the candidate collision shape for the guest fixture."));
		return false;
	}
	ACharacter* Guest = World.SpawnActor<ACharacter>(ACharacter::StaticClass(), GuestCenter, FRotator::ZeroRotator);
	TestNotNull(TEXT("Guest capsule is placed over the baseline candidate"), Guest);
	if (!Guest)
	{
		return false;
	}

	TArray<FTransform> SafeTransforms;
	FRandomStream SafeStream(31415);
	TestTrue(TEXT("Placement retries around a guest Pawn"), FShopUnboxingPlacement::FindSpawnTransforms(
		World,
		*Player,
		*Player->GetCapsuleComponent(),
		*Box,
		ShopUnboxTest::MakeShapes(Definitions),
		ShopUnboxTest::MakeRequest(*Player),
		ShopUnboxTest::MakeFloorStageTuning(500.0f, 8.0f),
		SafeStream,
		SafeTransforms,
		FailureReason));
	if (SafeTransforms.Num() == 1)
	{
		FVector CandidateCenter = FVector::ZeroVector;
		FQuat CandidateRotation = FQuat::Identity;
		FCollisionShape CandidateShape;
		TestTrue(TEXT("Safe result has a definition collision shape"),
			BuildCollisionQuery(*Definition, SafeTransforms[0], CandidateCenter, CandidateRotation, CandidateShape));
		FCollisionObjectQueryParams PawnObjects;
		PawnObjects.AddObjectTypesToQuery(ECC_Pawn);
		FCollisionQueryParams PawnParams(SCENE_QUERY_STAT(ShopGuestPlacementAssertion), false);
		PawnParams.AddIgnoredActor(Player);
		TestFalse(TEXT("Returned candidate clears guest capsules with Dc padding"), World.OverlapAnyTestByObjectType(
			MakeUnboxingClearanceCenter(CandidateCenter, 8.0f),
			CandidateRotation,
			PawnObjects,
			MakeUnboxingClearanceShape(CandidateShape, 8.0f),
			PawnParams));
	}
	Guest->Destroy();

	const FVector BaselineClearanceCenter = MakeUnboxingClearanceCenter(GuestCenter, 8.0f);
	const FCollisionShape BaselineClearanceShape = MakeUnboxingClearanceShape(GuestShape, 8.0f);
	const float GuestRadius = Player->GetCapsuleComponent()->GetScaledCapsuleRadius();
	const FVector EdgeGuestCenter = GuestCenter + GuestRotation.RotateVector(
		FVector(GuestShape.GetExtent().X + GuestRadius + 4.0f, 0.0f, 0.0f));
	ACharacter* EdgeGuest = World.SpawnActor<ACharacter>(
		ACharacter::StaticClass(), EdgeGuestCenter, FRotator::ZeroRotator);
	TestNotNull(TEXT("Edge guest capsule is placed outside the item but inside Dc padding"), EdgeGuest);
	if (!EdgeGuest)
	{
		return false;
	}
	FCollisionObjectQueryParams PawnObjects;
	PawnObjects.AddObjectTypesToQuery(ECC_Pawn);
	FCollisionQueryParams EdgeGuestParams(SCENE_QUERY_STAT(ShopEdgeGuestPlacementAssertion), false);
	EdgeGuestParams.AddIgnoredActor(Player);
	TestFalse(TEXT("Edge guest starts outside the original item shape"), World.OverlapAnyTestByObjectType(
		GuestCenter, GuestRotation, PawnObjects, GuestShape, EdgeGuestParams));
	TestTrue(TEXT("Edge guest overlaps the horizontal Dc clearance shape"), World.OverlapAnyTestByObjectType(
		BaselineClearanceCenter, GuestRotation, PawnObjects, BaselineClearanceShape, EdgeGuestParams));

	TArray<FTransform> EdgeSafeTransforms;
	FRandomStream EdgeSafeStream(31415);
	TestTrue(TEXT("Placement retries around a guest only inside horizontal Dc clearance"),
		FShopUnboxingPlacement::FindSpawnTransforms(
		World,
		*Player,
		*Player->GetCapsuleComponent(),
		*Box,
		ShopUnboxTest::MakeShapes(Definitions),
		ShopUnboxTest::MakeRequest(*Player),
		ShopUnboxTest::MakeFloorStageTuning(500.0f, 8.0f),
		EdgeSafeStream,
		EdgeSafeTransforms,
		FailureReason));
	if (EdgeSafeTransforms.Num() == 1)
	{
		FVector CandidateCenter = FVector::ZeroVector;
		FQuat CandidateRotation = FQuat::Identity;
		FCollisionShape CandidateShape;
		TestTrue(TEXT("Edge-guest safe result has a definition collision shape"),
			BuildCollisionQuery(*Definition, EdgeSafeTransforms[0], CandidateCenter, CandidateRotation, CandidateShape));
		FCollisionQueryParams SafeEdgeParams(SCENE_QUERY_STAT(ShopEdgeGuestSafePlacementAssertion), false);
		SafeEdgeParams.AddIgnoredActor(Player);
		TestFalse(TEXT("Returned layout clears the edge guest with horizontal and upper Dc padding"),
			World.OverlapAnyTestByObjectType(
				MakeUnboxingClearanceCenter(CandidateCenter, 8.0f),
				CandidateRotation,
				PawnObjects,
				MakeUnboxingClearanceShape(CandidateShape, 8.0f),
				SafeEdgeParams));
	}
	return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShopSevenDefinitionSpawnAutomationTest,
	"BathhouseSim.Shop.SevenDefinitionSpawn",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShopSevenDefinitionSpawnAutomationTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FShopScatterAutomationWorld WorldFixture;
	if (!WorldFixture.Initialize(*this, TEXT("ShopSevenDefinitionWorld")))
	{
		return false;
	}
	UWorld& World = *WorldFixture.World;
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(ShopDefinitionPaths); ++Index)
	{
		UFacilityPlacementDefinition* Definition = LoadShopDefinition(*this, Index);
		if (!Definition)
		{
			return false;
		}
		const FVector SpawnLocation(5000.0f * Index, 0.0f, 1000.0f);
		const FRotator RequestedRotation(0.0f, 37.0f, 0.0f);
		const FTransform SpawnTransform(RequestedRotation, SpawnLocation);
		FVector QueryCenter = FVector::ZeroVector;
		FQuat QueryRotation = FQuat::Identity;
		FCollisionShape QueryShape;
		TestTrue(FString::Printf(TEXT("Definition %d exposes collision query dimensions"), Index),
			BuildCollisionQuery(*Definition, SpawnTransform, QueryCenter, QueryRotation, QueryShape));
		const FRotator ResolvedRotation = QueryRotation.Rotator().GetNormalized();
		const bool bRotationMatchesYawOnly = FMath::IsNearlyZero(ResolvedRotation.Pitch, 0.01f)
			&& FMath::IsNearlyZero(ResolvedRotation.Roll, 0.01f)
			&& FMath::Abs(FMath::FindDeltaAngleDegrees(ResolvedRotation.Yaw, RequestedRotation.Yaw)) <= 0.01f;
		TestTrue(FString::Printf(TEXT("Definition %d collision query has only the requested yaw"), Index),
			bRotationMatchesYawOnly);
		AddInfo(FString::Printf(TEXT("SevenDefinitionSpawn index=%d requested yaw=%.2f query pitch=%.3f yaw=%.3f roll=%.3f"),
			Index, RequestedRotation.Yaw, ResolvedRotation.Pitch, ResolvedRotation.Yaw, ResolvedRotation.Roll));
		FText FailureReason;
		APlaceableFacilityItemActor* Item = APlaceableFacilityItemActor::SpawnFreshItem(
			World, *Definition, SpawnTransform, FailureReason);
		TestNotNull(FString::Printf(TEXT("Definition %d creates a fresh-install item"), Index), Item);
		if (!Item)
		{
			AddError(FailureReason.ToString());
			return false;
		}
		TestTrue(FString::Printf(TEXT("Definition %d activates in the free world"), Index),
			Item->ActivateFreeWorld(SpawnTransform, FailureReason));
		UPrimitiveComponent* Primitive = Item->GetPhysicalCarryPrimitive();
		TestTrue(FString::Printf(TEXT("Definition %d ignores Pawn collision"), Index),
			Primitive && Primitive->GetCollisionResponseToChannel(ECC_Pawn) == ECR_Ignore);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShopDeliveryBoxScaleAutomationTest,
	"BathhouseSim.Shop.DeliveryBoxScale",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShopDeliveryBoxScaleAutomationTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	UFacilityPlacementDefinition* Definition = LoadShopDefinition(*this, 1);
	if (!Definition)
	{
		return false;
	}
	const AShopDeliveryBoxScaleAutomationActor* BoxCDO =
		GetDefault<AShopDeliveryBoxScaleAutomationActor>();
	const UStaticMesh* Mesh = BoxCDO && BoxCDO->GetBoxMesh() ? BoxCDO->GetBoxMesh()->GetStaticMesh() : nullptr;
	TestNotNull(TEXT("Scale fixture CDO has a static mesh"), Mesh);
	if (Mesh)
	{
		TestTrue(TEXT("CDO box half extent applies root scale once"),
			BoxCDO->GetBoxHalfExtent().Equals(Mesh->GetBounds().BoxExtent * 0.8f, 0.01f));
	}

#if WITH_EDITOR
	AShopDeliveryBoxScaleAutomationActor* ValidationCDO =
		const_cast<AShopDeliveryBoxScaleAutomationActor*>(BoxCDO);
	TestNotNull(TEXT("Scale fixture CDO is available for authoring validation"), ValidationCDO);
	if (!ValidationCDO || !ValidationCDO->GetBoxMesh())
	{
		return false;
	}
	const FVector SavedRootScale = ValidationCDO->GetBoxMesh()->GetRelativeScale3D();
	const FVector SavedRootLocation = ValidationCDO->GetBoxMesh()->GetRelativeLocation();
	ValidationCDO->SetHeldTransformForTest(FTransform(
		FRotator::ZeroRotator, FVector::ZeroVector, FVector(2.0f)));
	FDataValidationContext WarningContext;
	TestEqual(TEXT("Non-unit held transform scale only warns"),
		ValidationCDO->IsDataValid(WarningContext), EDataValidationResult::Valid);
	TestEqual(TEXT("Ignored held transform scale emits one validation warning"),
		WarningContext.GetNumWarnings(), 1u);
	TestEqual(TEXT("Ignored held transform scale emits no validation errors"),
		WarningContext.GetNumErrors(), 0u);
	ValidationCDO->GetBoxMesh()->SetRelativeScale3D(FVector::ZeroVector);
	FDataValidationContext ZeroScaleContext;
	TestEqual(TEXT("Zero root scale is invalid"),
		ValidationCDO->IsDataValid(ZeroScaleContext), EDataValidationResult::Invalid);
	ValidationCDO->GetBoxMesh()->SetRelativeScale3D(SavedRootScale);
	ValidationCDO->GetBoxMesh()->SetRelativeLocation(FVector(1.0f, 0.0f, 0.0f));
	FDataValidationContext OffsetRootContext;
	TestEqual(TEXT("Nonzero root relative location is invalid"),
		ValidationCDO->IsDataValid(OffsetRootContext), EDataValidationResult::Invalid);
	ValidationCDO->GetBoxMesh()->SetRelativeLocation(SavedRootLocation);
	ValidationCDO->SetHeldTransformForTest(FTransform::Identity);
#endif

	FShopScatterAutomationWorld WorldFixture;
	if (!WorldFixture.Initialize(*this, TEXT("ShopDeliveryBoxScaleWorld")))
	{
		return false;
	}
	UWorld& World = *WorldFixture.World;
	const FTransform DropTransform(FRotator::ZeroRotator, FVector(9000.0f, 0.0f, 1000.0f));
	AShopDeliveryBoxScaleAutomationActor* Box = World.SpawnActorDeferred<AShopDeliveryBoxScaleAutomationActor>(
		AShopDeliveryBoxScaleAutomationActor::StaticClass(),
		DropTransform,
		nullptr,
		nullptr,
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	TestNotNull(TEXT("Scaled delivery box is deferred-spawned"), Box);
	if (!Box)
	{
		return false;
	}
	FShopOrderLine Line;
	Line.ProductId = TEXT("Shower");
	Line.PlacementDefinition = Definition;
	Line.DisplayName = NSLOCTEXT("ShopScatterTests", "ShowerName", "샤워기");
	Line.Quantity = 1;
	TArray<FShopOrderLine> Contents = { Line };
	TestTrue(TEXT("Scale fixture initializes valid delivery contents"), Box->InitializeContents(901, Contents));
	const FTransform AuthoredHeldTransform(
		FRotator(15.0f, 25.0f, 35.0f), FVector(10.0f, 50.0f, -60.0f), FVector(2.0f));
	Box->SetHeldTransformForTest(AuthoredHeldTransform);
	Box->FinishSpawning(DropTransform);
	TestTrue(TEXT("Spawn composes unit drop transform with the 0.8 CDO root scale"),
		Box->GetActorScale3D().Equals(FVector(0.8f), 0.01f));
	FText FailureReason;
	TestTrue(TEXT("Activation keeps the authored CDO root scale"), Box->ActivateFreeWorld(DropTransform, FailureReason));
	TestTrue(TEXT("Free-world activation preserves scale"),
		Box->GetActorScale3D().Equals(FVector(0.8f), 0.01f));
	TestTrue(TEXT("Runtime held transform ignores authored scale"),
		Box->GetHeldTransform().GetScale3D().Equals(FVector::OneVector));
	TestTrue(TEXT("Runtime held transform retains authored location and rotation"),
		Box->GetHeldTransform().GetLocation().Equals(AuthoredHeldTransform.GetLocation())
		&& Box->GetHeldTransform().GetRotation().Equals(AuthoredHeldTransform.GetRotation()));

	ACharacter* Player = World.SpawnActor<ACharacter>(
		ACharacter::StaticClass(), FVector(12000.0f, 0.0f, 1000.0f), FRotator::ZeroRotator);
	TestNotNull(TEXT("Carry owner exists for scale transition coverage"), Player);
	if (!Player)
	{
		return false;
	}
	USceneComponent* HeldAnchor = NewObject<USceneComponent>(Player, TEXT("ShopScaleHeldAnchor"));
	HeldAnchor->SetupAttachment(Player->GetRootComponent());
	Player->AddInstanceComponent(HeldAnchor);
	HeldAnchor->RegisterComponent();
	UPlayerCarryComponent* Carry = NewObject<UPlayerCarryComponent>(Player, TEXT("ShopScaleCarry"));
	Player->AddInstanceComponent(Carry);
	Carry->RegisterComponent();
	Carry->ConfigureHeldAnchor(HeldAnchor);
	TestTrue(TEXT("Scaled delivery box can be picked up"), Carry->TryTakePhysicalObject(Box, FailureReason));
	TestTrue(TEXT("Pickup keeps the root scale"), Box->GetActorScale3D().Equals(FVector(0.8f), 0.01f));
	TestTrue(TEXT("Scaled delivery box can be dropped"), Carry->TryFreeDropHeldObject(FVector::ForwardVector).bSucceeded);
	TestTrue(TEXT("Drop keeps the root scale"), Box->GetActorScale3D().Equals(FVector(0.8f), 0.01f));
	Box->RecoverPhysicalCarryable(Carry);
	TestTrue(TEXT("Last-safe recovery keeps the root scale"), Box->GetActorScale3D().Equals(FVector(0.8f), 0.01f));

	return true;
}


namespace
{
FVector GetBoxAxis(const FQuat& Rotation, const int32 AxisIndex)
{
	switch (AxisIndex)
	{
	case 0: return Rotation.RotateVector(FVector::ForwardVector);
	case 1: return Rotation.RotateVector(FVector::RightVector);
	default: return Rotation.RotateVector(FVector::UpVector);
	}
}

float GetBoxProjectedRadius(const FVector& HalfExtent, const FQuat& Rotation, const FVector& Axis)
{
	return FMath::Abs(FVector::DotProduct(GetBoxAxis(Rotation, 0), Axis)) * HalfExtent.X
		+ FMath::Abs(FVector::DotProduct(GetBoxAxis(Rotation, 1), Axis)) * HalfExtent.Y
		+ FMath::Abs(FVector::DotProduct(GetBoxAxis(Rotation, 2), Axis)) * HalfExtent.Z;
}

float ComputeOrientedBoxPenetrationDepth(
	const FVector& CenterA,
	const FQuat& RotationA,
	const FVector& HalfExtentA,
	const FVector& CenterB,
	const FQuat& RotationB,
	const FVector& HalfExtentB)
{
	TArray<FVector, TInlineAllocator<15>> Axes;
	Axes.Reserve(15);
	for (int32 AxisIndex = 0; AxisIndex < 3; ++AxisIndex)
	{
		Axes.Add(GetBoxAxis(RotationA, AxisIndex));
		Axes.Add(GetBoxAxis(RotationB, AxisIndex));
	}
	for (int32 AxisIndexA = 0; AxisIndexA < 3; ++AxisIndexA)
	{
		for (int32 AxisIndexB = 0; AxisIndexB < 3; ++AxisIndexB)
		{
			const FVector CrossAxis = FVector::CrossProduct(
				GetBoxAxis(RotationA, AxisIndexA),
				GetBoxAxis(RotationB, AxisIndexB));
			if (!CrossAxis.IsNearlyZero())
			{
				Axes.Add(CrossAxis.GetSafeNormal());
			}
		}
	}

	const FVector CenterDelta = CenterB - CenterA;
	float MinimumOverlap = TNumericLimits<float>::Max();
	for (const FVector& Axis : Axes)
	{
		const float Overlap = GetBoxProjectedRadius(HalfExtentA, RotationA, Axis)
			+ GetBoxProjectedRadius(HalfExtentB, RotationB, Axis)
			- FMath::Abs(FVector::DotProduct(CenterDelta, Axis));
		MinimumOverlap = FMath::Min(MinimumOverlap, Overlap);
	}
	return MinimumOverlap;
}

// World::Tick dispatches each tick function once per GFrameCounter value, so every
// simulated step must advance the frame counter or physics only integrates once.
void TickShopPhysicsStep(UWorld& World, const float StepSeconds)
{
	++GFrameCounter;
	World.Tick(LEVELTICK_All, StepSeconds);
}

// Sanity gate: a lone item in free fall must accelerate with world gravity before any
// overlap or bounce measurement from the same fixture is trusted.
bool RunShopFreeFallSanityGate(FAutomationTestBase& Test)
{
	FShopScatterAutomationWorld WorldFixture;
	if (!WorldFixture.Initialize(Test, TEXT("ShopFreeFallSanityWorld"), true))
	{
		return false;
	}
	UWorld& World = *WorldFixture.World;
	UFacilityPlacementDefinition* Definition = LoadShopDefinition(Test, 1);
	if (!Definition)
	{
		return false;
	}
	const FTransform Transform(FRotator::ZeroRotator, FVector(0.0f, 0.0f, 5000.0f));
	FText FailureReason;
	APlaceableFacilityItemActor* Item = APlaceableFacilityItemActor::SpawnFreshItem(
		World, *Definition, Transform, FailureReason);
	if (!Item || !Item->ActivateFreeWorld(Transform, FailureReason))
	{
		Test.AddError(FString::Printf(TEXT("Free-fall sanity item could not be activated: %s"), *FailureReason.ToString()));
		return false;
	}
	UPrimitiveComponent* Primitive = Item->GetPhysicalCarryPrimitive();
	if (!Test.TestNotNull(TEXT("Free-fall sanity item has a physical primitive"), Primitive)
		|| !Test.TestTrue(TEXT("Free-fall sanity item simulates physics"), Primitive->IsSimulatingPhysics()))
	{
		return false;
	}

	constexpr int32 StepCount = 30;
	constexpr float StepSeconds = 1.0f / 60.0f;
	const float StartZ = Item->GetActorLocation().Z;
	for (int32 Step = 0; Step < StepCount; ++Step)
	{
		TickShopPhysicsStep(World, StepSeconds);
	}
	const float ElapsedSeconds = StepCount * StepSeconds;
	const float GravityZ = World.GetGravityZ();
	const float ExpectedSpeed = FMath::Abs(GravityZ) * ElapsedSeconds;
	const float ExpectedDrop = 0.5f * FMath::Abs(GravityZ) * ElapsedSeconds * ElapsedSeconds;
	const float MeasuredDownSpeed = -Primitive->GetPhysicsLinearVelocity().Z;
	const float MeasuredDrop = StartZ - Item->GetActorLocation().Z;
	Test.AddInfo(FString::Printf(
		TEXT("UnboxingPhysics sanity gravity=%.1fcm/s2 after=%.3fs downSpeed=%.3fcm/s expected=%.3f drop=%.3fcm expected=%.3f"),
		GravityZ, ElapsedSeconds, MeasuredDownSpeed, ExpectedSpeed, MeasuredDrop, ExpectedDrop));
	const bool bSpeedMatches = FMath::Abs(MeasuredDownSpeed - ExpectedSpeed) <= ExpectedSpeed * 0.2f;
	const bool bDropMatches = FMath::Abs(MeasuredDrop - ExpectedDrop) <= ExpectedDrop * 0.25f;
	Test.TestTrue(TEXT("Sanity gate: free-fall speed follows world gravity within 20%"), bSpeedMatches);
	Test.TestTrue(TEXT("Sanity gate: free-fall distance follows world gravity within 25%"), bDropMatches);
	return bSpeedMatches && bDropMatches;
}

float RunShopOverlapPhysicsCase(
	FAutomationTestBase& Test,
	const float DepthCm,
	const int32 Seed,
	float& OutInitialDepth,
	float& OutFinalDepth,
	float& OutPeakSeparationSpeed,
	float& OutEarlyPeakSeparationSpeed)
{
	OutInitialDepth = -TNumericLimits<float>::Max();
	OutEarlyPeakSeparationSpeed = 0.0f;
	FShopScatterAutomationWorld WorldFixture;
	if (!WorldFixture.Initialize(Test, TEXT("ShopOverlapPhysicsWorld"), true))
	{
		return 0.0f;
	}
	UWorld& World = *WorldFixture.World;
	UFacilityPlacementDefinition* Definition = LoadShopDefinition(Test, 1);
	if (!Definition)
	{
		return 0.0f;
	}
	FVector HalfExtent = FVector::ZeroVector;
	if (!GetDefinitionHalfExtent(Test, *Definition, HalfExtent))
	{
		return 0.0f;
	}
	FRandomStream Stream(Seed);
	TArray<FShopUnboxClusterItem> Layout;
	TArray<FVector> PairExtents = { HalfExtent, HalfExtent };
	Test.TestTrue(FString::Printf(TEXT("Two-item physics layout builds at D=%.0f"), DepthCm),
		FShopUnboxingCluster::BuildLayout(PairExtents, DepthCm, ShopUnboxTest::MakeTuning().Cluster, Stream, Layout));
	if (Layout.Num() != 2)
	{
		return 0.0f;
	}

	const APlaceableFacilityItemActor* ItemCDO = Definition->RecoveryItemClass
		? Definition->RecoveryItemClass->GetDefaultObject<APlaceableFacilityItemActor>()
		: nullptr;
	if (!ItemCDO)
	{
		Test.AddError(TEXT("Physics test recovery item CDO is missing."));
		return 0.0f;
	}
	FVector ItemScale = FVector::OneVector;
	if (!GetShopItemTemplateScale(Test, *Definition, ItemScale))
	{
		return 0.0f;
	}
	TArray<APlaceableFacilityItemActor*> Items;
	for (int32 Index = 0; Index < Layout.Num(); ++Index)
	{
		// Layout centers are collision-shape centers. Convert them to actor origins exactly like
		// FShopUnboxingPlacement does: actor = shapeCenter - R * (scale * boundsOrigin).
		const FRotator LayoutRotation(0.0f, Layout[Index].YawDegrees, 0.0f);
		FVector RelativeShapeCenter = FVector::ZeroVector;
		FQuat ProbeRotation = FQuat::Identity;
		FCollisionShape ProbeShape;
		if (!BuildCollisionQuery(*Definition, FTransform(LayoutRotation, FVector::ZeroVector, ItemScale),
			RelativeShapeCenter, ProbeRotation, ProbeShape))
		{
			Test.AddError(TEXT("Physics test could not resolve the relative collision shape center."));
			return 0.0f;
		}
		const FVector ShapeCenter = FVector(0.0f, 0.0f, 5000.0f) + Layout[Index].Center;
		const FTransform Transform(ProbeRotation, ShapeCenter - RelativeShapeCenter, ItemScale);
		FText FailureReason;
		APlaceableFacilityItemActor* Item = APlaceableFacilityItemActor::SpawnFreshItem(
			World, *Definition, Transform, FailureReason);
		Test.TestNotNull(TEXT("Physics test creates a real shower item"), Item);
		if (!Item || !Item->ActivateFreeWorld(Transform, FailureReason))
		{
			Test.AddError(FailureReason.ToString());
			return 0.0f;
		}
		Items.Add(Item);
	}

	// Initial overlap sanity: the spawned bodies must overlap by the requested layout depth,
	// otherwise the bounce measurements below say nothing about depenetration.
	{
		FVector InitialCenters[2] = { FVector::ZeroVector, FVector::ZeroVector };
		FQuat InitialRotations[2] = { FQuat::Identity, FQuat::Identity };
		FCollisionShape InitialShapes[2];
		bool bInitialQueriesResolved = true;
		for (int32 Index = 0; Index < Items.Num(); ++Index)
		{
			const FVector ActorScale = Items[Index]->GetActorScale3D();
			Test.AddInfo(FString::Printf(
				TEXT("UnboxingPhysics D=%.0fcm item=%d actorScale=(%.4f,%.4f,%.4f) templateScale=(%.4f,%.4f,%.4f)"),
				DepthCm, Index, ActorScale.X, ActorScale.Y, ActorScale.Z, ItemScale.X, ItemScale.Y, ItemScale.Z));
			Test.TestTrue(FString::Printf(TEXT("Physics item %d spawns at its template scale"), Index),
				ActorScale.Equals(ItemScale, 1.0e-3f));
			bInitialQueriesResolved &= BuildCollisionQuery(
				*Definition, Items[Index]->GetActorTransform(),
				InitialCenters[Index], InitialRotations[Index], InitialShapes[Index]);
		}
		if (bInitialQueriesResolved)
		{
			OutInitialDepth = ComputeOrientedBoxPenetrationDepth(
				InitialCenters[0], InitialRotations[0], InitialShapes[0].GetExtent().GetAbs(),
				InitialCenters[1], InitialRotations[1], InitialShapes[1].GetExtent().GetAbs());
		}
		// The architecture defines the effective pair target as min(requested D, half the
		// smaller half-extent). The corrected 0.6 shower scale caps D=20 at 15cm.
		const float ExpectedInitialDepth = FMath::Min(DepthCm, ShopUnboxTest::MakeTuning().Cluster.PairDepthExtentRatio * HalfExtent.GetMin());
		Test.TestTrue(FString::Printf(TEXT("Initial overlap sanity: effective target %.1fcm (requested D=%.1fcm, measured %.3fcm)"),
			ExpectedInitialDepth, DepthCm, OutInitialDepth),
			bInitialQueriesResolved && FMath::Abs(OutInitialDepth - ExpectedInitialDepth) <= 0.5f);
	}

	float MaxLinearSpeed = 0.0f;
	OutPeakSeparationSpeed = 0.0f;
	for (int32 Step = 0; Step < 30; ++Step)
	{
		TickShopPhysicsStep(World, 1.0f / 60.0f);
		FVector TickCenters[2] = { FVector::ZeroVector, FVector::ZeroVector };
		FQuat TickRotations[2] = { FQuat::Identity, FQuat::Identity };
		FCollisionShape TickShapes[2];
		UPrimitiveComponent* Primitives[2] = { nullptr, nullptr };
		bool bTickQueriesResolved = true;
		for (int32 Index = 0; Index < Items.Num(); ++Index)
		{
			Primitives[Index] = Items[Index] ? Items[Index]->GetPhysicalCarryPrimitive() : nullptr;
			if (Primitives[Index])
			{
				MaxLinearSpeed = FMath::Max(MaxLinearSpeed, Primitives[Index]->GetPhysicsLinearVelocity().Size());
			}
			bTickQueriesResolved &= Items[Index] && BuildCollisionQuery(
				*Definition, Items[Index]->GetActorTransform(),
				TickCenters[Index], TickRotations[Index], TickShapes[Index]);
		}
		if (bTickQueriesResolved && Primitives[0] && Primitives[1])
		{
			const FVector SeparationAxis = (TickCenters[1] - TickCenters[0]).GetSafeNormal();
			const FVector RelativeVelocity = Primitives[1]->GetPhysicsLinearVelocity()
				- Primitives[0]->GetPhysicsLinearVelocity();
			const float SeparationSpeed = FMath::Max(0.0f, FVector::DotProduct(RelativeVelocity, SeparationAxis));
			OutPeakSeparationSpeed = FMath::Max(OutPeakSeparationSpeed, SeparationSpeed);
			if (Step < 3)
			{
				// Initial depenetration is resolved in the first solver steps.
				OutEarlyPeakSeparationSpeed = FMath::Max(OutEarlyPeakSeparationSpeed, SeparationSpeed);
			}
		}
	}

	FVector Centers[2] = { FVector::ZeroVector, FVector::ZeroVector };
	FQuat Rotations[2] = { FQuat::Identity, FQuat::Identity };
	FCollisionShape Shapes[2];
	for (int32 Index = 0; Index < Items.Num(); ++Index)
	{
		Test.TestTrue(TEXT("Physics result retains a definition collision query"),
			BuildCollisionQuery(*Definition, Items[Index]->GetActorTransform(), Centers[Index], Rotations[Index], Shapes[Index]));
	}
	OutFinalDepth = ComputeOrientedBoxPenetrationDepth(
		Centers[0], Rotations[0], Shapes[0].GetExtent().GetAbs(),
		Centers[1], Rotations[1], Shapes[1].GetExtent().GetAbs());
	return MaxLinearSpeed;
}

UBoxComponent* AddShopRoomBlocker(
	UWorld& World,
	const TCHAR* Name,
	const FVector& Center,
	const FVector& HalfExtent,
	const FRotator& Rotation = FRotator::ZeroRotator)
{
	AActor* Owner = World.SpawnActor<AActor>(
		AActor::StaticClass(), FTransform(Rotation, Center));
	if (!Owner)
	{
		return nullptr;
	}
	UBoxComponent* Component = NewObject<UBoxComponent>(Owner, Name);
	Owner->SetRootComponent(Component);
	Owner->AddInstanceComponent(Component);
	Component->SetBoxExtent(HalfExtent);
	Component->SetCollisionObjectType(ECC_WorldStatic);
	Component->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	Component->SetCollisionResponseToAllChannels(ECR_Block);
	Component->RegisterComponent();
	Owner->SetActorTransform(FTransform(Rotation, Center), false, nullptr, ETeleportType::TeleportPhysics);
	Component->UpdateComponentToWorld();
	return Component;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShopUnboxingEnvironmentClearanceAutomationTest,
	"BathhouseSim.Shop.UnboxingEnvironmentClearance",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShopUnboxingEnvironmentClearanceAutomationTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	UFacilityPlacementDefinition* Definition = LoadShopDefinition(*this, 1);
	if (!Definition)
	{
		return false;
	}
	FShopScatterAutomationWorld WorldFixture;
	if (!WorldFixture.Initialize(*this, TEXT("ShopUnboxingEnvironmentClearanceWorld")))
	{
		return false;
	}
	UWorld& World = *WorldFixture.World;
	ACharacter* Player = World.SpawnActor<ACharacter>(
		ACharacter::StaticClass(), FVector(0.0f, 0.0f, 1000.0f), FRotator::ZeroRotator);
	AShopDeliveryBoxActor* Box = World.SpawnActor<AShopDeliveryBoxActor>(
		AShopDeliveryBoxActor::StaticClass(), FVector(9000.0f, 9000.0f, 1000.0f), FRotator::ZeroRotator);
	TestNotNull(TEXT("Player exists for the environment-clearance test"), Player);
	TestNotNull(TEXT("Delivery box exists for the environment-clearance test"), Box);
	if (!Player || !Box || !Player->GetCapsuleComponent())
	{
		return false;
	}

	const FVector FootLocation = Player->GetActorLocation()
		- FVector::UpVector * Player->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	TestNotNull(TEXT("Environment-clearance test creates a broad open floor"),
		AddShopRoomBlocker(World, TEXT("ShopClearanceFloor"),
			FootLocation - FVector::UpVector * 50.0f, FVector(10000.0f, 10000.0f, 50.0f)));
	TArray<UFacilityPlacementDefinition*> Definitions = { Definition };
	constexpr float ForwardDistanceCm = 500.0f;
	constexpr float ClearanceCm = 8.0f;
	constexpr int32 LayoutSeed = 71129;
	auto FindBaseline = [&]()
	{
		TArray<FTransform> Result;
		FRandomStream Stream(LayoutSeed);
		FText FailureReason;
		TestTrue(TEXT("Clear environment accepts the baseline front candidate"),
			FShopUnboxingPlacement::FindSpawnTransforms(
		World,
		*Player,
		*Player->GetCapsuleComponent(),
		*Box,
		ShopUnboxTest::MakeShapes(Definitions),
		ShopUnboxTest::MakeRequest(*Player),
		ShopUnboxTest::MakeFloorStageTuning(ForwardDistanceCm, ClearanceCm),
		Stream,
		Result,
		FailureReason));
		if (Result.Num() != 1 && !FailureReason.IsEmpty())
		{
			AddError(FailureReason.ToString());
		}
		return Result;
	};
	auto ResolveBaselineShape = [&](const TArray<FTransform>& Transforms,
		FVector& OutCenter, FQuat& OutRotation, FCollisionShape& OutShape,
		const UPrimitiveComponent*& OutCollisionTemplate)
	{
		FText FailureReason;
		return Transforms.Num() == 1
			&& APlaceableFacilityItemActor::BuildDefinitionCollisionQuery(
				*Definition, Transforms[0], OutCenter, OutRotation, OutShape,
				OutCollisionTemplate, FailureReason);
	};

	TArray<FTransform> BaselineTransforms = FindBaseline();
	FVector BaselineCenter = FVector::ZeroVector;
	FQuat BaselineRotation = FQuat::Identity;
	FCollisionShape BaselineShape;
	const UPrimitiveComponent* CollisionTemplate = nullptr;
	TestTrue(TEXT("Baseline candidate resolves its collision query"),
		ResolveBaselineShape(BaselineTransforms, BaselineCenter, BaselineRotation, BaselineShape, CollisionTemplate));
	if (!CollisionTemplate)
	{
		return false;
	}
	FCollisionQueryParams EnvironmentParams(SCENE_QUERY_STAT(ShopClearanceEnvironmentAssertion), false);
	EnvironmentParams.AddIgnoredActor(Player);
	EnvironmentParams.AddIgnoredActor(Box);
	const FVector BaseExtent = BaselineShape.GetExtent().GetAbs();
	const FCollisionShape ClearanceShape = MakeUnboxingClearanceShape(BaselineShape, ClearanceCm);
	const FVector ClearanceCenter = MakeUnboxingClearanceCenter(BaselineCenter, ClearanceCm);

	const FVector SideWallCenter = BaselineCenter + BaselineRotation.RotateVector(
		FVector(0.0f, BaseExtent.Y + ClearanceCm * 0.5f + 1.0f, 0.0f));
	UBoxComponent* SideWall = AddShopRoomBlocker(
		World, TEXT("ShopUnboxSideClearanceWall"), SideWallCenter,
		FVector(1000.0f, 1.0f, 400.0f), BaselineRotation.Rotator());
	TestNotNull(TEXT("Side wall is placed inside horizontal Dc clearance"), SideWall);
	if (!SideWall)
	{
		return false;
	}
	SideWall->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);
	TestFalse(TEXT("Side wall does not overlap the original item shape"),
		FacilityPlacementCollision::HasBlockingOverlap(
			World, BaselineCenter, BaselineRotation, BaselineShape, *CollisionTemplate, EnvironmentParams));
	TestTrue(TEXT("Side wall overlaps the horizontally expanded clearance shape"),
		FacilityPlacementCollision::HasBlockingOverlap(
			World, ClearanceCenter, BaselineRotation, ClearanceShape, *CollisionTemplate, EnvironmentParams));

	TArray<FTransform> SideSafeTransforms;
	FRandomStream SideSafeStream(LayoutSeed);
	FText FailureReason;
	TestTrue(TEXT("Placement rejects or relocates a layout near a side wall"),
		FShopUnboxingPlacement::FindSpawnTransforms(
		World,
		*Player,
		*Player->GetCapsuleComponent(),
		*Box,
		ShopUnboxTest::MakeShapes(Definitions),
		ShopUnboxTest::MakeRequest(*Player),
		ShopUnboxTest::MakeFloorStageTuning(ForwardDistanceCm, ClearanceCm),
		SideSafeStream,
		SideSafeTransforms,
		FailureReason));
	if (SideSafeTransforms.Num() != 1)
	{
		AddError(FailureReason.ToString());
		return false;
	}
	FVector SideSafeCenter = FVector::ZeroVector;
	FQuat SideSafeRotation = FQuat::Identity;
	FCollisionShape SideSafeShape;
	const UPrimitiveComponent* SideSafeTemplate = nullptr;
	TestTrue(TEXT("Relocated side-wall candidate resolves its collision query"),
		APlaceableFacilityItemActor::BuildDefinitionCollisionQuery(
			*Definition, SideSafeTransforms[0], SideSafeCenter, SideSafeRotation,
			SideSafeShape, SideSafeTemplate, FailureReason));
	TestFalse(TEXT("Returned candidate clears the wall with Dc padding"),
		FacilityPlacementCollision::HasBlockingOverlap(
			World,
			MakeUnboxingClearanceCenter(SideSafeCenter, ClearanceCm),
			SideSafeRotation,
			MakeUnboxingClearanceShape(SideSafeShape, ClearanceCm),
			*SideSafeTemplate,
			EnvironmentParams));
	SideWall->GetOwner()->Destroy();

	BaselineTransforms = FindBaseline();
	BaselineCenter = FVector::ZeroVector;
	BaselineRotation = FQuat::Identity;
	BaselineShape = FCollisionShape();
	CollisionTemplate = nullptr;
	TestTrue(TEXT("Ceiling baseline candidate resolves its collision query"),
		ResolveBaselineShape(BaselineTransforms, BaselineCenter, BaselineRotation, BaselineShape, CollisionTemplate));
	if (!CollisionTemplate)
	{
		return false;
	}
	const float CeilingBottom = BaselineCenter.Z + BaselineShape.GetExtent().Z + ClearanceCm * 0.5f;
	constexpr float CeilingHalfThickness = 1.0f;
	UBoxComponent* Ceiling = AddShopRoomBlocker(
		World, TEXT("ShopUnboxUpperClearanceCeiling"),
		FVector(BaselineCenter.X, BaselineCenter.Y, CeilingBottom + CeilingHalfThickness),
		FVector(2000.0f, 2000.0f, CeilingHalfThickness));
	TestNotNull(TEXT("Ceiling is placed inside the upper Dc clearance"), Ceiling);
	if (!Ceiling)
	{
		return false;
	}
	Ceiling->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);
	TestFalse(TEXT("Ceiling does not overlap the original item shape"),
		FacilityPlacementCollision::HasBlockingOverlap(
			World, BaselineCenter, BaselineRotation, BaselineShape, *CollisionTemplate, EnvironmentParams));
	TestTrue(TEXT("Ceiling overlaps the upward-expanded clearance shape"),
		FacilityPlacementCollision::HasBlockingOverlap(
			World,
			MakeUnboxingClearanceCenter(BaselineCenter, ClearanceCm),
			BaselineRotation,
			MakeUnboxingClearanceShape(BaselineShape, ClearanceCm),
			*CollisionTemplate,
			EnvironmentParams));

	TArray<FTransform> CeilingSafeTransforms;
	FRandomStream CeilingSafeStream(LayoutSeed);
	TestTrue(TEXT("Placement rejects or relocates a layout below the ceiling"),
		FShopUnboxingPlacement::FindSpawnTransforms(
		World,
		*Player,
		*Player->GetCapsuleComponent(),
		*Box,
		ShopUnboxTest::MakeShapes(Definitions),
		ShopUnboxTest::MakeRequest(*Player),
		ShopUnboxTest::MakeFloorStageTuning(ForwardDistanceCm, ClearanceCm),
		CeilingSafeStream,
		CeilingSafeTransforms,
		FailureReason));
	if (CeilingSafeTransforms.Num() != 1)
	{
		AddError(FailureReason.ToString());
		return false;
	}
	FVector CeilingSafeCenter = FVector::ZeroVector;
	FQuat CeilingSafeRotation = FQuat::Identity;
	FCollisionShape CeilingSafeShape;
	const UPrimitiveComponent* CeilingSafeTemplate = nullptr;
	TestTrue(TEXT("Relocated ceiling candidate resolves its collision query"),
		APlaceableFacilityItemActor::BuildDefinitionCollisionQuery(
			*Definition, CeilingSafeTransforms[0], CeilingSafeCenter, CeilingSafeRotation,
			CeilingSafeShape, CeilingSafeTemplate, FailureReason));
	TestFalse(TEXT("Returned candidate clears the ceiling with Dc padding"),
		FacilityPlacementCollision::HasBlockingOverlap(
			World,
			MakeUnboxingClearanceCenter(CeilingSafeCenter, ClearanceCm),
			CeilingSafeRotation,
			MakeUnboxingClearanceShape(CeilingSafeShape, ClearanceCm),
			*CeilingSafeTemplate,
			EnvironmentParams));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShopUnboxingDepthPlacementObservationTest,
	"BathhouseSim.Shop.UnboxingDepthPlacementObservation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShopUnboxingDepthPlacementObservationTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FShopScatterAutomationWorld WorldFixture;
	if (!WorldFixture.Initialize(*this, TEXT("ShopUnboxingDepthObservationWorld")))
	{
		return false;
	}
	UWorld& World = *WorldFixture.World;
	UFacilityPlacementDefinition* Definition = LoadShopDefinition(*this, 1);
	ACharacter* Player = World.SpawnActor<ACharacter>(
		ACharacter::StaticClass(), FVector(0.0f, 0.0f, 1000.0f), FRotator::ZeroRotator);
	AShopDeliveryBoxActor* Box = World.SpawnActor<AShopDeliveryBoxActor>(
		AShopDeliveryBoxActor::StaticClass(), FVector(9000.0f, 9000.0f, 1000.0f), FRotator::ZeroRotator);
	TestNotNull(TEXT("Open-floor depth observation has a shower definition"), Definition);
	TestNotNull(TEXT("Open-floor depth observation has a player"), Player);
	TestNotNull(TEXT("Open-floor depth observation has a delivery box"), Box);
	if (!Definition || !Player || !Box || !Player->GetCapsuleComponent())
	{
		return false;
	}

	const FVector FootLocation = Player->GetActorLocation()
		- FVector::UpVector * Player->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	TestNotNull(TEXT("Open-floor observation creates a large clear floor"), AddShopRoomBlocker(
		World, TEXT("ShopDepthObservationFloor"),
		FootLocation - FVector::UpVector * 50.0f,
		FVector(10000.0f, 10000.0f, 50.0f)));
	TArray<UFacilityPlacementDefinition*> Definitions = { Definition };
	const FVector Forward = FRotationMatrix(FRotator(0.0f, 0.0f, 0.0f)).GetUnitAxis(EAxis::X);
	// Overlap depth sweep: both property clamp ends from the property metadata plus values in between.
	const FProperty* DepthProperty = UShopSettings::StaticClass()->FindPropertyByName(TEXT("UnboxOverlapDepthCm"));
	const float DepthMin = DepthProperty ? FCString::Atof(*DepthProperty->GetMetaData(TEXT("ClampMin"))) : 0.0f;
	const float DepthMax = DepthProperty ? FCString::Atof(*DepthProperty->GetMetaData(TEXT("ClampMax"))) : 0.0f;
	TestTrue(TEXT("Overlap depth property exposes a clamp range"), DepthMax > DepthMin);
	const TArray<float> DepthSweep = {
		DepthMin, DepthMin + (DepthMax - DepthMin) * 0.16f, DepthMin + (DepthMax - DepthMin) * 0.4f,
		DepthMin + (DepthMax - DepthMin) * 0.6f, DepthMax };
	for (const float ForwardDistanceCm : { 100.0f, 500.0f })
	{
		for (const float DepthCm : DepthSweep)
		{
			FRandomStream Stream(FMath::RoundToInt(DepthCm * 100.0f + ForwardDistanceCm * 10.0f) + 31415);
			TArray<FTransform> SpawnTransforms;
			FText FailureReason;
			const bool bFound = FShopUnboxingPlacement::FindSpawnTransforms(
		World,
		*Player,
		*Player->GetCapsuleComponent(),
		*Box,
		ShopUnboxTest::MakeShapes(Definitions),
		ShopUnboxTest::MakeRequest(*Player),
		ShopUnboxTest::MakeFloorStageTuning(ForwardDistanceCm, DepthCm),
		Stream,
		SpawnTransforms,
		FailureReason);
			TestTrue(FString::Printf(TEXT("Open-floor placement succeeds at D=%.0f cm and forward distance %.0f cm"),
				DepthCm, ForwardDistanceCm), bFound);
			if (!bFound || SpawnTransforms.Num() != 1)
			{
				if (!FailureReason.IsEmpty())
				{
					AddError(FailureReason.ToString());
				}
				continue;
			}

			FVector ShapeCenter = FVector::ZeroVector;
			FQuat ShapeRotation = FQuat::Identity;
			FCollisionShape Shape;
			if (!BuildCollisionQuery(*Definition, SpawnTransforms[0], ShapeCenter, ShapeRotation, Shape))
			{
				AddError(FString::Printf(TEXT("D=%.0f cm candidate has no definition collision query"), DepthCm));
				continue;
			}
			const float LowestBottom = ShapeCenter.Z - Shape.GetExtent().Z;
			const FVector ExpectedXYCenter = FootLocation + Forward * ForwardDistanceCm;
			TestTrue(FString::Printf(TEXT("D=%.0f cm at %.0f cm selects the front stage"),
				DepthCm, ForwardDistanceCm), FMath::IsNearlyEqual(LowestBottom, FootLocation.Z + ShopUnboxTest::MakeTuning().ForwardFloorClearanceCm, 0.5f));
			TestTrue(FString::Printf(TEXT("D=%.0f cm at %.0f cm keeps the requested front position"),
				DepthCm, ForwardDistanceCm),
				FVector::Dist2D(ShapeCenter, ExpectedXYCenter) <= 0.5f);
			AddInfo(FString::Printf(
				TEXT("UnboxingDepthPlacement D=%.0fcm forward=%.0fcm stage=front lowestBottom=%.2fcm expected=%.2fcm"),
				DepthCm, ForwardDistanceCm, LowestBottom,
				FootLocation.Z + ShopUnboxTest::MakeTuning().ForwardFloorClearanceCm));
		}
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FShopUnboxingPhysicsAutomationTest,
	"BathhouseSim.Shop.UnboxingPhysics",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShopUnboxingPhysicsAutomationTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	// Every bounce judgement below requires a world whose physics actually integrates.
	if (!RunShopFreeFallSanityGate(*this))
	{
		AddError(TEXT("UnboxingPhysics sanity gate failed: the test world did not simulate gravity, so overlap bounce results are not evaluated."));
		return false;
	}

	float DepthEightInitial = 0.0f;
	float DepthEightFinal = TNumericLimits<float>::Max();
	float DepthEightSeparationSpeed = 0.0f;
	float DepthEightEarlySeparationSpeed = 0.0f;
	const float DepthEightMaxSpeed = RunShopOverlapPhysicsCase(
		*this, 8.0f, 54321, DepthEightInitial, DepthEightFinal, DepthEightSeparationSpeed, DepthEightEarlySeparationSpeed);
	AddInfo(FString::Printf(TEXT("UnboxingPhysics D=8cm initialPenetration=%.3fcm after=0.5s penetration=%.3fcm peakLinearSpeed=%.3fcm/s peakSeparationSpeed=%.3fcm/s first3StepsSeparationSpeed=%.3fcm/s"),
		DepthEightInitial, DepthEightFinal, DepthEightMaxSpeed, DepthEightSeparationSpeed, DepthEightEarlySeparationSpeed));
	TestTrue(TEXT("Default overlap depth separates two items within half a second"), DepthEightFinal < 1.0f);
	// Both items free-fall, so absolute speed is dominated by gravity. The bounce contract is
	// judged on the relative velocity along the separation axis, where gravity cancels out.
	TestTrue(TEXT("Resolved overlap produces a nonzero separation velocity"), DepthEightSeparationSpeed > 0.1f);

	float DepthTwoInitial = 0.0f;
	float DepthTwentyInitial = 0.0f;
	float DepthTwoFinal = TNumericLimits<float>::Max();
	float DepthTwentyFinal = TNumericLimits<float>::Max();
	float DepthTwoSeparationSpeed = 0.0f;
	float DepthTwentySeparationSpeed = 0.0f;
	float DepthTwoEarlySeparationSpeed = 0.0f;
	float DepthTwentyEarlySeparationSpeed = 0.0f;
	const float DepthTwoMaxSpeed = RunShopOverlapPhysicsCase(
		*this, 2.0f, 10027, DepthTwoInitial, DepthTwoFinal, DepthTwoSeparationSpeed, DepthTwoEarlySeparationSpeed);
	const float DepthTwentyMaxSpeed = RunShopOverlapPhysicsCase(
		*this, 20.0f, 10027, DepthTwentyInitial, DepthTwentyFinal, DepthTwentySeparationSpeed, DepthTwentyEarlySeparationSpeed);
	AddInfo(FString::Printf(TEXT("UnboxingPhysics D=2cm initialPenetration=%.3fcm after=0.5s penetration=%.3fcm peakLinearSpeed=%.3fcm/s peakSeparationSpeed=%.3fcm/s first3StepsSeparationSpeed=%.3fcm/s"),
		DepthTwoInitial, DepthTwoFinal, DepthTwoMaxSpeed, DepthTwoSeparationSpeed, DepthTwoEarlySeparationSpeed));
	AddInfo(FString::Printf(TEXT("UnboxingPhysics D=20cm initialPenetration=%.3fcm after=0.5s penetration=%.3fcm peakLinearSpeed=%.3fcm/s peakSeparationSpeed=%.3fcm/s first3StepsSeparationSpeed=%.3fcm/s"),
		DepthTwentyInitial, DepthTwentyFinal, DepthTwentyMaxSpeed, DepthTwentySeparationSpeed, DepthTwentyEarlySeparationSpeed));
	TestTrue(TEXT("Higher initial overlap depth creates greater peak separation speed"),
		DepthTwentySeparationSpeed > DepthTwoSeparationSpeed);

	FShopScatterAutomationWorld RoomFixture;
	if (!RoomFixture.Initialize(*this, TEXT("ShopUnboxingRoomWorld"), true))
	{
		return false;
	}
	UWorld& RoomWorld = *RoomFixture.World;
	constexpr float InnerHalfWidth = 6000.0f;
	constexpr float InnerCeiling = 4000.0f;
	TestNotNull(TEXT("Room floor is created"), AddShopRoomBlocker(RoomWorld, TEXT("ShopRoomFloor"), FVector(0.0f, 0.0f, -50.0f), FVector(InnerHalfWidth, InnerHalfWidth, 50.0f)));
	TestNotNull(TEXT("Room ceiling is created"), AddShopRoomBlocker(RoomWorld, TEXT("ShopRoomCeiling"), FVector(0.0f, 0.0f, InnerCeiling + 50.0f), FVector(InnerHalfWidth, InnerHalfWidth, 50.0f)));
	TestNotNull(TEXT("Room north wall is created"), AddShopRoomBlocker(RoomWorld, TEXT("ShopRoomNorth"), FVector(InnerHalfWidth + 50.0f, 0.0f, InnerCeiling * 0.5f), FVector(50.0f, InnerHalfWidth, InnerCeiling * 0.5f)));
	TestNotNull(TEXT("Room south wall is created"), AddShopRoomBlocker(RoomWorld, TEXT("ShopRoomSouth"), FVector(-InnerHalfWidth - 50.0f, 0.0f, InnerCeiling * 0.5f), FVector(50.0f, InnerHalfWidth, InnerCeiling * 0.5f)));
	TestNotNull(TEXT("Room east wall is created"), AddShopRoomBlocker(RoomWorld, TEXT("ShopRoomEast"), FVector(0.0f, InnerHalfWidth + 50.0f, InnerCeiling * 0.5f), FVector(InnerHalfWidth, 50.0f, InnerCeiling * 0.5f)));
	TestNotNull(TEXT("Room west wall is created"), AddShopRoomBlocker(RoomWorld, TEXT("ShopRoomWest"), FVector(0.0f, -InnerHalfWidth - 50.0f, InnerCeiling * 0.5f), FVector(InnerHalfWidth, 50.0f, InnerCeiling * 0.5f)));

	ACharacter* Player = RoomWorld.SpawnActor<ACharacter>(
		ACharacter::StaticClass(), FVector::ZeroVector + FVector(0.0f, 0.0f, 1000.0f), FRotator::ZeroRotator);
	AShopDeliveryBoxActor* Box = RoomWorld.SpawnActor<AShopDeliveryBoxActor>(
		AShopDeliveryBoxActor::StaticClass(), FVector(30000.0f, 0.0f, 1000.0f), FRotator::ZeroRotator);
	TestNotNull(TEXT("Player is placed inside the closed physics room"), Player);
	TestNotNull(TEXT("Query delivery box is created outside the room"), Box);
	if (!Player || !Box || !Player->GetCapsuleComponent())
	{
		return false;
	}

	TArray<UFacilityPlacementDefinition*> Definitions;
	TArray<FVector> HalfExtents;
	for (int32 Index = 0; Index < 10; ++Index)
	{
		UFacilityPlacementDefinition* Definition = LoadShopDefinition(*this, Index % UE_ARRAY_COUNT(ShopDefinitionPaths));
		if (!Definition)
		{
			return false;
		}
		FVector HalfExtent = FVector::ZeroVector;
		if (!GetDefinitionHalfExtent(*this, *Definition, HalfExtent))
		{
			return false;
		}
		Definitions.Add(Definition);
		HalfExtents.Add(HalfExtent);
	}
	const FVector FootLocation = Player->GetActorLocation()
		- FVector::UpVector * Player->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	TArray<FTransform> SpawnTransforms;
	FRandomStream RoomStream(20260928);
	FText FailureReason;
	TestTrue(TEXT("Ten mixed items find a safe opening inside the closed room"), FShopUnboxingPlacement::FindSpawnTransforms(
		RoomWorld,
		*Player,
		*Player->GetCapsuleComponent(),
		*Box,
		ShopUnboxTest::MakeShapes(Definitions),
		ShopUnboxTest::MakeRequest(*Player),
		ShopUnboxTest::MakeFloorStageTuning(100.0f, 8.0f),
		RoomStream,
		SpawnTransforms,
		FailureReason));
	TestEqual(TEXT("Closed-room unboxing returns ten transforms"), SpawnTransforms.Num(), 10);
	TArray<APlaceableFacilityItemActor*> SpawnedItems;
	for (int32 Index = 0; Index < FMath::Min(Definitions.Num(), SpawnTransforms.Num()); ++Index)
	{
		APlaceableFacilityItemActor* Item = APlaceableFacilityItemActor::SpawnFreshItem(
			RoomWorld, *Definitions[Index], SpawnTransforms[Index], FailureReason);
		if (Item && Item->ActivateFreeWorld(SpawnTransforms[Index], FailureReason))
		{
			FVector TemplateScale = FVector::OneVector;
			if (!GetShopItemTemplateScale(*this, *Definitions[Index], TemplateScale))
			{
				return false;
			}
			const FVector ActorScale = Item->GetActorScale3D();
			AddInfo(FString::Printf(TEXT("UnboxingPhysics room item=%d actorScale=(%.4f,%.4f,%.4f) templateScale=(%.4f,%.4f,%.4f)"),
				Index, ActorScale.X, ActorScale.Y, ActorScale.Z, TemplateScale.X, TemplateScale.Y, TemplateScale.Z));
			TestTrue(FString::Printf(TEXT("Room item %d spawns at its template scale"), Index),
				ActorScale.Equals(TemplateScale, 1.0e-3f));
			SpawnedItems.Add(Item);
		}
		else
		{
			AddError(FailureReason.ToString());
		}
	}
	TestEqual(TEXT("All ten fresh items activate in the test room"), SpawnedItems.Num(), 10);
	TArray<float> SpawnBottomZ;
	SpawnBottomZ.Reserve(SpawnedItems.Num());
	for (int32 Index = 0; Index < SpawnedItems.Num(); ++Index)
	{
		FVector SpawnCenter = FVector::ZeroVector;
		FQuat SpawnRotation = FQuat::Identity;
		FCollisionShape SpawnShape;
		BuildCollisionQuery(*Definitions[Index], SpawnedItems[Index]->GetActorTransform(), SpawnCenter, SpawnRotation, SpawnShape);
		SpawnBottomZ.Add(SpawnCenter.Z - GetBoxProjectedRadius(SpawnShape.GetExtent().GetAbs(), SpawnRotation, FVector::UpVector));
	}
	for (int32 Step = 0; Step < 180; ++Step)
	{
		TickShopPhysicsStep(RoomWorld, 1.0f / 60.0f);
	}
	float LowestBottomZ = TNumericLimits<float>::Max();
	for (int32 Index = 0; Index < SpawnedItems.Num(); ++Index)
	{
		FVector Center = FVector::ZeroVector;
		FQuat Rotation = FQuat::Identity;
		FCollisionShape Shape;
		TestTrue(FString::Printf(TEXT("Room item %d keeps a collision query after ticking"), Index),
			BuildCollisionQuery(*Definitions[Index], SpawnedItems[Index]->GetActorTransform(), Center, Rotation, Shape));
		const FVector Extent = Shape.GetExtent().GetAbs();
		const float RadiusX = GetBoxProjectedRadius(Extent, Rotation, FVector::ForwardVector);
		const float RadiusY = GetBoxProjectedRadius(Extent, Rotation, FVector::RightVector);
		const float RadiusZ = GetBoxProjectedRadius(Extent, Rotation, FVector::UpVector);
		AddInfo(FString::Printf(TEXT("UnboxingPhysics room item=%d center=(%.2f,%.2f,%.2f) boundsMin=(%.2f,%.2f,%.2f) boundsMax=(%.2f,%.2f,%.2f) after=3.0s"),
			Index, Center.X, Center.Y, Center.Z,
			Center.X - RadiusX, Center.Y - RadiusY, Center.Z - RadiusZ,
			Center.X + RadiusX, Center.Y + RadiusY, Center.Z + RadiusZ));
		TestTrue(FString::Printf(TEXT("Room item %d remains inside the side walls"), Index),
			Center.X - RadiusX >= -InnerHalfWidth - 1.0f && Center.X + RadiusX <= InnerHalfWidth + 1.0f
			&& Center.Y - RadiusY >= -InnerHalfWidth - 1.0f && Center.Y + RadiusY <= InnerHalfWidth + 1.0f);
		TestTrue(FString::Printf(TEXT("Room item %d remains above the floor and below the ceiling"), Index),
			Center.Z - RadiusZ >= -5.0f && Center.Z + RadiusZ <= InnerCeiling + 5.0f);
		// Items spawn roughly 900cm above the floor; after 3s of gravity each must have fallen.
		if (SpawnBottomZ.IsValidIndex(Index))
		{
			TestTrue(FString::Printf(TEXT("Room item %d fell under gravity (spawn bottom %.1fcm, now %.1fcm)"),
				Index, SpawnBottomZ[Index], Center.Z - RadiusZ),
				Center.Z - RadiusZ < SpawnBottomZ[Index] - 200.0f);
		}
		LowestBottomZ = FMath::Min(LowestBottomZ, Center.Z - RadiusZ);
	}
	AddInfo(FString::Printf(TEXT("UnboxingPhysics room lowestBottom=%.2fcm floorTop=0.00cm after=3.0s"), LowestBottomZ));
	TestTrue(TEXT("Room items settle on the floor (lowest collision bottom within 5cm of the floor top)"),
		SpawnedItems.Num() > 0 && FMath::Abs(LowestBottomZ) <= 5.0f);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
