#include "Service/ScrubTableActor.h"
#include "Service/PlayerScrubFocusComponent.h"
#include "Service/ServiceAmenityTypes.h"
#include "Facility/BathhouseFacilitySlotComponent.h"
#include "Facility/FacilityPlacementExtensionUtils.h"
#include "Placement/FacilityPlacementComponent.h"
#include "Economy/BathhouseCashPaymentActor.h"
#include "Interaction/PlayerCarryComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif
AScrubTableActor::AScrubTableActor()
{
	FacilityType = EBathhouseFacilityType::ScrubTable;
	ScrubCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("ScrubCamera"));
	ScrubCamera->SetupAttachment(SceneRoot);
	ScrubArea = CreateDefaultSubobject<UBoxComponent>(TEXT("ScrubArea"));
	ScrubArea->SetupAttachment(SceneRoot);
	ScrubArea->SetBoxExtent(FVector(90, 35, 1));
	ScrubArea->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ScrubArea->SetCanEverAffectNavigation(false);
	ScrubCursor = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ScrubCursor"));
	ScrubCursor->SetupAttachment(SceneRoot);
	ScrubCursor->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ScrubCursor->SetCanEverAffectNavigation(false);
	ScrubCursor->SetHiddenInGame(true);
	ScrubExitPoint = CreateDefaultSubobject<USceneComponent>(TEXT("ScrubExitPoint"));
	ScrubExitPoint->SetupAttachment(SceneRoot);
	CashOfferPoint = CreateDefaultSubobject<USceneComponent>(TEXT("CashOfferPoint"));
	CashOfferPoint->SetupAttachment(SceneRoot);
	CashStandPoint = CreateDefaultSubobject<USceneComponent>(TEXT("CashStandPoint"));
	CashStandPoint->SetupAttachment(SceneRoot);
}

void AScrubTableActor::BeginPlay()
{
	Super::BeginPlay();
	for (UBathhouseFacilitySlotComponent* Slot : GetFacilitySlots())
	{
		Slot->OnSlotStateChanged.AddDynamic(this, &AScrubTableActor::HandleSlotChanged);
	}
}

void AScrubTableActor::EndPlay(EEndPlayReason::Type Reason)
{
	GetWorldTimerManager().ClearTimer(WaitTimer);
	NotifySessionEnded();
	for (UBathhouseFacilitySlotComponent* Slot : GetFacilitySlots())
	{
		Slot->OnSlotStateChanged.RemoveDynamic(this, &AScrubTableActor::HandleSlotChanged);
		Slot->ForceRelease();
	}
	BindUser(nullptr);
	OnScrubSessionEnded.Clear();
	Super::EndPlay(Reason);
}

void AScrubTableActor::BindUser(AActor* User)
{
	if (auto* Previous = CurrentUser.Get())
	{
		Previous->OnEndPlay.RemoveDynamic(this, &AScrubTableActor::HandleUserEndPlay);
	}
	CurrentUser = User;
	if (User)
	{
		User->OnEndPlay.AddUniqueDynamic(this, &AScrubTableActor::HandleUserEndPlay);
	}
}

void AScrubTableActor::HandleSlotChanged(UBathhouseFacilitySlotComponent* Slot, EBathhouseFacilitySlotState Previous,
										 EBathhouseFacilitySlotState State)
{
	GetWorldTimerManager().ClearTimer(WaitTimer);
	RubDistanceCm = 0;
	WaitDeadline = 0;
	BindUser(Slot->GetCurrentUser());
	if (State == EBathhouseFacilitySlotState::Occupied)
	{
		WaitDeadline = GetWorld()->GetTimeSeconds() + WaitLimitSeconds;
		GetWorldTimerManager().SetTimer(WaitTimer, this, &AScrubTableActor::ExpireWait, WaitLimitSeconds, false);
	}
	else if (Previous == EBathhouseFacilitySlotState::Occupied)
	{
		NotifySessionEnded();
	}
}

void AScrubTableActor::HandleUserEndPlay(AActor* User, EEndPlayReason::Type Reason)
{
	for (UBathhouseFacilitySlotComponent* Slot : GetFacilitySlots())
	{
		if (Slot->GetCurrentUser() == User)
		{
			Slot->ForceRelease();
		}
	}
	BindUser(nullptr);
	NotifySessionEnded();
}

bool AScrubTableActor::HasOccupiedUser() const
{
	return CurrentUser.IsValid() && GetFacilitySlots().Num() == 1 &&
		   GetFacilitySlots()[0]->GetCurrentUser() == CurrentUser.Get() &&
		   GetFacilitySlots()[0]->GetSlotState() == EBathhouseFacilitySlotState::Occupied && FacilityPlacement &&
		   FacilityPlacement->IsPlacedDomainActive() && !FacilityPlacement->IsStagedPlacement();
}

float AScrubTableActor::GetScrubProgress() const
{
	return FMath::Clamp(RubDistanceCm / FMath::Max(UE_SMALL_NUMBER, RequiredRubDistanceCm), 0.f, 1.f);
}

float AScrubTableActor::GetRemainingWaitSeconds() const
{
	return HasOccupiedUser() ? FMath::Max(0.0, WaitDeadline - GetWorld()->GetTimeSeconds()) : 0;
}

FTransform AScrubTableActor::GetExitFootTransform() const
{
	return ScrubExitPoint->GetComponentTransform();
}

bool AScrubTableActor::TryBeginScrubSession(UPlayerScrubFocusComponent& Scrubber)
{
	if (!HasOccupiedUser() || ActiveScrubber.IsValid() || bCompleting)
	{
		return false;
	}
	ActiveScrubber = &Scrubber;
	return true;
}

void AScrubTableActor::EndScrubSession(UPlayerScrubFocusComponent& Scrubber)
{
	if (ActiveScrubber.Get() == &Scrubber)
	{
		ActiveScrubber.Reset();
	}
}

void AScrubTableActor::NotifySessionEnded()
{
	// Keep the reservation through focus-out; its owner releases it after restoring input.
	OnScrubSessionEnded.Broadcast();
}

void AScrubTableActor::ExpireWait()
{
	if (!HasOccupiedUser())
	{
		return;
	}
	AActor* User = CurrentUser.Get();
	auto* Slot = GetFacilitySlots()[0].Get();
	Slot->EndUse(User);
	Slot->Release(User);
	if (auto* ServiceUser = Cast<IServiceFacilityUser>(User))
	{
		ServiceUser->HandleServiceUseEnded(*this, EServiceUseEndReason::Abandoned);
	}
}

void AScrubTableActor::AddRubDistance(UPlayerScrubFocusComponent& Scrubber, float Cm)
{
	if (bCompleting || ActiveScrubber.Get() != &Scrubber || !HasOccupiedUser() || !FMath::IsFinite(Cm) || Cm <= 0)
	{
		return;
	}
	RubDistanceCm = FMath::Min(RequiredRubDistanceCm, RubDistanceCm + Cm);
	if (RubDistanceCm >= RequiredRubDistanceCm)
	{
		CompleteScrub();
	}
}

void AScrubTableActor::CompleteScrub()
{
	TGuardValue<bool> Guard(bCompleting, true);
	AActor* User = CurrentUser.Get();
	auto* Cash = CashOfferClass ? GetWorld()->SpawnActorDeferred<ABathhouseCashPaymentActor>(
									  CashOfferClass, CashOfferPoint->GetComponentTransform(), nullptr, nullptr,
									  ESpawnActorCollisionHandlingMethod::AlwaysSpawn)
								: nullptr;
	if (Cash)
	{
		Cash->ConfigurePaymentAmount(ScrubFee);
		Cash->FinishSpawning(CashOfferPoint->GetComponentTransform());
	}
	if (!IsValid(Cash))
	{
		RubDistanceCm = FMath::Max(0.f, RequiredRubDistanceCm - FMath::Max(.001f, RequiredRubDistanceCm * .0001f));
		UE_LOG(LogTemp, Error, TEXT("Scrub cash spawn failed; completion was not committed."));
		return;
	}
	// Construction callbacks may remove the waiting user/table. Never charge that abandoned session.
	if (!HasOccupiedUser() || CurrentUser.Get() != User)
	{
		Cash->Destroy();
		return;
	}
	auto* Slot = GetFacilitySlots()[0].Get();
	Slot->EndUse(User);
	Slot->Release(User);
	if (auto* ServiceUser = Cast<IServiceFacilityUser>(User))
	{
		ServiceUser->HandleScrubCashOffered(*this, *Cash, CashStandPoint->GetComponentTransform());
	}
}

FPlayerInteractionQuery AScrubTableActor::QueryInteraction(const FPlayerInteractionContext& Context) const
{
	FPlayerInteractionQuery Query;
	if (!FacilityPlacement || !FacilityPlacement->IsPlacedDomainActive() || FacilityPlacement->IsStagedPlacement())
	{
		return Query;
	}
	Query.bVisible = true;
	Query.TargetName = HasOccupiedUser() ? FText::Format(FText::FromString(TEXT("세신대 · 대기 {0}초 · {1}%")),
														 FText::AsNumber(FMath::CeilToInt(GetRemainingWaitSeconds())),
														 FText::AsNumber(FMath::RoundToInt(GetScrubProgress() * 100)))
										 : FText::FromString(TEXT("세신대"));
	Query.ActionName = FText::FromString(TEXT("세신"));
	const bool Towel =
		Context.CarryComponent && Context.CarryComponent->GetHeldKind() == EPhysicalCarryKind::ScrubTowel;
	Query.bCanInteract = Towel && HasOccupiedUser() && !ActiveScrubber.IsValid() && !bCompleting;
	if (!Query.bCanInteract)
	{
		Query.FailureReason = FText::FromString(!Towel				 ? TEXT("때수건이 필요합니다")
												: !HasOccupiedUser() ? TEXT("세신할 손님 없음")
																	 : TEXT("이미 세신 중입니다"));
	}
	return Query;
}

FPlayerInteractionResult AScrubTableActor::ExecuteInteraction(const FPlayerInteractionContext& Context)
{
	const auto Query = QueryInteraction(Context);
	if (!Query.bCanInteract)
	{
		return FPlayerInteractionResult::Failed(Query.FailureReason);
	}
	auto* Focus = Context.Interactor ? Context.Interactor->FindComponentByClass<UPlayerScrubFocusComponent>() : nullptr;
	return Focus && Focus->BeginScrubFocus(this)
			   ? FPlayerInteractionResult::Succeeded()
			   : FPlayerInteractionResult::Failed(FText::FromString(TEXT("세신 포커스에 진입할 수 없습니다.")));
}
#if WITH_EDITOR
EDataValidationResult AScrubTableActor::IsDataValid(FDataValidationContext& Context) const
{
	auto Result = Super::IsDataValid(Context);
	TArray<UActorComponent*> Components;
	FacilityPlacementExtensionUtils::CollectAuthoringComponents(*this, Components);
	const int32 Slots = Components
							.FilterByPredicate(
								[](auto* C)
								{
									return C && C->IsA<UBathhouseFacilitySlotComponent>();
								})
							.Num();
	const FVector Extent = ScrubArea ? ScrubArea->GetUnscaledBoxExtent() : FVector::ZeroVector;
	const auto Positive = [](float V)
	{
		return FMath::IsFinite(V) && V > 0;
	};
	const auto NonNegative = [](float V)
	{
		return FMath::IsFinite(V) && V >= 0;
	};
	if (Slots != 1 || Extent.ContainsNaN() || Extent.GetMin() <= 0 || ScrubFee <= 0 || !Positive(WaitLimitSeconds) ||
		!Positive(RequiredRubDistanceCm) || !Positive(RubCmPerInputUnit) || !NonNegative(ExitSearchRadiusCm) ||
		!NonNegative(FocusBlendInSeconds) || !NonNegative(FocusBlendOutSeconds) || !CashOfferClass)
	{
		Context.AddError(
			FText::FromString(TEXT("세신대는 slot 1개, 양의 범위·요금·대기·거리·감도와 현금 class가 필요합니다.")));
		return EDataValidationResult::Invalid;
	}
	return Result == EDataValidationResult::NotValidated ? EDataValidationResult::Valid : Result;
}
#endif
