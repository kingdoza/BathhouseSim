#include "Building/BathhouseExpansionPurchaseSubsystem.h"

#include "Building/BathhouseSpaceActor.h"
#include "Economy/BathhousePlayerState.h"
#include "Economy/PlayerWalletComponent.h"
#include "Engine/World.h"
#include "Facility/BathhouseExpansionAuthority.h"
#include "Facility/BathhouseExpansionDefinition.h"
#include "Facility/BathhouseFacilitySubsystem.h"
#include "Facility/LockerCapacitySubsystem.h"

DEFINE_LOG_CATEGORY(LogBathhouseExpansion);

namespace
{
	int32 KindIndex(const EBathhouseSpaceKind Kind)
	{
		return FMath::Clamp(static_cast<int32>(Kind), 0, 2);
	}

	FVector2D RectSize(const FBox2D& Rect)
	{
		return FVector2D(Rect.Max.X - Rect.Min.X, Rect.Max.Y - Rect.Min.Y);
	}
}

void UBathhouseExpansionPurchaseSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Collection.InitializeDependency<UBathhouseFacilitySubsystem>();
	Super::Initialize(Collection);
	if (UBathhouseFacilitySubsystem* Facility = GetWorld() ? GetWorld()->GetSubsystem<UBathhouseFacilitySubsystem>() : nullptr)
	{
		AuthorityChangedHandle = Facility->OnExpansionAuthorityChanged.AddUObject(
			this, &UBathhouseExpansionPurchaseSubsystem::HandleAuthorityChanged);
	}
}

void UBathhouseExpansionPurchaseSubsystem::Deinitialize()
{
	if (AuthorityChangedHandle.IsValid())
	{
		if (UBathhouseFacilitySubsystem* Facility = GetWorld() ? GetWorld()->GetSubsystem<UBathhouseFacilitySubsystem>() : nullptr)
		{
			Facility->OnExpansionAuthorityChanged.Remove(AuthorityChangedHandle);
		}
	}
	AuthorityChangedHandle.Reset();
	OnExpansionChanged.Clear();
	for (TWeakObjectPtr<ABathhouseSpaceActor>& Space : Spaces)
	{
		Space.Reset();
	}
	Super::Deinitialize();
}

void UBathhouseExpansionPurchaseSubsystem::HandleAuthorityChanged(ABathhouseExpansionAuthority* Authority)
{
	(void)Authority;
	// 구입 transaction 안의 tier 상승은 commit 방송 한 번으로 묶는다.
	if (!bPurchasing)
	{
		OnExpansionChanged.Broadcast();
	}
}

void UBathhouseExpansionPurchaseSubsystem::RegisterSpace(ABathhouseSpaceActor& Space)
{
	TWeakObjectPtr<ABathhouseSpaceActor>& Slot = Spaces[KindIndex(Space.GetSpaceKind())];
	if (Slot.IsValid() && Slot.Get() != &Space)
	{
		UE_LOG(LogBathhouseExpansion, Error,
			TEXT("%s: 같은 종류의 공간 %s이(가) 이미 등록되어 있어 이 공간은 확장 구입에서 제외됩니다."),
			*Space.GetPathName(), *Slot->GetPathName());
		return;
	}
	Slot = &Space;
	OnExpansionChanged.Broadcast();
}

void UBathhouseExpansionPurchaseSubsystem::UnregisterSpace(ABathhouseSpaceActor& Space)
{
	TWeakObjectPtr<ABathhouseSpaceActor>& Slot = Spaces[KindIndex(Space.GetSpaceKind())];
	if (Slot.Get() == &Space)
	{
		Slot.Reset();
		OnExpansionChanged.Broadcast();
	}
}

ABathhouseSpaceActor* UBathhouseExpansionPurchaseSubsystem::FindSpace(const EBathhouseSpaceKind Kind) const
{
	return Spaces[KindIndex(Kind)].Get();
}

int32 UBathhouseExpansionPurchaseSubsystem::GetPurchaseCount() const
{
	int32 Total = 0;
	for (const TWeakObjectPtr<ABathhouseSpaceActor>& Space : Spaces)
	{
		if (const ABathhouseSpaceActor* Actor = Space.Get())
		{
			Total += Actor->GetAppliedExpansionCount();
		}
	}
	return Total;
}

void UBathhouseExpansionPurchaseSubsystem::LogUnavailableOnce(const EUnavailableCause Cause, const FString& Detail) const
{
	const uint32 Bit = 1u << static_cast<uint32>(Cause);
	if ((LoggedCauseMask & Bit) == 0)
	{
		LoggedCauseMask |= Bit;
		UE_LOG(LogBathhouseExpansion, Error, TEXT("확장을 사용할 수 없습니다: %s"), *Detail);
	}
}

UBathhouseExpansionPurchaseSubsystem::FResolved UBathhouseExpansionPurchaseSubsystem::Resolve(const APlayerState* Buyer) const
{
	FResolved Result;
	const ABathhousePlayerState* BuyerState = Cast<ABathhousePlayerState>(Buyer);
	Result.Wallet = BuyerState ? BuyerState->GetWallet() : nullptr;
	// 구매자(wallet)가 없는 평가는 정상 상태(사용자 없음, BeginPlay 순서)라 로그 없이 사용 불가로 끝낸다.
	if (!Result.Wallet)
	{
		return Result;
	}

	const UWorld* World = GetWorld();
	const UBathhouseFacilitySubsystem* Facility = World ? World->GetSubsystem<UBathhouseFacilitySubsystem>() : nullptr;
	Result.Authority = Facility ? Facility->GetExpansionAuthority() : nullptr;
	if (!Result.Authority)
	{
		LogUnavailableOnce(EUnavailableCause::NoAuthority, TEXT("확장 관리자(BathhouseExpansionAuthority)가 월드에 등록되어 있지 않습니다."));
		return Result;
	}
	Result.Definition = Result.Authority->GetExpansionDefinition();
	if (!Result.Definition)
	{
		LogUnavailableOnce(EUnavailableCause::NoDefinition, TEXT("확장 관리자에 확장 정의(Expansion Definition)가 없습니다."));
		return Result;
	}
	FText Reason;
	if (!Result.Definition->ValidatePurchaseData(Reason))
	{
		LogUnavailableOnce(EUnavailableCause::InvalidDefinition,
			FString::Printf(TEXT("확장 정의가 올바르지 않습니다. %s"), *Reason.ToString()));
		return Result;
	}
	bool bAnySpace = false;
	for (const TWeakObjectPtr<ABathhouseSpaceActor>& Space : Spaces)
	{
		bAnySpace |= Space.IsValid();
	}
	if (!bAnySpace)
	{
		LogUnavailableOnce(EUnavailableCause::NoSpaces, TEXT("등록된 공간 Actor가 없습니다."));
		return Result;
	}
	const ABathhouseSpaceActor* Hall = FindSpace(EBathhouseSpaceKind::Hall);
	Result.HallCount = Hall ? Hall->GetAppliedExpansionCount() : 0;
	const int32 ExpectedTier = Result.Definition->GetHallEffectIndex(Result.HallCount);
	// transaction 도중에는 공간 횟수와 tier가 잠시 어긋난다(결제 callback 등). 그때는 확인하지 않는다.
	if (!bPurchasing && Result.Authority->GetCurrentTierIndex() != ExpectedTier)
	{
		LogUnavailableOnce(EUnavailableCause::TierMismatch,
			FString::Printf(TEXT("확장 관리자의 현재 효과 줄(%d)이 홀 넓힘 횟수(%d)가 뜻하는 줄(%d)과 다릅니다."),
				Result.Authority->GetCurrentTierIndex(), Result.HallCount, ExpectedTier));
		return Result;
	}
	Result.bDataValid = true;
	return Result;
}

FBathhouseExpansionView UBathhouseExpansionPurchaseSubsystem::BuildView(const APlayerState* Buyer) const
{
	FBathhouseExpansionView View;
	View.bBusy = bPurchasing;
	const FResolved Resolved = Resolve(Buyer);
	View.Balance = Resolved.Wallet ? Resolved.Wallet->GetCurrentMoney() : 0;
	if (!Resolved.bDataValid || !Resolved.Wallet)
	{
		return View;
	}
	View.bAvailable = true;
	View.PurchaseCount = GetPurchaseCount();
	View.MaxPurchaseCount = Resolved.Definition->GetMaxPurchaseCount();
	View.bMaxReached = View.PurchaseCount >= View.MaxPurchaseCount;
	if (!View.bMaxReached)
	{
		int32 Price = 0;
		if (Resolved.Definition->TryGetPurchasePrice(View.PurchaseCount, Price))
		{
			View.NextPrice = Price;
			View.Shortfall = FMath::Max(0, Price - View.Balance);
		}
	}
	if (const ULockerCapacitySubsystem* Lockers = GetWorld()->GetSubsystem<ULockerCapacitySubsystem>())
	{
		View.InstalledLockerSlots = Lockers->GetInstalledLockerCapacity();
	}
	if (const UBathhouseFacilitySubsystem* Facility = GetWorld()->GetSubsystem<UBathhouseFacilitySubsystem>())
	{
		View.LockerSlotLimit = Facility->GetMaxInstalledLockerSlots();
	}
	for (int32 Index = 0; Index < 3; ++Index)
	{
		FBathhouseExpansionOptionView& Option = View.Options[Index];
		const ABathhouseSpaceActor* Space = Spaces[Index].Get();
		Option.Kind = static_cast<EBathhouseSpaceKind>(Index);
		Option.bPresent = Space != nullptr;
		if (!Space)
		{
			continue;
		}
		Option.CurrentSizeCm = RectSize(Space->GetInteriorRect());
		FText Unused;
		Option.bCanExpand = Space->CanApplyNextExpansion(Unused);
		if (Option.bCanExpand)
		{
			Option.NextSizeCm = RectSize(Space->GetInteriorRectForCount(Space->GetAppliedExpansionCount() + 1));
		}
		if (Option.Kind == EBathhouseSpaceKind::Hall)
		{
			Option.bHasHallEffect = true;
			const FBathhouseExpansionTier* Now = Resolved.Definition->GetHallEffect(Resolved.HallCount);
			const FBathhouseExpansionTier* Next = Resolved.Definition->GetHallEffect(Resolved.HallCount + 1);
			Option.KeysNow = Now ? Now->KeyPoolSize : 0;
			Option.LockerLimitNow = Now ? Now->MaxInstalledLockerSlots : 0;
			Option.KeysNext = Next ? Next->KeyPoolSize : Option.KeysNow;
			Option.LockerLimitNext = Next ? Next->MaxInstalledLockerSlots : Option.LockerLimitNow;
		}
	}
	return View;
}

EBathhouseExpansionFailure UBathhouseExpansionPurchaseSubsystem::EvaluatePurchase(
	const APlayerState* Buyer, const EBathhouseSpaceKind Kind, const int32 ExpectedPurchaseCount, int32* OutPrice) const
{
	if (bPurchasing)
	{
		return EBathhouseExpansionFailure::Busy;
	}
	const FResolved Resolved = Resolve(Buyer);
	if (!Resolved.bDataValid || !Resolved.Wallet)
	{
		return EBathhouseExpansionFailure::Unavailable;
	}
	const int32 PurchaseCount = GetPurchaseCount();
	if (ExpectedPurchaseCount != PurchaseCount)
	{
		return EBathhouseExpansionFailure::StaleState;
	}
	if (PurchaseCount >= Resolved.Definition->GetMaxPurchaseCount())
	{
		return EBathhouseExpansionFailure::MaxPurchasesReached;
	}
	const ABathhouseSpaceActor* Space = FindSpace(Kind);
	if (!Space)
	{
		return EBathhouseExpansionFailure::SpaceUnavailable;
	}
	FText Unused;
	if (!Space->CanApplyNextExpansion(Unused))
	{
		return EBathhouseExpansionFailure::SpaceMaxReached;
	}
	int32 Price = 0;
	if (!Resolved.Definition->TryGetPurchasePrice(PurchaseCount, Price))
	{
		return EBathhouseExpansionFailure::Unavailable;
	}
	if (!Resolved.Wallet->CanSpendMoney(Price))
	{
		return EBathhouseExpansionFailure::InsufficientMoney;
	}
	if (Kind == EBathhouseSpaceKind::Hall)
	{
		const int32 NewIndex = Resolved.Definition->GetHallEffectIndex(Resolved.HallCount + 1);
		if (NewIndex == INDEX_NONE || !Resolved.Definition->GetTier(NewIndex)
			|| NewIndex < Resolved.Authority->GetCurrentTierIndex())
		{
			return EBathhouseExpansionFailure::Unavailable;
		}
	}
	if (OutPrice)
	{
		*OutPrice = Price;
	}
	return EBathhouseExpansionFailure::None;
}

EBathhouseExpansionFailure UBathhouseExpansionPurchaseSubsystem::TryPurchase(
	APlayerState* Buyer, const EBathhouseSpaceKind Kind, const int32 ExpectedPurchaseCount)
{
	int32 Price = 0;
	const EBathhouseExpansionFailure Evaluation = EvaluatePurchase(Buyer, Kind, ExpectedPurchaseCount, &Price);
	if (Evaluation != EBathhouseExpansionFailure::None)
	{
		return Evaluation;
	}
	{
		TGuardValue<bool> Guard(bPurchasing, true);
		const FResolved Resolved = Resolve(Buyer);
		ABathhouseSpaceActor* Space = FindSpace(Kind);
		if (!Space || !Resolved.Wallet)
		{
			return EBathhouseExpansionFailure::Unavailable;
		}
		FBathhouseSpaceExpansionUndo Undo;
		FText Reason;
		if (!Space->ApplyNextExpansion(Undo, Reason))
		{
			UE_LOG(LogBathhouseExpansion, Error, TEXT("%s: 넓힘 적용 실패. %s"), *Space->GetPathName(), *Reason.ToString());
			return EBathhouseExpansionFailure::ApplyFailed;
		}
#if WITH_DEV_AUTOMATION_TESTS
		if (InjectedFailureStep == 4)
		{
			Space->UndoExpansion(Undo);
			return EBathhouseExpansionFailure::ApplyFailed;
		}
		const bool bInjectSpendFailure = InjectedFailureStep == 5;
#else
		const bool bInjectSpendFailure = false;
#endif
		if (bInjectSpendFailure || !Resolved.Wallet->TrySpendMoney(Price))
		{
			Space->UndoExpansion(Undo);
			return EBathhouseExpansionFailure::InsufficientMoney;
		}
		if (Kind == EBathhouseSpaceKind::Hall)
		{
			const int32 NewIndex = Resolved.Definition->GetHallEffectIndex(Resolved.HallCount + 1);
			if (NewIndex > Resolved.Authority->GetCurrentTierIndex())
			{
				FText TierReason;
#if WITH_DEV_AUTOMATION_TESTS
				const bool bInjectTierFailure = InjectedFailureStep == 6;
#else
				const bool bInjectTierFailure = false;
#endif
				if (bInjectTierFailure || !Resolved.Authority->TryAdvanceToTier(NewIndex, TierReason))
				{
					UE_LOG(LogBathhouseExpansion, Error, TEXT("확장 효과 줄 상승 실패, 환불하고 되돌립니다. %s"), *TierReason.ToString());
					Resolved.Wallet->TryAddMoney(Price);
					Space->UndoExpansion(Undo);
					return EBathhouseExpansionFailure::ApplyFailed;
				}
			}
		}
	}
	OnExpansionChanged.Broadcast();
	return EBathhouseExpansionFailure::None;
}
