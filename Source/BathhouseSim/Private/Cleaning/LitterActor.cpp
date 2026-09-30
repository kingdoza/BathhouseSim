#include "Cleaning/LitterActor.h"
#include "Cleaning/LitterSpawnZoneActor.h"
#include "Cleaning/CleaningWorldSubsystem.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "Interaction/PhysicalCarryable.h"
#include "Interaction/PlayerCarryComponent.h"
#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif
#define LOCTEXT_NAMESPACE "LitterActor"

ALitterActor::ALitterActor()
{
	PrimaryActorTick.bCanEverTick = false;
	InteractionCollision = CreateDefaultSubobject<USphereComponent>(TEXT("InteractionCollision"));
	SetRootComponent(InteractionCollision);
	InteractionCollision->InitSphereRadius(12);
	InteractionCollision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	InteractionCollision->SetCollisionResponseToAllChannels(ECR_Ignore);
	InteractionCollision->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	InteractionCollision->SetCanEverAffectNavigation(false);
	LitterMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LitterMesh"));
	LitterMesh->SetupAttachment(InteractionCollision);
	LitterMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	LitterMesh->SetCanEverAffectNavigation(false);
}

void ALitterActor::ConfigureVisualVariationSeed(int32 Seed)
{
	if (!bVisualInitialized)
	{
		VisualSeed = Seed;
		bSeedConfigured = true;
	}
}

void ALitterActor::BeginPlay()
{
	Super::BeginPlay();
	if (!bVisualInitialized)
	{
		bVisualInitialized = true;
		FRandomStream Stream(bSeedConfigured ? VisualSeed : FMath::Rand());
		TArray<UStaticMesh*> Valid;
		for (UStaticMesh* Mesh : MeshVariants)
		{
			if (IsValid(Mesh))
			{
				Valid.Add(Mesh);
			}
		}
		if (!Valid.IsEmpty())
		{
			LitterMesh->SetStaticMesh(Valid[Stream.RandHelper(Valid.Num())]);
		}
		LitterMesh->SetRelativeRotation(FRotator(0, Stream.FRandRange(0, 360), 0));
	}
	if (auto* Subsystem = GetWorld()->GetSubsystem<UCleaningWorldSubsystem>())
	{
		Subsystem->RegisterLitter(this);
	}
}

void ALitterActor::EndPlay(const EEndPlayReason::Type Reason)
{
	if (auto* World = GetWorld())
	{
		if (auto* S = World->GetSubsystem<UCleaningWorldSubsystem>())
		{
			S->UnregisterLitter(this);
		}
	}
	Super::EndPlay(Reason);
}

FPlayerInteractionQuery ALitterActor::QueryInteraction(const FPlayerInteractionContext& Context) const
{
	FPlayerInteractionQuery Query;
	if (bRemoved)
	{
		return Query;
	}
	Query.bVisible = true;
	Query.TargetName = LOCTEXT("LitterName", "쓰레기");
	const IPhysicalCarryable* Held =
		Context.CarryComponent ? Cast<IPhysicalCarryable>(Context.CarryComponent->GetHeldObject()) : nullptr;
	if (!Held || Held->GetPhysicalCarryKind() != EPhysicalCarryKind::Facility)
	{
		Query.bHeldApplyVisible = true;
		Query.HeldApplyFailureReason = LOCTEXT("TongsRequired", "집게가 필요합니다");
	}
	return Query;
}

FPlayerInteractionResult ALitterActor::ExecuteInteraction(const FPlayerInteractionContext& Context)
{
	return FPlayerInteractionResult::Failed(FText::GetEmpty());
}

bool ALitterActor::CommitCollected()
{
	if (bRemoved)
	{
		return false;
	}
	bRemoved = true;
	InteractionCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	if (auto* S = GetWorld()->GetSubsystem<UCleaningWorldSubsystem>())
	{
		S->UnregisterLitter(this);
	}
	Destroy();
	return true;
}

void ALitterActor::ClearForFacilityPlacement()
{
	CommitCollected();
}
#if WITH_EDITOR
EDataValidationResult ALitterActor::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = Super::IsDataValid(Context);
	if (!MeshVariants.ContainsByPredicate(
			[](const UStaticMesh* Mesh)
			{
				return IsValid(Mesh);
			}) ||
		!FMath::IsFinite(FloorRadiusCm) || FloorRadiusCm <= 0)
	{
		Context.AddError(
			LOCTEXT("InvalidLitter", "Litter requires a valid mesh variant and a positive finite floor radius."));
		return EDataValidationResult::Invalid;
	}
	return Result == EDataValidationResult::NotValidated ? EDataValidationResult::Valid : Result;
}
#endif
#undef LOCTEXT_NAMESPACE

void ALitterActor::SetSpawnZone(ALitterSpawnZoneActor* Zone)
{
	SpawnZone = Zone;
}
