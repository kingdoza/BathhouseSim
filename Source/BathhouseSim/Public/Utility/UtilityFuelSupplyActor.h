#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/PlayerInteractable.h"
#include "Utility/UtilityFuelTypes.h"
#include "UtilityFuelSupplyActor.generated.h"

class UPlayerCarryComponent;
class USceneComponent;
class UStaticMeshComponent;

UCLASS(Blueprintable)
class BATHHOUSESIM_API AUtilityFuelSupplyActor : public AActor, public IPlayerInteractable
{
	GENERATED_BODY()

public:
	AUtilityFuelSupplyActor();
	virtual FPlayerInteractionQuery QueryInteraction(const FPlayerInteractionContext& Context) const override;
	virtual FPlayerInteractionResult ExecuteInteraction(const FPlayerInteractionContext& Context) override;

	UStaticMeshComponent* GetSupplyMesh() const { return SupplyMesh; }
	EUtilityFuelKind GetFuelKind() const { return FuelKind; }
	float GetScoopPoints() const { return ScoopPoints; }
	bool HasValidAuthoring(FText& OutFailureReason) const;
	bool CanScoop(const AActor* Shovel, FUtilityFuelLoad& OutLoad, FText& OutFailureReason) const;
	bool CanReturn(const FUtilityFuelLoad& Load, FText& OutFailureReason) const;

#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Utility|Fuel")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Utility|Fuel")
	TObjectPtr<UStaticMeshComponent> SupplyMesh;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Utility|Fuel")
	EUtilityFuelKind FuelKind = EUtilityFuelKind::Coal;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Utility|Fuel", meta = (ClampMin = "0.01"))
	float ScoopPoints = 25.0f;

private:
	friend class FUtilityFuelTransaction;
	bool TryAcquireMutationGuard();
	void ReleaseMutationGuard();
	bool bFuelMutationInProgress = false;
};
