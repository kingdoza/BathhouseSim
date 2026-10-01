#pragma once
#include "CoreMinimal.h"
#include "Components/BoxComponent.h"
#include "Interaction/PlayerInteractable.h"
#include "Interaction/PlayerInteractionFocusObserver.h"
#include "DisplayFacilityTargetComponent.generated.h"
class UDisplaySpaceComponent;
UCLASS(ClassGroup = (Bathhouse), meta = (BlueprintSpawnableComponent))

class BATHHOUSESIM_API UDisplayFacilityTargetComponent : public UBoxComponent,
														 public IPlayerInteractable,
														 public IPlayerInteractionFocusObserver
{
	GENERATED_BODY()
public:

	UDisplayFacilityTargetComponent();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Service Display")
	FText FacilityDisplayName;

	bool ValidateAuthoring(FText& OutFailureReason) const;
#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif
	virtual FPlayerInteractionQuery QueryInteraction(const FPlayerInteractionContext& Context) const override;
	virtual FPlayerInteractionResult ExecuteInteraction(const FPlayerInteractionContext& Context) override;
	virtual FPlayerInteractionResult ExecuteHeldTargetUse(const FPlayerInteractionContext& Context,
														  EPlayerHeldTargetUseDirection Direction) override;
	virtual void NotifyInteractionFocusChanged(const UPlayerInteractionComponent& Source,
											   const FPlayerInteractionQuery& Query) override;
	virtual void NotifyInteractionFocusEnded(const UPlayerInteractionComponent& Source) override;

private:

	UDisplaySpaceComponent* SelectSpace(const FPlayerInteractionContext& Context,
										const TArray<UDisplaySpaceComponent*>& Spaces) const;
	bool CollectSpaces(TArray<UDisplaySpaceComponent*>& Out) const;
};
