#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Facility/BathWaterOperationsTypes.h"
#include "BathWaterMapWidget.generated.h"

class ABathhouseBathFacilityActor;
class AFacilityPlacementZoneActor;
class UBathWaterBathTileWidget;
class UBorder;
class UCanvasPanel;
class USizeBox;
class UTextBlock;

struct FBathWaterMapProjection
{
	FVector2D Position = FVector2D::ZeroVector;
	FVector2D Size = FVector2D::ZeroVector;
	float AngleDegrees = 0.0f;
	TArray<FVector2D, TInlineAllocator<4>> Corners;
};

DECLARE_MULTICAST_DELEGATE_OneParam(FOnBathWaterMapSelectionChanged, ABathhouseBathFacilityActor*);

UCLASS()
class BATHHOUSESIM_API UBathWaterMapWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetPlacementZone(AFacilityPlacementZoneActor* InZone);
	void ApplySnapshot(const FBathWaterOperationsSnapshot& Snapshot, ABathhouseBathFacilityActor* SelectedBath);
	void ClearTiles();
	static bool ProjectFootprint(
		const FTransform& ZoneTransform,
		const FVector& ZoneUnscaledHalfExtent,
		const FVector2D& CanvasSize,
		const FTransform& FootprintTransform,
		const FVector& FootprintUnscaledHalfExtent,
		FBathWaterMapProjection& OutProjection,
		bool& bOutInsideZone);
	FOnBathWaterMapSelectionChanged OnSelectionChanged;

protected:
	virtual void NativeDestruct() override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<USizeBox> MapSize;
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UBorder> MapFrame;
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCanvasPanel> GridCanvas;
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCanvasPanel> BathTileCanvas;
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> EmptyStateText;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Bath Water Map")
	TSubclassOf<UBathWaterBathTileWidget> BathTileWidgetClass;

private:
	FVector2D ResolveCanvasSize(const UCanvasPanel* Canvas) const;
	bool IsSnapshotInsideZone(const FBathWaterBathSnapshot& Snapshot) const;
	void RebuildTiles(const FBathWaterOperationsSnapshot& Snapshot);
	void RebuildGrid(const FTransform& ZoneTransform, const FVector& ZoneExtent);
	void ClearGrid();
	void LayoutTile(UBathWaterBathTileWidget& Tile, const FBathWaterBathSnapshot& Snapshot);
	void HandleTileSelected(ABathhouseBathFacilityActor* Bath);

	TWeakObjectPtr<AFacilityPlacementZoneActor> PlacementZone;
	TMap<TWeakObjectPtr<ABathhouseBathFacilityActor>, TObjectPtr<UBathWaterBathTileWidget>> Tiles;
	TMap<TWeakObjectPtr<ABathhouseBathFacilityActor>, FBathWaterMapProjection> CachedProjections;
	uint64 LastTopologyRevision = MAX_uint64;
	FTransform LastZoneTransform;
	FVector LastZoneExtent = FVector::ZeroVector;
	FVector2D LastGridCanvasSize = FVector2D::ZeroVector;
	FTransform LastGridZoneTransform;
	FVector LastGridZoneExtent = FVector::ZeroVector;
	float LastMajorGridSpacingCm = 0.0f;
#if WITH_DEV_AUTOMATION_TESTS
	friend class FBathWaterOperationsUIWidgetTest;
	int32 RebuildCount = 0;
	int32 LayoutWriteCount = 0;
#endif
};
