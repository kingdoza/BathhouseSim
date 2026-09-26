#include "Utility/UtilityShovelActor.h"

#include "Components/PrimitiveComponent.h"
#include "Materials/MaterialInterface.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Interaction/PhysicalCarryFixedSlot.h"
#include "Interaction/PlayerCarryComponent.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

#define LOCTEXT_NAMESPACE "UtilityShovelActor"

AUtilityShovelActor::AUtilityShovelActor()
{
	PrimaryActorTick.bCanEverTick = false;
	WorldMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WorldMesh"));
	SetRootComponent(WorldMesh);
	WorldMesh->SetCollisionProfileName(TEXT("PhysicsActor"));
	WorldMesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
	WorldMesh->SetUseCCD(true);
	LoadVisual = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LoadVisual"));
	LoadVisual->SetupAttachment(WorldMesh);
	LoadVisual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	LoadVisual->SetCanEverAffectNavigation(false);
	LoadVisual->SetVisibility(false, true);
}

void AUtilityShovelActor::BeginPlay()
{
	Super::BeginPlay();
	LastSafeTransform = GetActorTransform();
	ApplyLoadPresentation();
}

void AUtilityShovelActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
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
	OnFuelLoadChanged.Clear();
	Super::EndPlay(EndPlayReason);
}

void AUtilityShovelActor::FellOutOfWorld(const UDamageType& DamageType)
{
	RecoverPhysicalCarryable(Carrier);
}

FPlayerInteractionQuery AUtilityShovelActor::QueryInteraction(
	const FPlayerInteractionContext& Context) const
{
	FPlayerInteractionQuery Query;
	FText AuthoringFailure;
	if (!HasValidAuthoring(AuthoringFailure))
	{
		Query.bVisible = true;
		Query.TargetName = GetPhysicalCarryDisplayName();
		Query.FailureReason = AuthoringFailure;
		return Query;
	}
	if (Carrier || IsStoredInAssignedPhysicalCarryFixedSlot())
	{
		return Query;
	}
	Query.bVisible = true;
	Query.TargetName = GetPhysicalCarryDisplayName();
	Query.ActionName = LOCTEXT("TakeShovel", "삽 들기");
	Query.bCanInteract = Context.CarryComponent && Context.CarryComponent->IsHandEmpty();
	if (!Query.bCanInteract)
	{
		Query.FailureReason = LOCTEXT("HandOccupied", "이미 다른 물건을 들고 있습니다.");
	}
	return Query;
}

FPlayerInteractionResult AUtilityShovelActor::ExecuteInteraction(
	const FPlayerInteractionContext& Context)
{
	FText FailureReason;
	return Context.CarryComponent && Context.CarryComponent->TryTakePhysicalObject(this, FailureReason)
		? FPlayerInteractionResult::Succeeded()
		: FPlayerInteractionResult::Failed(FailureReason);
}

FText AUtilityShovelActor::GetPhysicalCarryDisplayName() const
{
	return LOCTEXT("ShovelName", "삽");
}

FTransform AUtilityShovelActor::GetHeldTransform() const
{
	FTransform Result = HeldTransform;
	Result.SetScale3D(FVector::OneVector);
	return Result;
}

bool AUtilityShovelActor::CanBeTakenBy(
	const UPlayerCarryComponent& Carry,
	FText& OutFailureReason) const
{
	if (!HasValidAuthoring(OutFailureReason))
	{
		return false;
	}
	if (IsStoredInAssignedPhysicalCarryFixedSlot())
	{
		OutFailureReason = LOCTEXT("TakeFromSlotRequired", "삽은 전용 슬롯에서 가져와야 합니다.");
		return false;
	}
	if (Carrier || !Carry.IsHandEmpty())
	{
		OutFailureReason = LOCTEXT("HandOccupied", "이미 다른 물건을 들고 있습니다.");
		return false;
	}
	return true;
}

bool AUtilityShovelActor::HandleTakenBy(UPlayerCarryComponent& Carry, USceneComponent* HeldAnchor)
{
	FText FailureReason;
	if (Carrier || !HasValidAuthoring(FailureReason))
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

bool AUtilityShovelActor::CanFreeDrop(FText& OutFailureReason) const
{
	if (!HasValidAuthoring(OutFailureReason))
	{
		return false;
	}
	if (!Carrier || Carrier->GetHeldObject() != this)
	{
		OutFailureReason = LOCTEXT("ShovelNotHeldForDrop", "삽을 들고 있어야 내려놓을 수 있습니다.");
		return false;
	}
	OutFailureReason = FText::GetEmpty();
	return true;
}

UPrimitiveComponent* AUtilityShovelActor::GetPhysicalCarryPrimitive() const
{
	return WorldMesh;
}

bool AUtilityShovelActor::TryBindPhysicalCarryFixedSlot(
	AActor& SlotActor,
	FText& OutFailureReason)
{
	if (!HasValidAuthoring(OutFailureReason))
	{
		return false;
	}
	if (bFixedSlotBindingConflict)
	{
		OutFailureReason = LOCTEXT("SlotBindingConflict", "삽이 여러 슬롯에 연결되어 있습니다.");
		return false;
	}
	IPhysicalCarryFixedSlot* Slot = Cast<IPhysicalCarryFixedSlot>(&SlotActor);
	if (!Slot || Slot->GetAssignedPhysicalCarryItem() != this)
	{
		OutFailureReason = LOCTEXT("InvalidSlotBinding", "삽과 슬롯 연결이 올바르지 않습니다.");
		return false;
	}
	if (FixedSlot.IsValid() && FixedSlot.Get() != &SlotActor)
	{
		OutFailureReason = LOCTEXT("DuplicateSlotBinding", "삽이 여러 슬롯에 연결되어 있습니다.");
		return false;
	}
	FixedSlot = &SlotActor;
	OutFailureReason = FText::GetEmpty();
	return true;
}

void AUtilityShovelActor::ClearPhysicalCarryFixedSlotBinding(AActor& ExpectedSlot)
{
	if (FixedSlot.Get() == &ExpectedSlot)
	{
		FixedSlot.Reset();
	}
}

bool AUtilityShovelActor::IsStoredInAssignedPhysicalCarryFixedSlot() const
{
	const IPhysicalCarryFixedSlot* Slot = Cast<IPhysicalCarryFixedSlot>(FixedSlot.Get());
	return Slot && Slot->GetStoredPhysicalCarryItem() == this;
}

bool AUtilityShovelActor::NotifyTakenFromFixedSlotCommitted(
	UPlayerCarryComponent& Carry,
	AActor& SlotActor)
{
	if (FixedSlot.Get() != &SlotActor || Carrier)
	{
		return false;
	}
	Carrier = &Carry;
	return true;
}

bool AUtilityShovelActor::NotifyStoredInFixedSlotCommitted(
	UPlayerCarryComponent& Carry,
	AActor& SlotActor)
{
	if (FixedSlot.Get() != &SlotActor || Carrier != &Carry)
	{
		return false;
	}
	Carrier = nullptr;
	LastSafeTransform = GetActorTransform();
	return true;
}

bool AUtilityShovelActor::NotifyRecoveredToFixedSlotCommitted(AActor& SlotActor)
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

void AUtilityShovelActor::NotifyFixedSlotDestroyed(AActor& SlotActor)
{
	if (FixedSlot.Get() != &SlotActor)
	{
		return;
	}
	FixedSlot.Reset();
	Carrier = nullptr;
	LastSafeTransform = GetActorTransform();
}

bool AUtilityShovelActor::NotifyPhysicalDropCommitted(UPlayerCarryComponent& Carry)
{
	if (Carrier != &Carry)
	{
		return false;
	}
	Carrier = nullptr;
	LastSafeTransform = GetActorTransform();
	return true;
}

void AUtilityShovelActor::PublishPhysicalCarryCommit(const EPhysicalCarryCommitTransition Transition)
{
	(void)Transition;
}

void AUtilityShovelActor::RecoverPhysicalCarryable(UPlayerCarryComponent* PreviousCarry)
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

bool AUtilityShovelActor::CanAcceptLoad(
	const FUtilityFuelLoad& Load,
	FText& OutFailureReason) const
{
	if (bFuelMutationInProgress)
	{
		OutFailureReason = LOCTEXT("ShovelBusy", "삽의 연료 이동이 이미 처리 중입니다.");
		return false;
	}
	if (!Load.IsValid() || Load.IsEmpty() || !IsSupportedUtilityFuelKind(Load.Kind))
	{
		OutFailureReason = LOCTEXT("InvalidShovelLoad", "삽에 담을 연료 종류와 양이 올바르지 않습니다.");
		return false;
	}
	if (!FuelLoad.IsEmpty())
	{
		OutFailureReason = LOCTEXT("ShovelAlreadyLoaded", "삽에 이미 연료가 담겨 있습니다.");
		return false;
	}
	OutFailureReason = FText::GetEmpty();
	return true;
}

bool AUtilityShovelActor::HasValidAuthoring(FText& OutFailureReason) const
{
	const bool bIsHeld = Carrier && Carrier->GetHeldObject() == this;
	const bool bIsStored = IsStoredInAssignedPhysicalCarryFixedSlot();
	const ECollisionEnabled::Type ExpectedWorldCollision = bIsHeld || bIsStored
		? ECollisionEnabled::NoCollision
		: ECollisionEnabled::QueryAndPhysics;
	const FUtilityShovelLoadAppearance* CoalAppearance = LoadAppearances.Find(EUtilityFuelKind::Coal);
	const FUtilityShovelLoadAppearance* DryIceAppearance = LoadAppearances.Find(EUtilityFuelKind::DryIce);
	const bool bValidCoalAppearance = CoalAppearance && (CoalAppearance->Mesh || CoalAppearance->Material);
	const bool bValidDryIceAppearance = DryIceAppearance && (DryIceAppearance->Mesh || DryIceAppearance->Material);
	const bool bAppearanceCombinationsDiffer = CoalAppearance && DryIceAppearance
		&& (CoalAppearance->Mesh != nullptr) != (DryIceAppearance->Mesh != nullptr)
		|| (CoalAppearance && DryIceAppearance
			&& (CoalAppearance->Material != nullptr) != (DryIceAppearance->Material != nullptr));
	const bool bNeedsFallbackMesh = !CoalAppearance || !CoalAppearance->Mesh || !DryIceAppearance || !DryIceAppearance->Mesh;
	if (!WorldMesh || WorldMesh != GetRootComponent() || !WorldMesh->GetStaticMesh()
		|| !LoadVisual || (bNeedsFallbackMesh && !LoadVisual->GetStaticMesh())
		|| LoadVisual->GetAttachParent() != WorldMesh
		|| !HeldTransform.GetScale3D().Equals(FVector::OneVector)
		|| WorldMesh->GetCollisionEnabled() != ExpectedWorldCollision
		|| WorldMesh->GetCollisionResponseToChannel(ECC_WorldStatic) != ECR_Block
		|| WorldMesh->GetCollisionResponseToChannel(ECC_Pawn) != ECR_Ignore
		|| !WorldMesh->BodyInstance.bUseCCD
		|| LoadVisual->GetCollisionEnabled() != ECollisionEnabled::NoCollision
		|| LoadVisual->CanEverAffectNavigation()
		|| !bValidCoalAppearance || !bValidDryIceAppearance || !bAppearanceCombinationsDiffer)
	{
		OutFailureReason = LOCTEXT(
			"InvalidShovelAuthoring",
			"삽의 월드 메시, 연료별 적재 외형, hierarchy, Pawn ignore, CCD 또는 held transform authoring이 올바르지 않습니다.");
		return false;
	}
	OutFailureReason = FText::GetEmpty();
	return true;
}

bool AUtilityShovelActor::TryAcquireMutationGuard()
{
	if (bFuelMutationInProgress)
	{
		return false;
	}
	bFuelMutationInProgress = true;
	return true;
}

void AUtilityShovelActor::ReleaseMutationGuard()
{
	bFuelMutationInProgress = false;
}

void AUtilityShovelActor::SetFuelLoadSilently(const FUtilityFuelLoad& NewLoad)
{
	FuelLoad = NewLoad;
	++FuelLoadRevision;
	ApplyLoadPresentation();
}

void AUtilityShovelActor::PublishFuelLoadChanged()
{
	OnFuelLoadChanged.Broadcast(FuelLoad);
}

void AUtilityShovelActor::SetLoadAppearance(
	const EUtilityFuelKind Kind,
	const FUtilityShovelLoadAppearance& Appearance)
{
	if (!IsSupportedUtilityFuelKind(Kind))
	{
		return;
	}
	LoadAppearances.Add(Kind, Appearance);
	ApplyLoadPresentation();
}

void AUtilityShovelActor::CaptureAuthoredLoadAppearance()
{
	if (bCapturedAuthoredLoadAppearance || !LoadVisual)
	{
		return;
	}
	AuthoredLoadMesh = LoadVisual->GetStaticMesh();
	AuthoredLoadMaterial = LoadVisual->GetMaterial(0);
	bCapturedAuthoredLoadAppearance = true;
}

void AUtilityShovelActor::ApplyLoadPresentation()
{
	if (!LoadVisual)
	{
		return;
	}
	CaptureAuthoredLoadAppearance();
	if (FuelLoad.IsEmpty())
	{
		LoadVisual->SetStaticMesh(AuthoredLoadMesh);
		LoadVisual->SetMaterial(0, AuthoredLoadMaterial);
		LoadVisual->SetVisibility(false, true);
		return;
	}
	const FUtilityShovelLoadAppearance* Appearance = LoadAppearances.Find(FuelLoad.Kind);
	LoadVisual->SetStaticMesh(Appearance && Appearance->Mesh ? Appearance->Mesh : AuthoredLoadMesh);
	LoadVisual->SetMaterial(0, Appearance && Appearance->Material ? Appearance->Material : nullptr);
	LoadVisual->SetVisibility(true, true);
}

void AUtilityShovelActor::SetWorldPhysics(const bool bEnabled)
{
	if (!WorldMesh)
	{
		return;
	}
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

void AUtilityShovelActor::ApplyHeldTransform()
{
	if (USceneComponent* Root = GetRootComponent())
	{
		const FTransform LocalTransform = GetHeldTransform();
		Root->SetRelativeLocationAndRotation(
			LocalTransform.GetLocation(),
			LocalTransform.GetRotation());
	}
}

#if WITH_EDITOR
EDataValidationResult AUtilityShovelActor::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = Super::IsDataValid(Context);
	FText FailureReason;
	if (!HasValidAuthoring(FailureReason)
		|| !WorldMesh || WorldMesh->GetCollisionEnabled() != ECollisionEnabled::QueryAndPhysics)
	{
		Context.AddError(FailureReason.IsEmpty()
			? LOCTEXT("InvalidShovelCollision", "삽의 월드 루트 collision authoring은 QueryAndPhysics여야 합니다.")
			: FailureReason);
		Result = EDataValidationResult::Invalid;
	}
	return Result == EDataValidationResult::NotValidated ? EDataValidationResult::Valid : Result;
}
#endif

#undef LOCTEXT_NAMESPACE
