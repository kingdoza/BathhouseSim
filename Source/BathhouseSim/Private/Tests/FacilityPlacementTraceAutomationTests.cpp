#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Camera/CameraComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/BoxComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Interaction/BathhouseKeyActor.h"
#include "Interaction/PlayerInteractionComponent.h"
#include "Placement/FacilityPlacementTypes.h"
#include "Placement/FacilityPlacementZoneActor.h"
#include "UObject/UObjectGlobals.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBathhouseFacilityPlacementTraceIsolationTest,
	"BathhouseSim.Placement.TraceChannelIsolation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBathhouseFacilityPlacementTraceIsolationTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	if (!GEngine)
	{
		AddError(TEXT("GEngine is required for the placement trace isolation test."));
		return false;
	}

	const FName WorldName = MakeUniqueObjectName(
		nullptr,
		UWorld::StaticClass(),
		TEXT("FacilityPlacementTraceAutomationWorld"));
	FWorldContext& WorldContext = GEngine->CreateNewWorldContext(EWorldType::Game);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, WorldName, GetTransientPackage());
	if (!World)
	{
		GEngine->DestroyWorldContext(World);
		AddError(TEXT("Failed to create the placement trace isolation world."));
		return false;
	}

	World->AddToRoot();
	WorldContext.SetCurrentWorld(World);
	World->InitializeActorsForPlay(FURL());
	World->BeginPlay();

	AActor* TraceOwner = World->SpawnActor<AActor>();
	UCameraComponent* Camera = NewObject<UCameraComponent>(TraceOwner, TEXT("TraceIsolationCamera"));
	TraceOwner->AddInstanceComponent(Camera);
	TraceOwner->SetRootComponent(Camera);
	Camera->RegisterComponent();
	Camera->SetWorldLocationAndRotation(
		FVector(0.0f, 0.0f, 150.0f),
		FRotator(-90.0f, 0.0f, 0.0f));

	UClass* ZoneBlueprintClass = LoadObject<UClass>(
		nullptr,
		TEXT("/Game/Bathhouse/Blueprints/Placement/BP_FacilityPlacementZone.BP_FacilityPlacementZone_C"));
	TestNotNull(TEXT("The authored Placement Zone Blueprint class loads"), ZoneBlueprintClass);
	AFacilityPlacementZoneActor* Zone = ZoneBlueprintClass
		? World->SpawnActor<AFacilityPlacementZoneActor>(
			ZoneBlueprintClass,
			FVector::ZeroVector,
			FRotator::ZeroRotator)
		: nullptr;
	if (!Zone)
	{
		World->DestroyWorld(false);
		GEngine->DestroyWorldContext(World);
		World->RemoveFromRoot();
		AddError(TEXT("Failed to spawn the authored Placement Zone Blueprint."));
		return false;
	}
	Zone->GetZoneBounds()->SetBoxExtent(FVector(100.0f, 100.0f, 5.0f));

	ABathhouseKeyActor* ThinInteractable = World->SpawnActor<ABathhouseKeyActor>();
	UPrimitiveComponent* ThinCollision = ThinInteractable
		? ThinInteractable->GetPhysicalCarryPrimitive()
		: nullptr;
	if (ThinCollision)
	{
		ThinCollision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		ThinCollision->SetCollisionResponseToAllChannels(ECR_Ignore);
		ThinCollision->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	}

	UPlayerInteractionComponent* Interaction = NewObject<UPlayerInteractionComponent>(
		TraceOwner,
		TEXT("TraceIsolationInteraction"));
	TraceOwner->AddInstanceComponent(Interaction);
	Interaction->RegisterComponent();
	Interaction->Configure(Camera, nullptr);

	TestEqual(TEXT("Dedicated placement zone trace channel is registered by project config"),
		UCollisionProfile::Get()->ReturnChannelNameFromContainerIndex(
			static_cast<int32>(BathhousePlacementCollision::ZoneTraceChannel)),
		FName(TEXT("FacilityPlacementZone")));
	TestEqual(TEXT("Placement Zone ignores generic interaction Visibility"),
		Zone->GetZoneBounds()->GetCollisionResponseToChannel(ECC_Visibility),
		ECR_Ignore);
	TestEqual(TEXT("Placement Zone blocks the dedicated placement trace"),
		Zone->GetZoneBounds()->GetCollisionResponseToChannel(
			BathhousePlacementCollision::ZoneTraceChannel),
		ECR_Block);

	FHitResult InteractionHit;
	TestTrue(TEXT("Visibility interaction trace reaches the thin interactable inside the Zone"),
		Interaction->GetCurrentFocusHit(InteractionHit)
		&& InteractionHit.GetActor() == ThinInteractable);

	FHitResult PlacementHit;
	FCollisionQueryParams PlacementParams(
		SCENE_QUERY_STAT(FacilityPlacementTraceIsolation),
		true,
		TraceOwner);
	const bool bHitPlacementZone = World->LineTraceSingleByChannel(
		PlacementHit,
		Camera->GetComponentLocation(),
		Camera->GetComponentLocation() + Camera->GetForwardVector() * 500.0f,
		BathhousePlacementCollision::ZoneTraceChannel,
		PlacementParams);
	TestTrue(TEXT("Dedicated placement trace ignores the thin interactable and reaches the Zone"),
		bHitPlacementZone && PlacementHit.GetActor() == Zone);

	World->DestroyWorld(false);
	GEngine->DestroyWorldContext(World);
	World->RemoveFromRoot();
	return true;
}

#endif
