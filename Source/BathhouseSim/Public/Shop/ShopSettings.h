#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "ShopSettings.generated.h"

class AItemBoxActor;
class AShopDeliveryBoxActor;
class UShopCatalog;

UCLASS(Config = Game, defaultconfig, meta = (DisplayName = "Bathhouse Shop"))
class BATHHOUSESIM_API UShopSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	static constexpr float DefaultDeliveryDelaySeconds = 10.0f;
	static constexpr float DefaultDeliveryNoticeSeconds = 3.0f;
	static constexpr float DefaultDeliveryAttemptIntervalSeconds = 0.25f;
	static constexpr float DefaultUnboxForwardDistanceCm = 100.0f;
	static constexpr float DefaultUnboxOverlapDepthCm = 8.0f;
	static constexpr float MaxUnboxOverlapDepthCm = 50.0f;
	static constexpr float UnboxMinStepCm = 0.01f;
	static constexpr float DefaultUnboxViewDistanceCm = 100.0f;
	static constexpr float DefaultUnboxViewMinDistanceCm = 30.0f;
	static constexpr float DefaultUnboxViewPullStepCm = 10.0f;
	static constexpr int32 DefaultUnboxViewLayoutAttempts = 4;
	static constexpr float DefaultUnboxMinForwardDistanceCm = 0.0f;
	static constexpr float DefaultUnboxForwardPullStepCm = 10.0f;
	static constexpr int32 DefaultUnboxForwardLayoutAttempts = 4;
	static constexpr float DefaultUnboxForwardFloorClearanceCm = 20.0f;
	static constexpr float DefaultUnboxCameraClearanceCm = 10.0f;
	static constexpr float DefaultUnboxOverheadClearanceCm = 10.0f;
	static constexpr float DefaultUnboxOverheadStepCm = 25.0f;
	static constexpr int32 DefaultUnboxOverheadStepCount = 8;
	static constexpr int32 DefaultUnboxOverheadLayoutAttempts = 3;
	static constexpr int32 DefaultUnboxClusterPlacementAttempts = 12;
	static constexpr float DefaultUnboxClusterMinElevationDegrees = 15.0f;
	static constexpr float DefaultUnboxClusterMaxElevationDegrees = 45.0f;
	static constexpr float DefaultUnboxClusterDepthToleranceCm = 0.5f;
	static constexpr float DefaultUnboxClusterPairDepthExtentRatio = 0.5f;

	UShopCatalog* LoadCatalog() const;
	TSubclassOf<AShopDeliveryBoxActor> LoadDeliveryBoxClass() const;
	TSubclassOf<AItemBoxActor> LoadItemBoxClass() const;
	int32 GetCartTotalQuantityLimit() const { return FMath::Max(1, CartTotalQuantityLimit); }
	int32 GetPerProductQuantityLimit() const { return FMath::Max(1, PerProductQuantityLimit); }
	float GetDeliveryDelaySeconds() const;
	float GetUnboxForwardDistanceCm() const;
	float GetUnboxOverlapDepthCm() const;
	float GetDeliveryNoticeSeconds() const;
	float GetDeliveryAttemptIntervalSeconds() const;
	float GetUnboxViewDistanceCm() const;
	float GetUnboxViewMinDistanceCm() const;
	float GetUnboxViewPullStepCm() const;
	int32 GetUnboxViewLayoutAttempts() const;
	float GetUnboxMinForwardDistanceCm() const;
	float GetUnboxForwardPullStepCm() const;
	int32 GetUnboxForwardLayoutAttempts() const;
	float GetUnboxForwardFloorClearanceCm() const;
	float GetUnboxCameraClearanceCm() const;
	float GetUnboxOverheadClearanceCm() const;
	float GetUnboxOverheadStepCm() const;
	int32 GetUnboxOverheadStepCount() const;
	int32 GetUnboxOverheadLayoutAttempts() const;
	int32 GetUnboxClusterPlacementAttempts() const;
	float GetUnboxClusterMinElevationDegrees() const;
	float GetUnboxClusterMaxElevationDegrees() const;
	float GetUnboxClusterDepthToleranceCm() const;
	float GetUnboxClusterPairDepthExtentRatio() const;

	UPROPERTY(Config, EditAnywhere, Category = "Shop")
	TSoftObjectPtr<UShopCatalog> Catalog;

	UPROPERTY(Config, EditAnywhere, Category = "Shop")
	TSoftClassPtr<AShopDeliveryBoxActor> DeliveryBoxClass;

	UPROPERTY(Config, EditAnywhere, Category = "Shop")
	TSoftClassPtr<AItemBoxActor> ItemBoxClass;

	UPROPERTY(Config, EditAnywhere, Category = "Shop", meta = (ClampMin = "1"))
	int32 CartTotalQuantityLimit = 10;

	UPROPERTY(Config, EditAnywhere, Category = "Shop", meta = (ClampMin = "1"))
	int32 PerProductQuantityLimit = 99;

	UPROPERTY(Config, EditAnywhere, Category = "Shop", meta = (ClampMin = "0.0"))
	float DeliveryDelaySeconds = DefaultDeliveryDelaySeconds;

	/** 배송 대기 주문의 도착 재시도 간격. 0이면 매 Tick 시도 */
	UPROPERTY(Config, EditAnywhere, Category = "Shop", meta = (ClampMin = "0.0"))
	float DeliveryAttemptIntervalSeconds = DefaultDeliveryAttemptIntervalSeconds;

	/** 2단계(바닥 정면)의 발바닥 기준 수평 중심 거리 */
	UPROPERTY(Config, EditAnywhere, Category = "Shop", meta = (ClampMin = "0.0"))
	float UnboxForwardDistanceCm = DefaultUnboxForwardDistanceCm;

	UPROPERTY(Config, EditAnywhere, Category = "Shop", meta = (ClampMin = "0.0", ClampMax = "50.0"))
	float UnboxOverlapDepthCm = DefaultUnboxOverlapDepthCm;

	UPROPERTY(Config, EditAnywhere, Category = "Shop", meta = (ClampMin = "0.01"))
	float DeliveryNoticeSeconds = DefaultDeliveryNoticeSeconds;

	/** 1단계(시선 앞) 시작 거리: 카메라에서 무리의 가장 가까운 부분까지(시선 방향) */
	UPROPERTY(Config, EditAnywhere, Category = "Shop|Unboxing", meta = (ClampMin = "0.0"))
	float UnboxViewDistanceCm = DefaultUnboxViewDistanceCm;

	/** 1단계 시선 당김 하한 */
	UPROPERTY(Config, EditAnywhere, Category = "Shop|Unboxing", meta = (ClampMin = "0.0"))
	float UnboxViewMinDistanceCm = DefaultUnboxViewMinDistanceCm;

	/** 1단계 시선 당김 간격 */
	UPROPERTY(Config, EditAnywhere, Category = "Shop|Unboxing", meta = (ClampMin = "0.01"))
	float UnboxViewPullStepCm = DefaultUnboxViewPullStepCm;

	/** 1단계 거리당 layout 시도 수 */
	UPROPERTY(Config, EditAnywhere, Category = "Shop|Unboxing", meta = (ClampMin = "1"))
	int32 UnboxViewLayoutAttempts = DefaultUnboxViewLayoutAttempts;

	/** 2단계(바닥 정면) 당김 하한 */
	UPROPERTY(Config, EditAnywhere, Category = "Shop|Unboxing", meta = (ClampMin = "0.0"))
	float UnboxMinForwardDistanceCm = DefaultUnboxMinForwardDistanceCm;

	/** 2단계 당김 간격 */
	UPROPERTY(Config, EditAnywhere, Category = "Shop|Unboxing", meta = (ClampMin = "0.01"))
	float UnboxForwardPullStepCm = DefaultUnboxForwardPullStepCm;

	/** 2단계 거리당 layout 시도 수 */
	UPROPERTY(Config, EditAnywhere, Category = "Shop|Unboxing", meta = (ClampMin = "1"))
	int32 UnboxForwardLayoutAttempts = DefaultUnboxForwardLayoutAttempts;

	/** 2단계 최저 바닥면의 발바닥 위 높이 */
	UPROPERTY(Config, EditAnywhere, Category = "Shop|Unboxing", meta = (ClampMin = "0.0"))
	float UnboxForwardFloorClearanceCm = DefaultUnboxForwardFloorClearanceCm;

	/** 바닥 정면 단계 무리와 카메라의 최소 분리 여유 */
	UPROPERTY(Config, EditAnywhere, Category = "Shop|Unboxing", meta = (ClampMin = "0.0"))
	float UnboxCameraClearanceCm = DefaultUnboxCameraClearanceCm;

	/** 3단계(머리 위) capsule 윗면 위 시작 여유 */
	UPROPERTY(Config, EditAnywhere, Category = "Shop|Unboxing", meta = (ClampMin = "0.0"))
	float UnboxOverheadClearanceCm = DefaultUnboxOverheadClearanceCm;

	/** 3단계 단 간격 */
	UPROPERTY(Config, EditAnywhere, Category = "Shop|Unboxing", meta = (ClampMin = "0.01"))
	float UnboxOverheadStepCm = DefaultUnboxOverheadStepCm;

	/** 3단계 단 수 */
	UPROPERTY(Config, EditAnywhere, Category = "Shop|Unboxing", meta = (ClampMin = "1"))
	int32 UnboxOverheadStepCount = DefaultUnboxOverheadStepCount;

	/** 3단계 단당 layout 시도 수 */
	UPROPERTY(Config, EditAnywhere, Category = "Shop|Unboxing", meta = (ClampMin = "1"))
	int32 UnboxOverheadLayoutAttempts = DefaultUnboxOverheadLayoutAttempts;

	/** 무리: 물품당 배치 시도 수 */
	UPROPERTY(Config, EditAnywhere, Category = "Shop|Unboxing", meta = (ClampMin = "1"))
	int32 UnboxClusterPlacementAttempts = DefaultUnboxClusterPlacementAttempts;

	/** 무리: 고도각 크기 하한 */
	UPROPERTY(Config, EditAnywhere, Category = "Shop|Unboxing", meta = (ClampMin = "0.0", ClampMax = "90.0"))
	float UnboxClusterMinElevationDegrees = DefaultUnboxClusterMinElevationDegrees;

	/** 무리: 고도각 크기 상한(하한 이상) */
	UPROPERTY(Config, EditAnywhere, Category = "Shop|Unboxing", meta = (ClampMin = "0.0", ClampMax = "90.0"))
	float UnboxClusterMaxElevationDegrees = DefaultUnboxClusterMaxElevationDegrees;

	/** 무리: 다른 쌍 깊이 허용 오차 */
	UPROPERTY(Config, EditAnywhere, Category = "Shop|Unboxing", meta = (ClampMin = "0.0"))
	float UnboxClusterDepthToleranceCm = DefaultUnboxClusterDepthToleranceCm;

	/** 무리: 쌍 목표 깊이 상한 = 비율 x 두 물품 half extent 최솟값 */
	UPROPERTY(Config, EditAnywhere, Category = "Shop|Unboxing", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float UnboxClusterPairDepthExtentRatio = DefaultUnboxClusterPairDepthExtentRatio;
};
