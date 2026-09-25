#include "Facility/BathWaterUtilityFacilityActor.h"

#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Facility/BathWaterOperationsSubsystem.h"
#include "Facility/BathWaterUtilityCapacityComponent.h"
#include "Facility/BathWaterUtilityPlacementInstanceData.h"
#include "Interaction/PlayerCarryComponent.h"
#include "Placement/FacilityActorConversionTransaction.h"
#include "Placement/FacilityPlacementComponent.h"
#include "Placement/FacilityPlacementDefinition.h"
#include "Placement/FacilityPlacementSettings.h"
#include "Placement/FacilityPlacementZoneActor.h"
#include "Placement/PlaceableFacilityItemActor.h"
#include "Utility/UtilityOperationComponent.h"
#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

#define LOCTEXT_NAMESPACE "BathWaterUtilityFacilityActor"

DEFINE_LOG_CATEGORY_STATIC(LogBathWaterUtility, Log, All);

ABathWaterUtilityFacilityActor::ABathWaterUtilityFacilityActor()
{
	PrimaryActorTick.bCanEverTick = false;
	PackagePhysicalRoot = CreateDefaultSubobject<UBoxComponent>(TEXT("PackagePhysicalRoot"));
	SetRootComponent(PackagePhysicalRoot);
	PackagePhysicalRoot->SetBoxExtent(FVector(5.0f));
	PackagePhysicalRoot->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PackagePhysicalRoot->SetCanEverAffectNavigation(false);
	PackagePhysicalRoot->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
	PackagePhysicalRoot->BodyInstance.bUseCCD = true;
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SceneRoot->SetupAttachment(PackagePhysicalRoot);
	VisualMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VisualMesh"));
	VisualMesh->SetupAttachment(SceneRoot);
	VisualMesh->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	VisualMesh->SetCanEverAffectNavigation(true);
	PlacementFootprint = CreateDefaultSubobject<UBoxComponent>(TEXT("PlacementFootprint"));
	PlacementFootprint->SetupAttachment(SceneRoot);
	PlacementFootprint->SetBoxExtent(FVector(30.0f, 30.0f, 50.0f));
	PlacementFootprint->SetRelativeLocation(FVector(0.0f, 0.0f, 50.0f));
	PlacementFootprint->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PlacementFootprint->SetCanEverAffectNavigation(false);
	FacilityPlacement = CreateDefaultSubobject<UFacilityPlacementComponent>(TEXT("FacilityPlacement"));
	FacilityPlacement->Configure(PlacementFootprint, PackagePhysicalRoot);
	Capacity = CreateDefaultSubobject<UBathWaterUtilityCapacityComponent>(TEXT("Capacity"));
}

void ABathWaterUtilityFacilityActor::BeginPlay()
{
	Super::BeginPlay();
	BindUtilityOperation();
	if (FacilityPlacement && FacilityPlacement->IsPlacedDomainActive())
	{
		FText FailureReason;
		if (HasValidUtilityAuthoring(FailureReason))
		{
			if (UUtilityOperationComponent* Operation = GetUtilityOperation())
			{
				Operation->StartPlacedClock(false);
			}
			if (UBathWaterOperationsSubsystem* Operations = GetWorld()->GetSubsystem<UBathWaterOperationsSubsystem>())
			{
				bProviderRegistered = Operations->RegisterProvider(Capacity);
			}
			if (!bProviderRegistered)
			{
				if (UUtilityOperationComponent* Operation = GetUtilityOperation())
				{
					Operation->StopPlacedClock(false);
				}
			}
		}
		else
		{
			UE_LOG(LogBathWaterUtility, Warning, TEXT("Rejected invalid placed utility %s: %s"),
				*GetName(), *FailureReason.ToString());
		}
	}
}

void ABathWaterUtilityFacilityActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnbindUtilityOperation();
	if (UUtilityOperationComponent* Operation = GetUtilityOperation())
	{
		Operation->SetLaborBlocked(true);
		Operation->StopPlacedClock(false);
	}
	if (bProviderRegistered)
	{
		if (UBathWaterOperationsSubsystem* Operations = GetWorld()
			? GetWorld()->GetSubsystem<UBathWaterOperationsSubsystem>() : nullptr)
		{
			Operations->UnregisterProvider(Capacity, true, EndPlayReason == EEndPlayReason::Destroyed);
		}
		bProviderRegistered = false;
	}
	Super::EndPlay(EndPlayReason);
}

void ABathWaterUtilityFacilityActor::FellOutOfWorld(const UDamageType& DamageType)
{
	Super::FellOutOfWorld(DamageType);
}

FPlayerInteractionQuery ABathWaterUtilityFacilityActor::QueryInteraction(const FPlayerInteractionContext& Context) const
{
	(void)Context;
	return FPlayerInteractionQuery();
}

FPlayerInteractionResult ABathWaterUtilityFacilityActor::ExecuteInteraction(const FPlayerInteractionContext& Context)
{
	(void)Context;
	return FPlayerInteractionResult::Failed(LOCTEXT("NoDirectUse", "이 설비는 직접 조작하지 않습니다."));
}

UUtilityOperationComponent* ABathWaterUtilityFacilityActor::GetUtilityOperation() const
{
	return nullptr;
}

bool ABathWaterUtilityFacilityActor::HasValidUtilityAuthoring(FText& OutFailureReason) const
{
	if (!Capacity || !Capacity->HasValidAuthoring(OutFailureReason))
	{
		if (OutFailureReason.IsEmpty())
		{
			OutFailureReason = LOCTEXT("MissingUtilityCapacity", "설비 용량 component가 없습니다.");
		}
		return false;
	}
	if (RequiresLaborOperation())
	{
		const UUtilityOperationComponent* Operation = GetUtilityOperation();
		if (!Operation || !Operation->HasValidAuthoring(OutFailureReason))
		{
			if (OutFailureReason.IsEmpty())
			{
				OutFailureReason = LOCTEXT("MissingRequiredOperation", "노동 가동 설비에 Operation component가 없습니다.");
			}
			return false;
		}
	}
	OutFailureReason = FText::GetEmpty();
	return true;
}

void ABathWaterUtilityFacilityActor::BindUtilityOperation()
{
	if (Capacity)
	{
		Capacity->SetUtilityOperation(GetUtilityOperation(), RequiresLaborOperation());
	}
	if (UUtilityOperationComponent* Operation = GetUtilityOperation())
	{
		BoundUtilityOperation = Operation;
		Operation->OnOperatingChanged.AddUObject(
			this, &ABathWaterUtilityFacilityActor::HandleUtilityOperatingChanged);
	}
}

void ABathWaterUtilityFacilityActor::UnbindUtilityOperation()
{
	if (BoundUtilityOperation.IsValid())
	{
		BoundUtilityOperation->OnOperatingChanged.RemoveAll(this);
	}
	BoundUtilityOperation.Reset();
}

void ABathWaterUtilityFacilityActor::HandleUtilityOperatingChanged(const bool bIsOperating)
{
	(void)bIsOperating;
	if (!bProviderRegistered)
	{
		return;
	}
	if (UBathWaterOperationsSubsystem* Operations = GetWorld()
		? GetWorld()->GetSubsystem<UBathWaterOperationsSubsystem>() : nullptr)
	{
		Operations->PublishMutation(false, true);
	}
}

FPlayerInteractionQuery ABathWaterUtilityFacilityActor::MergeSupplementalInteractionQuery(
	const FPlayerInteractionQuery& BaseQuery) const
{
	FPlayerInteractionQuery Query = BaseQuery;
	if (!FacilityPlacement || FacilityPlacement->GetMode() != EPlaceableFacilityMode::Placed
		|| FacilityPlacement->IsStagedPlacement())
	{
		return Query;
	}
	const FFacilityPlacementTransactionResult Recovery = QueryFacilityRecovery();
	Query.bRecoveryVisible = true;
	Query.bCanRecover = Recovery.bSucceeded;
	Query.RecoveryActionName = LOCTEXT("RecoverUtility", "설비 회수");
	Query.RecoveryFailureReason = Recovery.FailureReason;
	Query.RecoveryProgress = 0.0f;
	return Query;
}

FFacilityPlacementTransactionResult ABathWaterUtilityFacilityActor::QueryFacilityPlacement(
	const FTransform& CandidateTransform,
	const AFacilityPlacementZoneActor& Zone) const
{
	(void)CandidateTransform;
	FText FailureReason;
	if (!FacilityPlacement || !HasValidUtilityAuthoring(FailureReason)
		|| !FacilityPlacement->IsOperational(FailureReason)
		|| !FacilityPlacement->GetDefinition()->ValidateRuntime(FailureReason))
	{
		return FFacilityPlacementTransactionResult::Failed(EFacilityPlacementFailureCode::InvalidComponents, FailureReason);
	}
	if (FacilityPlacement->GetMode() != EPlaceableFacilityMode::Placed || !FacilityPlacement->IsStagedPlacement())
	{
		return FFacilityPlacementTransactionResult::Failed(EFacilityPlacementFailureCode::WrongMode,
			LOCTEXT("UtilityNotStaged", "새로 생성된 staged 설비만 설치할 수 있습니다."));
	}
	if (!Zone.IsDefinitionAllowed(*FacilityPlacement->GetDefinition()))
	{
		return FFacilityPlacementTransactionResult::Failed(EFacilityPlacementFailureCode::NoCompatibleZone,
			LOCTEXT("UtilityZoneMismatch", "이 구역에는 해당 설비를 설치할 수 없습니다."));
	}
	return FFacilityPlacementTransactionResult::Succeeded();
}

FFacilityPlacementTransactionResult ABathWaterUtilityFacilityActor::QueryFacilityRecovery() const
{
	FText FailureReason;
	if (!FacilityPlacement || !Capacity || !FacilityPlacement->IsOperational(FailureReason)
		|| FacilityPlacement->GetMode() != EPlaceableFacilityMode::Placed
		|| FacilityPlacement->IsStagedPlacement() || !FacilityPlacement->IsPlacedDomainActive()
		|| !bProviderRegistered || !HasValidUtilityAuthoring(FailureReason))
	{
		return FFacilityPlacementTransactionResult::Failed(EFacilityPlacementFailureCode::WrongMode,
			FailureReason.IsEmpty() ? LOCTEXT("UtilityNotPlaced", "설치된 설비만 회수할 수 있습니다.") : FailureReason);
	}
	UBathWaterOperationsSubsystem* Operations = GetWorld()
		? GetWorld()->GetSubsystem<UBathWaterOperationsSubsystem>() : nullptr;
	float Deficit = 0.0f;
	if (!Operations || !Operations->CanRemoveProvider(Capacity, Deficit))
	{
		return FFacilityPlacementTransactionResult::Failed(EFacilityPlacementFailureCode::DomainCondition,
			FText::Format(LOCTEXT("UtilityCapacityInUse", "회수 후 용량이 {0}포인트 부족합니다."), FText::AsNumber(Deficit)));
	}
	FTransform ItemTransform;
	if (!FFacilityActorConversionTransaction::ValidateRecoveryCandidate(
		*const_cast<ABathWaterUtilityFacilityActor*>(this), ItemTransform, FailureReason))
	{
		return FFacilityPlacementTransactionResult::Failed(EFacilityPlacementFailureCode::Blocked, FailureReason);
	}
	return FFacilityPlacementTransactionResult::Succeeded();
}

bool ABathWaterUtilityFacilityActor::TryBeginFacilityRecoveryHold(FText& OutFailureReason)
{
	if (bRecoveryHoldActive)
	{
		OutFailureReason = LOCTEXT("UtilityRecoveryAlreadyHeld", "설비 회수가 이미 진행 중입니다.");
		return false;
	}
	const FFacilityPlacementTransactionResult Query = QueryFacilityRecovery();
	if (!Query.bSucceeded)
	{
		OutFailureReason = Query.FailureReason;
		return false;
	}
	bRecoveryHoldActive = true;
	if (UUtilityOperationComponent* Operation = GetUtilityOperation())
	{
		Operation->SetLaborBlocked(true);
	}
	return true;
}

void ABathWaterUtilityFacilityActor::CancelFacilityRecoveryHold()
{
	bRecoveryHoldActive = false;
	if (UUtilityOperationComponent* Operation = GetUtilityOperation())
	{
		Operation->SetLaborBlocked(false);
	}
}

bool ABathWaterUtilityFacilityActor::ExportPlacementPayload(
	APlaceableFacilityItemActor& Item,
	FFacilityPlacementPayload& OutPayload,
	FText& OutFailureReason) const
{
	if (!FacilityPlacement || !FacilityPlacement->GetDefinition() || !HasValidUtilityAuthoring(OutFailureReason))
	{
		return false;
	}
	UBathWaterUtilityPlacementInstanceData* Data = NewObject<UBathWaterUtilityPlacementInstanceData>(&Item);
	if (!Data)
	{
		OutFailureReason = LOCTEXT("UtilityPayloadAllocationFailed", "설비 변환 데이터를 생성할 수 없습니다.");
		return false;
	}
	Data->CapacityKind = Capacity->GetCapacityKind();
	Data->CapacityPoints = Capacity->GetCapacityPoints();
	if (RequiresLaborOperation())
	{
		const UUtilityOperationComponent* Operation = GetUtilityOperation();
		if (!Operation)
		{
			OutFailureReason = LOCTEXT("MissingOperationForExport", "가동 잔량을 회수 payload에 저장할 수 없습니다.");
			return false;
		}
		Data->bHasOperationState = true;
		Data->RemainingOperationPoints = Operation->GetRemainingPoints();
	}
	OutPayload.Definition = FacilityPlacement->GetDefinition();
	OutPayload.InstanceData = Data;
	return OutPayload.Validate(Item, OutFailureReason);
}

bool ABathWaterUtilityFacilityActor::ImportPlacementPayload(
	const APlaceableFacilityItemActor& Item,
	const FFacilityPlacementPayload& Payload,
	FText& OutFailureReason)
{
	const UBathWaterUtilityPlacementInstanceData* Data =
		Cast<UBathWaterUtilityPlacementInstanceData>(Payload.InstanceData);
	if (!Data || !Payload.Validate(Item, OutFailureReason) || !Capacity || !FacilityPlacement
		|| !FacilityPlacement->IsStagedPlacement()
		|| FacilityPlacement->GetDefinition() != Payload.Definition
		|| Payload.Definition->PlacedFacilityClass.Get() != GetClass()
		|| Data->CapacityKind != Capacity->GetCapacityKind()
		|| !FMath::IsFinite(Data->CapacityPoints) || Data->CapacityPoints < 0.0f
		|| !HasValidUtilityAuthoring(OutFailureReason))
	{
		if (OutFailureReason.IsEmpty())
		{
			OutFailureReason = LOCTEXT("InvalidUtilityPayload", "설비 종류 또는 용량 변환 데이터가 올바르지 않습니다.");
		}
		return false;
	}
	Capacity->RestoreCapacity(Data->CapacityKind, Data->CapacityPoints);
	if (RequiresLaborOperation())
	{
		UUtilityOperationComponent* Operation = GetUtilityOperation();
		const float ImportedPoints = Data->bHasOperationState ? Data->RemainingOperationPoints : 0.0f;
		if (!Operation || !Operation->ImportOperationState(ImportedPoints, OutFailureReason))
		{
			return false;
		}
	}
	else if (Data->bHasOperationState)
	{
		OutFailureReason = LOCTEXT("UnexpectedOperationPayload", "가동 상태가 없는 설비에 가동 payload를 가져올 수 없습니다.");
		return false;
	}
	return true;
}

bool ABathWaterUtilityFacilityActor::StagePlacedDomainRegistration(FText& OutFailureReason)
{
	if (!FacilityPlacement || !FacilityPlacement->IsStagedPlacement())
	{
		OutFailureReason = LOCTEXT("UtilityRegistrationNotStaged", "새 설비가 staged 상태가 아닙니다.");
		return false;
	}
	UBathWaterOperationsSubsystem* Operations = GetWorld()
		? GetWorld()->GetSubsystem<UBathWaterOperationsSubsystem>() : nullptr;
	if (!HasValidUtilityAuthoring(OutFailureReason))
	{
		return false;
	}
	if (!Operations)
	{
		OutFailureReason = LOCTEXT("UtilityOperationsUnavailable", "설비 용량을 등록할 운영 시스템을 찾을 수 없습니다.");
		return false;
	}
	if (!Operations->RegisterProvider(Capacity, false))
	{
		if (OutFailureReason.IsEmpty())
		{
			OutFailureReason = LOCTEXT("UtilityRegistrationFailed", "설비 용량을 등록할 수 없습니다.");
		}
		return false;
	}
	bProviderRegistered = true;
	if (UUtilityOperationComponent* Operation = GetUtilityOperation())
	{
		if (!Operation->StartPlacedClock(false))
		{
			Operations->UnregisterProvider(Capacity, false);
			bProviderRegistered = false;
			OutFailureReason = LOCTEXT("OperationClockStartFailed", "설비 가동 시계를 시작할 수 없습니다.");
			return false;
		}
	}
	return true;
}

void ABathWaterUtilityFacilityActor::RollbackPlacedDomainRegistration()
{
	if (bProviderRegistered)
	{
		if (UBathWaterOperationsSubsystem* Operations = GetWorld()
			? GetWorld()->GetSubsystem<UBathWaterOperationsSubsystem>() : nullptr)
		{
			Operations->UnregisterProvider(Capacity, false);
		}
		bProviderRegistered = false;
	}
	if (UUtilityOperationComponent* Operation = GetUtilityOperation())
	{
		Operation->StopPlacedClock(false);
	}
	if (FacilityPlacement)
	{
		FacilityPlacement->SetPlacedDomainActive(false);
	}
}

bool ABathWaterUtilityFacilityActor::StagePlacedDomainUnregistration(
	FFacilityPlacementPublication& OutPublication,
	FText& OutFailureReason)
{
	if (!bRecoveryHoldActive || !FacilityPlacement || FacilityPlacement->IsStagedPlacement()
		|| !FacilityPlacement->IsPlacedDomainActive() || !bProviderRegistered)
	{
		OutFailureReason = LOCTEXT("UtilityDomainNotPlaced", "설비가 회수 가능한 설치 상태가 아닙니다.");
		return false;
	}
	UBathWaterOperationsSubsystem* Operations = GetWorld()
		? GetWorld()->GetSubsystem<UBathWaterOperationsSubsystem>() : nullptr;
	UUtilityOperationComponent* Operation = GetUtilityOperation();
	if (Operation)
	{
		Operation->StopPlacedClock(false);
	}
	if (!Operations || !Operations->UnregisterProvider(Capacity, false))
	{
		if (Operation)
		{
			Operation->StartPlacedClock(true);
		}
		OutFailureReason = LOCTEXT("UtilityUnregistrationFailed", "설비 용량 등록을 해제할 수 없습니다.");
		return false;
	}
	bProviderRegistered = false;
	if (!FacilityPlacement->CaptureAndDisableActorCollision(OutFailureReason))
	{
		bProviderRegistered = Operations->RegisterProvider(Capacity, false);
		if (Operation)
		{
			Operation->StartPlacedClock(true);
		}
		return false;
	}
	FacilityPlacement->SetPlacedDomainActive(false);
	TWeakObjectPtr<UBathWaterOperationsSubsystem> WeakOperations(Operations);
	OutPublication.Callback = [WeakOperations]()
	{
		if (WeakOperations.IsValid())
		{
			WeakOperations->PublishMutation();
		}
	};
	return true;
}

bool ABathWaterUtilityFacilityActor::RollbackPlacedDomainUnregistration(FText& OutFailureReason)
{
	if (!FacilityPlacement || IsActorBeingDestroyed())
	{
		OutFailureReason = LOCTEXT("UtilityRollbackUnavailable", "설비 설치 상태를 복구할 수 없습니다.");
		return false;
	}
	UBathWaterOperationsSubsystem* Operations = GetWorld()
		? GetWorld()->GetSubsystem<UBathWaterOperationsSubsystem>() : nullptr;
	if (!Operations || !Operations->RegisterProvider(Capacity, false))
	{
		OutFailureReason = LOCTEXT("UtilityRollbackRegistrationFailed", "설비 용량 등록을 복구할 수 없습니다.");
		return false;
	}
	bProviderRegistered = true;
	if (!FacilityPlacement->RestoreActorCollisionSnapshot(OutFailureReason))
	{
		Operations->UnregisterProvider(Capacity, false);
		bProviderRegistered = false;
		return false;
	}
	FacilityPlacement->SetPlacedDomainActive(true);
	bRecoveryHoldActive = false;
	if (UUtilityOperationComponent* Operation = GetUtilityOperation())
	{
		Operation->SetLaborBlocked(false);
		if (!Operation->StartPlacedClock(true))
		{
			OutFailureReason = LOCTEXT("UtilityRollbackClockFailed", "설비 가동 시계를 복구할 수 없습니다.");
			return false;
		}
	}
	return true;
}

void ABathWaterUtilityFacilityActor::PublishPlacedDomainRegistration()
{
	if (UBathWaterOperationsSubsystem* Operations = GetWorld()
		? GetWorld()->GetSubsystem<UBathWaterOperationsSubsystem>() : nullptr)
	{
		Operations->PublishMutation();
	}
}

bool ABathWaterUtilityFacilityActor::CommitPlaceableFacilityMode(
	const EPlaceableFacilityMode NewMode,
	FText& OutFailureReason)
{
	if (NewMode == EPlaceableFacilityMode::Placed && FacilityPlacement
		&& FacilityPlacement->GetMode() == EPlaceableFacilityMode::Placed)
	{
		return true;
	}
	OutFailureReason = LOCTEXT("UtilityLegacyModeDisabled", "설비의 legacy mode 전환은 비활성화되었습니다.");
	return false;
}

FText ABathWaterUtilityFacilityActor::GetPhysicalCarryDisplayName() const
{
	return LOCTEXT("UtilityPackage", "포장 물 설비");
}

FTransform ABathWaterUtilityFacilityActor::GetHeldTransform() const
{
	return GetDefault<UFacilityPlacementSettings>()->GetFacilityItemHeldTransform();
}

bool ABathWaterUtilityFacilityActor::CanBeTakenBy(const UPlayerCarryComponent& Carry, FText& OutFailureReason) const
{
	(void)Carry;
	OutFailureReason = LOCTEXT("PlacedUtilityNotCarryable", "배치된 설비는 직접 들 수 없습니다.");
	return false;
}

bool ABathWaterUtilityFacilityActor::HandleTakenBy(UPlayerCarryComponent& Carry, USceneComponent* HeldAnchor)
{
	(void)Carry; (void)HeldAnchor; return false;
}
bool ABathWaterUtilityFacilityActor::CanFreeDrop(FText& OutFailureReason) const
{
	OutFailureReason = LOCTEXT("PlacedUtilityNoDrop", "배치된 설비는 내려놓을 수 없습니다."); return false;
}
UPrimitiveComponent* ABathWaterUtilityFacilityActor::GetPhysicalCarryPrimitive() const { return nullptr; }
float ABathWaterUtilityFacilityActor::GetThrowImpulseStrength() const { return FacilityPlacement ? FacilityPlacement->GetThrowImpulseStrength() : 120.0f; }
float ABathWaterUtilityFacilityActor::GetUpwardThrowImpulseStrength() const { return FacilityPlacement ? FacilityPlacement->GetUpwardThrowImpulseStrength() : 15.0f; }
AActor* ABathWaterUtilityFacilityActor::GetAssignedPhysicalCarryFixedSlot() const { return nullptr; }
bool ABathWaterUtilityFacilityActor::TryBindPhysicalCarryFixedSlot(AActor& SlotActor, FText& OutFailureReason) { (void)SlotActor; OutFailureReason = LOCTEXT("UtilityNoFixedSlot", "설비는 고정 슬롯을 지원하지 않습니다."); return false; }
void ABathWaterUtilityFacilityActor::ClearPhysicalCarryFixedSlotBinding(AActor& ExpectedSlot) { (void)ExpectedSlot; }
void ABathWaterUtilityFacilityActor::NotifyPhysicalCarryFixedSlotBindingConflict() {}
bool ABathWaterUtilityFacilityActor::IsStoredInAssignedPhysicalCarryFixedSlot() const { return false; }
bool ABathWaterUtilityFacilityActor::NotifyTakenFromFixedSlotCommitted(UPlayerCarryComponent& Carry, AActor& SlotActor) { (void)Carry; (void)SlotActor; return false; }
bool ABathWaterUtilityFacilityActor::NotifyStoredInFixedSlotCommitted(UPlayerCarryComponent& Carry, AActor& SlotActor) { (void)Carry; (void)SlotActor; return false; }
bool ABathWaterUtilityFacilityActor::NotifyRecoveredToFixedSlotCommitted(AActor& SlotActor) { (void)SlotActor; return false; }
void ABathWaterUtilityFacilityActor::NotifyFixedSlotDestroyed(AActor& SlotActor) { (void)SlotActor; }
bool ABathWaterUtilityFacilityActor::NotifyPhysicalDropCommitted(UPlayerCarryComponent& Carry) { (void)Carry; return false; }
void ABathWaterUtilityFacilityActor::PublishPhysicalCarryCommit(EPhysicalCarryCommitTransition Transition) { (void)Transition; }
void ABathWaterUtilityFacilityActor::RecoverPhysicalCarryable(UPlayerCarryComponent* PreviousCarry) { (void)PreviousCarry; }

#if WITH_EDITOR
EDataValidationResult ABathWaterUtilityFacilityActor::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = Super::IsDataValid(Context);
	FText FailureReason;
	if (!Capacity || !FacilityPlacement || !PlacementFootprint || !PackagePhysicalRoot || !VisualMesh
		|| !HasValidUtilityAuthoring(FailureReason))
	{
		Context.AddError(FailureReason.IsEmpty()
			? LOCTEXT("MissingUtilityComponents", "물 설비 native component 구성이 완전하지 않습니다.")
			: FailureReason);
		Result = EDataValidationResult::Invalid;
	}
	return Result == EDataValidationResult::NotValidated ? EDataValidationResult::Valid : Result;
}
#endif

#undef LOCTEXT_NAMESPACE
