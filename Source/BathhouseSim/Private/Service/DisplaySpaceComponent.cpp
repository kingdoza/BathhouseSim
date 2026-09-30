#include "Service/DisplaySpaceComponent.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/Actor.h"
#include "Interaction/PlayerCarryComponent.h"
#include "Interaction/PlayerInteractionComponent.h"
#include "Materials/MaterialInterface.h"
#include "PhysicsEngine/BodyInstance.h"
#include "Placement/FacilityPlacementComponent.h"
#include "Placement/PlaceableFacility.h"
#include "Service/ItemBoxActor.h"
#include "Service/DisplayCueComponent.h"
#include "Service/DisplayStockRules.h"
#include "Service/ServiceDisplaySettings.h"
#include "Service/ServiceItemDefinition.h"
#include "Service/ServiceItemTransfer.h"

#define LOCTEXT_NAMESPACE "DisplaySpaceComponent"

namespace
{
bool IsValidSlotTransform(const FTransform& Transform)
{
	const FVector Scale = Transform.GetScale3D();
	return !Transform.ContainsNaN()
		&& FMath::IsFinite(Scale.X) && FMath::IsFinite(Scale.Y) && FMath::IsFinite(Scale.Z)
		&& Scale.X > KINDA_SMALL_NUMBER && Scale.Y > KINDA_SMALL_NUMBER && Scale.Z > KINDA_SMALL_NUMBER;
}

const UPlayerCarryComponent* FindCarry(const UPlayerInteractionComponent& Source)
{
	const AActor* SourceOwner = Source.GetOwner();
	return SourceOwner ? SourceOwner->FindComponentByClass<UPlayerCarryComponent>() : nullptr;
}
}

UDisplaySpaceComponent::UDisplaySpaceComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	InitBoxExtent(FVector(20.0f));
	SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	SetCollisionResponseToAllChannels(ECR_Ignore);
	SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	SetCanEverAffectNavigation(false);
	SetGenerateOverlapEvents(false);
	SlotTransforms.Init(FTransform::Identity, 6);
}

void UDisplaySpaceComponent::OnRegister()
{
	Super::OnRegister();
	if (TargetMode == EDisplaySpaceTargetMode::FacilityRouted)
	{
		SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
	EnsurePresentationComponents();
}

void UDisplaySpaceComponent::OnComponentDestroyed(const bool bDestroyingHierarchy)
{
	DestroyPresentationComponents();
	Super::OnComponentDestroyed(bDestroyingHierarchy);
}

void UDisplaySpaceComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	HidePresentation();
	Super::EndPlay(EndPlayReason);
}

void UDisplaySpaceComponent::EnsurePresentationComponents()
{
	AActor* Owner = GetOwner();
	if (!Owner || !GetWorld())
	{
		return;
	}
	auto Finish = [this](UPrimitiveComponent* Comp)
	{
		Comp->SetupAttachment(this);
		Comp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Comp->SetCanEverAffectNavigation(false);
		Comp->SetGenerateOverlapEvents(false);
		Comp->SetVisibility(false);
		Comp->RegisterComponent();
	};
	if (!StockVisual)
	{
		StockVisual = NewObject<UInstancedStaticMeshComponent>(Owner, NAME_None, RF_Transient);
		StockVisual->CastShadow = true;
		Finish(StockVisual);
	}
	if (!DisplayCue)
	{
		DisplayCue = NewObject<UDisplayCueComponent>(Owner, NAME_None, RF_Transient);
		DisplayCue->SetupAttachment(this);
		DisplayCue->RegisterComponent();
	}
	InsertPreview = DisplayCue->GetInsertPreview();
	TakeHighlightProxy = DisplayCue->GetTakeHighlightProxy();
	RefreshStockVisual();
}

void UDisplaySpaceComponent::DestroyPresentationComponents()
{
	if (IsValid(StockVisual))
	{
		StockVisual->DestroyComponent();
	}
	if (IsValid(DisplayCue))
	{
		DisplayCue->DestroyComponent();
	}
	StockVisual = nullptr;
	DisplayCue = nullptr;
	InsertPreview = nullptr;
	TakeHighlightProxy = nullptr;
}

FDisplaySpaceSnapshot UDisplaySpaceComponent::BuildSnapshot() const
{
	FDisplaySpaceSnapshot Snapshot;
	Snapshot.SpaceIndex = SpaceIndex;
	Snapshot.Kind = Stock.Kind;
	Snapshot.Count = Stock.Count;
	Snapshot.InUseRemaining = Stock.InUseRemaining;
	return Snapshot;
}

bool UDisplaySpaceComponent::ValidateAuthoring(FText& OutFailureReason) const
{
	OutFailureReason = FText::GetEmpty();
	if (SpaceIndex < 0 || (TargetMode == EDisplaySpaceTargetMode::SelfAim ? !AcceptedCategory.IsValid() : !FixedKind) ||
		SlotTransforms.IsEmpty())
	{
		OutFailureReason = LOCTEXT("InvalidSpaceAuthoring", "진열 공간에는 0 이상의 SpaceIndex, 분류 태그와 자리 1개 이상이 필요합니다.");
		return false;
	}
	for (const FTransform& SlotTransform : SlotTransforms)
	{
		if (!IsValidSlotTransform(SlotTransform))
		{
			OutFailureReason = LOCTEXT("InvalidSlotTransform", "진열 공간 자리 transform이 올바르지 않습니다.");
			return false;
		}
	}
	const FVector Extent = GetUnscaledBoxExtent();
	// Ignore the owner's enable-collision flag: staged placement disables it while authoring is still valid.
	const FBodyInstance* Body = GetBodyInstance();
	if (Extent.GetMin() <= 0.0f || !Body ||
		Body->GetCollisionEnabled(false) != (TargetMode == EDisplaySpaceTargetMode::SelfAim
												 ? ECollisionEnabled::QueryOnly
												 : ECollisionEnabled::NoCollision) ||
		(TargetMode == EDisplaySpaceTargetMode::SelfAim &&
		 GetCollisionResponseToChannel(ECC_Visibility) != ECR_Block) ||
		CanEverAffectNavigation())
	{
		OutFailureReason = LOCTEXT("InvalidSpaceCollision", "진열 공간 Box는 양수 extent, QueryOnly, Visibility Block, Navigation off여야 합니다.");
		return false;
	}
	return true;
}

bool UDisplaySpaceComponent::IsOperational() const
{
	FText Failure;
	if (!ValidateAuthoring(Failure))
	{
		return false;
	}
	const IPlaceableFacility* Facility = Cast<IPlaceableFacility>(GetOwner());
	if (!Facility)
	{
		return true;
	}
	const UFacilityPlacementComponent* Placement = Facility->GetFacilityPlacementComponent();
	return Placement && Placement->GetMode() == EPlaceableFacilityMode::Placed
		&& !Placement->IsStagedPlacement() && Placement->IsPlacedDomainActive();
}

bool UDisplaySpaceComponent::CanAcceptKind(const UServiceItemDefinition* Kind) const
{
	return TargetMode == EDisplaySpaceTargetMode::FacilityRouted
			   ? Kind && Kind == FixedKind
			   : Kind && AcceptedCategory.IsValid() && Kind->DisplayCategories.HasTag(AcceptedCategory);
}

bool UDisplaySpaceComponent::ValidateStockImport(const UServiceItemDefinition* Kind, const int32 Count,
												 FText& OutFailureReason, const int32 InUseRemaining) const
{
	OutFailureReason = FText::GetEmpty();
	if (!FDisplayStockRules::ValidateImport(Kind, Count, InUseRemaining, GetCapacity()) ||
		(Kind && !CanAcceptKind(Kind)))
	{
		OutFailureReason = LOCTEXT("InvalidStockImport", "진열 공간에 적용할 수 없는 물건 데이터입니다.");
		return false;
	}
	return true;
}

bool UDisplaySpaceComponent::ImportStock(UServiceItemDefinition* Kind, const int32 Count, FText& OutFailureReason,
										 const int32 InUseRemaining)
{
	if (!ValidateStockImport(Kind, Count, OutFailureReason, InUseRemaining))
	{
		return false;
	}
	const bool bChanged = Stock.Kind != Kind || Stock.Count != Count || Stock.InUseRemaining != InUseRemaining;
	Stock.Kind = Count > 0 ? Kind : nullptr;
	Stock.Count = Count;
	Stock.InUseRemaining = InUseRemaining;
	if (bChanged)
	{
		++Stock.Revision;
	}
	RefreshStockVisual();
	if (bChanged)
	{
		PublishStockChanged();
	}
	return true;
}

bool UDisplaySpaceComponent::RemoveOneForCustomer(UServiceItemDefinition*& OutKind, FText& OutFailureReason)
{
	if (!IsOperational())
	{
		OutKind = nullptr;
		OutFailureReason = LOCTEXT("SpaceNotOperational", "진열 공간을 사용할 수 없습니다.");
		return false;
	}
	if (!FServiceItemTransfer::TryRemoveOne(Stock, OutKind, OutFailureReason))
	{
		return false;
	}
	RefreshStockVisual();
	return true;
}

void UDisplaySpaceComponent::PublishStockChanged()
{
	OnStockChanged.Broadcast(BuildSnapshot());
}

FTransform UDisplaySpaceComponent::GetSlotWorldRelativeTransform(
	const int32 SlotIndex,
	const UServiceItemDefinition& Kind) const
{
	if (!SlotTransforms.IsValidIndex(SlotIndex))
	{
		return FTransform::Identity;
	}
	// Kind->DisplayOffset is slot-local, so it composes before the slot (UE order: child * parent).
	return Kind.DisplayOffset * SlotTransforms[SlotIndex];
}

void UDisplaySpaceComponent::RefreshStockVisual()
{
	if (!StockVisual)
	{
		return;
	}
	StockVisual->ClearInstances();
	UStaticMesh* Mesh = Stock.Kind ? Stock.Kind->ResolveDisplayMesh() : nullptr;
	if (!Mesh || Stock.Count <= 0)
	{
		StockVisual->SetVisibility(false);
		return;
	}
	StockVisual->SetStaticMesh(Mesh);
	const int32 Visible = FMath::Min(Stock.Count, SlotTransforms.Num());
	for (int32 Index = 0; Index < Visible; ++Index)
	{
		StockVisual->AddInstance(GetSlotWorldRelativeTransform(Index, *Stock.Kind), false);
	}
	StockVisual->SetVisibility(true);
}

void UDisplaySpaceComponent::HidePresentation()
{
	if (DisplayCue)
	{
		DisplayCue->HideAll();
	}
}

void UDisplaySpaceComponent::ShowInsertPreview(const UServiceItemDefinition& Kind)
{
	if (DisplayCue && Stock.Count < SlotTransforms.Num())
	{
		DisplayCue->ShowInsertPreview(Kind.ResolveDisplayMesh(), GetSlotWorldRelativeTransform(Stock.Count, Kind));
	}
}

void UDisplaySpaceComponent::ShowTakeHighlight()
{
	if (DisplayCue && Stock.Kind && FDisplayStockRules::GetTakeableCount(Stock) > 0)
	{
		DisplayCue->ShowTakeHighlight(Stock.Kind->ResolveDisplayMesh(),
									  GetSlotWorldRelativeTransform(Stock.Count - 1, *Stock.Kind));
	}
}

FVector UDisplaySpaceComponent::GetSlotsWorldCenter() const
{
	FVector Center = FVector::ZeroVector;
	for (const FTransform& Slot : SlotTransforms)
	{
		Center += GetComponentTransform().TransformPosition(Slot.GetLocation());
	}
	return SlotTransforms.IsEmpty() ? GetComponentLocation() : Center / SlotTransforms.Num();
}

bool UDisplaySpaceComponent::ConsumeOneUse(bool& OutDepleted)
{
	if (!FDisplayStockRules::ConsumeOneUse(Stock, FixedKind, OutDepleted))
	{
		return false;
	}
	RefreshStockVisual();
	PublishStockChanged();
	return true;
}

FText UDisplaySpaceComponent::GetStockSummary() const
{
	const UServiceItemDefinition* Kind = Stock.Kind ? Stock.Kind.Get() : FixedKind.Get();
	if (Kind && Stock.InUseRemaining > 0)
	{
		return FText::Format(LOCTEXT("InUseSummary", "{0} 새것 {1} + 사용 중 {2}/{3}회"), Kind->DisplayName,
							 Stock.Count - 1, Stock.InUseRemaining, Kind->ConsumableUses);
	}
	return Kind ? FText::Format(LOCTEXT("StockSummary", "{0} {1}/{2}"), Kind->DisplayName, Stock.Count, GetCapacity())
				: FText::Format(LOCTEXT("EmptySummary", "빈 공간 0/{0}"), GetCapacity());
}
FPlayerInteractionQuery UDisplaySpaceComponent::QueryInteraction(const FPlayerInteractionContext& Context) const
{
	return TargetMode == EDisplaySpaceTargetMode::SelfAim ? BuildHeldUseQuery(Context) : FPlayerInteractionQuery();
}

FPlayerInteractionQuery UDisplaySpaceComponent::BuildHeldUseQuery(const FPlayerInteractionContext& Context) const
{
	FPlayerInteractionQuery Query;
	if (!IsOperational())
	{
		return Query;
	}
	Query.bVisible = true;
	const int32 Capacity = GetCapacity();
	Query.TargetName = GetStockSummary();

	const AItemBoxActor* Box = Context.CarryComponent
		? Cast<AItemBoxActor>(Context.CarryComponent->GetHeldObject())
		: nullptr;
	if (!IsValid(Box) || !Box->IsHeld())
	{
		return Query;
	}
	const FServiceTransferEvaluation Apply = FServiceItemTransfer::EvaluateApply(
		Box->GetContents(), Stock, AcceptedCategory, Capacity,
		TargetMode == EDisplaySpaceTargetMode::FacilityRouted ? FixedKind.Get() : nullptr);
	Query.bHeldApplyVisible = true;
	Query.bCanHeldApply = Apply.bCan;
	Query.HeldApplyActionName = LOCTEXT("ApplyAction", "넣기");
	Query.HeldApplyFailureReason = Apply.Reason;
	Query.HeldApplyActivationMode = EPlayerInteractionActivationMode::Repeat;
	const FServiceTransferEvaluation Take = FServiceItemTransfer::EvaluateTake(
		Box->GetContents(), Stock, AcceptedCategory, Capacity,
		TargetMode == EDisplaySpaceTargetMode::FacilityRouted ? FixedKind.Get() : nullptr);
	Query.bHeldTakeVisible = true;
	Query.bCanHeldTake = Take.bCan;
	Query.HeldTakeActionName = LOCTEXT("TakeAction", "빼기");
	Query.HeldTakeFailureReason = Take.Reason;
	Query.HeldTakeActivationMode = EPlayerInteractionActivationMode::Repeat;
	return Query;
}

FPlayerInteractionResult UDisplaySpaceComponent::ExecuteInteraction(const FPlayerInteractionContext& Context)
{
	(void)Context;
	return FPlayerInteractionResult::Failed(FText::GetEmpty(), EPlayerInteractionIntent::Primary);
}

FPlayerInteractionResult UDisplaySpaceComponent::ExecuteHeldTargetUse(const FPlayerInteractionContext& Context,
																	  const EPlayerHeldTargetUseDirection Direction)
{
	return TargetMode == EDisplaySpaceTargetMode::SelfAim ? ExecuteRoutedHeldTargetUse(Context, Direction)
														  : FPlayerInteractionResult::Failed(FText::GetEmpty());
}

FPlayerInteractionResult UDisplaySpaceComponent::ExecuteRoutedHeldTargetUse(
	const FPlayerInteractionContext& Context, const EPlayerHeldTargetUseDirection Direction)
{
	const bool bApply = Direction == EPlayerHeldTargetUseDirection::Apply;
	const EPlayerInteractionIntent Intent = bApply
		? EPlayerInteractionIntent::HeldApply
		: EPlayerInteractionIntent::HeldTake;
	AItemBoxActor* Box = Context.CarryComponent
		? Cast<AItemBoxActor>(Context.CarryComponent->GetHeldObject())
		: nullptr;
	if (!IsOperational() || !IsValid(Box) || !Box->IsHeld())
	{
		return FPlayerInteractionResult::Failed(
			LOCTEXT("NoBoxHeld", "품목 박스를 들고 있어야 합니다."),
			Intent);
	}
	FText Failure;
	const bool bMoved = bApply ? FServiceItemTransfer::TryApplyOne(
									 Box->GetMutableContents(), Stock, AcceptedCategory, GetCapacity(), Failure,
									 TargetMode == EDisplaySpaceTargetMode::FacilityRouted ? FixedKind.Get() : nullptr)
							   : FServiceItemTransfer::TryTakeOne(
									 Box->GetMutableContents(), Stock, AcceptedCategory, GetCapacity(), Failure,
									 TargetMode == EDisplaySpaceTargetMode::FacilityRouted ? FixedKind.Get() : nullptr);
	if (!bMoved)
	{
		return FPlayerInteractionResult::Failed(Failure, Intent);
	}
	Box->NotifyContentsChanged();
	RefreshStockVisual();
	PublishStockChanged();
	return FPlayerInteractionResult::Succeeded(Intent);
}

void UDisplaySpaceComponent::NotifyInteractionFocusChanged(
	const UPlayerInteractionComponent& Source,
	const FPlayerInteractionQuery& Query)
{
	HidePresentation();
	if (!IsOperational())
	{
		return;
	}
	if (Query.bHeldApplyVisible && Query.bCanHeldApply)
	{
		const UPlayerCarryComponent* Carry = FindCarry(Source);
		const AItemBoxActor* Box = Carry ? Cast<AItemBoxActor>(Carry->GetHeldObject()) : nullptr;
		if (IsValid(Box) && Box->GetContents().Kind)
		{
			ShowInsertPreview(*Box->GetContents().Kind);
		}
	}
	if (Query.bHeldTakeVisible && Query.bCanHeldTake)
	{
		ShowTakeHighlight();
	}
}

void UDisplaySpaceComponent::NotifyInteractionFocusEnded(const UPlayerInteractionComponent& Source)
{
	(void)Source;
	HidePresentation();
}

#undef LOCTEXT_NAMESPACE
