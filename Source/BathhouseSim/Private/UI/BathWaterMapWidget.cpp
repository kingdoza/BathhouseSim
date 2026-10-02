#include "UI/BathWaterMapWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/BoxComponent.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Facility/BathhouseBathFacilityActor.h"
#include "Placement/FacilityPlacementSettings.h"
#include "Placement/FacilityPlacementZoneActor.h"
#include "UI/BathWaterBathTileWidget.h"
#include "UI/BathWaterMapGridLayout.h"

void UBathWaterMapWidget::SetPlacementZone(AFacilityPlacementZoneActor* InZone)
{
	if (PlacementZone.Get() != InZone)
	{
		PlacementZone = InZone;
		LastTopologyRevision = MAX_uint64;
		ClearTiles();
		ClearGrid();
	}
}

void UBathWaterMapWidget::ApplySnapshot(
	const FBathWaterOperationsSnapshot& Snapshot,
	ABathhouseBathFacilityActor* SelectedBath)
{
	if (!PlacementZone.IsValid() || !BathTileCanvas)
	{
		ClearTiles();
		ClearGrid();
		return;
	}
	const UBoxComponent* Bounds = PlacementZone->GetZoneBounds();
	if (!Bounds)
	{
		ClearTiles();
		ClearGrid();
		return;
	}
	const FTransform ZoneTransform = Bounds->GetComponentTransform();
	const FVector ZoneExtent = Bounds->GetUnscaledBoxExtent();
	RebuildGrid(ZoneTransform, ZoneExtent);
	const bool bZoneChanged = !ZoneTransform.Equals(LastZoneTransform) || !ZoneExtent.Equals(LastZoneExtent);
	if (LastTopologyRevision != Snapshot.TopologyRevision || bZoneChanged)
	{
		RebuildTiles(Snapshot);
		LastTopologyRevision = Snapshot.TopologyRevision;
		LastZoneTransform = ZoneTransform;
		LastZoneExtent = ZoneExtent;
	}
	for (const FBathWaterBathSnapshot& BathSnapshot : Snapshot.Baths)
	{
		if (TObjectPtr<UBathWaterBathTileWidget>* Tile = Tiles.Find(BathSnapshot.BathActor))
		{
			(*Tile)->ApplyBathSnapshot(BathSnapshot, BathSnapshot.BathActor.Get() == SelectedBath);
			LayoutTile(**Tile, BathSnapshot);
		}
	}
	if (EmptyStateText)
	{
		const ESlateVisibility DesiredVisibility = Tiles.IsEmpty()
			? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed;
		if (EmptyStateText->GetVisibility() != DesiredVisibility)
		{
			EmptyStateText->SetVisibility(DesiredVisibility);
		}
	}
}

void UBathWaterMapWidget::ClearTiles()
{
	for (TPair<TWeakObjectPtr<ABathhouseBathFacilityActor>, TObjectPtr<UBathWaterBathTileWidget>>& Entry : Tiles)
	{
		if (Entry.Value)
		{
			Entry.Value->OnTileSelected.RemoveAll(this);
		}
	}
	Tiles.Reset();
	CachedProjections.Reset();
	if (BathTileCanvas)
	{
		BathTileCanvas->ClearChildren();
	}
	if (EmptyStateText)
	{
		if (EmptyStateText->GetVisibility() != ESlateVisibility::HitTestInvisible)
		{
			EmptyStateText->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
	}
}

void UBathWaterMapWidget::ClearGrid()
{
	if (GridCanvas)
	{
		GridCanvas->ClearChildren();
	}
	LastGridCanvasSize = FVector2D::ZeroVector;
	LastGridZoneExtent = FVector::ZeroVector;
	LastMajorGridSpacingCm = 0.0f;
	LastGridRenderScale = FVector2D::ZeroVector;
}

FVector2D UBathWaterMapWidget::ResolveCanvasSize(const UCanvasPanel* Canvas) const
{
	if (!Canvas)
	{
		return FVector2D::ZeroVector;
	}
	const FVector2D CachedSize = Canvas->GetCachedGeometry().GetLocalSize();
	if (CachedSize.X > 2.0f && CachedSize.Y > 2.0f)
	{
		return CachedSize;
	}
	if (!MapSize || !MapFrame)
	{
		return FVector2D::ZeroVector;
	}
	const FVector2D DesiredSize = MapSize->GetDesiredSize();
	const FMargin FramePadding = MapFrame->GetPadding();
	return FVector2D(FMath::Max(0.0f, DesiredSize.X - FramePadding.Left - FramePadding.Right),
		FMath::Max(0.0f, DesiredSize.Y - FramePadding.Top - FramePadding.Bottom));
}

void UBathWaterMapWidget::RebuildGrid(const FTransform& ZoneTransform, const FVector& ZoneExtent)
{
	if (!GridCanvas || !PlacementZone.IsValid())
	{
		return;
	}
	const FVector2D CanvasSize = ResolveCanvasSize(GridCanvas);
	if (CanvasSize.X <= 1.0f || CanvasSize.Y <= 1.0f)
	{
		return;
	}
	const float MajorSpacing = GetDefault<UFacilityPlacementSettings>()->GetGridSizeCm()
		* PlacementZone->GetMajorGridIntervalCells();
	const FVector2D RenderScale = ResolveRenderPixelsPerLayoutUnit(GridCanvas->GetPaintSpaceGeometry());
	if (LastGridCanvasSize.Equals(CanvasSize) && LastGridZoneTransform.Equals(ZoneTransform)
		&& LastGridZoneExtent.Equals(ZoneExtent)
		&& FMath::IsNearlyEqual(LastMajorGridSpacingCm, MajorSpacing)
		&& LastGridRenderScale.Equals(RenderScale))
	{
		return;
	}
	GridCanvas->ClearChildren();
	LastGridRenderScale = RenderScale;
	LastGridCanvasSize = CanvasSize;
	LastGridZoneTransform = ZoneTransform;
	LastGridZoneExtent = ZoneExtent;
	LastMajorGridSpacingCm = MajorSpacing;
	GridCanvas->SetVisibility(ESlateVisibility::HitTestInvisible);

	const FVector ZoneScale = ZoneTransform.GetScale3D().GetAbs();
	FBathWaterMapGridInput Input;
	Input.CanvasSize = CanvasSize;
	Input.ZoneHalfXCm = ZoneExtent.X * ZoneScale.X;
	Input.ZoneHalfYCm = ZoneExtent.Y * ZoneScale.Y;
	Input.MajorSpacingCm = MajorSpacing;
	Input.RenderPixelsPerLayoutUnit = RenderScale;
	Input.GridThicknessPx = GridLineThicknessPx;
	Input.BoundaryThicknessPx = BoundaryLineThicknessPx;
	TArray<FBathWaterMapGridLine> Lines;
	if (!BuildBathWaterMapGridLines(Input, Lines))
	{
		return;
	}
	for (const FBathWaterMapGridLine& Entry : Lines)
	{
		UBorder* Line = WidgetTree->ConstructWidget<UBorder>();
		Line->SetBrushColor(Entry.bBoundary ? BoundaryLineColor : GridLineColor);
		Line->SetVisibility(ESlateVisibility::HitTestInvisible);
		UCanvasPanelSlot* LineSlot = GridCanvas->AddChildToCanvas(Line);
		LineSlot->SetPosition(Entry.Position);
		LineSlot->SetSize(Entry.Size);
	}
}

void UBathWaterMapWidget::NativeDestruct()
{
	ClearTiles();
	ClearGrid();
	PlacementZone.Reset();
	OnSelectionChanged.Clear();
	Super::NativeDestruct();
}

bool UBathWaterMapWidget::IsSnapshotInsideZone(const FBathWaterBathSnapshot& Snapshot) const
{
	const UBoxComponent* Bounds = PlacementZone.IsValid() ? PlacementZone->GetZoneBounds() : nullptr;
	if (!Bounds || !Snapshot.BathActor.IsValid())
	{
		return false;
	}
	FBathWaterMapProjection Projection;
	bool bInside = false;
	return ProjectFootprint(Bounds->GetComponentTransform(), Bounds->GetUnscaledBoxExtent(),
		FVector2D(1.0f), Snapshot.FootprintTransform, Snapshot.FootprintHalfExtent, Projection, bInside)
		&& bInside;
}

bool UBathWaterMapWidget::ProjectFootprint(
	const FTransform& ZoneTransform,
	const FVector& ZoneUnscaledHalfExtent,
	const FVector2D& CanvasSize,
	const FTransform& FootprintTransform,
	const FVector& FootprintUnscaledHalfExtent,
	FBathWaterMapProjection& OutProjection,
	bool& bOutInsideZone)
{
	OutProjection = FBathWaterMapProjection();
	bOutInsideZone = false;
	const FVector ZoneScale = ZoneTransform.GetScale3D().GetAbs();
	const FVector ZoneX = ZoneTransform.GetUnitAxis(EAxis::X);
	const FVector ZoneY = ZoneTransform.GetUnitAxis(EAxis::Y);
	const float HalfX = ZoneUnscaledHalfExtent.X * ZoneScale.X;
	const float HalfY = ZoneUnscaledHalfExtent.Y * ZoneScale.Y;
	if (!ZoneTransform.IsValid() || !FootprintTransform.IsValid()
		|| !FMath::IsFinite(HalfX) || !FMath::IsFinite(HalfY)
		|| !FMath::IsFinite(FootprintUnscaledHalfExtent.X)
		|| !FMath::IsFinite(FootprintUnscaledHalfExtent.Y)
		|| HalfX <= KINDA_SMALL_NUMBER || HalfY <= KINDA_SMALL_NUMBER
		|| FootprintUnscaledHalfExtent.X <= KINDA_SMALL_NUMBER
		|| FootprintUnscaledHalfExtent.Y <= KINDA_SMALL_NUMBER
		|| !FMath::IsFinite(CanvasSize.X) || !FMath::IsFinite(CanvasSize.Y)
		|| CanvasSize.X <= 0.0f || CanvasSize.Y <= 0.0f)
	{
		return false;
	}

	FBathWaterMapLetterbox Letterbox;
	if (!ComputeBathWaterMapLetterbox(CanvasSize, HalfX, HalfY, Letterbox))
	{
		return false;
	}
	const float PixelsPerUnit = Letterbox.PixelsPerCm;
	const FVector2D Origin = Letterbox.Origin;
	const FVector ZoneCenter = ZoneTransform.GetLocation();
	const FVector2D Signs[4] = {
		FVector2D(-1.0f, -1.0f), FVector2D(-1.0f, 1.0f),
		FVector2D(1.0f, 1.0f), FVector2D(1.0f, -1.0f) };
	bOutInsideZone = true;
	for (const FVector2D& Sign : Signs)
	{
		const FVector WorldCorner = FootprintTransform.TransformPosition(FVector(
			FootprintUnscaledHalfExtent.X * Sign.X,
			FootprintUnscaledHalfExtent.Y * Sign.Y, 0.0f));
		const FVector Delta = WorldCorner - ZoneCenter;
		const float LocalX = FVector::DotProduct(Delta, ZoneX);
		const float LocalY = FVector::DotProduct(Delta, ZoneY);
		bOutInsideZone &= FMath::Abs(LocalX) <= HalfX + KINDA_SMALL_NUMBER
			&& FMath::Abs(LocalY) <= HalfY + KINDA_SMALL_NUMBER;
		OutProjection.Corners.Add(Origin + FVector2D(
			(LocalY + HalfY) * PixelsPerUnit,
			(HalfX - LocalX) * PixelsPerUnit));
	}
	OutProjection.Position = (OutProjection.Corners[0] + OutProjection.Corners[2]) * 0.5f;
	const FVector2D WidthEdge = OutProjection.Corners[1] - OutProjection.Corners[0];
	const FVector2D HeightEdge = OutProjection.Corners[3] - OutProjection.Corners[0];
	OutProjection.Size = FVector2D(WidthEdge.Size(), HeightEdge.Size());
	OutProjection.AngleDegrees = FMath::RadiansToDegrees(FMath::Atan2(WidthEdge.Y, WidthEdge.X));
	return true;
}

void UBathWaterMapWidget::RebuildTiles(const FBathWaterOperationsSnapshot& Snapshot)
{
	ClearTiles();
#if WITH_DEV_AUTOMATION_TESTS
	++RebuildCount;
#endif
	if (!BathTileCanvas || !BathTileWidgetClass || !PlacementZone.IsValid())
	{
		return;
	}
	for (const FBathWaterBathSnapshot& BathSnapshot : Snapshot.Baths)
	{
		if (!IsSnapshotInsideZone(BathSnapshot))
		{
			continue;
		}
		UBathWaterBathTileWidget* Tile = CreateWidget<UBathWaterBathTileWidget>(GetWorld(), BathTileWidgetClass);
		if (!Tile)
		{
			continue;
		}
		BathTileCanvas->AddChildToCanvas(Tile);
		Tile->OnTileSelected.AddUObject(this, &UBathWaterMapWidget::HandleTileSelected);
		Tile->ApplyBathSnapshot(BathSnapshot, false);
		Tiles.Add(BathSnapshot.BathActor, Tile);
		LayoutTile(*Tile, BathSnapshot);
	}
}

void UBathWaterMapWidget::LayoutTile(
	UBathWaterBathTileWidget& Tile,
	const FBathWaterBathSnapshot& Snapshot)
{
	const UBoxComponent* Bounds = PlacementZone.IsValid() ? PlacementZone->GetZoneBounds() : nullptr;
	UCanvasPanelSlot* PanelSlot = Cast<UCanvasPanelSlot>(Tile.Slot);
	if (!Bounds || !PanelSlot)
	{
		return;
	}
	const FVector2D CanvasSize = ResolveCanvasSize(BathTileCanvas);
	if (CanvasSize.X <= 1.0f || CanvasSize.Y <= 1.0f)
	{
		return;
	}
	FBathWaterMapProjection Projection;
	bool bInside = false;
	if (!ProjectFootprint(Bounds->GetComponentTransform(), Bounds->GetUnscaledBoxExtent(), CanvasSize,
		Snapshot.FootprintTransform, Snapshot.FootprintHalfExtent, Projection, bInside) || !bInside)
	{
		return;
	}
	const FBathWaterMapProjection* Cached = CachedProjections.Find(Snapshot.BathActor);
	if (Cached && Cached->Position.Equals(Projection.Position) && Cached->Size.Equals(Projection.Size)
		&& FMath::IsNearlyEqual(Cached->AngleDegrees, Projection.AngleDegrees))
	{
		return;
	}
	PanelSlot->SetAnchors(FAnchors(0.0f, 0.0f));
	PanelSlot->SetAlignment(FVector2D(0.5f));
	PanelSlot->SetPosition(Projection.Position);
	PanelSlot->SetSize(Projection.Size);
	Tile.SetRenderTransformPivot(FVector2D(0.5f));
	Tile.SetRenderTransformAngle(Projection.AngleDegrees);
	CachedProjections.Add(Snapshot.BathActor, Projection);
#if WITH_DEV_AUTOMATION_TESTS
	++LayoutWriteCount;
#endif
}

void UBathWaterMapWidget::HandleTileSelected(ABathhouseBathFacilityActor* Bath)
{
	OnSelectionChanged.Broadcast(Bath);
}
