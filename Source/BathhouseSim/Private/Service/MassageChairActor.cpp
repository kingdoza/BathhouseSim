#include "Service/MassageChairActor.h"
#include "Service/MassageChairPlacementInstanceData.h"
#include "Service/ServiceAmenityTypes.h"
#include "Facility/BathhouseFacilitySlotComponent.h"
#include "Facility/BathhouseFacilitySubsystem.h"
#include "Facility/FacilityPlacementExtensionUtils.h"
#include "Placement/FacilityPlacementComponent.h"
#include "Economy/BathhousePlayerState.h"
#include "Economy/PlayerWalletComponent.h"
#include "GameFramework/Controller.h"
#include "Interaction/PlayerCarryComponent.h"
#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif
namespace
{
	UPlayerWalletComponent* ChairWallet(AActor* Actor)
	{
		APawn* Pawn = Cast<APawn>(Actor);
		if (auto* Controller = Cast<AController>(Actor))
		{
			Pawn = Controller->GetPawn();
		}
		auto* State = Pawn ? Pawn->GetPlayerState<ABathhousePlayerState>() : nullptr;
		return State ? State->GetWallet() : nullptr;
	}
} // namespace

AMassageChairActor::AMassageChairActor()
{
	FacilityType = EBathhouseFacilityType::MassageChair;
}

void AMassageChairActor::BeginPlay()
{
	Super::BeginPlay();
	for (UBathhouseFacilitySlotComponent* Slot : GetFacilitySlots())
	{
		Slot->OnSlotStateChanged.AddDynamic(this, &AMassageChairActor::HandleSlotChanged);
	}
}

void AMassageChairActor::EndPlay(EEndPlayReason::Type Reason)
{
	GetWorldTimerManager().ClearTimer(UseTimer);
	for (UBathhouseFacilitySlotComponent* Slot : GetFacilitySlots())
	{
		Slot->OnSlotStateChanged.RemoveDynamic(this, &AMassageChairActor::HandleSlotChanged);
		Slot->ForceRelease();
	}
	BindUser(nullptr);
	Super::EndPlay(Reason);
}

void AMassageChairActor::BindUser(AActor* User)
{
	if (AActor* Previous = CurrentUser.Get())
	{
		Previous->OnEndPlay.RemoveDynamic(this, &AMassageChairActor::HandleUserEndPlay);
	}
	CurrentUser = User;
	if (User)
	{
		User->OnEndPlay.AddUniqueDynamic(this, &AMassageChairActor::HandleUserEndPlay);
	}
}

void AMassageChairActor::HandleSlotChanged(UBathhouseFacilitySlotComponent* Slot, EBathhouseFacilitySlotState Previous,
										   EBathhouseFacilitySlotState State)
{
	GetWorldTimerManager().ClearTimer(UseTimer);
	BindUser(Slot->GetCurrentUser());
	if (State == EBathhouseFacilitySlotState::Occupied && IsPlacedActive())
	{
		GetWorldTimerManager().SetTimer(UseTimer, this, &AMassageChairActor::CompleteUse, UseSeconds, false);
	}
}

void AMassageChairActor::HandleUserEndPlay(AActor* User, EEndPlayReason::Type Reason)
{
	// OnEndPlay may run after the weak pointer becomes invalid. Compare the slot's raw user.
	for (UBathhouseFacilitySlotComponent* Slot : GetFacilitySlots())
	{
		if (Slot->GetCurrentUser() == User)
		{
			Slot->ForceRelease();
		}
	}
	BindUser(nullptr);
	GetWorldTimerManager().ClearTimer(UseTimer);
}

bool AMassageChairActor::IsPlacedActive() const
{
	return FacilityPlacement && FacilityPlacement->GetMode() == EPlaceableFacilityMode::Placed &&
		   !FacilityPlacement->IsStagedPlacement() && FacilityPlacement->IsPlacedDomainActive();
}

void AMassageChairActor::PublishAvailability()
{
	if (auto* System = GetWorld()->GetSubsystem<UBathhouseFacilitySubsystem>())
	{
		System->NotifyFacilityAvailabilityChanged(FacilityType);
	}
}

bool AMassageChairActor::IsAvailableForReservation() const
{
	return Super::IsAvailableForReservation() && !bBroken;
}

void AMassageChairActor::CompleteUse()
{
	AActor* User = CurrentUser.Get();
	auto* Slot = GetFacilitySlots().Num() == 1 ? GetFacilitySlots()[0].Get() : nullptr;
	if (!User || !Slot || Slot->GetSlotState() != EBathhouseFacilitySlotState::Occupied ||
		Slot->GetCurrentUser() != User || !IsPlacedActive())
	{
		return;
	}
	const int64 Sum = int64(CoinBalance) + UseFee;
	if (Sum > MAX_int32)
	{
		UE_LOG(LogTemp, Warning, TEXT("Massage chair coin balance clamped at int32 maximum."));
	}
	CoinBalance = int32(FMath::Min<int64>(Sum, MAX_int32));
	bBroken = ShouldBreak(FMath::FRand() * 100.f, BreakChancePercent);
	Slot->EndUse(User);
	Slot->Release(User);
	OnCoinBalanceChanged(CoinBalance);
	if (bBroken)
	{
		OnBrokenStateChanged(true);
	}
	if (auto* ServiceUser = Cast<IServiceFacilityUser>(User))
	{
		ServiceUser->HandleServiceUseEnded(*this, EServiceUseEndReason::Completed);
	}
	PublishAvailability();
}

FPlayerInteractionQuery AMassageChairActor::QueryInteraction(const FPlayerInteractionContext& Context) const
{
	FPlayerInteractionQuery Query;
	if (!IsPlacedActive())
	{
		return Query;
	}
	Query.bVisible = true;
	const bool Busy = GetFacilitySlots().ContainsByPredicate(
		[](const auto& Slot)
		{
			return Slot && Slot->GetSlotState() == EBathhouseFacilitySlotState::Occupied;
		});
	Query.TargetName = FText::Format(FText::FromString(TEXT("안마의자 · {0}원 · {1}")), FText::AsNumber(CoinBalance),
									 FText::FromString(Busy		 ? TEXT("이용 중")
													   : bBroken ? TEXT("고장")
																 : TEXT("정상")));
	Query.ActionName = FText::FromString(TEXT("수금"));
	auto* Wallet = ChairWallet(Context.Interactor);
	Query.bCanInteract = CoinBalance > 0 && !bCollecting && Wallet && Wallet->CanAddMoney(CoinBalance);
	if (!Query.bCanInteract)
	{
		Query.FailureReason = FText::FromString(CoinBalance == 0 ? TEXT("모인 돈 없음") : TEXT("수금할 수 없습니다."));
	}
	if (bBroken && (!Context.CarryComponent || Context.CarryComponent->GetHeldKind() != EPhysicalCarryKind::Facility))
	{
		Query.bHeldApplyVisible = true;
		Query.HeldApplyFailureReason = FText::FromString(TEXT("몽키스패너가 필요합니다"));
	}
	return Query;
}

FPlayerInteractionResult AMassageChairActor::ExecuteInteraction(const FPlayerInteractionContext& Context)
{
	const auto Query = QueryInteraction(Context);
	if (!Query.bCanInteract)
	{
		return FPlayerInteractionResult::Failed(Query.FailureReason);
	}
	TGuardValue<bool> Guard(bCollecting, true);
	auto* Wallet = ChairWallet(Context.Interactor);
	if (!Wallet || !Wallet->TryAddMoney(CoinBalance))
	{
		return FPlayerInteractionResult::Failed(FText::FromString(TEXT("수금할 수 없습니다.")));
	}
	CoinBalance = 0;
	OnCoinBalanceChanged(0);
	return FPlayerInteractionResult::Succeeded();
}

bool AMassageChairActor::IsWrenchRepairRequired() const
{
	return bBroken && IsPlacedActive();
}

bool AMassageChairActor::CommitWrenchRepair(FText& Failure)
{
	if (!IsWrenchRepairRequired())
	{
		return false;
	}
	bBroken = false;
	OnBrokenStateChanged(false);
	PublishAvailability();
	return true;
}

UBathhouseFacilityPlacementInstanceData* AMassageChairActor::CreateFacilityPlacementInstanceData(UObject* Outer) const
{
	return NewObject<UMassageChairPlacementInstanceData>(Outer);
}

bool AMassageChairActor::ExportFacilityExtension(UBathhouseFacilityPlacementInstanceData& Data, FText& Failure) const
{
	auto* ChairData = Cast<UMassageChairPlacementInstanceData>(&Data);
	if (!ChairData || !Super::ExportFacilityExtension(Data, Failure))
	{
		return false;
	}
	ChairData->bBroken = bBroken;
	return true;
}

bool AMassageChairActor::ImportFacilityExtension(const UBathhouseFacilityPlacementInstanceData* Data, FText& Failure)
{
	const auto* ChairData = Cast<UMassageChairPlacementInstanceData>(Data);
	if ((Data && !ChairData) || !Super::ImportFacilityExtension(Data, Failure))
	{
		return false;
	}
	bBroken = ChairData && ChairData->bBroken;
	return true;
}

bool AMassageChairActor::StagePlacedDomainUnregistration(FFacilityPlacementPublication& Publication, FText& Failure)
{
	if (!Super::StagePlacedDomainUnregistration(Publication, Failure))
	{
		return false;
	}
	auto* Wallet = ChairWallet(GetFacilityRecoveryInstigator());
	if (!Wallet || (CoinBalance > 0 && !Wallet->CanAddMoney(CoinBalance)))
	{
		Failure = FText::FromString(TEXT("회수 수행자의 지갑에 수금할 수 없습니다."));
		// The transaction considers a failed stage untouched, so restore the successful base stage here.
		FText RollbackFailure;
		ensureMsgf(Super::RollbackPlacedDomainUnregistration(RollbackFailure), TEXT("Chair stage rollback failed: %s"),
				   *RollbackFailure.ToString());
		Publication.Callback = nullptr;
		return false;
	}
	TWeakObjectPtr<UPlayerWalletComponent> Payee(Wallet);
	TWeakObjectPtr<AMassageChairActor> Chair(this);
	const int32 Amount = CoinBalance;
	auto BasePublication = MoveTemp(Publication.Callback);
	Publication.Callback =
		[Payee, Chair, Amount, BasePublication = MoveTemp(BasePublication), bPublished = false]() mutable
	{
		if (bPublished)
		{
			return;
		}
		bPublished = true;
		// Payment precedes availability observers; source EndPlay has already completed.
		if (Payee.IsValid() && (Amount == 0 || Payee->TryAddMoney(Amount)))
		{
			if (auto* Source = Chair.Get(true))
			{
				Source->CoinBalance = 0;
			}
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("Committed massage chair recovery payment failed (%d)."), Amount);
		}
		if (BasePublication)
		{
			BasePublication();
		}
	};
	return true;
}
#if WITH_EDITOR
EDataValidationResult AMassageChairActor::IsDataValid(FDataValidationContext& Context) const
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
	if (Slots != 1 || !FMath::IsFinite(UseSeconds) || UseSeconds <= 0 || UseFee < 0 ||
		!FMath::IsFinite(BreakChancePercent) || BreakChancePercent < 0 || BreakChancePercent > 100 ||
		!FMath::IsFinite(RepairSeconds) || RepairSeconds <= 0)
	{
		Context.AddError(FText::FromString(TEXT("안마의자는 slot 1개와 유효한 이용·요금·고장·수리 값이 필요합니다.")));
		return EDataValidationResult::Invalid;
	}
	return Result == EDataValidationResult::NotValidated ? EDataValidationResult::Valid : Result;
}
#endif
