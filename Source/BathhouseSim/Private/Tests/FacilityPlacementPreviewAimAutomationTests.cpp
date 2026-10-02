#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Camera/CameraComponent.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Facility/BathhouseFacilityActor.h"
#include "Interaction/PlayerCarryComponent.h"
#include "Interaction/PlayerInteractionComponent.h"
#include "Materials/Material.h"
#include "Placement/FacilityActorConversionTransaction.h"
#include "Placement/FacilityPlacementTypes.h"
#include "Placement/FacilityPlacementDefinition.h"
#include "Placement/FacilityPlacementPreviewActor.h"
#include "Placement/FacilityPlacementSettings.h"
#include "Placement/FacilityPlacementZoneActor.h"
#include "Placement/PlaceableFacilityItemActor.h"
#include "Placement/PlayerFacilityPlacementComponent.h"
#include "Tests/FacilityPlacementAutomationTestProbe.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBathhouseFacilityPlacementPreviewAimTest,
	"BathhouseSim.Placement.PreviewHiddenWithoutAim",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBathhouseFacilityPlacementPreviewAimTest::RunTest(const FString& Parameters)
{
	// EXP-050·051·052: 놓을 후보 위치가 없으면 미리보기를 숨기고(파괴하지 않음), 후보가 있으면 보인다.
	(void)Parameters;
	if (!GEngine)
	{
		AddError(TEXT("GEngine is required."));
		return false;
	}
	UFacilityPlacementSettings* Settings = GetMutableDefault<UFacilityPlacementSettings>();
	const TSoftObjectPtr<UMaterialInterface> SavedValid = Settings->ValidPreviewMaterial;
	const TSoftObjectPtr<UMaterialInterface> SavedInvalid = Settings->InvalidPreviewMaterial;
	const float SavedGrid = Settings->GridSizeCm;
	UMaterial* ValidMaterial = NewObject<UMaterial>();
	UMaterial* InvalidMaterial = NewObject<UMaterial>();
	ValidMaterial->BlendMode = BLEND_Translucent;
	InvalidMaterial->BlendMode = BLEND_Translucent;
	Settings->ValidPreviewMaterial = ValidMaterial;
	Settings->InvalidPreviewMaterial = InvalidMaterial;
	Settings->GridSizeCm = 10.0f;

	const FName WorldName = MakeUniqueObjectName(nullptr, UWorld::StaticClass(), TEXT("PlacementPreviewAimWorld"));
	FWorldContext& WorldContext = GEngine->CreateNewWorldContext(EWorldType::Game);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, WorldName, GetTransientPackage());
	if (!World)
	{
		Settings->ValidPreviewMaterial = SavedValid;
		Settings->InvalidPreviewMaterial = SavedInvalid;
		Settings->GridSizeCm = SavedGrid;
		AddError(TEXT("Failed to create the preview aim world."));
		return false;
	}
	World->AddToRoot();
	WorldContext.SetCurrentWorld(World);
	World->InitializeActorsForPlay(FURL());
	World->BeginPlay();

	const FGameplayTag PlacementTag = FGameplayTag::RequestGameplayTag(TEXT("Facility.Placeable"));
	UFacilityPlacementDefinition* Definition = NewObject<UFacilityPlacementDefinition>();
	Definition->StableId = TEXT("PreviewAimShower");
	Definition->FacilityTags.AddTag(PlacementTag);
	Definition->PlacedFacilityClass = AFacilityPlacementAutomationActor::StaticClass();
	Definition->RecoveryItemClass = AFacilityPlacementItemAutomationActor::StaticClass();
	Definition->RecoveryItemMesh = nullptr;
	Definition->LockerSlotCount = 0;

	AFacilityPlacementAutomationActor* Source = World->SpawnActorDeferred<AFacilityPlacementAutomationActor>(
		AFacilityPlacementAutomationActor::StaticClass(), FTransform(FVector(6000.0f, 0.0f, 100.0f)));
	Source->ConfigureForTest(*Definition, EBathhouseFacilityType::Shower, 17);
	Source->FinishSpawning(FTransform(FVector(6000.0f, 0.0f, 100.0f)));
	if (!Source->HasActorBegunPlay())
	{
		Source->DispatchBeginPlay();
	}
	FText Failure;
	APlaceableFacilityItemActor* Item = FFacilityActorConversionTransaction::RecoverFacilityToItem(*Source, Failure);
	if (!TestNotNull(TEXT("A facility item is created for the preview"), Item))
	{
		World->DestroyWorld(false);
		GEngine->DestroyWorldContext(World);
		World->RemoveFromRoot();
		Settings->ValidPreviewMaterial = SavedValid;
		Settings->InvalidPreviewMaterial = SavedInvalid;
		Settings->GridSizeCm = SavedGrid;
		return false;
	}

	const FVector ZoneCenter(5000.0f, 1000.0f, 0.0f);
	UStaticMesh* GridMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Plane.Plane"));
	UMaterial* GridMaterial = NewObject<UMaterial>();
	auto SpawnZone = [&](const FVector& Location)
	{
		AFacilityPlacementZoneAutomationActor* Zone = World->SpawnActor<AFacilityPlacementZoneAutomationActor>(
			AFacilityPlacementZoneAutomationActor::StaticClass(), Location, FRotator::ZeroRotator);
		Zone->GetZoneBounds()->SetBoxExtent(FVector(100.0f, 100.0f, 5.0f));
		Zone->MarkLevelAuthoredForTest();
		Zone->ConfigureGridForTest(GridMesh, GridMaterial);
		return Zone;
	};
	auto SpawnBlockingBox = [&](const FVector& Location, const FVector& Extent, const ECollisionChannel ObjectType, const bool bIgnoreZoneTrace = false)
	{
		AActor* Actor = World->SpawnActor<AActor>();
		UBoxComponent* Box = NewObject<UBoxComponent>(Actor, TEXT("Box"));
		Actor->AddInstanceComponent(Box);
		Actor->SetRootComponent(Box);
		Box->SetBoxExtent(Extent);
		Box->SetCollisionObjectType(ObjectType);
		Box->SetCollisionResponseToAllChannels(ECR_Block);
		if (bIgnoreZoneTrace)
		{
			Box->SetCollisionResponseToChannel(BathhousePlacementCollision::ZoneTraceChannel, ECR_Ignore);
		}
		Box->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		Box->RegisterComponent();
		Actor->SetActorLocation(Location);
		return Actor;
	};
	AFacilityPlacementZoneAutomationActor* Zone = SpawnZone(ZoneCenter);
	SpawnBlockingBox(ZoneCenter + FVector(0.0f, 0.0f, -5.0f), FVector(100.0f, 100.0f, 5.0f), ECC_WorldStatic);
	// 공간 불허 구역: 허용 태그가 설비 정의 태그와 달라 후보는 있지만 거절된다.
	AFacilityPlacementZoneAutomationActor* DeniedZone = SpawnZone(FVector(5500.0f, 1000.0f, 0.0f));
	DeniedZone->AddAllowedTag(FGameplayTag::RequestGameplayTag(TEXT("Facility.Type.Bath")));
	SpawnBlockingBox(FVector(5500.0f, 1000.0f, -5.0f), FVector(100.0f, 100.0f, 5.0f), ECC_WorldStatic);

	AActor* Player = World->SpawnActor<AActor>(AActor::StaticClass(), FVector(3900.0f, 0.0f, 100.0f), FRotator::ZeroRotator);
	if (!Player->HasActorBegunPlay())
	{
		Player->DispatchBeginPlay();
	}
	UCameraComponent* Camera = NewObject<UCameraComponent>(Player, TEXT("PlacementCamera"));
	Player->AddInstanceComponent(Camera);
	Player->SetRootComponent(Camera);
	Camera->RegisterComponent();
	UPlayerCarryComponent* Carry = NewObject<UPlayerCarryComponent>(Player);
	Player->AddInstanceComponent(Carry);
	Carry->ConfigureHeldAnchor(Camera);
	Carry->RegisterComponent();
	FPlayerInteractionContext TakeContext;
	TakeContext.Interactor = Player;
	TakeContext.CarryComponent = Carry;
	TakeContext.HitActor = Item;
	TestTrue(TEXT("The item is picked up"), Item->ExecuteInteraction(TakeContext).bSucceeded);
	UPlayerInteractionComponent* Interaction = NewObject<UPlayerInteractionComponent>(Player);
	Player->AddInstanceComponent(Interaction);
	Interaction->RegisterComponent();
	Interaction->Configure(Camera, nullptr);
	UPlayerFacilityPlacementComponent* Placement = NewObject<UPlayerFacilityPlacementComponent>(Player);
	Player->AddInstanceComponent(Placement);
	Placement->RegisterComponent();
	Interaction->ConfigureSupplementalIntentSource(Placement);

	const float Distance = Settings->GetPlacementTraceDistance();
	auto Aim = [&](const FVector& Location, const FRotator& Rotation)
	{
		Player->SetActorLocationAndRotation(Location, Rotation);
		Camera->SetWorldLocationAndRotation(Location, Rotation);
	};
	const FRotator Down(-90.0f, 0.0f, 0.0f);
	const FVector AtZone(ZoneCenter.X, ZoneCenter.Y, 150.0f);
	const FText NoZoneText = NSLOCTEXT("PlayerFacilityPlacementValidation", "NoZone", "설치 가능한 구역을 바라보세요.");

	// 미리보기 시작 시 조준 없음 -> 처음부터 숨김(하늘 조준).
	Aim(AtZone, FRotator(90.0f, 0.0f, 0.0f));
	Placement->Configure(Camera, Carry, Interaction);
	AFacilityPlacementPreviewActor* Preview = Placement->PreviewActor.Get();
	if (!TestNotNull(TEXT("A preview actor exists after the session starts"), Preview))
	{
		Placement->CancelAllSessions();
		World->DestroyWorld(false);
		GEngine->DestroyWorldContext(World);
		World->RemoveFromRoot();
		Settings->ValidPreviewMaterial = SavedValid;
		Settings->InvalidPreviewMaterial = SavedInvalid;
		Settings->GridSizeCm = SavedGrid;
		return false;
	}
	TestTrue(TEXT("The preview starts hidden when nothing is aimed at"), Preview->IsHidden());
	for (const UStaticMeshComponent* Mesh : Preview->GetPreviewMeshes())
	{
		TestFalse(TEXT("A hidden preview mesh is not rendered in the game world"), Mesh && Mesh->ShouldRender());
	}

	// 구역 바닥 조준 -> 보임, 후보 위치.
	Aim(AtZone, Down);
	Placement->AddRotationInput(1.0f);
	const float YawAfterRotation = Placement->AccumulatedYaw;
	TestTrue(TEXT("Rotation input accumulates yaw"), !FMath::IsNearlyZero(YawAfterRotation));
	Placement->RefreshPreview();
	TestTrue(TEXT("Aiming at a zone floor shows the preview"), !Preview->IsHidden());
	for (const UStaticMeshComponent* Mesh : Preview->GetPreviewMeshes())
	{
		TestTrue(TEXT("A visible preview mesh renders in the game world"), Mesh && Mesh->ShouldRender());
	}
	TestTrue(TEXT("The valid zone gives a valid placement query"), Placement->CurrentPlacementQuery.bSucceeded);
	const FTransform VisibleTransform = Preview->GetActorTransform();
	TestTrue(TEXT("The preview sits at the aimed floor point"),
		FVector2D(VisibleTransform.GetLocation()).Equals(FVector2D(ZoneCenter), 15.0));
	const FTransform CandidateAtZone = Placement->CurrentCandidate;
	const int32 VisibleGridCount = Placement->VisibleGridZones.Num();
	TestTrue(TEXT("Compatible zone grids are visible"), VisibleGridCount > 0 && Zone->IsGridVisible());

	auto ExpectHiddenWithoutCandidate = [&](const TCHAR* Label)
	{
		Placement->RefreshPreview();
		TestTrue(*FString::Printf(TEXT("%s: the preview actor stays alive"), Label), Placement->PreviewActor.Get() == Preview && IsValid(Preview));
		TestTrue(*FString::Printf(TEXT("%s: the preview is hidden"), Label), Preview->IsHidden());
		TestTrue(*FString::Printf(TEXT("%s: the preview does not move"), Label), Preview->GetActorTransform().Equals(VisibleTransform));
		for (const UStaticMeshComponent* Mesh : Preview->GetPreviewMeshes())
		{
			TestFalse(*FString::Printf(TEXT("%s: the preview mesh is not rendered"), Label), Mesh && Mesh->ShouldRender());
		}
		TestEqual(*FString::Printf(TEXT("%s: the grids stay as they were"), Label), Placement->VisibleGridZones.Num(), VisibleGridCount);
		TestTrue(*FString::Printf(TEXT("%s: the guidance text is the existing no-zone text"), Label),
			Placement->CurrentPlacementQuery.FailureReason.EqualTo(NoZoneText));
		TestFalse(*FString::Printf(TEXT("%s: the placement query fails"), Label), Placement->CurrentPlacementQuery.bSucceeded);
		const FPlayerInteractionResult Result = Placement->ConfirmPlacement();
		TestFalse(*FString::Printf(TEXT("%s: confirming placement is rejected"), Label), Result.bSucceeded);
		TestTrue(*FString::Printf(TEXT("%s: confirm reports the no-zone text"), Label), Result.FailureReason.EqualTo(NoZoneText));
		TestTrue(*FString::Printf(TEXT("%s: the item is still held"), Label), Carry->GetHeldObject() == Item);
		TestTrue(*FString::Printf(TEXT("%s: the preview actor is not replaced"), Label), Placement->PreviewActor.Get() == Preview);
		TestTrue(*FString::Printf(TEXT("%s: the placement session continues"), Label), Placement->IsPlacementActive());
	};
	auto ExpectVisibleAgain = [&](const TCHAR* Label)
	{
		Placement->RefreshPreview();
		TestTrue(*FString::Printf(TEXT("%s: the preview is visible again"), Label), !Preview->IsHidden());
		for (const UStaticMeshComponent* Mesh : Preview->GetPreviewMeshes())
		{
			TestTrue(*FString::Printf(TEXT("%s: the preview mesh renders again"), Label), Mesh && Mesh->ShouldRender());
		}
		TestTrue(*FString::Printf(TEXT("%s: the preview returns to the new candidate"), Label), Preview->GetActorTransform().Equals(CandidateAtZone, 0.5f));
		TestTrue(*FString::Printf(TEXT("%s: the accumulated yaw is kept"), Label), FMath::IsNearlyEqual(Placement->AccumulatedYaw, YawAfterRotation));
	};

	// (a) 하늘(hit 없음).
	Aim(AtZone, FRotator(90.0f, 0.0f, 0.0f));
	ExpectHiddenWithoutCandidate(TEXT("Sky"));
	Aim(AtZone, Down);
	ExpectVisibleAgain(TEXT("After the sky"));

	// (b) 배치 거리 밖의 구역.
	Aim(FVector(ZoneCenter.X, ZoneCenter.Y, Distance + 100.0f), Down);
	ExpectHiddenWithoutCandidate(TEXT("Beyond the placement distance"));
	Aim(AtZone, Down);
	ExpectVisibleAgain(TEXT("After the distant zone"));

	// (c) 배치 trace 채널을 막는 비구역 물체(벽 대용).
	AActor* Wall = SpawnBlockingBox(ZoneCenter + FVector(0.0f, 0.0f, 60.0f), FVector(80.0f, 80.0f, 5.0f), ECC_WorldStatic);
	ExpectHiddenWithoutCandidate(TEXT("A non-zone blocker in the way"));
	Wall->Destroy();
	ExpectVisibleAgain(TEXT("After the blocker"));

	// EXP-052: 구역 불허는 후보가 있으므로 숨지 않고 조준한 자리에 빨간 미리보기.
	Aim(FVector(5500.0f, 1000.0f, 150.0f), Down);
	Placement->RefreshPreview();
	TestTrue(TEXT("A zone that does not allow the facility still shows the preview"), !Preview->IsHidden());
	TestFalse(TEXT("The disallowed zone fails the query"), Placement->CurrentPlacementQuery.bSucceeded);
	TestFalse(TEXT("The disallowed zone gives a reason other than the no-zone text"),
		Placement->CurrentPlacementQuery.FailureReason.EqualTo(NoZoneText));
	TestTrue(TEXT("The preview moved to the disallowed zone"),
		FVector2D(Preview->GetActorLocation()).Equals(FVector2D(5500.0f, 1000.0f), 15.0));
	// 겹침: 같은 자리의 구역 안에 물체를 두면 후보는 있고 막힘으로 거절된다.
	Aim(AtZone, Down);
	AActor* Overlap = SpawnBlockingBox(ZoneCenter + FVector(0.0f, 0.0f, 20.0f), FVector(30.0f, 30.0f, 20.0f), ECC_WorldDynamic, true);
	Placement->RefreshPreview();
	TestTrue(TEXT("An overlapped placement still shows the preview"), !Preview->IsHidden());
	TestFalse(TEXT("An overlapped placement fails the query"), Placement->CurrentPlacementQuery.bSucceeded);
	Overlap->Destroy();
	Placement->RefreshPreview();
	TestTrue(TEXT("The preview is valid again once the overlap is gone"), !Preview->IsHidden() && Placement->CurrentPlacementQuery.bSucceeded);

	Placement->CancelAllSessions();
	World->DestroyWorld(false);
	GEngine->DestroyWorldContext(World);
	World->RemoveFromRoot();
	Settings->ValidPreviewMaterial = SavedValid;
	Settings->InvalidPreviewMaterial = SavedInvalid;
	Settings->GridSizeCm = SavedGrid;
	return true;
}

#endif
