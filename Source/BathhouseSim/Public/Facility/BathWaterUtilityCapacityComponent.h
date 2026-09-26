#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Facility/BathWaterOperationsTypes.h"
#include "BathWaterUtilityCapacityComponent.generated.h"

class UUtilityOperationComponent;

UCLASS(ClassGroup = (Bathhouse), meta = (BlueprintSpawnableComponent))
class BATHHOUSESIM_API UBathWaterUtilityCapacityComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UBathWaterUtilityCapacityComponent();

	EBathWaterCapacityKind GetCapacityKind() const { return CapacityKind; }
	float GetCapacityPoints() const;
	float GetActiveCapacityPoints() const;
	void SetUtilityOperation(UUtilityOperationComponent* InOperation);
	bool HasValidAuthoring(FText& OutFailureReason) const;
	void RestoreCapacity(EBathWaterCapacityKind InKind, float InPoints);

#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Bath Water Utility")
	EBathWaterCapacityKind CapacityKind = EBathWaterCapacityKind::Circulation;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bath Water Utility", meta = (ClampMin = "0.0"))
	float CapacityPoints = 100.0f;

	UPROPERTY(Transient)
	TObjectPtr<UUtilityOperationComponent> UtilityOperation = nullptr;

};
