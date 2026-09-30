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
	if (!InsertPreview)
	{
		InsertPreview = NewObject<UStaticMeshComponent>(Owner, NAME_None, RF_Transient);
		InsertPreview->CastShadow = false;
		Finish(InsertPreview);
	}
	if (!TakeHighlightProxy)
	{
		TakeHighlightProxy = NewObject<UStaticMeshComponent>(Owner, NAME_None, RF_Transient);
		TakeHighlightProxy->CastShadow = false;
		Finish(TakeHighlightProxy);
		TakeHighlightProxy->SetRenderInMainPass(false);
		TakeHighlightProxy->SetRenderInDepthPass(false);
		TakeHighlightProxy->SetRenderCustomDepth(true);
		TakeHighlightProxy->SetCustomDepthStencilValue(
			GetDefault<UServiceDisplaySettings>()->GetTakeHighlightStencilValue());
	}
	RefreshStockVisual();
}

void UDisplaySpaceComponent::DestroyPresentationComponents()
{
	for (UPrimitiveComponent* Comp : { static_cast<UPrimitiveComponent*>(StockVisual.Get()),
		static_cast<UPrimitiveComponent*>(InsertPreview.Get()),
		static_cast<UPrimitiveComponent*>(TakeHighlightProxy.Get()) })
	{
		if (IsValid(Comp))
		{
			Comp->DestroyComponent();
		}
	}
	StockVisual = nullptr;
	InsertPreview = nullptr;
	TakeHighlightProxy = nullptr;
}

FDisplaySpaceSnapshot UDisplaySpaceComponent::BuildSnapshot() const
{
	FDisplaySpaceSnapshot Snapshot;
	Snapshot.SpaceIndex = SpaceIndex;
	Snapshot.Kind = Stock.Kind;
	Snapshot.Count = Stock.Count;
	return Snapshot;
}

bool UDisplaySpaceComponent::ValidateAuthoring(FText& OutFailureReason) const
{
	OutFailureReason = FText::GetEmpty();
	if (SpaceIndex < 0 || !AcceptedCategory.IsValid() || SlotTransforms.IsEmpty())
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
	if (Extent.GetMin() <= 0.0f || !Body || Body->GetCollisionEnabled(false) != ECollisionEnabled::QueryOnly
		|| GetCollisionResponseToChannel(ECC_Visibility) != ECR_Block || CanEverAffectNavigation())
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
	return Kind && AcceptedCategory.IsValid() && Kind->DisplayCategories.HasTag(AcceptedCategory);
}

bool UDisplaySpaceComponent::ValidateStockImport(
	const UServiceItemDefinition* Kind,
	const int32 Count,
	FText& OutFailureReason) const
{
	OutFailureReason = FText::GetEmpty();
	if (Count < 0 || Count > GetCapacity() || (Count == 0) != (Kind == nullptr)
		|| (Kind && !CanAcceptKind(Kind)))
	{
		OutFailureReason = LOCTEXT("InvalidStockImport", "진열 공간에 적용할 수 없는 물건 데이터입니다.");
		return false;
	}
	return true;
}

bool UDisplaySpaceComponent::ImportStock(
	UServiceItemDefinition* Kind,
	const int32 Count,
	FText& OutFailureReason)
{
	if (!ValidateStockImport(Kind, Count, OutFailureReason))
	{
		return false;
	}
	const bool bChanged = Stock.Kind != Kind || Stock.Count != Count;
	Stock.Kind = Count > 0 ? Kind : nullptr;
	Stock.Count = Count;
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
	if (InsertPreview)
	{
		InsertPreview->SetVisibility(false);
	}
	if (TakeHighlightProxy)
	{
		TakeHighlightProxy->SetVisibility(false);
	}
}

void UDisplaySpaceComponent::ShowInsertPreview(const UServiceItemDefinition& BoxKind)
{
	UMaterialInterface* Material = GetDefault<UServiceDisplaySettings>()->LoadInsertPreviewMaterial();
	UStaticMesh* Mesh = BoxKind.ResolveDisplayMesh();
	if (!InsertPreview || !Mesh || Stock.Count >= SlotTransforms.Num())
	{
		return;
	}
	if (!Material)
	{
		if (!bWarnedMissingPreviewMaterial)
		{
			bWarnedMissingPreviewMaterial = true;
			UE_LOG(LogTemp, Warning, TEXT("ServiceDisplaySettings.InsertPreviewMaterial is not set; display insert preview is omitted."));
		}
		return;
	}
	InsertPreview->SetStaticMesh(Mesh);
	for (int32 MaterialIndex = 0; MaterialIndex < InsertPreview->GetNumMaterials(); ++MaterialIndex)
	{
		InsertPreview->SetMaterial(MaterialIndex, Material);
	}
	InsertPreview->SetRelativeTransform(GetSlotWorldRelativeTransform(Stock.Count, BoxKind));
	InsertPreview->SetVisibility(true);
}

void UDisplaySpaceComponent::ShowTakeHighlight()
{
	const UServiceDisplaySettings* Settings = GetDefault<UServiceDisplaySettings>();
	UStaticMesh* Mesh = Stock.Kind ? Stock.Kind->ResolveDisplayMesh() : nullptr;
	if (!TakeHighlightProxy || !Settings->ShouldShowTakeHighlight() || !Mesh || Stock.Count <= 0)
	{
		return;
	}
	TakeHighlightProxy->SetStaticMesh(Mesh);
	TakeHighlightProxy->SetRenderInMainPass(false);
	TakeHighlightProxy->SetRenderInDepthPass(false);
	TakeHighlightProxy->SetRenderCustomDepth(true);
	TakeHighlightProxy->SetCustomDepthStencilValue(Settings->GetTakeHighlightStencilValue());
	TakeHighlightProxy->SetRelativeTransform(GetSlotWorldRelativeTransform(Stock.Count - 1, *Stock.Kind));
	TakeHighlightProxy->SetVisibility(true);
}

FPlayerInteractionQuery UDisplaySpaceComponent::QueryInteraction(const FPlayerInteractionContext& Context) const
{
	FPlayerInteractionQuery Query;
	if (!IsOperational())
	{
		return Query;
	}
	Query.bVisible = true;
	const int32 Capacity = GetCapacity();
	Query.TargetName = Stock.Count > 0 && Stock.Kind
		? FText::Format(
			LOCTEXT("StockedName", "{0} {1}/{2}"),
			Stock.Kind->DisplayName,
			FText::AsNumber(Stock.Count),
			FText::AsNumber(Capacity))
		: FText::Format(LOCTEXT("EmptyName", "빈 공간 0/{0}"), FText::AsNumber(Capacity));

	const AItemBoxActor* Box = Context.CarryComponent
		? Cast<AItemBoxActor>(Context.CarryComponent->GetHeldObject())
		: nullptr;
	if (!IsValid(Box) || !Box->IsHeld())
	{
		return Query;
	}
	const FServiceTransferEvaluation Apply =
		FServiceItemTransfer::EvaluateApply(Box->GetContents(), Stock, AcceptedCategory, Capacity);
	Query.bHeldApplyVisible = true;
	Query.bCanHeldApply = Apply.bCan;
	Query.HeldApplyActionName = LOCTEXT("ApplyAction", "넣기");
	Query.HeldApplyFailureReason = Apply.Reason;
	Query.HeldApplyActivationMode = EPlayerInteractionActivationMode::Repeat;
	const FServiceTransferEvaluation Take =
		FServiceItemTransfer::EvaluateTake(Box->GetContents(), Stock, AcceptedCategory, Capacity);
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

FPlayerInteractionResult UDisplaySpaceComponent::ExecuteHeldTargetUse(
	const FPlayerInteractionContext& Context,
	const EPlayerHeldTargetUseDirection Direction)
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
	const bool bMoved = bApply
		? FServiceItemTransfer::TryApplyOne(Box->GetMutableContents(), Stock, AcceptedCategory, GetCapacity(), Failure)
		: FServiceItemTransfer::TryTakeOne(Box->GetMutableContents(), Stock, AcceptedCategory, GetCapacity(), Failure);
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
