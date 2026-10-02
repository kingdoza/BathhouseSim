#pragma once

#include "CoreMinimal.h"
#include "Building/BathhouseExpansionTypes.h"
#include "Subsystems/WorldSubsystem.h"
#include "BathhouseExpansionPurchaseSubsystem.generated.h"

class ABathhouseExpansionAuthority;
class ABathhouseSpaceActor;
class APlayerState;
class UBathhouseExpansionDefinition;
class UPlayerWalletComponent;

DECLARE_LOG_CATEGORY_EXTERN(LogBathhouseExpansion, Log, All);
DECLARE_MULTICAST_DELEGATE(FOnBathhouseExpansionChangedNative);

/**
 * 확장 구입 조율. 공간 등록부, 화면 view, 구입 transaction과 변경 방송을 맡는다.
 * 넓힌 횟수·가격·상한은 각 공간 Actor가 가진다(저장하지 않는다). 전체 구입 횟수·상한은 없다.
 */
UCLASS()
class BATHHOUSESIM_API UBathhouseExpansionPurchaseSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	void RegisterSpace(ABathhouseSpaceActor& Space);
	void UnregisterSpace(ABathhouseSpaceActor& Space);

	FBathhouseExpansionView BuildView(const APlayerState* Buyer) const;

	/**
	 * 부작용 없이 구입 가능 여부를 순서대로 판정한다. 가능하면 None이고 OutPrice에 고른 공간의 다음 줄 가격을 돌려준다.
	 * ExpectedAppliedCount는 화면이 본 그 공간의 넓힌 횟수다.
	 */
	EBathhouseExpansionFailure EvaluatePurchase(
		const APlayerState* Buyer, EBathhouseSpaceKind Kind, int32 ExpectedAppliedCount, int32* OutPrice = nullptr) const;

	/** 동기 transaction. 실패하면 돈·공간·열쇠·한도가 구입 전과 같다. */
	EBathhouseExpansionFailure TryPurchase(APlayerState* Buyer, EBathhouseSpaceKind Kind, int32 ExpectedAppliedCount);

	/** 구입 commit, 공간 등록·해제, 확장 관리자 등록 변경 때 한 번 방송한다. */
	FOnBathhouseExpansionChangedNative OnExpansionChanged;

private:
	friend class FBathhouseExpansionAutomationAccess;

	/** BuildView·EvaluatePurchase가 함께 쓰는 사용 가능 판정 결과. */
	struct FResolved
	{
		ABathhouseExpansionAuthority* Authority = nullptr;
		const UBathhouseExpansionDefinition* Definition = nullptr;
		UPlayerWalletComponent* Wallet = nullptr;
		bool bDataValid = false;
		int32 HallCount = 0;
	};

	enum class EUnavailableCause : uint8
	{
		NoAuthority,
		NoDefinition,
		InvalidDefinition,
		NoSpaces,
		TierMismatch,
		Count
	};

	FResolved Resolve(const APlayerState* Buyer) const;
	ABathhouseSpaceActor* FindSpace(EBathhouseSpaceKind Kind) const;
	void LogUnavailableOnce(EUnavailableCause Cause, const FString& Detail) const;
	void HandleAuthorityChanged(ABathhouseExpansionAuthority* Authority);

	TWeakObjectPtr<ABathhouseSpaceActor> Spaces[3];
	bool bPurchasing = false;
	mutable uint32 LoggedCauseMask = 0;
	FDelegateHandle AuthorityChangedHandle;
#if WITH_DEV_AUTOMATION_TESTS
	/** 자동화 전용 실패 주입(4: 적용 직후 실패, 5: 결제 실패, 6: tier 상승 실패). 0이면 끔. */
	int32 InjectedFailureStep = 0;
#endif
};
