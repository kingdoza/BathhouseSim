#pragma once

#include "CoreMinimal.h"
#include "Components/BoxComponent.h"
#include "GameplayTagContainer.h"
#include "Interaction/PlayerInteractionFocusObserver.h"
#include "Interaction/PlayerInteractable.h"
#include "Service/ServiceItemTypes.h"
#include "DisplaySpaceComponent.generated.h"

class UInstancedStaticMeshComponent;
class UStaticMeshComponent;
class UDisplayCueComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDisplaySpaceStockChanged, const FDisplaySpaceSnapshot&, Snapshot);

/**
 * One display space = one aim target. Accepts item boxes through held-use Apply/Take, owns its stock stack and
 * drives presentation-only insert preview and take highlight proxies from focus notifications.
 */
UCLASS(ClassGroup = (Bathhouse), meta = (BlueprintSpawnableComponent))
class BATHHOUSESIM_API UDisplaySpaceComponent
	: public UBoxComponent
	, public IPlayerInteractable
	, public IPlayerInteractionFocusObserver
{
	GENERATED_BODY()

public:
	UDisplaySpaceComponent();

	virtual void OnRegister() override;
	virtual void OnComponentDestroyed(bool bDestroyingHierarchy) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	virtual FPlayerInteractionQuery QueryInteraction(const FPlayerInteractionContext& Context) const override;
	virtual FPlayerInteractionResult ExecuteInteraction(const FPlayerInteractionContext& Context) override;
	virtual FPlayerInteractionResult ExecuteHeldTargetUse(
		const FPlayerInteractionContext& Context,
		EPlayerHeldTargetUseDirection Direction) override;
	virtual void NotifyInteractionFocusChanged(
		const UPlayerInteractionComponent& Source,
		const FPlayerInteractionQuery& Query) override;
	virtual void NotifyInteractionFocusEnded(const UPlayerInteractionComponent& Source) override;

	int32 GetSpaceIndex() const
	{
		return SpaceIndex;
	}

	EDisplaySpaceTargetMode GetTargetMode() const
	{
		return TargetMode;
	}

	UServiceItemDefinition* GetFixedKind() const
	{
		return FixedKind;
	}

	FVector GetSlotsWorldCenter() const;
	FPlayerInteractionQuery BuildHeldUseQuery(const FPlayerInteractionContext& Context) const;
	FPlayerInteractionResult ExecuteRoutedHeldTargetUse(const FPlayerInteractionContext& Context,
														EPlayerHeldTargetUseDirection Direction);
	bool ConsumeOneUse(bool& OutDepleted);
	FText GetStockSummary() const;
	FGameplayTag GetAcceptedCategory() const { return AcceptedCategory; }
	int32 GetCapacity() const { return SlotTransforms.Num(); }
	const FServiceItemStack& GetStock() const { return Stock; }
	FDisplaySpaceSnapshot BuildSnapshot() const;

	bool ValidateAuthoring(FText& OutFailureReason) const;
	/** Authoring is valid and the owner, if it is a placeable facility, is in its active placed domain. */
	bool IsOperational() const;
	bool CanAcceptKind(const UServiceItemDefinition* Kind) const;
	/** All-or-nothing check for payload import. Does not mutate. */
	bool ValidateStockImport(const UServiceItemDefinition* Kind, int32 Count, FText& OutFailureReason,
							 int32 InUseRemaining = 0) const;
	/** Applies a validated import and refreshes presentation. Broadcasts only when the stock actually changed. */
	bool ImportStock(UServiceItemDefinition* Kind, int32 Count, FText& OutFailureReason, int32 InUseRemaining = 0);
	/** Customer removal (LIFO). Does not broadcast; the caller publishes after its own commit. */
	bool RemoveOneForCustomer(UServiceItemDefinition*& OutKind, FText& OutFailureReason);
	void PublishStockChanged();

	/** Test/inspection accessors for the transient presentation components. */
	UInstancedStaticMeshComponent* GetStockVisual() const { return StockVisual; }
	UStaticMeshComponent* GetInsertPreview() const { return InsertPreview; }
	UStaticMeshComponent* GetTakeHighlightProxy() const { return TakeHighlightProxy; }

	UPROPERTY(BlueprintAssignable, Category = "Display Space")
	FOnDisplaySpaceStockChanged OnStockChanged;

protected:

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Display Space")
	EDisplaySpaceTargetMode TargetMode = EDisplaySpaceTargetMode::SelfAim;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Display Space")
	TObjectPtr<UServiceItemDefinition> FixedKind;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Display Space", meta = (ClampMin = "0"))
	int32 SpaceIndex = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Display Space")
	FGameplayTag AcceptedCategory;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Display Space", meta = (MakeEditWidget = true))
	TArray<FTransform> SlotTransforms;

private:
	void EnsurePresentationComponents();
	void DestroyPresentationComponents();
	void RefreshStockVisual();
	void HidePresentation();
	FTransform GetSlotWorldRelativeTransform(int32 SlotIndex, const UServiceItemDefinition& Kind) const;
	void ShowInsertPreview(const UServiceItemDefinition& BoxKind);
	void ShowTakeHighlight();

	UPROPERTY(Transient)
	FServiceItemStack Stock;

	UPROPERTY(Transient)
	TObjectPtr<UInstancedStaticMeshComponent> StockVisual;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> InsertPreview;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> TakeHighlightProxy;

	UPROPERTY(Transient)
	TObjectPtr<UDisplayCueComponent> DisplayCue;
};
