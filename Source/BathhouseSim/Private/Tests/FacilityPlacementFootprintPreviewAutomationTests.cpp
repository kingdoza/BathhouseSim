#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "AssetRegistry/AssetRegistryModule.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/DataValidation.h"
#include "Placement/FacilityPlacementComponent.h"
#include "Placement/FacilityPlacementDefinition.h"
#include "Placement/FacilityPlacementFootprintPreview.h"
#include "Placement/FacilityPlacementPreviewActor.h"
#include "Placement/FacilityPlacementSettings.h"
#include "Placement/PlaceableFacility.h"
#include "Placement/PlaceableFacilityItemActor.h"
#include "Tests/FacilityPlacementAutomationTestProbe.h"
#include "Tests/FacilityPlacementFootprintPreviewTestProbe.h"

#if WITH_EDITOR
#include "Materials/MaterialExpressionScalarParameter.h"
#include "Materials/MaterialExpressionVectorParameter.h"
#endif

namespace
{
namespace Footprint = FacilityPlacementFootprintPreview;

// Saves and restores the global placement settings these tests change.
struct FFootprintTestSettingsGuard
{
	UFacilityPlacementSettings* Settings = GetMutableDefault<UFacilityPlacementSettings>();
	const float Grid = Settings->GridSizeCm;
	const TSoftObjectPtr<UMaterialInterface> Valid = Settings->ValidPreviewMaterial;
	const TSoftObjectPtr<UMaterialInterface> Invalid = Settings->InvalidPreviewMaterial;
	const TSoftObjectPtr<UStaticMesh> Mesh = Settings->FootprintPreviewMesh;
	const TSoftObjectPtr<UMaterialInterface> Surface = Settings->FootprintPreviewMaterial;
	~FFootprintTestSettingsGuard()
	{
		Settings->GridSizeCm = Grid;
		Settings->ValidPreviewMaterial = Valid;
		Settings->InvalidPreviewMaterial = Invalid;
		Settings->FootprintPreviewMesh = Mesh;
		Settings->FootprintPreviewMaterial = Surface;
	}
};

// Sets a class-default footprint extent and restores it (CDO state is shared with other tests).
struct FScopedCdoFootprintExtent
{
	UBoxComponent* Box = nullptr;
	FVector Saved = FVector::ZeroVector;
	FScopedCdoFootprintExtent(const TSubclassOf<AActor> Class, const FVector& Extent)
	{
		const IPlaceableFacility* Placeable = Cast<IPlaceableFacility>(Class->GetDefaultObject<AActor>());
		Box = Placeable ? Placeable->GetFacilityPlacementComponent()->GetPlacementFootprint() : nullptr;
		if (Box)
		{
			Saved = Box->GetUnscaledBoxExtent();
			Box->SetBoxExtent(Extent);
		}
	}
	~FScopedCdoFootprintExtent()
	{
		if (Box)
		{
			Box->SetBoxExtent(Saved);
		}
	}
};

struct FFootprintTestWorld
{
	UWorld* World = nullptr;
	FFootprintTestWorld()
	{
		if (!GEngine)
		{
			return;
		}
		const FName Name = MakeUniqueObjectName(nullptr, UWorld::StaticClass(), TEXT("FootprintPreviewAutomationWorld"));
		FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
		World = UWorld::CreateWorld(EWorldType::Game, false, Name, GetTransientPackage());
		if (!World)
		{
			GEngine->DestroyWorldContext(World);
			return;
		}
		World->AddToRoot();
		Context.SetCurrentWorld(World);
		World->InitializeActorsForPlay(FURL());
		World->BeginPlay();
	}
	~FFootprintTestWorld()
	{
		if (World)
		{
			World->DestroyWorld(false);
			GEngine->DestroyWorldContext(World);
			World->RemoveFromRoot();
		}
	}
};

UFacilityPlacementDefinition* MakeDefinition(const TSubclassOf<AActor> PlacedClass)
{
	UFacilityPlacementDefinition* Definition = NewObject<UFacilityPlacementDefinition>();
	Definition->StableId = TEXT("FootprintPreviewAutomation");
	Definition->FacilityTags.AddTag(FGameplayTag::RequestGameplayTag(TEXT("Facility.Placeable")));
	Definition->PlacedFacilityClass = PlacedClass;
	Definition->RecoveryItemClass = APlaceableFacilityItemActor::StaticClass();
	return Definition;
}

const UFacilityPlacementComponent* GetPlacement(const TSubclassOf<AActor> Class)
{
	const IPlaceableFacility* Placeable = Cast<IPlaceableFacility>(Class->GetDefaultObject<AActor>());
	return Placeable ? Placeable->GetFacilityPlacementComponent() : nullptr;
}

FBoxSphereBounds PlaneBounds(const FVector& Origin, const FVector& Extent)
{
	return FBoxSphereBounds(Origin, Extent, Extent.Size());
}

#if WITH_EDITOR
// Transient translucent material exposing the named vector/scalar parameters to cached parameter lookup.
UMaterial* MakeParameterMaterial(
	const TArray<TPair<FName, FLinearColor>>& Vectors,
	const TArray<FName>& Scalars)
{
	UMaterial* Material = NewObject<UMaterial>(GetTransientPackage());
	Material->BlendMode = BLEND_Translucent;
	for (const TPair<FName, FLinearColor>& Vector : Vectors)
	{
		UMaterialExpressionVectorParameter* Expression = NewObject<UMaterialExpressionVectorParameter>(Material);
		Expression->ParameterName = Vector.Key;
		Expression->DefaultValue = Vector.Value;
		Expression->Material = Material;
		Material->GetExpressionCollection().AddExpression(Expression);
	}
	for (const FName& Scalar : Scalars)
	{
		UMaterialExpressionScalarParameter* Expression = NewObject<UMaterialExpressionScalarParameter>(Material);
		Expression->ParameterName = Scalar;
		Expression->DefaultValue = 1.0f;
		Expression->Material = Material;
		Material->GetExpressionCollection().AddExpression(Expression);
	}
	Material->UpdateCachedExpressionData();
	return Material;
}
#endif
}

// FPV-001, 003, 004: surface math equals the judgment footprint floor face (RelativeFootprint * Candidate) lifted by the height.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FFacilityPlacementFootprintSurfaceMathTest,
	"BathhouseSim.Placement.FootprintPreview.SurfaceMath",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFacilityPlacementFootprintSurfaceMathTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	const FVector PlaneExtent(50.0f, 50.0f, 0.0f);
	const FVector FootprintExtent(30.0f, 45.0f, 40.0f);
	const FVector RootScale(1.5f, 1.5f, 1.5f);
	const float Height = GetDefault<UFacilityPlacementSettings>()->GetFootprintPreviewFloorOffsetCm();
	const FTransform FootprintRelative(
		FRotator(0.0f, -90.0f, 0.0f), FVector(12.0f, -8.0f, 40.0f), FVector(1.0f, 2.0f, 1.0f));

	Footprint::FSurfaceLayout Layout;
	FText Failure;
	if (!TestTrue(TEXT("Valid fixture computes a surface"),
		Footprint::ComputeSurface(FootprintRelative, FootprintExtent, RootScale,
			PlaneBounds(FVector::ZeroVector, PlaneExtent), Height, Layout, Failure)))
	{
		return false;
	}
	const FTransform Candidate(FRotator(0.0f, 33.0f, 0.0f), FVector(100.0f, -50.0f, 25.0f), RootScale);
	const FTransform JudgmentFootprint = FootprintRelative * Candidate;
	const FTransform SurfaceWorld = Layout.RelativeToRoot * Candidate;
	const FVector FloorNormal = Candidate.GetUnitAxis(EAxis::Z);
	for (const FVector2D Corner : {
		FVector2D(-1.0f, -1.0f), FVector2D(-1.0f, 1.0f), FVector2D(1.0f, -1.0f), FVector2D(1.0f, 1.0f) })
	{
		const FVector Expected = JudgmentFootprint.TransformPosition(
			FVector(Corner.X * FootprintExtent.X, Corner.Y * FootprintExtent.Y, -FootprintExtent.Z))
			+ FloorNormal * Height;
		const FVector Actual = SurfaceWorld.TransformPosition(
			FVector(Corner.X * PlaneExtent.X, Corner.Y * PlaneExtent.Y, 0.0f));
		TestTrue(FString::Printf(TEXT("FPV-001 plane corner (%g,%g) matches the judgment floor corner; expected=%s actual=%s"),
			Corner.X, Corner.Y, *Expected.ToString(), *Actual.ToString()),
			Expected.Equals(Actual, 0.01f));
	}
	const FVector FullSize = UFacilityPlacementComponent::ComputeScaledFootprintFullSize(
		FootprintRelative, FootprintExtent, RootScale);
	TestTrue(TEXT("World size output equals the shared static full size"),
		FMath::IsNearlyEqual(Layout.WorldSizeCm.X, FullSize.X, 0.01f)
		&& FMath::IsNearlyEqual(Layout.WorldSizeCm.Y, FullSize.Y, 0.01f));

	Footprint::FSurfaceLayout Ignored;
	TestFalse(TEXT("Off-center plane bounds are rejected"),
		Footprint::ComputeSurface(FootprintRelative, FootprintExtent, RootScale,
			PlaneBounds(FVector(5.0f, 0.0f, 0.0f), PlaneExtent), Height, Ignored, Failure));
	TestFalse(TEXT("Thick plane bounds are rejected"),
		Footprint::ComputeSurface(FootprintRelative, FootprintExtent, RootScale,
			PlaneBounds(FVector::ZeroVector, FVector(50.0f, 50.0f, 5.0f)), Height, Ignored, Failure));
	TestFalse(TEXT("Zero-size plane bounds are rejected"),
		Footprint::ComputeSurface(FootprintRelative, FootprintExtent, RootScale,
			PlaneBounds(FVector::ZeroVector, FVector(0.0f, 50.0f, 0.0f)), Height, Ignored, Failure));
	TestFalse(TEXT("Zero composed scale is rejected"),
		Footprint::ComputeSurface(FootprintRelative, FootprintExtent, FVector(1.0f, 0.0f, 1.0f),
			PlaneBounds(FVector::ZeroVector, PlaneExtent), Height, Ignored, Failure));
	TestFalse(TEXT("Negative composed scale is rejected"),
		Footprint::ComputeSurface(FootprintRelative, FootprintExtent, FVector(-1.0f, 1.0f, 1.0f),
			PlaneBounds(FVector::ZeroVector, PlaneExtent), Height, Ignored, Failure));
	return true;
}

// FPV-016, design 5.2: CDO cell derivation includes the root relative scale and sub-component scale.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FFacilityPlacementFootprintCellScaleTest,
	"BathhouseSim.Placement.FootprintPreview.CellsIncludeRootScale",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFacilityPlacementFootprintCellScaleTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FFootprintTestSettingsGuard Guard;
	const float Grid = Guard.Settings->GetGridSizeCm();
	const TSubclassOf<AActor> Class = AFacilityPlacementFootprintScaleProbe::StaticClass();
	const UFacilityPlacementComponent* Placement = GetPlacement(Class);
	if (!TestNotNull(TEXT("Probe exposes a placement component"), Placement))
	{
		return false;
	}
	const AActor* CDO = Class->GetDefaultObject<AActor>();
	const FVector RootScale = CDO->GetRootComponent()->GetRelativeScale3D();
	// The probe SceneRoot carries the sub-component scale; read it back from the hierarchy instead of copying a literal.
	const FVector SubScale = Placement->GetPlacementFootprint()->GetAttachParent()->GetRelativeScale3D();
	const FVector ExpectedScale = RootScale * SubScale;
	TestFalse(TEXT("Fixture is meaningful: composed scale differs from one"), ExpectedScale.Equals(FVector::OneVector));

	{
		// Integer cells only after both scales apply: 2 and 3 cells.
		const FVector Extent(Grid * 0.5f / ExpectedScale.X * 2.0f, Grid * 0.5f / ExpectedScale.Y * 3.0f, 50.0f);
		FScopedCdoFootprintExtent Scope(Class, Extent);
		FIntPoint Cells;
		FText Failure;
		TestTrue(FString::Printf(TEXT("FPV-016 integer after scale derives cells: %s"), *Failure.ToString()),
			Placement->DeriveFootprintCells(Cells, Failure));
		const FIntPoint Expected(
			FMath::RoundToInt(2.0f * Extent.X * ExpectedScale.X / Grid),
			FMath::RoundToInt(2.0f * Extent.Y * ExpectedScale.Y / Grid));
		TestEqual(TEXT("Cells use root and sub-component scale (X)"), Cells.X, Expected.X);
		TestEqual(TEXT("Cells use root and sub-component scale (Y)"), Cells.Y, Expected.Y);
		TestTrue(TEXT("Cells reflect the scale: differ from the unscaled extent ratio"),
			Cells.X != FMath::RoundToInt(2.0f * Extent.X / Grid)
			|| Cells.Y != FMath::RoundToInt(2.0f * Extent.Y / Grid));
	}
	{
		// Integer unscaled, non-integer after scale (cooler shape).
		const FVector Extent(Grid * 0.75f / ExpectedScale.X, Grid * 0.5f / ExpectedScale.Y * 2.0f, 50.0f);
		FScopedCdoFootprintExtent Scope(Class, Extent);
		FIntPoint Cells;
		FText Failure;
		TestFalse(TEXT("FPV-016 non-integer cells after scale fail DeriveFootprintCells"),
			Placement->DeriveFootprintCells(Cells, Failure));
		UFacilityPlacementDefinition* Definition = MakeDefinition(Class);
		FDataValidationContext Context;
		TestEqual(TEXT("FPV-016 non-integer footprint is a Data Validation error"),
			Definition->IsDataValid(Context), EDataValidationResult::Invalid);
		TestFalse(TEXT("Definition runtime validation also fails"), Definition->ValidateRuntime(Failure));
	}
	return true;
}

// FPV-016, design 5.2: Data Validation rejects footprints turned off the grid axes; runtime stays unchanged.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FFacilityPlacementFootprintAxisAlignmentTest,
	"BathhouseSim.Placement.FootprintPreview.AxisAlignment",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFacilityPlacementFootprintAxisAlignmentTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FFootprintTestSettingsGuard Guard;
	const float Grid = Guard.Settings->GetGridSizeCm();
	const FVector SquareExtent(Grid * 0.5f, Grid * 0.5f, 50.0f);
	const TSubclassOf<AActor> Aligned[] = {
		AFacilityPlacementAutomationActor::StaticClass(),
		AFacilityPlacementFootprintYawPlus90Probe::StaticClass(),
		AFacilityPlacementFootprintYawMinus90Probe::StaticClass() };
	for (const TSubclassOf<AActor> Class : Aligned)
	{
		FScopedCdoFootprintExtent Scope(Class, SquareExtent);
		FText Failure;
		TestTrue(*FString::Printf(TEXT("Axis aligned footprint passes: %s"), *Class->GetName()),
			GetPlacement(Class)->ValidateFootprintGridAxisAlignment(Failure));
	}

	const TSubclassOf<AActor> Small = AFacilityPlacementFootprintYawSmallProbe::StaticClass();
	FScopedCdoFootprintExtent Scope(Small, SquareExtent);
	const UFacilityPlacementComponent* Placement = GetPlacement(Small);
	FText AxisFailure;
	TestFalse(TEXT("Small yaw footprint fails the axis check"),
		Placement->ValidateFootprintGridAxisAlignment(AxisFailure));
	UFacilityPlacementDefinition* Definition = MakeDefinition(Small);
	FDataValidationContext Context;
	TestEqual(TEXT("Small yaw footprint is a Data Validation error"),
		Definition->IsDataValid(Context), EDataValidationResult::Invalid);
	bool bHasAxisIssue = false;
	for (const FDataValidationContext::FIssue& Issue : Context.GetIssues())
	{
		bHasAxisIssue |= Issue.Message.EqualTo(AxisFailure);
	}
	TestTrue(TEXT("Data Validation reports the axis alignment message"), bHasAxisIssue);
	FText RuntimeFailure;
	TestTrue(TEXT("Runtime validation is unchanged when cells are integer"),
		Definition->ValidateRuntime(RuntimeFailure));
	return true;
}

// FPV-015: display preparation failure keeps the mesh preview and has no footprint component.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FFacilityPlacementFootprintFallbackTest,
	"BathhouseSim.Placement.FootprintPreview.PreparationFailureFallback",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFacilityPlacementFootprintFallbackTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FFootprintTestSettingsGuard Guard;
	UMaterial* ValidMaterial = NewObject<UMaterial>();
	UMaterial* InvalidMaterial = NewObject<UMaterial>();
	ValidMaterial->BlendMode = BLEND_Translucent;
	InvalidMaterial->BlendMode = BLEND_Translucent;
	Guard.Settings->GridSizeCm = 10.0f;
	Guard.Settings->ValidPreviewMaterial = ValidMaterial;
	Guard.Settings->InvalidPreviewMaterial = InvalidMaterial;
	Guard.Settings->FootprintPreviewMaterial = nullptr;
	FFootprintTestWorld Fixture;
	if (!TestNotNull(TEXT("Fixture world"), Fixture.World))
	{
		return false;
	}
	AddExpectedMessage(TEXT("Facility placement footprint display unavailable"),
		ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, 1);
	AFacilityPlacementPreviewActor* Preview = Fixture.World->SpawnActor<AFacilityPlacementPreviewActor>();
	FText Failure;
	TestTrue(FString::Printf(TEXT("Initialization still succeeds: %s"), *Failure.ToString()),
		Preview->InitializeFromPlacedClass(AFacilityPlacementAutomationActor::StaticClass(), Failure));
	TestNull(TEXT("No footprint component without a footprint material"), Preview->GetFootprintSurface());
	TestTrue(TEXT("Mesh preview exists"), Preview->GetPreviewMeshes().Num() > 0);
	for (const UStaticMeshComponent* Mesh : Preview->GetPreviewMeshes())
	{
		TestEqual(TEXT("Mesh preview sort priority comes from settings"),
			static_cast<int32>(Mesh->TranslucencySortPriority),
			Guard.Settings->GetPreviewMeshTranslucencySortPriority());
	}
	Preview->SetPlacementValidity(true, FText::GetEmpty());
	Preview->SetPlacementValidity(false, FText::GetEmpty());
	TestTrue(TEXT("Validity changes still apply to the mesh preview"),
		Preview->GetPreviewMeshes()[0]->GetMaterial(0) == InvalidMaterial);
	return true;
}

#if WITH_EDITOR
// FPV-001, 002, 003, 004, 007: footprint component follows the Actor and tracks the mesh material colors.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FFacilityPlacementFootprintSurfaceActorTest,
	"BathhouseSim.Placement.FootprintPreview.SurfaceActor",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFacilityPlacementFootprintSurfaceActorTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FFootprintTestSettingsGuard Guard;
	const FLinearColor ValidColor(0.1f, 0.7f, 0.2f, 1.0f);
	const FLinearColor InvalidColor(0.9f, 0.1f, 0.1f, 1.0f);
	UMaterial* ValidMaterial = MakeParameterMaterial({ { Footprint::PreviewMeshColorParameter, ValidColor } }, {});
	UMaterial* InvalidMaterial = MakeParameterMaterial({ { Footprint::PreviewMeshColorParameter, InvalidColor } }, {});
	UMaterial* SurfaceMaterial = MakeParameterMaterial(
		{ { Footprint::SurfaceColorParameter, FLinearColor::Black } },
		{ Footprint::SurfaceSizeXParameter, Footprint::SurfaceSizeYParameter });
	UStaticMesh* PlaneMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Plane.Plane"));
	if (!TestNotNull(TEXT("Engine plane mesh"), PlaneMesh))
	{
		return false;
	}
	FText Probe;
	FLinearColor Read;
	if (!Footprint::ValidateSurfaceMaterial(SurfaceMaterial, Probe)
		|| !Footprint::ReadPreviewMeshColor(ValidMaterial, Read, Probe))
	{
		AddError(FString::Printf(TEXT("Transient parameter lookup unavailable (%s); the content test covers this."), *Probe.ToString()));
		return true;
	}
	Guard.Settings->GridSizeCm = 10.0f;
	Guard.Settings->ValidPreviewMaterial = ValidMaterial;
	Guard.Settings->InvalidPreviewMaterial = InvalidMaterial;
	Guard.Settings->FootprintPreviewMesh = PlaneMesh;
	Guard.Settings->FootprintPreviewMaterial = SurfaceMaterial;
	FFootprintTestWorld Fixture;
	if (!TestNotNull(TEXT("Fixture world"), Fixture.World))
	{
		return false;
	}
	AFacilityPlacementPreviewActor* Preview = Fixture.World->SpawnActor<AFacilityPlacementPreviewActor>();
	const TSubclassOf<AActor> Class = AFacilityPlacementAutomationActor::StaticClass();
	FText Failure;
	TestTrue(FString::Printf(TEXT("Initialization succeeds: %s"), *Failure.ToString()),
		Preview->InitializeFromPlacedClass(Class, Failure));
	const UStaticMeshComponent* Surface = Preview->GetFootprintSurface();
	if (!TestNotNull(TEXT("Footprint component exists when preparation succeeds"), Surface))
	{
		return false;
	}
	TestEqual(TEXT("Footprint sort priority comes from settings"),
		static_cast<int32>(Surface->TranslucencySortPriority),
		Guard.Settings->GetFootprintPreviewTranslucencySortPriority());
	TestTrue(TEXT("Footprint draws below the mesh preview (draw order contract)"),
		Surface->TranslucencySortPriority < Preview->GetPreviewMeshes()[0]->TranslucencySortPriority);
	TestTrue(TEXT("Footprint has no collision"), Surface->GetCollisionEnabled() == ECollisionEnabled::NoCollision);
	TestFalse(TEXT("Footprint casts no shadow"), Surface->CastShadow);
	TestFalse(TEXT("Footprint component is not in the mesh list"),
		Preview->GetPreviewMeshes().Contains(Surface));

	// Corners follow the candidate transform exactly like the judgment footprint.
	const UBoxComponent* Box = GetPlacement(Class)->GetPlacementFootprint();
	const AActor* CDO = Class->GetDefaultObject<AActor>();
	FTransform FootprintRelative;
	GetPlacement(Class)->GetFootprintRelativeToRoot(FootprintRelative, Failure);
	const FTransform Candidate(
		FRotator(0.0f, 45.0f, 0.0f), FVector(300.0f, 120.0f, 25.0f), CDO->GetRootComponent()->GetRelativeScale3D());
	Preview->SetActorTransform(Candidate);
	const FVector Extent = Box->GetUnscaledBoxExtent();
	const float Height = Guard.Settings->GetFootprintPreviewFloorOffsetCm();
	const FBoxSphereBounds PlaneBoundsLocal = PlaneMesh->GetBounds();
	for (const FVector2D Corner : { FVector2D(-1.0f, -1.0f), FVector2D(1.0f, 1.0f) })
	{
		const FVector Expected = (FootprintRelative * Candidate).TransformPosition(
			FVector(Corner.X * Extent.X, Corner.Y * Extent.Y, -Extent.Z))
			+ Candidate.GetUnitAxis(EAxis::Z) * Height;
		const FVector Actual = Surface->GetComponentTransform().TransformPosition(
			FVector(Corner.X * PlaneBoundsLocal.BoxExtent.X, Corner.Y * PlaneBoundsLocal.BoxExtent.Y, 0.0f));
		TestTrue(FString::Printf(TEXT("FPV-003 surface corner follows the candidate; expected=%s actual=%s"),
			*Expected.ToString(), *Actual.ToString()), Expected.Equals(Actual, 0.05f));
	}

	FLinearColor Color;
	const UMaterialInstanceDynamic* Dynamic = Preview->GetFootprintMaterial();
	if (TestNotNull(TEXT("Footprint DMI"), Dynamic))
	{
		Dynamic->GetVectorParameterValue(FHashedMaterialParameterInfo(Footprint::SurfaceColorParameter), Color);
		TestTrue(TEXT("Initial footprint color is the invalid preview color"), Color.Equals(InvalidColor, 0.001f));
		Preview->SetPlacementValidity(true, FText::GetEmpty());
		Dynamic->GetVectorParameterValue(FHashedMaterialParameterInfo(Footprint::SurfaceColorParameter), Color);
		TestTrue(TEXT("FPV-004 valid footprint color equals the valid material Param"), Color.Equals(ValidColor, 0.001f));
		Preview->SetPlacementValidity(false, FText::GetEmpty());
		Dynamic->GetVectorParameterValue(FHashedMaterialParameterInfo(Footprint::SurfaceColorParameter), Color);
		TestTrue(TEXT("FPV-004 invalid footprint color equals the invalid material Param"), Color.Equals(InvalidColor, 0.001f));
	}

	TestTrue(TEXT("Footprint renders while the preview is shown"), Surface->ShouldRender());
	Preview->SetActorHiddenInGame(true);
	TestFalse(TEXT("FPV-007 hiding the preview hides the footprint with it"), Surface->ShouldRender());
	return true;
}
#endif

// FPV-011, 012, 016: every placement Definition gets a footprint whose size equals cells * grid and whose color is the preview material Param.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FFacilityPlacementFootprintContentTest,
	"BathhouseSim.PlacementContent.FootprintPreview",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFacilityPlacementFootprintContentTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FAssetRegistryModule& Registry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	TArray<FAssetData> Assets;
	Registry.Get().GetAssetsByClass(UFacilityPlacementDefinition::StaticClass()->GetClassPathName(), Assets, true);
	TestTrue(TEXT("Placement Definitions are discovered"), Assets.Num() > 0);

	const UFacilityPlacementSettings* Settings = GetDefault<UFacilityPlacementSettings>();
	const float Grid = Settings->GetGridSizeCm();
	UMaterialInterface* ValidMaterial = Settings->LoadValidPreviewMaterial();
	UMaterialInterface* InvalidMaterial = Settings->LoadInvalidPreviewMaterial();
	FLinearColor ValidColor, InvalidColor;
	FText Failure;
	if (!TestTrue(TEXT("Preview materials expose Param"),
		Footprint::ReadPreviewMeshColor(ValidMaterial, ValidColor, Failure)
		&& Footprint::ReadPreviewMeshColor(InvalidMaterial, InvalidColor, Failure)))
	{
		return false;
	}
	UStaticMesh* PlaneMesh = Settings->LoadFootprintPreviewMesh();
	if (!TestNotNull(TEXT("Project Settings FootprintPreviewMesh loads"), PlaneMesh)
		|| !TestTrue(TEXT("Project Settings FootprintPreviewMaterial satisfies the contract"),
			Footprint::ValidateSurfaceMaterial(Settings->LoadFootprintPreviewMaterial(), Failure)))
	{
		return false;
	}
	FFootprintTestWorld Fixture;
	if (!TestNotNull(TEXT("Fixture world"), Fixture.World))
	{
		return false;
	}
	const FVector PlaneSize = PlaneMesh->GetBounds().BoxExtent * 2.0f;
	for (const FAssetData& Asset : Assets)
	{
		const UFacilityPlacementDefinition* Definition = Cast<UFacilityPlacementDefinition>(Asset.GetAsset());
		if (!Definition || !Definition->PlacedFacilityClass)
		{
			continue;
		}
		const FString Name = Asset.AssetName.ToString();
		FIntPoint Cells;
		if (!TestTrue(*FString::Printf(TEXT("%s derives footprint cells"), *Name),
			Definition->DeriveFootprintCells(Cells, Failure)))
		{
			continue;
		}
		AFacilityPlacementPreviewActor* Preview = Fixture.World->SpawnActor<AFacilityPlacementPreviewActor>();
		if (!TestTrue(*FString::Printf(TEXT("%s preview initializes"), *Name),
			Preview->InitializeFromPlacedClass(Definition->PlacedFacilityClass, Failure)))
		{
			Preview->Destroy();
			continue;
		}
		const UStaticMeshComponent* Surface = Preview->GetFootprintSurface();
		if (TestNotNull(*FString::Printf(TEXT("%s has a footprint display"), *Name), Surface))
		{
			// Surface local scale is footprint-axis scale; the Actor carries the root relative scale.
			const AActor* CDO = Definition->PlacedFacilityClass->GetDefaultObject<AActor>();
			const FVector WorldScale = Surface->GetRelativeTransform().GetScale3D()
				* CDO->GetRootComponent()->GetRelativeScale3D();
			TestTrue(*FString::Printf(TEXT("%s surface size equals cells * grid (%d x %d)"), *Name, Cells.X, Cells.Y),
				FMath::IsNearlyEqual(WorldScale.X * PlaneSize.X, Cells.X * Grid, 0.1f)
				&& FMath::IsNearlyEqual(WorldScale.Y * PlaneSize.Y, Cells.Y * Grid, 0.1f));
			FLinearColor Color;
			Preview->SetPlacementValidity(true, FText::GetEmpty());
			Preview->GetFootprintMaterial()->GetVectorParameterValue(
				FHashedMaterialParameterInfo(Footprint::SurfaceColorParameter), Color);
			TestTrue(*FString::Printf(TEXT("%s valid color equals the preview material"), *Name), Color.Equals(ValidColor, 0.001f));
			Preview->SetPlacementValidity(false, FText::GetEmpty());
			Preview->GetFootprintMaterial()->GetVectorParameterValue(
				FHashedMaterialParameterInfo(Footprint::SurfaceColorParameter), Color);
			TestTrue(*FString::Printf(TEXT("%s invalid color equals the preview material"), *Name), Color.Equals(InvalidColor, 0.001f));
		}
		Preview->Destroy();
	}
	return true;
}

#endif
