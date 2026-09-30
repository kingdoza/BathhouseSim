#include "Towel/UsedTowelBinActor.h"
#include "Placement/FacilityPlacementComponent.h"

#include "Engine/World.h"
#include "Interaction/PlayerCarryComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Towel/TowelBasketActor.h"
#include "Towel/TowelCirculationSubsystem.h"
#include "Towel/TowelInventoryComponent.h"
#include "Towel/Presentation/TowelStackVisualComponent.h"
#include "Towel/TowelDisplayCueUtils.h"
#include "Service/DisplayCueComponent.h"
#include "Towel/TowelTransferSubsystem.h"
#include "Towel/WorldUsedTowelActor.h"
#include "Towel/TowelHeldTransferRules.h"

#define LOCTEXT_NAMESPACE "UsedTowelBinActor"

AUsedTowelBinActor::AUsedTowelBinActor()
{
	FacilityType = EBathhouseFacilityType::TowelBasket;
	Inventory = CreateDefaultSubobject<UTowelInventoryComponent>(TEXT("TowelInventory"));
	Inventory->ConfigureDefaults(ETowelState::None, 0, 20);
	TowelPresentationVisual = CreateDefaultSubobject<UTowelStackVisualComponent>(TEXT("TowelPresentationVisual"));
	TowelPresentationVisual->SetupAttachment(GetRootComponent());
	DisplayCue = CreateDefaultSubobject<UDisplayCueComponent>(TEXT("DisplayCue"));
	DisplayCue->SetupAttachment(TowelPresentationVisual);
}

void AUsedTowelBinActor::BeginPlay()
{
	Super::BeginPlay();
	TowelPresentationVisual->BindInventorySource(Inventory);
}

void AUsedTowelBinActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	TowelPresentationVisual->UnbindInventorySource();
	Super::EndPlay(EndPlayReason);
}

FPlayerInteractionQuery AUsedTowelBinActor::QueryInteraction(const FPlayerInteractionContext& Context) const
{
	const int64 PresentationRevision = TowelDisplayCueUtils::GetPresentationRevision(Context, Inventory);
	if (GetFacilityPlacementComponent()
		&& !GetFacilityPlacementComponent()->IsPlacedDomainActive())
	{
		FPlayerInteractionQuery Query = ABathhouseFacilityActor::QueryInteraction(Context);
		Query.PresentationRevision = PresentationRevision;
		return Query;
	}

	FPlayerInteractionQuery Query;
	Query.PresentationRevision = PresentationRevision;
	Query.bVisible = true;
	Query.TargetName = LOCTEXT("UsedBin", "사용 수건통");
	const ATowelBasketActor* Basket = Context.CarryComponent
		? Cast<ATowelBasketActor>(Context.CarryComponent->GetHeldObject())
		: nullptr;
	const FTowelInventorySnapshot BasketSnapshot = Basket && Basket->GetInventory()
		? Basket->GetInventory()->GetSnapshot()
		: FTowelInventorySnapshot();
	const FTowelHeldTransferQuery HeldUseQuery = FTowelHeldTransferRules::Build(
		ETowelHeldTransferTargetKind::UsedBin,
		Query.TargetName,
		Inventory ? Inventory->GetSnapshot() : FTowelInventorySnapshot(),
		Basket && Basket->GetInventory(),
		BasketSnapshot);
	FTowelHeldTransferRules::ApplyToQuery(Query, HeldUseQuery);
	return Query;
}

FPlayerInteractionResult AUsedTowelBinActor::ExecuteInteraction(const FPlayerInteractionContext& Context)
{
	if (GetFacilityPlacementComponent()
		&& !GetFacilityPlacementComponent()->IsPlacedDomainActive())
	{
		return ABathhouseFacilityActor::ExecuteInteraction(Context);
	}
	return FPlayerInteractionResult::Failed(FText::GetEmpty(), EPlayerInteractionIntent::Primary);
}

FPlayerInteractionResult AUsedTowelBinActor::ExecuteHeldTargetUse(
	const FPlayerInteractionContext& Context,
	const EPlayerHeldTargetUseDirection Direction)
{
	return TransferToHeldBasket(Context, Direction);
}

bool AUsedTowelBinActor::TryStageOverflowTowel(AWorldUsedTowelActor*& OutTowel)
{
	OutTowel = nullptr;
	if (GetFacilityPlacementComponent()
		&& !GetFacilityPlacementComponent()->IsPlacedDomainActive())
	{
		return false;
	}
	UWorld* World = GetWorld();
	UTowelCirculationSubsystem* Subsystem = World ? World->GetSubsystem<UTowelCirculationSubsystem>() : nullptr;
	if (!World || !Subsystem || !WorldUsedTowelClass)
	{
		return false;
	}
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(TowelOverflowPlacement), true, this);
	FRandomStream RandomStream(FMath::Rand());
	for (int32 Attempt = 0; Attempt < PlacementAttempts; ++Attempt)
	{
		const float Angle = RandomStream.FRandRange(0.0f, 2.0f * PI);
		const float Radius = RandomStream.FRandRange(
			FMath::Min(OverflowMinRadius, OverflowMaxRadius),
			FMath::Max(OverflowMinRadius, OverflowMaxRadius));
		const FVector Offset(FMath::Cos(Angle) * Radius, FMath::Sin(Angle) * Radius, 0.0f);
		const FVector Start = GetActorLocation() + Offset + FVector::UpVector * FloorTraceDistance * 0.5f;
		const FVector End = Start - FVector::UpVector * FloorTraceDistance;
		FHitResult FloorHit;
		if (!World->LineTraceSingleByChannel(FloorHit, Start, End, FloorTraceChannel, QueryParams)
			|| !Subsystem->IsWorldTowelLocationClear(FloorHit.ImpactPoint, TowelSpacing))
		{
			continue;
		}
		FCollisionObjectQueryParams ClearanceObjects;
		ClearanceObjects.AddObjectTypesToQuery(ECC_Pawn);
		ClearanceObjects.AddObjectTypesToQuery(ECC_WorldDynamic);
		if (World->OverlapAnyTestByObjectType(
			FloorHit.ImpactPoint + FVector::UpVector * 5.0f,
			FQuat::Identity,
			ClearanceObjects,
			FCollisionShape::MakeSphere(PawnClearance),
			QueryParams))
		{
			continue;
		}
		if (PawnClearance > 0.0f && World->OverlapBlockingTestByChannel(
			FloorHit.ImpactPoint + FVector::UpVector * (PawnClearance + 2.0f),
			FQuat::Identity,
			FloorTraceChannel,
			FCollisionShape::MakeSphere(PawnClearance),
			QueryParams))
		{
			continue;
		}
		const FTransform SpawnTransform(
			FRotationMatrix::MakeFromZ(FloorHit.ImpactNormal).ToQuat(),
			FloorHit.ImpactPoint);
		AWorldUsedTowelActor* Staged = World->SpawnActorDeferred<AWorldUsedTowelActor>(
			WorldUsedTowelClass,
			SpawnTransform,
			this,
			nullptr,
			ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
		if (!Staged)
		{
			continue;
		}
		Staged->SetPreferredBin(this);
		OutTowel = Cast<AWorldUsedTowelActor>(UGameplayStatics::FinishSpawningActor(Staged, SpawnTransform));
		return IsValid(OutTowel);
	}
	return false;
}

FPlayerInteractionResult AUsedTowelBinActor::TransferToHeldBasket(
	const FPlayerInteractionContext& Context,
	const EPlayerHeldTargetUseDirection Direction)
{
	const EPlayerInteractionIntent Intent = Direction == EPlayerHeldTargetUseDirection::Apply
		? EPlayerInteractionIntent::HeldApply
		: EPlayerInteractionIntent::HeldTake;
	if (GetFacilityPlacementComponent()
		&& !GetFacilityPlacementComponent()->IsPlacedDomainActive())
	{
		return FPlayerInteractionResult::Failed(FText::GetEmpty(), Intent);
	}

	const FPlayerInteractionQuery Query = QueryInteraction(Context);
	const bool bCanTransfer = Direction == EPlayerHeldTargetUseDirection::Apply
		? Query.bCanHeldApply
		: Query.bCanHeldTake;
	const FText& FailureReason = Direction == EPlayerHeldTargetUseDirection::Apply
		? Query.HeldApplyFailureReason
		: Query.HeldTakeFailureReason;
	if (!bCanTransfer)
	{
		return FPlayerInteractionResult::Failed(FailureReason, Intent);
	}

	ATowelBasketActor* Basket = Context.CarryComponent
		? Cast<ATowelBasketActor>(Context.CarryComponent->GetHeldObject())
		: nullptr;
	UTowelTransferSubsystem* Transfer = GetWorld()->GetSubsystem<UTowelTransferSubsystem>();
	if (!Basket || !Transfer || !Inventory || !Basket->GetInventory())
	{
		return FPlayerInteractionResult::Failed(
			LOCTEXT("TransferFailed", "수건을 바구니로 옮길 수 없습니다."),
			Intent);
	}
	FTowelTransferRequest Request;
	Request.Source = Direction == EPlayerHeldTargetUseDirection::Apply
		? Basket->GetInventory() : Inventory.Get();
	Request.Destination = Direction == EPlayerHeldTargetUseDirection::Apply
		? Inventory.Get() : Basket->GetInventory();
	Request.RequestedCount = 1;
	Request.ExpectedSourceRevision = Request.Source->GetSnapshot().Revision;
	Request.ExpectedDestinationRevision = Request.Destination->GetSnapshot().Revision;
	const FTowelTransferResult Result = Transfer->TryTransfer(Request);
	return Result.bSucceeded
		? FPlayerInteractionResult::Succeeded(Intent)
		: FPlayerInteractionResult::Failed(
			LOCTEXT("TransferFailed", "수건을 바구니로 옮길 수 없습니다."),
			Intent);
}

#undef LOCTEXT_NAMESPACE

void AUsedTowelBinActor::NotifyInteractionFocusChanged(const UPlayerInteractionComponent& Source,
													   const FPlayerInteractionQuery& Query)
{
	TowelDisplayCueUtils::Update(Source, Query, Inventory, TowelPresentationVisual, DisplayCue);
}

void AUsedTowelBinActor::NotifyInteractionFocusEnded(const UPlayerInteractionComponent& Source)
{
	DisplayCue->HideAll();
}
