#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "UtilityGaugeComponent.generated.h"

class USceneComponent;
class UUtilityOperationComponent;
class FUtilityPivotRotation;

UCLASS(ClassGroup = (Bathhouse), meta = (BlueprintSpawnableComponent))
class BATHHOUSESIM_API UUtilityGaugeComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UUtilityGaugeComponent();
	virtual ~UUtilityGaugeComponent();
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	void Configure(UUtilityOperationComponent* InOperation, USceneComponent* InNeedlePivot);
	void CaptureBaselineFromPivot();
	void ApplyCurrentOperation();
	void ApplyPoints(float RemainingPoints, float MaximumPoints);
	void ApplyConstructionPreview();
	bool HasValidAuthoring(FText& OutFailureReason) const;
static float CalculateDisplayFraction(float RemainingPoints, float MaximumPoints, float ActiveStartRatio);

#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Utility Gauge")
	FVector LocalRotationAxis = FVector::ForwardVector;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Utility Gauge")
	float ZeroAngleDegrees = -90.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Utility Gauge")
	float MaxAngleDegrees = 90.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Utility Gauge", meta = (ClampMin = "0.0", ClampMax = "0.99"))
	float ActiveStartRatio = 0.33333334f;

private:
	void BindOperationDelegate();

	UPROPERTY(Transient)
	TObjectPtr<UUtilityOperationComponent> Operation = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<USceneComponent> NeedlePivot = nullptr;

	FUtilityPivotRotation* PivotRotation = nullptr;
	bool bHasBegunPlay = false;
};
