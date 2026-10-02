#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "BathhouseExpansionDefinition.generated.h"

USTRUCT(BlueprintType)
struct BATHHOUSESIM_API FBathhouseExpansionTier
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Expansion", meta = (ClampMin = "0"))
	int32 KeyPoolSize = 0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Expansion", meta = (ClampMin = "0"))
	int32 MaxInstalledLockerSlots = 0;
};

UCLASS(BlueprintType)
class BATHHOUSESIM_API UBathhouseExpansionDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif

	const FBathhouseExpansionTier* GetTier(int32 TierIndex) const { return Tiers.IsValidIndex(TierIndex) ? &Tiers[TierIndex] : nullptr; }

	/** 홀을 HallExpansionCount회 넓혔을 때의 효과 줄. 표 끝을 넘으면 마지막 줄, 표가 비면 null. */
	const FBathhouseExpansionTier* GetHallEffect(int32 HallExpansionCount) const;
	/** GetHallEffect가 쓰는 Tiers index. 표가 비면 INDEX_NONE. */
	int32 GetHallEffectIndex(int32 HallExpansionCount) const;
	/** 런타임 사용 가능 판정과 Data Validation이 함께 쓰는 규칙. 첫 문제 문구를 돌려준다. */
	bool ValidatePurchaseData(FText& OutFailureReason) const;

	/** index = 홀 넓힘 횟수(0회부터). 표 끝을 넘으면 마지막 줄을 쓴다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Expansion",
		meta = (ToolTip = "index = 홀 넓힘 횟수(0회부터). 표 끝을 넘으면 마지막 줄. 줄 수는 홀 넓힘 줄 수 + 1 이상(공간 검사가 알림)."))
	TArray<FBathhouseExpansionTier> Tiers;

private:
	/** 모든 문제의 문구를 OutReasons에 덧붙인다. */
	void CollectDataProblems(TArray<FText>& OutReasons) const;
};
