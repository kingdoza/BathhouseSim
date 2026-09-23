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
	if (LastGridCanvasSize.Equals(CanvasSize) && LastGridZoneTransform.Equals(ZoneTransform)
		&& LastGridZoneExtent.Equals(ZoneExtent)
		&& FMath::IsNearlyEqual(LastMajorGridSpacingCm, MajorSpacing))
	{
		return;
	}
	GridCanvas->ClearChildren();
	LastGridCanvasSize = CanvasSize;
	LastGridZoneTransform = ZoneTransform;
	LastGridZoneExtent = ZoneExtent;
	LastMajorGridSpacingCm = MajorSpacing;
	GridCanvas->SetVisibility(ESlateVisibility::HitTestInvisible);

	const FVector ZoneScale = ZoneTransform.GetScale3D().GetAbs();
	const float HalfX = ZoneExtent.X * ZoneScale.X;
	const float HalfY = ZoneExtent.Y * ZoneScale.Y;
	if (!FMath::IsFinite(HalfX) || !FMath::IsFinite(HalfY) || HalfX <= 0.0f || HalfY <= 0.0f
		|| !FMath::IsFinite(MajorSpacing) || MajorSpacing <= 0.0f)
	{
		return;
	}
	const float PixelsPerUnit = FMath::Min(CanvasSize.X / (2.0f * HalfY), CanvasSize.Y / (2.0f * HalfX));
	const FVector2D ContentSize(2.0f * HalfY * PixelsPerUnit, 2.0f * HalfX * PixelsPerUnit);
	const FVector2D Origin = (CanvasSize - ContentSize) * 0.5f;
	const FLinearColor GridColor(0.20f, 0.49f, 0.56f, 0.36f);
	const FLinearColor BoundaryColor(0.25f, 0.78f, 0.85f, 0.85f);
	auto AddLine = [this](const FVector2D& Position, const FVector2D& Size, const FLinearColor& Color)
	{
		UBorder* Line = WidgetTree->ConstructWidget<UBorder>();
		Line->SetBrushColor(Color);
		Line->SetVisibility(ESlateVisibility::HitTestInvisible);
		UCanvasPanelSlot* Slot = GridCanvas->AddChildToCanvas(Line);
		Slot->SetPosition(Position);
		Slot->SetSize(Size);
	};
	if (HalfX / MajorSpacing < 200.0f && HalfY / MajorSpacing < 200.0f)
	{
		for (int32 Cell = FMath::CeilToInt(-HalfY / MajorSpacing); Cell <= FMath::FloorToInt(HalfY / MajorSpacing); ++Cell)
		{
			const float X = Origin.X + (Cell * MajorSpacing + HalfY) * PixelsPerUnit;
			AddLine(FVector2D(X, Origin.Y), FVector2D(1.0f, ContentSize.Y), GridColor);
		}
		for (int32 Cell = FMath::CeilToInt(-HalfX / MajorSpacing); Cell <= FMath::FloorToInt(HalfX / MajorSpacing); ++Cell)
		{
			const float Y = Origin.Y + (HalfX - Cell * MajorSpacing) * PixelsPerUnit;
			AddLine(FVector2D(Origin.X, Y), FVector2D(ContentSize.X, 1.0f), GridColor);
		}
	}
	AddLine(Origin, FVector2D(ContentSize.X, 2.0f), BoundaryColor);
	AddLine(Origin + FVector2D(0.0f, ContentSize.Y - 2.0f), FVector2D(ContentSize.X, 2.0f), BoundaryColor);
	AddLine(Origin, FVector2D(2.0f, ContentSize.Y), BoundaryColor);
	AddLine(Origin + FVector2D(ContentSize.X - 2.0f, 0.0f), FVector2D(2.0f, ContentSize.Y), BoundaryColor);
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

	const float PixelsPerUnit = FMath::Min(CanvasSize.X / (2.0f * HalfY), CanvasSize.Y / (2.0f * HalfX));
	const FVector2D ContentSize(2.0f * HalfY * PixelsPerUnit, 2.0f * HalfX * PixelsPerUnit);
	const FVector2D Origin = (CanvasSize - ContentSize) * 0.5f;
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
