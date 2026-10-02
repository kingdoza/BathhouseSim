#include "Building/BathhouseSpaceActor.h"

#include "Building/BathhouseBuildingSettings.h"
#include "Building/BathhouseCleaningChunkSpawner.h"
#include "Building/BathhouseSpaceEditorSync.h"
#include "Building/BathhouseSpaceLayout.h"
#include "Building/BathhouseSpaceShellComponent.h"
#include "Building/BathhouseSpaceValidation.h"
#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/World.h"
#include "Materials/MaterialInterface.h"
#include "Misc/DataValidation.h"

DEFINE_LOG_CATEGORY(LogBathhouseBuilding);

ABathhouseSpaceActor::ABathhouseSpaceActor()
{
	SpaceRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SpaceRoot"));
	SpaceRoot->SetMobility(EComponentMobility::Static);
	SetRootComponent(SpaceRoot);
	ZoneBounds->SetupAttachment(SpaceRoot);
	Shell = CreateDefaultSubobject<UBathhouseSpaceShellComponent>(TEXT("Shell"));
	Shell->SetupAttachment(SpaceRoot);
#if WITH_EDITORONLY_DATA
	// 형상 계산이 다른 공간의 authored 값을 읽으므로 세 공간은 항상 함께 로드돼야 한다.
	bIsSpatiallyLoaded = false;
#endif
}

FBox2D ABathhouseSpaceActor::GetInteriorRect() const
{
	return GetInteriorRectForCount(GetEffectiveExpansionCount());
}

FBox2D ABathhouseSpaceActor::GetBaseInteriorRect() const
{
	const FVector Location = GetActorLocation();
	const FVector2D Center(Location.X, Location.Y);
	const FVector2D Half(FloorSizeCm.X * 0.5, FloorSizeCm.Y * 0.5);
	return FBox2D(Center - Half, Center + Half);
}

void ABathhouseSpaceActor::FillSnapshot(FBathhouseSpaceSnapshot& Out) const
{
	const FVector Location = GetActorLocation();
	Out = FBathhouseSpaceSnapshot();
	Out.Actor = this;
	Out.Kind = SpaceKind;
	Out.DisplayName = FString::Printf(TEXT("%s(%s)"),
		*StaticEnum<EBathhouseSpaceKind>()->GetDisplayNameTextByValue(static_cast<int64>(SpaceKind)).ToString(),
		*GetActorNameOrLabel());
	Out.ActorXY = FVector2D(Location.X, Location.Y);
	Out.FloorZ = Location.Z;
	Out.BaseInterior = GetBaseInteriorRect();
	CollectStepSnapshots(Out.Steps);
	Out.ExpansionCount = GetEffectiveExpansionCount();
	Out.Interior = FBathhouseSpaceLayout::ExpandInterior(Out.BaseInterior, Out.Steps, Out.ExpansionCount);
	Out.CeilingHeightCm = CeilingHeightCm;
	Out.bTransformValid = GetActorRotation().IsNearlyZero(UE_KINDA_SMALL_NUMBER)
		&& GetActorScale3D().Equals(FVector::OneVector, UE_KINDA_SMALL_NUMBER);
	Out.bAllowedTagsEmpty = AllowedFacilityTags.IsEmpty();
	Out.LightSpacingCm = Lighting.SpacingCm;
	Out.LightCeilingOffsetCm = Lighting.CeilingOffsetCm;
	Out.ChunkKind = CleaningChunkKind;
	Out.bHasWallMaterial = Surfaces.WallMaterial != nullptr;
	Out.bHasFloorMaterial = Surfaces.FloorMaterial != nullptr;
	Out.bHasCeilingMaterial = Surfaces.CeilingMaterial != nullptr;
	for (const FBathhouseSpaceOpening& Opening : Openings)
	{
		FBathhouseOpeningSnapshot& Item = Out.Openings.AddDefaulted_GetRef();
		Item.Side = Opening.Side;
		Item.CenterOffsetCm = Opening.CenterOffsetCm;
		Item.WidthCm = Opening.WidthCm;
		Item.HeightCm = Opening.HeightCm;
		Item.ConnectedActor = Opening.ConnectedSpace;
	}
	for (const FBathhouseStairSpec& Stair : Stairs)
	{
		FBathhouseStairSnapshot& Item = Out.Stairs.AddDefaulted_GetRef();
		Item.LowerActor = Stair.LowerSpace;
		Item.TopEdgeOffsetCm = Stair.TopEdgeCenterOffsetCm;
		Item.DownSide = Stair.DownSide;
		Item.WidthCm = Stair.WidthCm;
		Item.RunCm = Stair.RunCm;
		Item.StepCount = Stair.StepCount;
		Item.GuardHeightCm = Stair.GuardHeightCm;
		Item.bHasStepMaterial = Stair.StepMaterial != nullptr;
		Item.bHasStairWallMaterial = Stair.StairWallMaterial != nullptr;
	}
}

void ABathhouseSpaceActor::ApplyZoneGeometry()
{
	if (!ZoneBounds || !(FloorSizeCm.X > 0.0) || !(FloorSizeCm.Y > 0.0))
	{
		return;
	}
	const FVector Extent = ZoneBounds->GetUnscaledBoxExtent();
	const FBox2D Interior = GetInteriorRect();
	ZoneBounds->SetBoxExtent(FVector(
		(Interior.Max.X - Interior.Min.X) * 0.5, (Interior.Max.Y - Interior.Min.Y) * 0.5, Extent.Z), false);
	const FVector Location = GetActorLocation();
	const FVector2D Offset = Interior.GetCenter() - FVector2D(Location.X, Location.Y);
	const FVector Relative = ZoneBounds->GetRelativeLocation();
	ZoneBounds->SetRelativeLocation(FVector(Offset.X, Offset.Y, Relative.Z));
}

bool ABathhouseSpaceActor::RebuildShell()
{
	UWorld* World = GetWorld();
	if (!World || !Shell)
	{
		return false;
	}
	TArray<FBathhouseSpaceSnapshot> Snapshots;
	FBathhouseSpaceValidation::GatherSnapshots(*World, Snapshots);
	int32 Index = Snapshots.IndexOfByPredicate(
		[this](const FBathhouseSpaceSnapshot& Item) { return Item.Actor.Get() == this; });
	if (Index == INDEX_NONE)
	{
		FillSnapshot(Snapshots.AddDefaulted_GetRef());
		Index = Snapshots.Num() - 1;
	}
	const UBathhouseBuildingSettings* Settings = GetDefault<UBathhouseBuildingSettings>();
	FBathhouseShellVisualInputs Inputs;
	Inputs.BoxMesh = Settings->LoadShellBoxMesh();
	if (!Snapshots[Index].bTransformValid || !Inputs.BoxMesh)
	{
		Shell->ClearGenerated();
		return false;
	}
	FBathhouseLayoutValues Values;
	Values.WallThicknessCm = Settings->GetWallThicknessCm();
	Values.SlabThicknessCm = Settings->GetSlabThicknessCm();
	Values.ChunkMaxSizeCm = Settings->GetCleaningChunkMaxSizeCm();
	const FBathhouseSpacePlan Plan = FBathhouseSpaceLayout::BuildPlan(Snapshots, Index, Values);

	Inputs.WallMaterial = Surfaces.WallMaterial;
	Inputs.FloorMaterial = Surfaces.FloorMaterial;
	Inputs.CeilingMaterial = Surfaces.CeilingMaterial;
	if (!Stairs.IsEmpty())
	{
		Inputs.StepMaterial = Stairs[0].StepMaterial;
		Inputs.StairWallMaterial = Stairs[0].StairWallMaterial;
	}
	Inputs.Lighting = Lighting;
	Inputs.bPreviewChunks = World->WorldType == EWorldType::Editor;
	if (Inputs.bPreviewChunks && Snapshots[Index].ExpansionCount > 0)
	{
		Inputs.PreviewLabel = BuildExpansionPreviewLabel(Snapshots, Index, Values);
		Inputs.PreviewLabelWorldSizeCm = Settings->GetEditorPreviewLabelWorldSizeCm();
		Inputs.PreviewLabelFontSize = Settings->GetEditorPreviewLabelFontSize();
		const FBathhousePreviewLabelPlacement Placement = FBathhouseSpaceLayout::PreviewLabelPlacement(
			Snapshots, Index, Values.WallThicknessCm, Values.SlabThicknessCm, Settings->GetEditorPreviewLabelHeightCm());
		Inputs.PreviewLabelLocation = Placement.Location;
		Inputs.bPreviewLabelSouthOfCenter = Placement.bSouthOfCenter;
	}
	Inputs.ChunkKind = CleaningChunkKind;
	Inputs.ChunkPreviewHalfHeightCm = static_cast<float>(Values.SlabThicknessCm * 0.5);
	Shell->Rebuild(Plan, Inputs);
	return true;
}

void ABathhouseSpaceActor::OnConstruction(const FTransform& Transform)
{
	ApplyZoneGeometry();
	Super::OnConstruction(Transform);
	UWorld* World = GetWorld();
	if (World && !World->IsGameWorld())
	{
		RebuildShell();
#if WITH_EDITOR
		FBathhouseSpaceEditorSync::RequestRebuild(World);
#endif
	}
}

void ABathhouseSpaceActor::BeginPlay()
{
	Super::BeginPlay();
	AppliedExpansionCount = 0;
	ApplyZoneGeometry();
	RebuildShell();
	SpawnCleaningChunks();
	LogValidationProblems();
	MarkExpansionNavigationDirty();
	RegisterWithPurchaseSubsystem();
}

void ABathhouseSpaceActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnregisterFromPurchaseSubsystem();
	DestroyCleaningChunks();
	Super::EndPlay(EndPlayReason);
}

void ABathhouseSpaceActor::SpawnCleaningChunks()
{
	DestroyCleaningChunks();
	if (CleaningChunkKind == EBathhouseCleaningChunkKind::None)
	{
		return;
	}
	FBathhouseSpaceSnapshot Self;
	FillSnapshot(Self);
	if (!Self.bTransformValid)
	{
		return;
	}
	TArray<FBox2D> Rects;
	FBathhouseSpaceLayout::SplitChunks(
		Self.Interior, GetDefault<UBathhouseBuildingSettings>()->GetCleaningChunkMaxSizeCm(), Rects);
	FBathhouseCleaningChunkSpawner::Spawn(*this, CleaningChunkKind, Rects, Self.FloorZ, CleaningChunks);
}

void ABathhouseSpaceActor::DestroyCleaningChunks()
{
	for (const TWeakObjectPtr<AActor>& Chunk : CleaningChunks)
	{
		if (AActor* Actor = Chunk.Get())
		{
			Actor->Destroy();
		}
	}
	CleaningChunks.Reset();
}

void ABathhouseSpaceActor::LogValidationProblems() const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	TArray<FBathhouseSpaceSnapshot> Snapshots;
	TArray<FBathhouseLayoutProblem> Problems;
	FBathhouseSpaceValidation::ValidateWorld(*World, Snapshots, Problems);
	const int32 Self = Snapshots.IndexOfByPredicate(
		[this](const FBathhouseSpaceSnapshot& Item) { return Item.Actor.Get() == this; });
	for (const FBathhouseLayoutProblem& Problem : Problems)
	{
		if (Self != INDEX_NONE && Problem.OwnerIndex == Self)
		{
			if (Problem.Severity == EBathhouseProblemSeverity::Error)
			{
				UE_LOG(LogBathhouseBuilding, Error, TEXT("%s"), *Problem.Message.ToString());
			}
			else
			{
				UE_LOG(LogBathhouseBuilding, Warning, TEXT("%s"), *Problem.Message.ToString());
			}
		}
	}
}

#if WITH_EDITOR
EDataValidationResult ABathhouseSpaceActor::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = Super::IsDataValid(Context);
	UWorld* World = GetWorld();
	if (!World || HasAnyFlags(RF_ClassDefaultObject))
	{
		return Result;
	}
	TArray<FBathhouseSpaceSnapshot> Snapshots;
	TArray<FBathhouseLayoutProblem> Problems;
	FBathhouseSpaceValidation::ValidateWorld(*World, Snapshots, Problems);
	const int32 Self = Snapshots.IndexOfByPredicate(
		[this](const FBathhouseSpaceSnapshot& Item) { return Item.Actor.Get() == this; });
	if (Self == INDEX_NONE)
	{
		return Result;
	}
	for (const FBathhouseLayoutProblem& Problem : Problems)
	{
		if (!Problem.Involves(Self))
		{
			continue;
		}
		if (Problem.Severity == EBathhouseProblemSeverity::Error)
		{
			Context.AddError(Problem.Message);
			Result = EDataValidationResult::Invalid;
		}
		else
		{
			Context.AddWarning(Problem.Message);
		}
	}
	return Result == EDataValidationResult::NotValidated ? EDataValidationResult::Valid : Result;
}
#endif
