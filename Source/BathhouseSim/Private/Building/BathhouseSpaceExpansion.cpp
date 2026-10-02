#include "Building/BathhouseSpaceActor.h"

#include "Building/BathhouseBuildingSettings.h"
#include "Building/BathhouseCleaningChunkSpawner.h"
#include "Building/BathhouseExpansionPurchaseSubsystem.h"
#include "Building/BathhouseSpaceLayout.h"
#include "Building/BathhouseSpaceValidation.h"
#include "Building/BathhouseSpaceValidationInternal.h"
#include "Engine/World.h"
#include "NavigationSystem.h"

// 공간 넓힘(효과 횟수, runtime 적용·되돌림, 편집 미리보기 문구, 구입 등록, Nav dirty).
// BathhouseSpaceActor.cpp는 크기 한도 때문에 건드리지 않는다.

#define LOCTEXT_NAMESPACE "BathhouseSpaceExpansion"

void ABathhouseSpaceActor::CollectStepSnapshots(TArray<FBathhouseExpansionStepSnapshot>& OutSteps) const
{
	OutSteps.Reset();
	for (const FBathhouseSpaceExpansionStep& Step : ExpansionSteps)
	{
		FBathhouseExpansionStepSnapshot& Item = OutSteps.AddDefaulted_GetRef();
		Item.Price = Step.Price;
		for (const FBathhouseSpaceExpansionSide& Wall : Step.Sides)
		{
			FBathhouseExpansionSideSnapshot& WallItem = Item.Sides.AddDefaulted_GetRef();
			WallItem.Side = Wall.Side;
			WallItem.AmountCm = Wall.AmountCm;
		}
	}
}

int32 ABathhouseSpaceActor::GetNextExpansionPrice() const
{
	return ExpansionSteps.IsValidIndex(AppliedExpansionCount) ? ExpansionSteps[AppliedExpansionCount].Price : 0;
}

bool ABathhouseSpaceActor::IsAtExpansionLimit() const
{
	return AppliedExpansionCount >= ExpansionSteps.Num();
}

FBox2D ABathhouseSpaceActor::GetInteriorRectForCount(const int32 Count) const
{
	TArray<FBathhouseExpansionStepSnapshot> Steps;
	CollectStepSnapshots(Steps);
	return FBathhouseSpaceLayout::ExpandInterior(GetBaseInteriorRect(), Steps, Count);
}

int32 ABathhouseSpaceActor::GetEffectiveExpansionCount() const
{
#if WITH_EDITORONLY_DATA
	const UWorld* World = GetWorld();
	if (World && World->WorldType == EWorldType::Editor)
	{
		return FMath::Clamp(EditorPreviewExpansionCount, 0, ExpansionSteps.Num());
	}
#endif
	return FMath::Clamp(AppliedExpansionCount, 0, ExpansionSteps.Num());
}

bool ABathhouseSpaceActor::CanApplyNextExpansion(FText& OutFailureReason) const
{
	const UWorld* World = GetWorld();
	if (!World || !World->IsGameWorld() || !HasActorBegunPlay())
	{
		OutFailureReason = LOCTEXT("NotPlaying", "게임이 시작된 world에서만 공간을 넓힐 수 있습니다.");
		return false;
	}
	if (AppliedExpansionCount >= ExpansionSteps.Num())
	{
		OutFailureReason = LOCTEXT("StepsExhausted", "이 공간은 더 넓힐 수 없습니다.");
		return false;
	}
	TArray<FBathhouseExpansionStepSnapshot> Steps;
	CollectStepSnapshots(Steps);
	const FBathhouseExpansionStepSnapshot& Next = Steps[AppliedExpansionCount];
	if (Next.Sides.IsEmpty())
	{
		OutFailureReason = LOCTEXT("NoSides", "다음 넓힘 줄에 물러날 벽이 없습니다.");
		return false;
	}
	for (int32 Index = 0; Index < Next.Sides.Num(); ++Index)
	{
		for (int32 Other = 0; Other < Index; ++Other)
		{
			if (Next.Sides[Other].Side == Next.Sides[Index].Side)
			{
				OutFailureReason = LOCTEXT("DuplicateSide", "다음 넓힘 줄에 같은 벽이 두 번 있습니다.");
				return false;
			}
		}
	}
	if (!FBathhouseSpaceLayout::IsStepApplicable(Next))
	{
		OutFailureReason = LOCTEXT("InvalidAmount", "다음 넓힘 줄의 벽 양이 0보다 큰 유한한 값이 아닙니다.");
		return false;
	}
	if (!(Next.Price > 0))
	{
		OutFailureReason = LOCTEXT("InvalidPrice", "다음 넓힘 줄의 가격이 0 이하입니다.");
		return false;
	}
	if (!GetActorRotation().IsNearlyZero(UE_KINDA_SMALL_NUMBER)
		|| !GetActorScale3D().Equals(FVector::OneVector, UE_KINDA_SMALL_NUMBER))
	{
		OutFailureReason = LOCTEXT("InvalidTransform", "공간 Actor의 Rotation은 0, Scale은 1이어야 합니다.");
		return false;
	}
	const UBathhouseBuildingSettings* Settings = GetDefault<UBathhouseBuildingSettings>();
	const double Wall = Settings->GetWallThicknessCm();
	const double Slab = Settings->GetSlabThicknessCm();
	if (!Settings->LoadShellBoxMesh() || !(Wall > 0.0) || !(Slab > 0.0) || !FMath::IsFinite(Wall) || !FMath::IsFinite(Slab))
	{
		OutFailureReason = LOCTEXT("InvalidSettings", "Bathhouse Building 설정(상자 mesh, 벽·판 두께)이 올바르지 않습니다.");
		return false;
	}
	return true;
}

bool ABathhouseSpaceActor::ApplyNextExpansion(FBathhouseSpaceExpansionUndo& OutUndo, FText& OutFailureReason)
{
	OutUndo = FBathhouseSpaceExpansionUndo();
	if (!CanApplyNextExpansion(OutFailureReason))
	{
		return false;
	}
	OutUndo.PreviousCount = AppliedExpansionCount;
	OutUndo.PreviousChunkCount = CleaningChunks.Num();
	++AppliedExpansionCount;
	ApplyZoneGeometry();
	if (!RebuildShell())
	{
		UndoExpansion(OutUndo);
		OutFailureReason = LOCTEXT("RebuildFailed", "넓힌 공간의 형상을 만들지 못했습니다.");
		return false;
	}
	if (CleaningChunkKind != EBathhouseCleaningChunkKind::None)
	{
		TArray<FBathhouseExpansionStepSnapshot> Steps;
		CollectStepSnapshots(Steps);
		TArray<FBox2D> Bands;
		FBathhouseSpaceLayout::ExpansionBandRects(GetBaseInteriorRect(), Steps, OutUndo.PreviousCount, Bands);
		const FVector2D ChunkMax = GetDefault<UBathhouseBuildingSettings>()->GetCleaningChunkMaxSizeCm();
		TArray<FBox2D> Rects;
		for (const FBox2D& Band : Bands)
		{
			TArray<FBox2D> BandChunks;
			FBathhouseSpaceLayout::SplitChunks(Band, ChunkMax, BandChunks);
			Rects.Append(BandChunks);
		}
		FBathhouseCleaningChunkSpawner::Spawn(*this, CleaningChunkKind, Rects, GetFloorZ(), CleaningChunks);
	}
	return true;
}

void ABathhouseSpaceActor::UndoExpansion(const FBathhouseSpaceExpansionUndo& Undo)
{
	if (!Undo.IsSet())
	{
		return;
	}
	for (int32 Index = CleaningChunks.Num() - 1; Index >= Undo.PreviousChunkCount && Index >= 0; --Index)
	{
		if (AActor* Chunk = CleaningChunks[Index].Get())
		{
			Chunk->Destroy();
		}
		CleaningChunks.RemoveAt(Index);
	}
	AppliedExpansionCount = Undo.PreviousCount;
	ApplyZoneGeometry();
	RebuildShell();
}

FString ABathhouseSpaceActor::BuildExpansionPreviewLabel(
	const TArray<FBathhouseSpaceSnapshot>& Snapshots, const int32 Index, const FBathhouseLayoutValues& Values) const
{
	using namespace BathhouseSpaceValidationDetail;
	if (!Snapshots.IsValidIndex(Index))
	{
		return FString();
	}
	FString Label = FString::Printf(TEXT("넓힘 미리보기 %d회"), Snapshots[Index].ExpansionCount);
	for (int32 Other = 0; Other < Snapshots.Num(); ++Other)
	{
		if (Other != Index && IsUsable(Snapshots[Index]) && IsUsable(Snapshots[Other])
			&& VolumesOverlap(Snapshots[Index], Snapshots[Other], Values))
		{
			Label += TEXT(" · 겹침 있음");
			break;
		}
	}
	return Label;
}

void ABathhouseSpaceActor::RegisterWithPurchaseSubsystem()
{
	if (UWorld* World = GetWorld())
	{
		if (UBathhouseExpansionPurchaseSubsystem* Subsystem = World->GetSubsystem<UBathhouseExpansionPurchaseSubsystem>())
		{
			Subsystem->RegisterSpace(*this);
		}
	}
}

void ABathhouseSpaceActor::UnregisterFromPurchaseSubsystem()
{
	if (UWorld* World = GetWorld())
	{
		if (UBathhouseExpansionPurchaseSubsystem* Subsystem = World->GetSubsystem<UBathhouseExpansionPurchaseSubsystem>())
		{
			Subsystem->UnregisterSpace(*this);
		}
	}
}

void ABathhouseSpaceActor::MarkExpansionNavigationDirty() const
{
	UWorld* World = GetWorld();
	UNavigationSystemV1* Nav = World ? FNavigationSystem::GetCurrent<UNavigationSystemV1>(World) : nullptr;
	if (!Nav)
	{
		return;
	}
	const UBathhouseBuildingSettings* Settings = GetDefault<UBathhouseBuildingSettings>();
	const double Wall = Settings->GetWallThicknessCm();
	const double Slab = Settings->GetSlabThicknessCm();
	const FBox2D End = GetInteriorRectForCount(ExpansionSteps.Num());
	const double Zf = GetFloorZ();
	const double Zc = GetCeilingZ();
	const FBox Area(
		FVector(End.Min.X - Wall, End.Min.Y - Wall, Zf - Slab),
		FVector(End.Max.X + Wall, End.Max.Y + Wall, Zc + Slab));
	Nav->AddDirtyArea(Area, ENavigationDirtyFlag::All, TEXT("BathhouseExpansionEndShape"));
}

#if WITH_EDITOR
void ABathhouseSpaceActor::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
#if WITH_EDITORONLY_DATA
	EditorPreviewExpansionCount = FMath::Clamp(EditorPreviewExpansionCount, 0, ExpansionSteps.Num());
#endif
	Super::PostEditChangeProperty(PropertyChangedEvent);
}
#endif

#undef LOCTEXT_NAMESPACE
