#include "Building/BathhouseSpaceShellComponent.h"

#include "Building/BathhouseSpaceActor.h"
#include "Building/BathhouseSpaceLayout.h"
#include "Components/BoxComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/Actor.h"
#include "Materials/MaterialInterface.h"
#include "Placement/FacilityPlacementTypes.h"

namespace
{
	void ConfigurePartCollision(UInstancedStaticMeshComponent& Component, const EBathhouseShellPart Part)
	{
		Component.SetGenerateOverlapEvents(false);
		switch (Part)
		{
		case EBathhouseShellPart::StairStep:
			Component.SetCollisionEnabled(ECollisionEnabled::NoCollision);
			Component.SetCanEverAffectNavigation(false);
			break;
		case EBathhouseShellPart::StairKeepClear:
			Component.SetVisibility(false);
			Component.SetCollisionEnabled(ECollisionEnabled::QueryOnly);
			Component.SetCollisionObjectType(ECC_WorldStatic);
			Component.SetCollisionResponseToAllChannels(ECR_Ignore);
			Component.SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
			Component.SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
			Component.SetCollisionResponseToChannel(ECC_PhysicsBody, ECR_Block);
			Component.SetCanEverAffectNavigation(false);
			Component.SetCastShadow(false);
			break;
		default:
			Component.SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
			Component.SetCollisionObjectType(ECC_WorldStatic);
			Component.SetCollisionResponseToAllChannels(ECR_Block);
			Component.SetCollisionResponseToChannel(BathhousePlacementCollision::ZoneTraceChannel, ECR_Block);
			Component.SetCanEverAffectNavigation(
				Part == EBathhouseShellPart::Floor || Part == EBathhouseShellPart::Wall
				|| Part == EBathhouseShellPart::StairWall);
			if (Part == EBathhouseShellPart::StairRamp)
			{
				Component.SetVisibility(false);
				Component.SetCastShadow(false);
			}
			break;
		}
	}

	UMaterialInterface* MaterialForPart(const FBathhouseShellVisualInputs& Inputs, const EBathhouseShellPart Part)
	{
		switch (Part)
		{
		case EBathhouseShellPart::Floor: return Inputs.FloorMaterial;
		case EBathhouseShellPart::Wall: return Inputs.WallMaterial;
		case EBathhouseShellPart::Ceiling: return Inputs.CeilingMaterial;
		case EBathhouseShellPart::StairStep: return Inputs.StepMaterial;
		case EBathhouseShellPart::StairWall: return Inputs.StairWallMaterial;
		default: return nullptr;
		}
	}
}

UBathhouseSpaceShellComponent::UBathhouseSpaceShellComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetMobility(EComponentMobility::Static);
	SetCanEverAffectNavigation(false);
	PartComponents.SetNum(static_cast<int32>(EBathhouseShellPart::Count));
}

void UBathhouseSpaceShellComponent::OnComponentDestroyed(const bool bDestroyingHierarchy)
{
	ClearGenerated();
	Super::OnComponentDestroyed(bDestroyingHierarchy);
}

void UBathhouseSpaceShellComponent::ClearGenerated()
{
	for (UInstancedStaticMeshComponent* Component : PartComponents)
	{
		if (IsValid(Component))
		{
			Component->DestroyComponent();
		}
	}
	for (UActorComponent* Component : LightComponents)
	{
		if (IsValid(Component))
		{
			Component->DestroyComponent();
		}
	}
	for (UActorComponent* Component : ChunkPreviewComponents)
	{
		if (IsValid(Component))
		{
			Component->DestroyComponent();
		}
	}
	PartComponents.Reset();
	PartComponents.SetNum(static_cast<int32>(EBathhouseShellPart::Count));
	LightComponents.Reset();
	ChunkPreviewComponents.Reset();
}

int32 UBathhouseSpaceShellComponent::GetGeneratedComponentCount() const
{
	int32 Count = 0;
	for (const UInstancedStaticMeshComponent* Component : PartComponents)
	{
		Count += IsValid(Component) ? 1 : 0;
	}
	for (const UActorComponent* Component : LightComponents)
	{
		Count += IsValid(Component) ? 1 : 0;
	}
	for (const UActorComponent* Component : ChunkPreviewComponents)
	{
		Count += IsValid(Component) ? 1 : 0;
	}
	return Count;
}

UInstancedStaticMeshComponent* UBathhouseSpaceShellComponent::FindPartComponent(const EBathhouseShellPart Part) const
{
	const int32 Index = static_cast<int32>(Part);
	return PartComponents.IsValidIndex(Index) && IsValid(PartComponents[Index]) ? PartComponents[Index].Get() : nullptr;
}

int32 UBathhouseSpaceShellComponent::GetPartInstanceCount(const EBathhouseShellPart Part) const
{
	const UInstancedStaticMeshComponent* Component = FindPartComponent(Part);
	return Component ? Component->GetInstanceCount() : 0;
}

void UBathhouseSpaceShellComponent::Rebuild(const FBathhouseSpacePlan& Plan, const FBathhouseShellVisualInputs& Inputs)
{
	ClearGenerated();
	AActor* Owner = GetOwner();
	if (!Owner || !Inputs.BoxMesh)
	{
		return;
	}
	const FBoxSphereBounds MeshBounds = Inputs.BoxMesh->GetBounds();
	const FVector MeshExtent = MeshBounds.BoxExtent;
	if (!(MeshExtent.X > UE_KINDA_SMALL_NUMBER) || !(MeshExtent.Y > UE_KINDA_SMALL_NUMBER)
		|| !(MeshExtent.Z > UE_KINDA_SMALL_NUMBER))
	{
		UE_LOG(LogBathhouseBuilding, Error, TEXT("%s: shell box mesh has no bounds."), *Owner->GetPathName());
		return;
	}
	const FVector ShellLocation = GetComponentLocation();

	for (int32 PartIndex = 0; PartIndex < static_cast<int32>(EBathhouseShellPart::Count); ++PartIndex)
	{
		const EBathhouseShellPart Part = static_cast<EBathhouseShellPart>(PartIndex);
		const TArray<FBathhouseBoxPart>& Boxes = Plan.Get(Part);
		if (Boxes.IsEmpty())
		{
			continue;
		}
		UInstancedStaticMeshComponent* Component = NewObject<UInstancedStaticMeshComponent>(
			Owner, NAME_None, RF_Transient);
		Component->CreationMethod = EComponentCreationMethod::UserConstructionScript;
		Component->SetMobility(EComponentMobility::Static);
		Component->SetStaticMesh(Inputs.BoxMesh);
		if (UMaterialInterface* Material = MaterialForPart(Inputs, Part))
		{
			for (int32 Slot = 0; Slot < Component->GetNumMaterials(); ++Slot)
			{
				Component->SetMaterial(Slot, Material);
			}
		}
		Component->SetupAttachment(this);
		ConfigurePartCollision(*Component, Part);
		Component->RegisterComponent();

		TArray<FTransform> Transforms;
		Transforms.Reserve(Boxes.Num());
		for (const FBathhouseBoxPart& Box : Boxes)
		{
			// mesh local bounds에서 scale·offset을 파생한다(원점 중심 100cm 상자라고 가정하지 않는다).
			const FVector Scale = Box.HalfExtent / MeshExtent;
			const FVector Translation = (Box.Center - ShellLocation)
				- Box.Rotation.RotateVector(Scale * MeshBounds.Origin);
			Transforms.Add(FTransform(Box.Rotation, Translation, Scale));
		}
		Component->AddInstances(Transforms, false, false);
		PartComponents[PartIndex] = Component;
	}

	for (const FVector& Location : Plan.LightLocations)
	{
		UPointLightComponent* Light = NewObject<UPointLightComponent>(Owner, NAME_None, RF_Transient);
		Light->CreationMethod = EComponentCreationMethod::UserConstructionScript;
		Light->SetMobility(EComponentMobility::Movable);
		Light->SetIntensityUnits(ELightUnits::Candelas);
		Light->SetIntensity(Inputs.Lighting.IntensityCandela);
		Light->SetAttenuationRadius(Inputs.Lighting.AttenuationRadiusCm);
		Light->SetLightColor(Inputs.Lighting.Color);
		Light->SetCastShadows(Inputs.Lighting.bCastShadows);
		Light->SetupAttachment(this);
		Light->RegisterComponent();
		Light->SetWorldLocation(Location);
		LightComponents.Add(Light);
	}

	if (Inputs.bPreviewChunks && Inputs.ChunkKind != EBathhouseCleaningChunkKind::None)
	{
		const FColor Color = Inputs.ChunkKind == EBathhouseCleaningChunkKind::Litter
			? FColor(255, 160, 0) : FColor(0, 160, 255);
		for (const FBox2D& Rect : Plan.ChunkRects)
		{
			UBoxComponent* Preview = NewObject<UBoxComponent>(Owner, NAME_None, RF_Transient);
			Preview->CreationMethod = EComponentCreationMethod::UserConstructionScript;
			Preview->bIsEditorOnly = true;
			Preview->SetMobility(EComponentMobility::Movable);
			Preview->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			Preview->SetCanEverAffectNavigation(false);
			Preview->SetHiddenInGame(true);
			Preview->ShapeColor = Color;
			Preview->SetupAttachment(this);
			Preview->SetBoxExtent(FVector(
				(Rect.Max.X - Rect.Min.X) * 0.5, (Rect.Max.Y - Rect.Min.Y) * 0.5, Inputs.ChunkPreviewHalfHeightCm));
			Preview->RegisterComponent();
			const FVector2D Center = Rect.GetCenter();
			const double FloorZ = ShellLocation.Z;
			Preview->SetWorldLocation(FVector(Center.X, Center.Y, FloorZ + Inputs.ChunkPreviewHalfHeightCm));
			ChunkPreviewComponents.Add(Preview);
		}
	}
}
