#include "Cleaning/TrashBagActor.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Interaction/PlayerCarryComponent.h"
#include "UObject/ConstructorHelpers.h"
#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif
#define LOCTEXT_NAMESPACE "TrashBagActor"

ATrashBagActor::ATrashBagActor()
{
	PrimaryActorTick.bCanEverTick = false;
	BagMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BagMesh"));
	SetRootComponent(BagMesh);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (Cube.Succeeded())
	{
		BagMesh->SetStaticMesh(Cube.Object);
	}
	BagMesh->SetCollisionProfileName(TEXT("PhysicsActor"));
	BagMesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
	BagMesh->SetUseCCD(true);
	BagMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	BagMesh->SetSimulatePhysics(false);
}

void ATrashBagActor::BeginPlay()
{
	Super::BeginPlay();
	if (Lifecycle == ELifecycle::FreeWorld)
	{
		LastSafeTransform = GetActorTransform();
	}
}

void ATrashBagActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (Lifecycle != ELifecycle::Consumed)
	{
		if (UPlayerCarryComponent* LiveCarrier = Carrier.Get())
		{
			LiveCarrier->NotifyHeldActorEnding(this);
		}
	}
	Carrier.Reset();
	Super::EndPlay(EndPlayReason);
}

void ATrashBagActor::FellOutOfWorld(const UDamageType& DamageType)
{
	(void)DamageType;
	RecoverPhysicalCarryable(Carrier.Get());
}

bool ATrashBagActor::ActivateFreeWorld(const FTransform& WorldTransform, FText& OutFailureReason)
{
	if (Lifecycle != ELifecycle::Staged || !bCountInitialized || WorldTransform.ContainsNaN())
	{
		OutFailureReason = LOCTEXT("InvalidBagActivation", "쓰레기봉투 초기화 상태가 올바르지 않습니다.");
		return false;
	}
	SetActorLocationAndRotation(WorldTransform.GetLocation(), WorldTransform.Rotator(), false, nullptr,
								ETeleportType::TeleportPhysics);
	SetFreeWorldPhysics(true);
	if (!BagMesh->IsSimulatingPhysics())
	{
		OutFailureReason = LOCTEXT("BagPhysicsFailed", "쓰레기봉투 물리를 활성화할 수 없습니다.");
		SetFreeWorldPhysics(false);
		return false;
	}
	BagMesh->SetPhysicsLinearVelocity(FVector::ZeroVector);
	BagMesh->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
	Lifecycle = ELifecycle::FreeWorld;
	LastSafeTransform = GetActorTransform();
	return true;
}

bool ATrashBagActor::BuildClassCollisionQuery(const TSubclassOf<ATrashBagActor> BagClass,
											  const FTransform& WorldTransform, FVector& OutLocation,
											  FQuat& OutRotation, FCollisionShape& OutShape,
											  const UPrimitiveComponent*& OutCollisionTemplate, FText& OutFailureReason)
{
	OutFailureReason = FText::GetEmpty();
	const ATrashBagActor* BagCDO = BagClass ? BagClass->GetDefaultObject<ATrashBagActor>() : nullptr;
	const UStaticMeshComponent* Root = BagCDO ? BagCDO->BagMesh.Get() : nullptr;
	const UStaticMesh* Mesh = Root ? Root->GetStaticMesh() : nullptr;
	if (!Mesh || WorldTransform.ContainsNaN())
	{
		OutFailureReason = LOCTEXT("InvalidBagClass", "쓰레기봉투 클래스를 확인할 수 없습니다.");
		return false;
	}
	const FVector RootScale = Root->GetRelativeScale3D();
	if (!FMath::IsFinite(RootScale.X) || !FMath::IsFinite(RootScale.Y) || !FMath::IsFinite(RootScale.Z) ||
		RootScale.X <= KINDA_SMALL_NUMBER || RootScale.Y <= KINDA_SMALL_NUMBER || RootScale.Z <= KINDA_SMALL_NUMBER)
	{
		OutFailureReason = LOCTEXT("InvalidBagClassScale", "쓰레기봉투 클래스의 루트 스케일이 올바르지 않습니다.");
		return false;
	}
	const FBoxSphereBounds Bounds = Mesh->GetBounds();
	const FVector Extent = Bounds.BoxExtent * RootScale;
	if (Extent.ContainsNaN() || Extent.GetMin() <= KINDA_SMALL_NUMBER)
	{
		OutFailureReason = LOCTEXT("InvalidBagClassBounds", "쓰레기봉투 크기가 올바르지 않습니다.");
		return false;
	}
	OutRotation = WorldTransform.GetRotation();
	OutLocation = WorldTransform.GetLocation() + OutRotation.RotateVector(Bounds.Origin * RootScale);
	OutShape = FCollisionShape::MakeBox(Extent);
	OutCollisionTemplate = Root;
	return true;
}

FPlayerInteractionQuery ATrashBagActor::QueryInteraction(const FPlayerInteractionContext& Context) const
{
	FPlayerInteractionQuery Query;
	if (Lifecycle != ELifecycle::FreeWorld || !bCountInitialized)
	{
		return Query;
	}
	Query.bVisible = true;
	Query.TargetName = GetBagSummary();
	Query.ActionName = LOCTEXT("TakeBox", "들기");
	FText Failure;
	Query.bCanInteract = Context.CarryComponent && CanBeTakenBy(*Context.CarryComponent, Failure);
	Query.FailureReason = Failure;
	return Query;
}

FPlayerInteractionResult ATrashBagActor::ExecuteInteraction(const FPlayerInteractionContext& Context)
{
	FText Failure;
	return Context.CarryComponent && Context.CarryComponent->TryTakePhysicalObject(this, Failure)
			   ? FPlayerInteractionResult::Succeeded()
			   : FPlayerInteractionResult::Failed(
					 Failure.IsEmpty() ? LOCTEXT("TakeBoxFailed", "쓰레기봉투를 들 수 없습니다.") : Failure);
}

FTransform ATrashBagActor::GetHeldTransform() const
{
	FTransform Result = HeldTransform;
	Result.SetScale3D(FVector::OneVector);
	return Result;
}

bool ATrashBagActor::CanBeTakenBy(const UPlayerCarryComponent& Carry, FText& OutFailureReason) const
{
	if (Lifecycle != ELifecycle::FreeWorld || !bCountInitialized || Carrier.IsValid() || !Carry.IsHandEmpty())
	{
		OutFailureReason = LOCTEXT("BoxUnavailable", "쓰레기봉투를 들 수 없습니다.");
		return false;
	}
	return true;
}

bool ATrashBagActor::HandleTakenBy(UPlayerCarryComponent& Carry, USceneComponent* HeldAnchor)
{
	if (Lifecycle != ELifecycle::FreeWorld || !bCountInitialized || Carrier.IsValid() ||
		Carry.GetHeldObject() != this || !HeldAnchor)
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
		SetActorLocationAndRotation(LastSafeTransform.GetLocation(), LastSafeTransform.Rotator(), false, nullptr,
									ETeleportType::TeleportPhysics);
		SetFreeWorldPhysics(true);
		return false;
	}
	const FTransform HeldPose = GetHeldTransform();
	BagMesh->SetRelativeLocationAndRotation(HeldPose.GetLocation(), HeldPose.GetRotation());
	return true;
}

bool ATrashBagActor::CanFreeDrop(FText& OutFailureReason) const
{
	if (Lifecycle != ELifecycle::Held || !Carrier.IsValid() || Carrier->GetHeldObject() != this)
	{
		OutFailureReason = LOCTEXT("BoxNotHeld", "쓰레기봉투를 들고 있어야 내려놓을 수 있습니다.");
		return false;
	}
	return true;
}

UPrimitiveComponent* ATrashBagActor::GetPhysicalCarryPrimitive() const
{
	return BagMesh;
}

bool ATrashBagActor::NotifyPhysicalDropCommitted(UPlayerCarryComponent& Carry)
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

void ATrashBagActor::PublishPhysicalCarryCommit(const EPhysicalCarryCommitTransition Transition)
{
	(void)Transition;
}

void ATrashBagActor::RecoverPhysicalCarryable(UPlayerCarryComponent* PreviousCarry)
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
	SetActorLocationAndRotation(LastSafeTransform.GetLocation(), LastSafeTransform.Rotator(), false, nullptr,
								ETeleportType::TeleportPhysics);
	SetFreeWorldPhysics(true);
	BagMesh->SetPhysicsLinearVelocity(FVector::ZeroVector);
	BagMesh->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
}

bool ATrashBagActor::CanDiscardCarriedObject(FText& OutFailureReason) const
{
	if (Lifecycle != ELifecycle::Held || !Carrier.IsValid() || Carrier->GetHeldObject() != this || !CanDiscardKind())
	{
		OutFailureReason = LOCTEXT("BagNotDiscardable", "이 쓰레기봉투는 버릴 수 없습니다.");
		return false;
	}
	return true;
}

void ATrashBagActor::HandleDiscardCommitted()
{
	if (Lifecycle != ELifecycle::Held)
	{
		return;
	}
	Carrier.Reset();
	LitterCount = 0;
	bCountInitialized = false;
	Lifecycle = ELifecycle::Consumed;
	Destroy();
}

void ATrashBagActor::SetFreeWorldPhysics(const bool bEnabled)
{
	if (!BagMesh)
	{
		return;
	}
	BagMesh->SetSimulatePhysics(false);
	BagMesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
	BagMesh->SetUseCCD(true);
	BagMesh->SetCollisionEnabled(bEnabled ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
	BagMesh->SetSimulatePhysics(bEnabled);
}

bool ATrashBagActor::InitializeCount(int32 Count)
{
	if (HasActorBegunPlay() || Lifecycle != ELifecycle::Staged || bCountInitialized || Count < 1)
	{
		return false;
	}
	LitterCount = Count;
	bCountInitialized = true;
	return true;
}

FText ATrashBagActor::GetBagSummary() const
{
	return FText::Format(LOCTEXT("BagSummary", "쓰레기봉투 ({0}개)"), FText::AsNumber(LitterCount));
}

ATrashBagActor* ATrashBagActor::SpawnTiedBag(UWorld& World, TSubclassOf<ATrashBagActor> BagClass, int32 Count,
											 const FTransform& Transform)
{
	if (!BagClass || Count < 1 || Transform.ContainsNaN())
	{
		return nullptr;
	}
	const auto* CDO = BagClass->GetDefaultObject<ATrashBagActor>();
	FTransform Spawn = Transform;
	Spawn.SetScale3D(CDO->BagMesh->GetRelativeScale3D());
	auto* Bag = World.SpawnActorDeferred<ATrashBagActor>(BagClass, Spawn, nullptr, nullptr,
														 ESpawnActorCollisionHandlingMethod::AlwaysSpawn,
														 ESpawnActorScaleMethod::OverrideRootScale);
	if (!Bag)
	{
		return nullptr;
	}
	if (!Bag->InitializeCount(Count))
	{
		Bag->Destroy();
		return nullptr;
	}
	Bag->FinishSpawning(Spawn, false, nullptr, ESpawnActorScaleMethod::OverrideRootScale);
	FText Failure;
	if (!IsValid(Bag) || !Bag->ActivateFreeWorld(Spawn, Failure))
	{
		if (IsValid(Bag))
		{
			Bag->Destroy();
		}
		return nullptr;
	}
	return Bag;
}

bool ATrashBagActor::CanDiscardFromWorld(FText& Failure) const
{
	return Lifecycle == ELifecycle::FreeWorld && !Carrier.IsValid() && CanDiscardKind();
}

void ATrashBagActor::HandleDiscardFromWorldCommitted()
{
	FText Failure;
	if (!CanDiscardFromWorld(Failure))
	{
		return;
	}
	LitterCount = 0;
	bCountInitialized = false;
	Lifecycle = ELifecycle::Consumed;
	Destroy();
}
#if WITH_EDITOR
EDataValidationResult ATrashBagActor::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = Super::IsDataValid(Context);
	auto Invalidate = [&Context, &Result](const FText& Message)
	{
		Context.AddError(Message);
		Result = EDataValidationResult::Invalid;
	};
	const FVector RootScale = BagMesh ? BagMesh->GetRelativeScale3D() : FVector::ZeroVector;
	const bool bValidRootScale = FMath::IsFinite(RootScale.X) && FMath::IsFinite(RootScale.Y) &&
								 FMath::IsFinite(RootScale.Z) && RootScale.X > KINDA_SMALL_NUMBER &&
								 RootScale.Y > KINDA_SMALL_NUMBER && RootScale.Z > KINDA_SMALL_NUMBER;
	if (!BagMesh || GetRootComponent() != BagMesh || !BagMesh->GetStaticMesh() ||
		BagMesh->GetCollisionEnabled() != ECollisionEnabled::QueryAndPhysics || !BagMesh->BodyInstance.bUseCCD ||
		BagMesh->GetCollisionResponseToChannel(ECC_Pawn) != ECR_Ignore ||
		!BagMesh->GetRelativeLocation().IsNearlyZero() || !BagMesh->GetRelativeRotation().IsNearlyZero() ||
		!bValidRootScale)
	{
		Invalidate(LOCTEXT("InvalidBagAuthoring",
						   "Trash bag requires a mesh root with QueryAndPhysics, CCD, Pawn Ignore, "
						   "zero relative location/rotation, and a positive finite scale."));
	}
	if (!HeldTransform.GetScale3D().Equals(FVector::OneVector))
	{
		Context.AddWarning(
			LOCTEXT("HeldTransformScaleIgnored",
					"HeldTransform scale is ignored at runtime. Author a unit scale and use location/rotation only."));
	}
	return Result == EDataValidationResult::NotValidated ? EDataValidationResult::Valid : Result;
}
#endif
#undef LOCTEXT_NAMESPACE
