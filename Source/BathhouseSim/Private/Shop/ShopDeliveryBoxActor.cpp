#include "Shop/ShopDeliveryBoxActor.h"

#include "Components/PrimitiveComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Interaction/PlayerCarryComponent.h"
#include "Placement/FacilityPlacementDefinition.h"
#include "Placement/FacilityPlacementTypes.h"
#include "Misc/DataValidation.h"
#include "Shop/ShopSettings.h"
#include "Shop/ShopUnboxingTransaction.h"
#include "UObject/ConstructorHelpers.h"

#define LOCTEXT_NAMESPACE "ShopDeliveryBoxActor"

AShopDeliveryBoxActor::AShopDeliveryBoxActor()
{
	PrimaryActorTick.bCanEverTick = false;
	BoxMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BoxMesh"));
	SetRootComponent(BoxMesh);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (Cube.Succeeded())
	{
		BoxMesh->SetStaticMesh(Cube.Object);
	}
	BoxMesh->SetCollisionProfileName(TEXT("PhysicsActor"));
	BoxMesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
	BoxMesh->SetUseCCD(true);
	BoxMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	BoxMesh->SetSimulatePhysics(false);
}

void AShopDeliveryBoxActor::BeginPlay()
{
	Super::BeginPlay();
	if (Lifecycle == ELifecycle::FreeWorld)
	{
		LastSafeTransform = GetActorTransform();
	}
}

void AShopDeliveryBoxActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (Lifecycle != ELifecycle::Consumed)
	{
		if (UPlayerCarryComponent* LiveCarrier = Carrier.Get())
		{
			LiveCarrier->NotifyHeldActorEnding(this);
		}
	}
	Carrier.Reset();
	Contents.Reset();
	Super::EndPlay(EndPlayReason);
}

bool AShopDeliveryBoxActor::InitializeContents(
	const int64 InOrderId,
	const TArray<FShopOrderLine>& InContents)
{
	const UShopSettings* Settings = GetDefault<UShopSettings>();
	if (HasActorBegunPlay() || Lifecycle != ELifecycle::Staged || bContentsInitialized
		|| InOrderId <= 0 || InContents.IsEmpty() || !Settings)
	{
		return false;
	}
	int64 TotalQuantity = 0;
	for (const FShopOrderLine& Line : InContents)
	{
		FText DefinitionFailure;
		if (Line.ProductId.IsNone() || !IsValid(Line.PlacementDefinition.Get())
			|| Line.Quantity <= 0 || Line.DisplayName.IsEmpty()
			|| Line.PlacementDefinition->LockerSlotCount != 0
			|| !Line.PlacementDefinition->FacilityTags.HasTag(TAG_Facility_Discardable)
			|| !Line.PlacementDefinition->ValidateRuntime(DefinitionFailure))
		{
			return false;
		}
		TotalQuantity += Line.Quantity;
		if (TotalQuantity > Settings->GetCartTotalQuantityLimit())
		{
			return false;
		}
	}
	OrderId = InOrderId;
	Contents = InContents;
	bContentsInitialized = true;
	return true;
}

bool AShopDeliveryBoxActor::ActivateFreeWorld(
	const FTransform& WorldTransform,
	FText& OutFailureReason)
{
	if (Lifecycle != ELifecycle::Staged || !bContentsInitialized || WorldTransform.ContainsNaN())
	{
		OutFailureReason = LOCTEXT("InvalidBoxActivation", "배송 상자 초기화 상태가 올바르지 않습니다.");
		return false;
	}
	SetActorLocationAndRotation(
		WorldTransform.GetLocation(),
		WorldTransform.Rotator(),
		false,
		nullptr,
		ETeleportType::TeleportPhysics);
	SetFreeWorldPhysics(true);
	if (!BoxMesh->IsSimulatingPhysics())
	{
		OutFailureReason = LOCTEXT("BoxPhysicsFailed", "배송 상자 물리를 활성화할 수 없습니다.");
		SetFreeWorldPhysics(false);
		return false;
	}
	BoxMesh->SetPhysicsLinearVelocity(FVector::ZeroVector);
	BoxMesh->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
	Lifecycle = ELifecycle::FreeWorld;
	LastSafeTransform = GetActorTransform();
	return true;
}

FVector AShopDeliveryBoxActor::GetBoxHalfExtent() const
{
	if (!BoxMesh || !BoxMesh->GetStaticMesh())
	{
		return FVector::ZeroVector;
	}
	return BoxMesh->GetStaticMesh()->GetBounds().BoxExtent
		* BoxMesh->GetRelativeScale3D().GetAbs();
}

FText AShopDeliveryBoxActor::GetContentsSummary() const
{
	TArray<FString> Parts;
	for (const FShopOrderLine& Line : Contents)
	{
		if (Line.Quantity > 0 && !Line.DisplayName.IsEmpty())
		{
			Parts.Add(FText::Format(
				LOCTEXT("BoxContentsLine", "{0} {1}"),
				Line.DisplayName,
				FText::AsNumber(Line.Quantity)).ToString());
		}
	}
	const FText ContentText = Parts.IsEmpty()
		? LOCTEXT("EmptyBox", "내용 없음")
		: FText::FromString(FString::Join(Parts, TEXT(", ")));
	return FText::Format(LOCTEXT("BoxContentsSummary", "배송 상자 — {0}"), ContentText);
}

FPlayerInteractionQuery AShopDeliveryBoxActor::QueryInteraction(const FPlayerInteractionContext& Context) const
{
	FPlayerInteractionQuery Query;
	if (Lifecycle != ELifecycle::FreeWorld || !bContentsInitialized)
	{
		return Query;
	}
	Query.bVisible = true;
	Query.TargetName = GetContentsSummary();
	Query.ActionName = LOCTEXT("TakeBox", "상자 들기");
	FText Failure;
	Query.bCanInteract = Context.CarryComponent && CanBeTakenBy(*Context.CarryComponent, Failure);
	Query.FailureReason = Failure;
	return Query;
}

FPlayerInteractionResult AShopDeliveryBoxActor::ExecuteInteraction(const FPlayerInteractionContext& Context)
{
	FText Failure;
	return Context.CarryComponent && Context.CarryComponent->TryTakePhysicalObject(this, Failure)
		? FPlayerInteractionResult::Succeeded()
		: FPlayerInteractionResult::Failed(
			Failure.IsEmpty() ? LOCTEXT("TakeBoxFailed", "배송 상자를 들 수 없습니다.") : Failure);
}

FTransform AShopDeliveryBoxActor::GetHeldTransform() const
{
	FTransform Result = HeldTransform;
	Result.SetScale3D(FVector::OneVector);
	return Result;
}

bool AShopDeliveryBoxActor::CanBeTakenBy(
	const UPlayerCarryComponent& Carry,
	FText& OutFailureReason) const
{
	if (Lifecycle != ELifecycle::FreeWorld || !bContentsInitialized || Carrier.IsValid()
		|| !Carry.IsHandEmpty())
	{
		OutFailureReason = LOCTEXT("BoxUnavailable", "배송 상자를 들 수 없습니다.");
		return false;
	}
	return true;
}

bool AShopDeliveryBoxActor::HandleTakenBy(
	UPlayerCarryComponent& Carry,
	USceneComponent* HeldAnchor)
{
	if (Lifecycle != ELifecycle::FreeWorld || !bContentsInitialized || Carrier.IsValid()
		|| Carry.GetHeldObject() != this || !HeldAnchor)
	{
		return false;
	}
	LastSafeTransform = GetActorTransform();
	Carrier = &Carry;
	Lifecycle = ELifecycle::Held;
	SetFreeWorldPhysics(false);
	DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	if (!AttachToComponent(HeldAnchor, FAttachmentTransformRules::SnapToTargetNotIncludingScale))
	{
		DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
		Carrier.Reset();
		Lifecycle = ELifecycle::FreeWorld;
		SetActorLocationAndRotation(
			LastSafeTransform.GetLocation(),
			LastSafeTransform.Rotator(),
			false,
			nullptr,
			ETeleportType::TeleportPhysics);
		SetFreeWorldPhysics(true);
		return false;
	}
	const FTransform HeldPose = GetHeldTransform();
	BoxMesh->SetRelativeLocationAndRotation(HeldPose.GetLocation(), HeldPose.GetRotation());
	return true;
}

bool AShopDeliveryBoxActor::CanFreeDrop(FText& OutFailureReason) const
{
	if (Lifecycle != ELifecycle::Held || !Carrier.IsValid() || Carrier->GetHeldObject() != this)
	{
		OutFailureReason = LOCTEXT("BoxNotHeld", "배송 상자를 들고 있어야 내려놓을 수 있습니다.");
		return false;
	}
	return true;
}

UPrimitiveComponent* AShopDeliveryBoxActor::GetPhysicalCarryPrimitive() const
{
	return BoxMesh;
}

bool AShopDeliveryBoxActor::NotifyPhysicalDropCommitted(UPlayerCarryComponent& Carry)
{
	if (Lifecycle != ELifecycle::Held || Carrier.Get() != &Carry)
	{
		return false;
	}
	Carrier.Reset();
	Lifecycle = ELifecycle::FreeWorld;
	LastSafeTransform = GetActorTransform();
	return true;
}

void AShopDeliveryBoxActor::PublishPhysicalCarryCommit(const EPhysicalCarryCommitTransition Transition)
{
	(void)Transition;
}

void AShopDeliveryBoxActor::RecoverPhysicalCarryable(UPlayerCarryComponent* PreviousCarry)
{
	if (Lifecycle == ELifecycle::Staged || Lifecycle == ELifecycle::Consumed)
	{
		return;
	}
	UPlayerCarryComponent* LiveCarrier = Carrier.Get();
	if (PreviousCarry && LiveCarrier && LiveCarrier != PreviousCarry)
	{
		return;
	}
	if (LiveCarrier && LiveCarrier->GetHeldObject() == this)
	{
		LiveCarrier->RecoverHeldPhysicalObject(this);
		return;
	}
	Carrier.Reset();
	Lifecycle = ELifecycle::FreeWorld;
	DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	SetActorLocationAndRotation(
		LastSafeTransform.GetLocation(),
		LastSafeTransform.Rotator(),
		false,
		nullptr,
		ETeleportType::TeleportPhysics);
	SetFreeWorldPhysics(true);
	BoxMesh->SetPhysicsLinearVelocity(FVector::ZeroVector);
	BoxMesh->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
}

bool AShopDeliveryBoxActor::CanDiscardCarriedObject(FText& OutFailureReason) const
{
	if (Lifecycle != ELifecycle::Held || !Carrier.IsValid() || Carrier->GetHeldObject() != this
		|| !bContentsInitialized)
	{
		OutFailureReason = LOCTEXT("BoxNotDiscardable", "이 배송 상자는 버릴 수 없습니다.");
		return false;
	}
	return true;
}

void AShopDeliveryBoxActor::HandleDiscardCommitted()
{
	if (Lifecycle != ELifecycle::Held)
	{
		return;
	}
	Carrier.Reset();
	Contents.Reset();
	bContentsInitialized = false;
	Lifecycle = ELifecycle::Consumed;
	Destroy();
}

FHeldEquipmentUseQuery AShopDeliveryBoxActor::QueryEquipmentUse(
	const FHeldEquipmentUseContext& Context) const
{
	FHeldEquipmentUseQuery Query;
	Query.DisplayName = GetContentsSummary();
	Query.ActionName = LOCTEXT("OpenBox", "상자 열기");
	Query.ActivationMode = EPlayerInteractionActivationMode::Instant;
	Query.bVisible = true;
	Query.bCanUse = Lifecycle == ELifecycle::Held && bContentsInitialized
		&& Context.CarryComponent && Context.CarryComponent->GetHeldObject() == this;
	if (!Query.bCanUse)
	{
		Query.FailureReason = LOCTEXT("BoxNotHeldForOpen", "배송 상자를 들고 있어야 열 수 있습니다.");
	}
	return Query;
}

FHeldEquipmentUseResult AShopDeliveryBoxActor::BeginEquipmentUse(
	const FHeldEquipmentUseContext& Context)
{
	FText Failure;
	return FShopUnboxingTransaction::Open(*this, Context, Failure)
		? FHeldEquipmentUseResult::Succeeded()
		: FHeldEquipmentUseResult::Failed(Failure);
}

FHeldEquipmentUseUpdate AShopDeliveryBoxActor::UpdateEquipmentUse(
	const FHeldEquipmentUseContext& Context,
	const float DeltaTime)
{
	(void)Context;
	(void)DeltaTime;
	FHeldEquipmentUseUpdate Update;
	Update.State = EPlayerHoldInteractionState::Succeeded;
	Update.Progress = 1.0f;
	return Update;
}

FHeldEquipmentUseResult AShopDeliveryBoxActor::EndEquipmentUse(const FHeldEquipmentUseContext& Context)
{
	(void)Context;
	return FHeldEquipmentUseResult::Succeeded();
}

void AShopDeliveryBoxActor::CancelEquipmentUse(const FHeldEquipmentUseContext& Context)
{
	(void)Context;
}

void AShopDeliveryBoxActor::SetFreeWorldPhysics(const bool bEnabled)
{
	if (!BoxMesh)
	{
		return;
	}
	BoxMesh->SetSimulatePhysics(false);
	BoxMesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
	BoxMesh->SetUseCCD(true);
	BoxMesh->SetCollisionEnabled(
		bEnabled ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
	BoxMesh->SetSimulatePhysics(bEnabled);
}

#if WITH_EDITOR
EDataValidationResult AShopDeliveryBoxActor::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = Super::IsDataValid(Context);
	auto Invalidate = [&Context, &Result](const FText& Message)
	{
		Context.AddError(Message);
		Result = EDataValidationResult::Invalid;
	};
	const FVector RootScale = BoxMesh ? BoxMesh->GetRelativeScale3D() : FVector::ZeroVector;
	const bool bValidRootScale = FMath::IsFinite(RootScale.X)
		&& FMath::IsFinite(RootScale.Y)
		&& FMath::IsFinite(RootScale.Z)
		&& RootScale.X > KINDA_SMALL_NUMBER
		&& RootScale.Y > KINDA_SMALL_NUMBER
		&& RootScale.Z > KINDA_SMALL_NUMBER;
	if (!BoxMesh || GetRootComponent() != BoxMesh || !BoxMesh->GetStaticMesh()
		|| BoxMesh->GetCollisionEnabled() != ECollisionEnabled::QueryAndPhysics
		|| !BoxMesh->BodyInstance.bUseCCD
		|| BoxMesh->GetCollisionResponseToChannel(ECC_Pawn) != ECR_Ignore
		|| !BoxMesh->GetRelativeLocation().IsNearlyZero()
		|| !BoxMesh->GetRelativeRotation().IsNearlyZero()
		|| !bValidRootScale)
	{
		Invalidate(NSLOCTEXT("ShopDeliveryBox", "InvalidBoxAuthoring", "Delivery box requires a mesh root with QueryAndPhysics, CCD, Pawn Ignore, zero relative location/rotation, and a positive finite scale."));
	}
	if (!HeldTransform.GetScale3D().Equals(FVector::OneVector))
	{
		Context.AddWarning(LOCTEXT(
			"HeldTransformScaleIgnored",
			"HeldTransform scale is ignored at runtime. Author a unit scale and use location/rotation only."));
	}
	return Result == EDataValidationResult::NotValidated ? EDataValidationResult::Valid : Result;
}
#endif

#undef LOCTEXT_NAMESPACE
