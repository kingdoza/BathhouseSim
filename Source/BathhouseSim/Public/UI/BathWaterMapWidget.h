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

	/** 굵은 칸 선의 render target px 두께. 1 미만은 선이 사라질 수 있어 허용하지 않는다. */
	UPROPERTY(EditDefaultsOnly, Category = "Bath Water Map|Grid", meta = (ClampMin = "1"))
	float GridLineThicknessPx = 1.0f;
	/** Zone 경계선의 render target px 두께. */
	UPROPERTY(EditDefaultsOnly, Category = "Bath Water Map|Grid", meta = (ClampMin = "1"))
	float BoundaryLineThicknessPx = 2.0f;
	UPROPERTY(EditDefaultsOnly, Category = "Bath Water Map|Grid")
	FLinearColor GridLineColor = FLinearColor(0.20f, 0.49f, 0.56f, 0.36f);
	UPROPERTY(EditDefaultsOnly, Category = "Bath Water Map|Grid")
	FLinearColor BoundaryLineColor = FLinearColor(0.25f, 0.78f, 0.85f, 0.85f);

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
	FVector2D LastGridRenderScale = FVector2D::ZeroVector;
#if WITH_DEV_AUTOMATION_TESTS
	friend class FBathWaterOperationsUIWidgetTest;
	friend class FBathWaterMapGridLineLayoutTest;
	int32 RebuildCount = 0;
	int32 LayoutWriteCount = 0;
#endif
};
