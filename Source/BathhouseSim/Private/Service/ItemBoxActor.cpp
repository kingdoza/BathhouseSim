#include "Service/ItemBoxActor.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Interaction/PlayerCarryComponent.h"
#include "Service/ServiceItemDefinition.h"
#include "UObject/ConstructorHelpers.h"
#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

#define LOCTEXT_NAMESPACE "ItemBoxActor"

AItemBoxActor::AItemBoxActor()
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

	ContentsVisual = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("ContentsVisual"));
	ContentsVisual->SetupAttachment(BoxMesh);
	ContentsVisual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ContentsVisual->SetCanEverAffectNavigation(false);
}

void AItemBoxActor::BeginPlay()
{
	Super::BeginPlay();
	// Game world always shows the real contents; any Editor preview is discarded here.
	RebuildContentsVisual(Contents.Kind, Contents.Count);
	if (Lifecycle == ELifecycle::FreeWorld)
	{
		LastSafeTransform = GetActorTransform();
	}
}

void AItemBoxActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
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

void AItemBoxActor::FellOutOfWorld(const UDamageType& DamageType)
{
	(void)DamageType;
	RecoverPhysicalCarryable(Carrier.Get());
}

bool AItemBoxActor::InitializeContents(UServiceItemDefinition* Kind, const int32 Count)
{
	if (HasActorBegunPlay() || Lifecycle != ELifecycle::Staged || bContentsInitialized || Count < 0)
	{
		return false;
	}
	if (Count > 0)
	{
		FText Failure;
		if (!IsValid(Kind) || !Kind->ValidateRuntime(Failure) || Count > Kind->BoxCapacity)
		{
			return false;
		}
	}
	Contents = FServiceItemStack();
	if (Count > 0)
	{
		Contents.Kind = Kind;
		Contents.Count = Count;
	}
	bContentsInitialized = true;
	RebuildContentsVisual(Contents.Kind, Contents.Count);
	return true;
}

bool AItemBoxActor::ActivateFreeWorld(const FTransform& WorldTransform, FText& OutFailureReason)
{
	if (Lifecycle != ELifecycle::Staged || !bContentsInitialized || WorldTransform.ContainsNaN())
	{
		OutFailureReason = LOCTEXT("InvalidBoxActivation", "품목 박스 초기화 상태가 올바르지 않습니다.");
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
		OutFailureReason = LOCTEXT("BoxPhysicsFailed", "품목 박스 물리를 활성화할 수 없습니다.");
		SetFreeWorldPhysics(false);
		return false;
	}
	BoxMesh->SetPhysicsLinearVelocity(FVector::ZeroVector);
	BoxMesh->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
	Lifecycle = ELifecycle::FreeWorld;
	LastSafeTransform = GetActorTransform();
	return true;
}

void AItemBoxActor::NotifyContentsChanged()
{
	RebuildContentsVisual(Contents.Kind, Contents.Count);
}

FText AItemBoxActor::GetContentsSummary() const
{
	if (Contents.Count <= 0 || !Contents.Kind)
	{
		return LOCTEXT("EmptyBox", "빈 박스");
	}
	return FText::Format(
		LOCTEXT("BoxSummary", "{0} {1}/{2}"),
		Contents.Kind->DisplayName,
		FText::AsNumber(Contents.Count),
		FText::AsNumber(Contents.Kind->BoxCapacity));
}

AItemBoxActor* AItemBoxActor::SpawnFilledBox(
	UWorld& World,
	const TSubclassOf<AItemBoxActor> BoxClass,
	UServiceItemDefinition* Kind,
	const int32 Count,
	const FTransform& WorldTransform,
	FText& OutFailureReason)
{
	if (!BoxClass || WorldTransform.ContainsNaN())
	{
		OutFailureReason = LOCTEXT("InvalidBoxSpawn", "품목 박스를 생성할 수 없습니다.");
		return nullptr;
	}
	AItemBoxActor* Box = World.SpawnActorDeferred<AItemBoxActor>(
		BoxClass,
		WorldTransform,
		nullptr,
		nullptr,
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn,
		ESpawnActorScaleMethod::OverrideRootScale);
	if (!Box)
	{
		OutFailureReason = LOCTEXT("BoxSpawnFailed", "품목 박스 생성을 시작할 수 없습니다.");
		return nullptr;
	}
	if (!Box->InitializeContents(Kind, Count))
	{
		Box->Destroy();
		OutFailureReason = LOCTEXT("BoxInitFailed", "품목 박스 내용을 초기화할 수 없습니다.");
		return nullptr;
	}
	Box->FinishSpawning(WorldTransform, false, nullptr, ESpawnActorScaleMethod::OverrideRootScale);
	if (!IsValid(Box))
	{
		OutFailureReason = LOCTEXT("BoxFinishFailed", "품목 박스 생성을 완료할 수 없습니다.");
		return nullptr;
	}
	return Box;
}

bool AItemBoxActor::BuildClassCollisionQuery(
	const TSubclassOf<AItemBoxActor> BoxClass,
	const FTransform& WorldTransform,
	FVector& OutLocation,
	FQuat& OutRotation,
	FCollisionShape& OutShape,
	const UPrimitiveComponent*& OutCollisionTemplate,
	FText& OutFailureReason)
{
	OutFailureReason = FText::GetEmpty();
	const AItemBoxActor* BoxCDO = BoxClass ? BoxClass->GetDefaultObject<AItemBoxActor>() : nullptr;
	const UStaticMeshComponent* Root = BoxCDO ? BoxCDO->BoxMesh.Get() : nullptr;
	const UStaticMesh* Mesh = Root ? Root->GetStaticMesh() : nullptr;
	if (!Mesh || WorldTransform.ContainsNaN())
	{
		OutFailureReason = LOCTEXT("InvalidBoxClass", "품목 박스 클래스를 확인할 수 없습니다.");
		return false;
	}
	const FVector RootScale = Root->GetRelativeScale3D();
	if (!FMath::IsFinite(RootScale.X) || !FMath::IsFinite(RootScale.Y) || !FMath::IsFinite(RootScale.Z)
		|| RootScale.X <= KINDA_SMALL_NUMBER || RootScale.Y <= KINDA_SMALL_NUMBER
		|| RootScale.Z <= KINDA_SMALL_NUMBER)
	{
		OutFailureReason = LOCTEXT("InvalidBoxClassScale", "품목 박스 클래스의 루트 스케일이 올바르지 않습니다.");
		return false;
	}
	const FBoxSphereBounds Bounds = Mesh->GetBounds();
	const FVector Extent = Bounds.BoxExtent * RootScale;
	if (Extent.ContainsNaN() || Extent.GetMin() <= KINDA_SMALL_NUMBER)
	{
		OutFailureReason = LOCTEXT("InvalidBoxClassBounds", "품목 박스 크기가 올바르지 않습니다.");
		return false;
	}
	OutRotation = WorldTransform.GetRotation();
	OutLocation = WorldTransform.GetLocation() + OutRotation.RotateVector(Bounds.Origin * RootScale);
	OutShape = FCollisionShape::MakeBox(Extent);
	OutCollisionTemplate = Root;
	return true;
}

FPlayerInteractionQuery AItemBoxActor::QueryInteraction(const FPlayerInteractionContext& Context) const
{
	FPlayerInteractionQuery Query;
	if (Lifecycle != ELifecycle::FreeWorld || !bContentsInitialized)
	{
		return Query;
	}
	Query.bVisible = true;
	Query.TargetName = GetContentsSummary();
	Query.ActionName = LOCTEXT("TakeBox", "들기");
	FText Failure;
	Query.bCanInteract = Context.CarryComponent && CanBeTakenBy(*Context.CarryComponent, Failure);
	Query.FailureReason = Failure;
	return Query;
}

FPlayerInteractionResult AItemBoxActor::ExecuteInteraction(const FPlayerInteractionContext& Context)
{
	FText Failure;
	return Context.CarryComponent && Context.CarryComponent->TryTakePhysicalObject(this, Failure)
		? FPlayerInteractionResult::Succeeded()
		: FPlayerInteractionResult::Failed(
			Failure.IsEmpty() ? LOCTEXT("TakeBoxFailed", "품목 박스를 들 수 없습니다.") : Failure);
}

FTransform AItemBoxActor::GetHeldTransform() const
{
	FTransform Result = HeldTransform;
	Result.SetScale3D(FVector::OneVector);
	return Result;
}

bool AItemBoxActor::CanBeTakenBy(const UPlayerCarryComponent& Carry, FText& OutFailureReason) const
{
	if (Lifecycle != ELifecycle::FreeWorld || !bContentsInitialized || Carrier.IsValid()
		|| !Carry.IsHandEmpty())
	{
		OutFailureReason = LOCTEXT("BoxUnavailable", "품목 박스를 들 수 없습니다.");
		return false;
	}
	return true;
}

bool AItemBoxActor::HandleTakenBy(UPlayerCarryComponent& Carry, USceneComponent* HeldAnchor)
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

bool AItemBoxActor::CanFreeDrop(FText& OutFailureReason) const
{
	if (Lifecycle != ELifecycle::Held || !Carrier.IsValid() || Carrier->GetHeldObject() != this)
	{
		OutFailureReason = LOCTEXT("BoxNotHeld", "품목 박스를 들고 있어야 내려놓을 수 있습니다.");
		return false;
	}
	return true;
}

UPrimitiveComponent* AItemBoxActor::GetPhysicalCarryPrimitive() const
{
	return BoxMesh;
}

bool AItemBoxActor::NotifyPhysicalDropCommitted(UPlayerCarryComponent& Carry)
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

void AItemBoxActor::PublishPhysicalCarryCommit(const EPhysicalCarryCommitTransition Transition)
{
	(void)Transition;
}

void AItemBoxActor::RecoverPhysicalCarryable(UPlayerCarryComponent* PreviousCarry)
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

bool AItemBoxActor::CanDiscardCarriedObject(FText& OutFailureReason) const
{
	if (Lifecycle != ELifecycle::Held || !Carrier.IsValid() || Carrier->GetHeldObject() != this
		|| !bContentsInitialized)
	{
		OutFailureReason = LOCTEXT("BoxNotDiscardable", "이 품목 박스는 버릴 수 없습니다.");
		return false;
	}
	return true;
}

void AItemBoxActor::HandleDiscardCommitted()
{
	if (Lifecycle != ELifecycle::Held)
	{
		return;
	}
	Carrier.Reset();
	Contents = FServiceItemStack();
	bContentsInitialized = false;
	Lifecycle = ELifecycle::Consumed;
	Destroy();
}

void AItemBoxActor::SetFreeWorldPhysics(const bool bEnabled)
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

void AItemBoxActor::RebuildContentsVisual(const UServiceItemDefinition* Kind, const int32 Count)
{
	if (!ContentsVisual)
	{
		return;
	}
	ContentsVisual->ClearInstances();
	if (!Kind || Count <= 0 || !Kind->ItemMesh)
	{
		return;
	}
	ContentsVisual->SetStaticMesh(Kind->ItemMesh);
	const int32 Visible = FMath::Min(Count, Kind->BoxSlotTransforms.Num());
	for (int32 Index = 0; Index < Visible; ++Index)
	{
		ContentsVisual->AddInstance(Kind->BoxSlotTransforms[Index], false);
	}
}

#if WITH_EDITOR
void AItemBoxActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	const UWorld* World = GetWorld();
	if (World && (World->WorldType == EWorldType::Editor || World->WorldType == EWorldType::EditorPreview))
	{
		RefreshEditorPreview();
	}
}

void AItemBoxActor::RefreshEditorPreview()
{
	const UWorld* World = GetWorld();
	if (!World || (World->WorldType != EWorldType::Editor && World->WorldType != EWorldType::EditorPreview))
	{
		return;
	}
	const UServiceItemDefinition* Kind = EditorPreviewKind;
	int32 Count = 0;
	if (Kind)
	{
		Count = FMath::Clamp(EditorPreviewCount, 0, Kind->BoxCapacity);
	}
	RebuildContentsVisual(Kind, Count);
}

EDataValidationResult AItemBoxActor::IsDataValid(FDataValidationContext& Context) const
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
		Invalidate(LOCTEXT("InvalidBoxAuthoring", "Item box requires a mesh root with QueryAndPhysics, CCD, Pawn Ignore, zero relative location/rotation, and a positive finite scale."));
	}
	if (!ContentsVisual || ContentsVisual->GetCollisionEnabled() != ECollisionEnabled::NoCollision)
	{
		Invalidate(LOCTEXT("InvalidContentsVisual", "Item box ContentsVisual must exist with NoCollision."));
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
