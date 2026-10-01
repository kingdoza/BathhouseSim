#include "Cleaning/LitterTongsActor.h"
#include "Cleaning/LitterActor.h"
#include "Cleaning/TrashBagActor.h"
#include "Cleaning/TrashBagDropPlacement.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Pawn.h"
#include "Engine/World.h"
#include "Interaction/PlayerCarryComponent.h"
#include "Interaction/PhysicalCarryFixedSlot.h"
#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif
#define LOCTEXT_NAMESPACE "LitterTongsActor"

ALitterTongsActor::ALitterTongsActor()
{
	PrimaryActorTick.bCanEverTick = false;
	WorldMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WorldMesh"));
	SetRootComponent(WorldMesh);
	WorldMesh->SetCollisionProfileName(TEXT("PhysicsActor"));
	WorldMesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
	WorldMesh->SetUseCCD(true);
}

void ALitterTongsActor::BeginPlay()
{
	Super::BeginPlay();
	LastSafeTransform = GetActorTransform();
}

void ALitterTongsActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	bEndingPlay = true;
	if (AActor* SlotActor = FixedSlot.Get())
	{
		if (IPhysicalCarryFixedSlot* Slot = Cast<IPhysicalCarryFixedSlot>(SlotActor))
		{
			Slot->NotifyAssignedPhysicalCarryItemEnding(*this);
		}
	}
	FixedSlot.Reset();
	if (Carrier)
	{
		Carrier->NotifyHeldActorEnding(this);
	}
	Carrier = nullptr;
	OnHeldPresentationChanged.Clear();
	Super::EndPlay(EndPlayReason);
}

void ALitterTongsActor::FellOutOfWorld(const UDamageType& DamageType)
{
	RecoverPhysicalCarryable(Carrier);
}

FPlayerInteractionQuery ALitterTongsActor::QueryInteraction(const FPlayerInteractionContext& Context) const
{
	FPlayerInteractionQuery Query;
	if (Carrier || IsStoredInAssignedPhysicalCarryFixedSlot())
	{
		return Query;
	}
	Query.bVisible = true;
	Query.TargetName = LOCTEXT("TongsTarget", "집게");
	Query.ActionName = LOCTEXT("TakeTongs", "집게 들기");
	Query.bCanInteract = Context.CarryComponent && Context.CarryComponent->IsHandEmpty();
	if (!Query.bCanInteract)
	{
		Query.FailureReason = LOCTEXT("HandOccupied", "이미 다른 물건을 들고 있습니다.");
	}
	return Query;
}

FPlayerInteractionResult ALitterTongsActor::ExecuteInteraction(const FPlayerInteractionContext& Context)
{
	FText FailureReason;
	return Context.CarryComponent && Context.CarryComponent->TryTakePhysicalObject(this, FailureReason)
			   ? FPlayerInteractionResult::Succeeded()
			   : FPlayerInteractionResult::Failed(FailureReason);
}

FText ALitterTongsActor::GetPhysicalCarryDisplayName() const
{
	return LOCTEXT("TongsTarget", "집게");
}

FTransform ALitterTongsActor::GetHeldTransform() const
{
	FTransform Result = HeldTransform;
	Result.SetScale3D(FVector::OneVector);
	return Result;
}

bool ALitterTongsActor::CanBeTakenBy(const UPlayerCarryComponent& Carry, FText& OutFailureReason) const
{
	if (IsStoredInAssignedPhysicalCarryFixedSlot())
	{
		OutFailureReason = LOCTEXT("TakeFromSlotRequired", "집게는 전용 슬롯에서 가져와야 합니다.");
		return false;
	}
	if (Carrier || !Carry.IsHandEmpty())
	{
		OutFailureReason = LOCTEXT("HandOccupied", "이미 다른 물건을 들고 있습니다.");
		return false;
	}
	return true;
}

bool ALitterTongsActor::HandleTakenBy(UPlayerCarryComponent& Carry, USceneComponent* HeldAnchor)
{
	if (Carrier)
	{
		return false;
	}
	LastSafeTransform = GetActorTransform();
	Carrier = &Carry;
	DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	SetWorldPhysics(false);
	if (HeldAnchor)
	{
		AttachToComponent(HeldAnchor, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
		ApplyHeldTransform();
	}
	return true;
}

UPrimitiveComponent* ALitterTongsActor::GetPhysicalCarryPrimitive() const
{
	return WorldMesh;
}

bool ALitterTongsActor::CanFreeDrop(FText& OutFailureReason) const
{
	if (!Carrier || Carrier->GetHeldObject() != this)
	{
		OutFailureReason = LOCTEXT("TongsNotHeldForDrop", "집게를 들고 있어야 내려놓을 수 있습니다.");
		return false;
	}
	return true;
}

bool ALitterTongsActor::TryBindPhysicalCarryFixedSlot(AActor& SlotActor, FText& OutFailureReason)
{
	if (bFixedSlotBindingConflict)
	{
		OutFailureReason = LOCTEXT("SlotBindingConflict", "집게가 여러 슬롯에 연결되어 있습니다.");
		return false;
	}
	IPhysicalCarryFixedSlot* Slot = Cast<IPhysicalCarryFixedSlot>(&SlotActor);
	if (!Slot || Slot->GetAssignedPhysicalCarryItem() != this)
	{
		OutFailureReason = LOCTEXT("InvalidSlotBinding", "집게와 슬롯 연결이 올바르지 않습니다.");
		return false;
	}
	if (FixedSlot.IsValid() && FixedSlot.Get() != &SlotActor)
	{
		OutFailureReason = LOCTEXT("DuplicateSlotBinding", "집게가 여러 슬롯에 연결되어 있습니다.");
		return false;
	}
	FixedSlot = &SlotActor;
	return true;
}

void ALitterTongsActor::ClearPhysicalCarryFixedSlotBinding(AActor& ExpectedSlot)
{
	if (FixedSlot.Get() == &ExpectedSlot)
	{
		FixedSlot.Reset();
	}
}

bool ALitterTongsActor::IsStoredInAssignedPhysicalCarryFixedSlot() const
{
	const IPhysicalCarryFixedSlot* Slot = Cast<IPhysicalCarryFixedSlot>(FixedSlot.Get());
	return Slot && Slot->GetStoredPhysicalCarryItem() == this;
}

bool ALitterTongsActor::NotifyTakenFromFixedSlotCommitted(UPlayerCarryComponent& Carry, AActor& SlotActor)
{
	if (FixedSlot.Get() != &SlotActor || Carrier)
	{
		return false;
	}
	Carrier = &Carry;
	return true;
}

bool ALitterTongsActor::NotifyStoredInFixedSlotCommitted(UPlayerCarryComponent& Carry, AActor& SlotActor)
{
	if (FixedSlot.Get() != &SlotActor || Carrier != &Carry)
	{
		return false;
	}
	Carrier = nullptr;
	LastSafeTransform = GetActorTransform();
	return true;
}

bool ALitterTongsActor::NotifyRecoveredToFixedSlotCommitted(AActor& SlotActor)
{
	if (FixedSlot.Get() != &SlotActor || bEndingPlay)
	{
		return false;
	}
	if (Carrier && Carrier->GetHeldObject() == this)
	{
		return false;
	}
	Carrier = nullptr;
	LastSafeTransform = GetActorTransform();
	return true;
}

void ALitterTongsActor::NotifyFixedSlotDestroyed(AActor& SlotActor)
{
	if (FixedSlot.Get() != &SlotActor)
	{
		return;
	}
	FixedSlot.Reset();
	Carrier = nullptr;
	LastSafeTransform = GetActorTransform();
}

bool ALitterTongsActor::NotifyPhysicalDropCommitted(UPlayerCarryComponent& Carry)
{
	if (Carrier != &Carry)
	{
		return false;
	}
	Carrier = nullptr;
	LastSafeTransform = GetActorTransform();
	return true;
}

void ALitterTongsActor::PublishPhysicalCarryCommit(const EPhysicalCarryCommitTransition Transition)
{
	OnHeldPresentationChanged.Broadcast(Transition == EPhysicalCarryCommitTransition::TakenIntoHand);
}

void ALitterTongsActor::RecoverPhysicalCarryable(UPlayerCarryComponent* PreviousCarry)
{
	if (Carrier && PreviousCarry && Carrier != PreviousCarry)
	{
		return;
	}
	if (Carrier && Carrier->GetHeldObject() == this && Carrier->RecoverHeldPhysicalObject(this))
	{
		return;
	}
	if (AActor* SlotActor = FixedSlot.Get())
	{
		if (IPhysicalCarryFixedSlot* Slot = Cast<IPhysicalCarryFixedSlot>(SlotActor))
		{
			if (Slot->TryRecoverAssignedPhysicalCarryItem(*this))
			{
				return;
			}
		}
	}
	Carrier = nullptr;
	DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	SetWorldPhysics(false);
	SetActorTransform(LastSafeTransform);
	SetWorldPhysics(true);
}

void ALitterTongsActor::SetWorldPhysics(const bool bEnabled)
{
	if (WorldMesh)
	{
		if (bEnabled)
		{
			WorldMesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
			WorldMesh->SetUseCCD(true);
			WorldMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
			WorldMesh->SetSimulatePhysics(true);
		}
		else
		{
			WorldMesh->SetSimulatePhysics(false);
			WorldMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}
	}
}

void ALitterTongsActor::ApplyHeldTransform()
{
	const FTransform LocalHeldTransform = GetHeldTransform();
	if (USceneComponent* Root = GetRootComponent())
	{
		Root->SetRelativeLocationAndRotation(LocalHeldTransform.GetLocation(), LocalHeldTransform.GetRotation());
	}
}

FText ALitterTongsActor::GetHeldSummaryText() const
{
	return FText::Format(LOCTEXT("BagCount", "봉투 {0}/{1}"), FText::AsNumber(BagCount), FText::AsNumber(BagCapacity));
}

FHeldEquipmentUseQuery ALitterTongsActor::QueryEquipmentUse(const FHeldEquipmentUseContext& Context) const
{
	FHeldEquipmentUseQuery Query;
	const auto* Litter = Cast<ALitterActor>(Context.FocusHit.GetActor());
	Query.DisplayName = GetPhysicalCarryDisplayName();
	Query.bVisible = IsValid(Litter) && Litter->IsActive();
	if (!Query.bVisible)
	{
		return Query;
	}
	Query.ActionName = LOCTEXT("Collect", "줍기");
	Query.bCanUse =
		Carrier == Context.CarryComponent && Carrier && Carrier->GetHeldObject() == this && BagCount < BagCapacity;
	if (!Query.bCanUse)
	{
		Query.FailureReason = BagCount >= BagCapacity ? LOCTEXT("FullBag", "봉투 가득 참")
													  : LOCTEXT("NotHeld", "집게를 들고 있어야 합니다");
	}
	return Query;
}

FHeldEquipmentUseResult ALitterTongsActor::BeginEquipmentUse(const FHeldEquipmentUseContext& Context)
{
	const auto Query = QueryEquipmentUse(Context);
	auto* Litter = Cast<ALitterActor>(Context.FocusHit.GetActor());
	if (!Query.bVisible || !Query.bCanUse || !IsValid(Litter) || !Litter->CommitCollected())
	{
		return FHeldEquipmentUseResult::Failed(Query.FailureReason);
	}
	++BagCount;
	return FHeldEquipmentUseResult::Succeeded();
}

FHeldEquipmentUseUpdate ALitterTongsActor::UpdateEquipmentUse(const FHeldEquipmentUseContext& Context, float DeltaTime)
{
	FHeldEquipmentUseUpdate Update;
	Update.State = EPlayerHoldInteractionState::Succeeded;
	return Update;
}

FHeldEquipmentUseResult ALitterTongsActor::EndEquipmentUse(const FHeldEquipmentUseContext& Context)
{
	return FHeldEquipmentUseResult::Succeeded();
}

void ALitterTongsActor::CancelEquipmentUse(const FHeldEquipmentUseContext& Context)
{
}

FHeldEquipmentUseQuery ALitterTongsActor::QuerySecondaryEquipmentUse(const FHeldEquipmentUseContext& Context) const
{
	FHeldEquipmentUseQuery Query;
	Query.DisplayName = GetPhysicalCarryDisplayName();
	Query.ActionName = LOCTEXT("TieBag", "봉투 묶기");
	Query.bCanUse = BagCount > 0 && Carrier == Context.CarryComponent && Carrier && Carrier->GetHeldObject() == this;
	if (!Query.bCanUse)
	{
		Query.FailureReason =
			BagCount == 0 ? LOCTEXT("EmptyBag", "봉투가 비어 있음") : LOCTEXT("NotHeld", "집게를 들고 있어야 합니다");
	}
	return Query;
}

void ALitterTongsActor::BuildTieDropRequest(const FHeldEquipmentUseContext& Context,
											FTrashBagDropRequest& OutRequest) const
{
	OutRequest.CameraOrigin = Context.CameraOrigin;
	OutRequest.CameraDirection = Context.CameraDirection;
	OutRequest.ViewDistanceCm = TieViewDistanceCm;
	OutRequest.ViewMinDistanceCm = TieViewMinDistanceCm;
	OutRequest.ViewPullStepCm = TieViewPullStepCm;
	OutRequest.FloorForwardDistanceCm = TieForwardDistanceCm;
	OutRequest.FloorMinForwardDistanceCm = TieMinForwardDistanceCm;
	OutRequest.FloorPullStepCm = TieForwardPullStepCm;
	OutRequest.FloorClearanceCm = TieFloorClearanceCm;
	OutRequest.CameraClearanceCm = TieCameraClearanceCm;
}

FHeldEquipmentUseResult ALitterTongsActor::ExecuteSecondaryEquipmentUse(const FHeldEquipmentUseContext& Context)
{
	const auto Query = QuerySecondaryEquipmentUse(Context);
	if (!Query.bCanUse)
	{
		return FHeldEquipmentUseResult::Failed(Query.FailureReason);
	}
	FTransform Transform;
	const TArray<AActor*> Ignored = {Context.User.Get(), this};
	FTrashBagDropRequest Request;
	BuildTieDropRequest(Context, Request);
	if (!GetWorld() ||
		!FTrashBagDropPlacement::Find(*GetWorld(), Cast<APawn>(Context.User), Ignored, TiedBagClass, Request,
									  Transform) ||
		!ATrashBagActor::SpawnTiedBag(*GetWorld(), TiedBagClass, BagCount, Transform))
	{
		return FHeldEquipmentUseResult::Failed(LOCTEXT("NoBagSpace", "봉투를 놓을 공간이 없음"));
	}
	BagCount = 0;
	return FHeldEquipmentUseResult::Succeeded();
}
#if WITH_EDITOR
EDataValidationResult ALitterTongsActor::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = Super::IsDataValid(Context);
	if (!HeldTransform.GetScale3D().Equals(FVector::OneVector))
	{
		Context.AddWarning(
			LOCTEXT("HeldTransformScaleIgnored",
					"HeldTransform scale is ignored at runtime. Author a unit scale and use location/rotation only."));
	}
	const auto ReportInvalid = [&Context, &Result](const FText& Message)
	{
		Context.AddError(Message);
		Result = EDataValidationResult::Invalid;
	};
	if (!FMath::IsFinite(TieViewMinDistanceCm) || TieViewMinDistanceCm <= 0.0f || !FMath::IsFinite(TieViewDistanceCm)
		|| TieViewDistanceCm < TieViewMinDistanceCm || !FMath::IsFinite(TieViewPullStepCm) || TieViewPullStepCm <= 0.0f)
	{
		ReportInvalid(LOCTEXT("InvalidTieView",
							  "Tie view-front values need Min > 0, Distance >= Min and Pull Step > 0; the stage is skipped otherwise."));
	}
	if (!FMath::IsFinite(TieMinForwardDistanceCm) || TieMinForwardDistanceCm <= 0.0f
		|| !FMath::IsFinite(TieForwardDistanceCm) || TieForwardDistanceCm < TieMinForwardDistanceCm
		|| !FMath::IsFinite(TieForwardPullStepCm) || TieForwardPullStepCm <= 0.0f)
	{
		ReportInvalid(LOCTEXT("InvalidTieFloor",
							  "Tie floor-front values need Min > 0, Distance >= Min and Pull Step > 0; the stage is skipped otherwise."));
	}
	if (!FMath::IsFinite(TieFloorClearanceCm) || TieFloorClearanceCm < 0.0f || !FMath::IsFinite(TieCameraClearanceCm)
		|| TieCameraClearanceCm < 0.0f)
	{
		ReportInvalid(LOCTEXT("InvalidTieClearance", "Tie floor and camera clearances must be finite and >= 0."));
	}
	return Result == EDataValidationResult::NotValidated ? EDataValidationResult::Valid : Result;
}
#endif
#undef LOCTEXT_NAMESPACE
