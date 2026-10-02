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
		Item.Side = Step.Side;
		Item.AmountCm = Step.AmountCm;
	}
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
	const float Amount = ExpansionSteps[AppliedExpansionCount].AmountCm;
	if (!(Amount > 0.0f) || !FMath::IsFinite(Amount))
	{
		OutFailureReason = LOCTEXT("InvalidAmount", "다음 넓힘 줄의 양이 0보다 큰 유한한 값이 아닙니다.");
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
		const FBox2D Band = FBathhouseSpaceLayout::ExpansionBand(GetBaseInteriorRect(), Steps, OutUndo.PreviousCount);
		TArray<FBox2D> Rects;
		FBathhouseSpaceLayout::SplitChunks(Band, GetDefault<UBathhouseBuildingSettings>()->GetCleaningChunkMaxSizeCm(), Rects);
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
