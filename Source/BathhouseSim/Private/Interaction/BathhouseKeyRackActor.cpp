#include "Interaction/BathhouseKeyRackActor.h"

#include "Components/SceneComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Facility/BathhouseExpansionAuthority.h"
#include "Facility/BathhouseExpansionDefinition.h"
#include "Facility/BathhouseFacilitySubsystem.h"
#include "Interaction/BathhouseKeyActor.h"
#include "Interaction/BathhouseKeyHookActor.h"
#include "Kismet/GameplayStatics.h"
#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

ABathhouseKeyRackActor::ABathhouseKeyRackActor()
{
	PrimaryActorTick.bCanEverTick = false;
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);
	KeyClass = ABathhouseKeyActor::StaticClass();
	HookClass = ABathhouseKeyHookActor::StaticClass();
}

void ABathhouseKeyRackActor::BeginPlay()
{
	Super::BeginPlay();
	if (UBathhouseFacilitySubsystem* Subsystem = GetWorld()->GetSubsystem<UBathhouseFacilitySubsystem>())
	{
		AuthorityRegistryHandle = Subsystem->OnExpansionAuthorityChanged.AddUObject(
			this,
			&ABathhouseKeyRackActor::HandleExpansionAuthorityChanged);
		BindAuthority(Subsystem->GetExpansionAuthority());
	}
}

void ABathhouseKeyRackActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	BindAuthority(nullptr);
	if (UBathhouseFacilitySubsystem* Subsystem = GetWorld() ? GetWorld()->GetSubsystem<UBathhouseFacilitySubsystem>() : nullptr)
	{
		if (AuthorityRegistryHandle.IsValid()) Subsystem->OnExpansionAuthorityChanged.Remove(AuthorityRegistryHandle);
	}
	AuthorityRegistryHandle.Reset();
	for (ABathhouseKeyActor* Key : OwnedKeys) if (IsValid(Key)) Key->Destroy();
	for (ABathhouseKeyHookActor* Hook : OwnedHooks) if (IsValid(Hook)) Hook->Destroy();
	OwnedKeys.Reset();
	OwnedHooks.Reset();
	Super::EndPlay(EndPlayReason);
}

#if WITH_EDITOR
EDataValidationResult ABathhouseKeyRackActor::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = Super::IsDataValid(Context);
	const UWorld* World = GetWorld();
	if (!World || HasAnyFlags(RF_ClassDefaultObject))
	{
		return Result;
	}
	for (TActorIterator<ABathhouseExpansionAuthority> It(World); It; ++It)
	{
		const UBathhouseExpansionDefinition* Definition = It->GetExpansionDefinition();
		if (!Definition || Definition->Tiers.IsEmpty())
		{
			continue;
		}
		// 효과 표의 모든 줄을 본다(홀 넓힘 줄 수는 Building이 알고 열쇠걸이는 알지 않는다).
		int32 MaxKeys = 0;
		for (const FBathhouseExpansionTier& Tier : Definition->Tiers)
		{
			MaxKeys = FMath::Max(MaxKeys, Tier.KeyPoolSize);
		}
		if (MaxKeys > PairTransforms.Num())
		{
			Context.AddError(FText::Format(
				NSLOCTEXT("BathhouseKeyRack", "TooManyKeys", "열쇠 수 {0}개가 열쇠걸이 자리 {1}개보다 많습니다."),
				MaxKeys, PairTransforms.Num()));
			Result = EDataValidationResult::Invalid;
		}
	}
	return Result == EDataValidationResult::NotValidated ? EDataValidationResult::Valid : Result;
}
#endif

void ABathhouseKeyRackActor::HandleExpansionTierChanged(const int32 NewTier)
{
	(void)NewTier;
	if (BoundAuthority) MaterializeToPoolSize(BoundAuthority->GetCurrentKeyPoolSize());
}

void ABathhouseKeyRackActor::HandleExpansionAuthorityChanged(ABathhouseExpansionAuthority* Authority)
{
	BindAuthority(Authority);
}

void ABathhouseKeyRackActor::BindAuthority(ABathhouseExpansionAuthority* Authority)
{
	if (BoundAuthority == Authority) return;
	if (BoundAuthority)
	{
		BoundAuthority->OnExpansionTierChanged.RemoveDynamic(this, &ABathhouseKeyRackActor::HandleExpansionTierChanged);
	}
	BoundAuthority = Authority;
	if (BoundAuthority)
	{
		BoundAuthority->OnExpansionTierChanged.AddUniqueDynamic(this, &ABathhouseKeyRackActor::HandleExpansionTierChanged);
		MaterializeToPoolSize(BoundAuthority->GetCurrentKeyPoolSize());
	}
}

void ABathhouseKeyRackActor::MaterializeToPoolSize(const int32 PoolSize)
{
	if (!GetWorld() || !KeyClass || !HookClass || PoolSize <= OwnedKeys.Num()) return;
	const int32 Target = FMath::Min(PoolSize, PairTransforms.Num());
	if (Target < PoolSize)
	{
		UE_LOG(LogTemp, Error, TEXT("Key rack %s needs %d authored pair transforms but has %d."), *GetName(), PoolSize, PairTransforms.Num());
	}
	for (int32 Index = OwnedKeys.Num(); Index < Target; ++Index)
	{
		const FTransform WorldTransform = PairTransforms[Index] * GetActorTransform();
		ABathhouseKeyHookActor* Hook = GetWorld()->SpawnActorDeferred<ABathhouseKeyHookActor>(HookClass, WorldTransform, this, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
		ABathhouseKeyActor* Key = GetWorld()->SpawnActorDeferred<ABathhouseKeyActor>(KeyClass, WorldTransform, this, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
		const int32 Number = FirstKeyNumber + Index;
		if (!Hook || !Key || !Hook->ConfigureRackPair(Number, Key) || !Key->ConfigureRackIdentity(Number, Hook))
		{
			if (Hook) Hook->Destroy();
			if (Key) Key->Destroy();
			break;
		}
		UGameplayStatics::FinishSpawningActor(Hook, WorldTransform);
		UGameplayStatics::FinishSpawningActor(Key, WorldTransform);
		OwnedHooks.Add(Hook);
		OwnedKeys.Add(Key);
	}
}
