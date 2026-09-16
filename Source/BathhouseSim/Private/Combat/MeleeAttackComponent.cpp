#include "Combat/MeleeAttackComponent.h"

#include "Camera/CameraComponent.h"
#include "Combat/CombatTypes.h"
#include "Combat/HealthComponent.h"
#include "Components/SceneComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "Interaction/HeldEquipmentMotionComponent.h"

#if ENABLE_DRAW_DEBUG
namespace
{
	TAutoConsoleVariable<int32> CVarBathhouseDebugMeleeHit(
		TEXT("bathhouse.Debug.MeleeHit"),
		0,
		TEXT("Draw the melee camera trace, attack sphere, raw hits, and damage targets.\n")
		TEXT("0: disabled, 1: enabled"),
		ECVF_Cheat);

	constexpr float MeleeDebugLifetimeSeconds = 2.0f;
}
#endif

UMeleeAttackComponent::UMeleeAttackComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	PrimaryComponentTick.TickGroup = TG_PostPhysics;
}

void UMeleeAttackComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	CancelAttack();
	OnAttackStarted.Clear();
	OnAttackHit.Clear();
	OnAttackEnded.Clear();
	Super::EndPlay(EndPlayReason);
}

bool UMeleeAttackComponent::StartAttack(
	AActor* User,
	AActor* Weapon,
	UCameraComponent* Camera,
	UHeldEquipmentMotionComponent* MotionComponent)
{
	if (bAttacking || !IsValid(User) || !IsValid(Weapon) || !IsValid(Camera))
	{
		return false;
	}
	bAttacking = true;
	bHitCommitted = false;
	ElapsedSeconds = 0.0f;
	AttackUser = User;
	AttackWeapon = Weapon;
	AttackCamera = Camera;
	ActiveMotion = MotionComponent;
	SetComponentTickEnabled(true);
	if (MotionComponent && Weapon->GetRootComponent())
	{
		MotionComponent->StartOneShot(
			Weapon->GetRootComponent(),
			SwingPositionCurve,
			SwingRotationCurve,
			FMath::Max(0.05f, AttackDurationSeconds));
	}
	OnAttackStarted.Broadcast();
	return true;
}

void UMeleeAttackComponent::TickComponent(
	const float DeltaTime,
	const ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (!bAttacking)
	{
		return;
	}
	ElapsedSeconds += FMath::Max(0.0f, DeltaTime);
	if (!bHitCommitted && ElapsedSeconds >= FMath::Clamp(HitTimeSeconds, 0.0f, AttackDurationSeconds))
	{
		bHitCommitted = true;
		PerformHit();
	}
	if (ElapsedSeconds >= FMath::Max(0.05f, AttackDurationSeconds))
	{
		FinishAttack();
	}
}

void UMeleeAttackComponent::CancelAttack()
{
	if (!bAttacking)
	{
		return;
	}
	if (ActiveMotion)
	{
		ActiveMotion->StopMotion();
	}
	FinishAttack();
}

void UMeleeAttackComponent::PerformHit()
{
	UWorld* World = GetWorld();
	if (!World || !AttackCamera || !AttackUser || !AttackWeapon)
	{
		return;
	}
	const FVector CameraOrigin = AttackCamera->GetComponentLocation();
	const FVector CameraDirection = AttackCamera->GetForwardVector().GetSafeNormal();
	if (CameraDirection.IsNearlyZero())
	{
		return;
	}
	const FVector Center = CameraOrigin + CameraDirection * FMath::Max(0.0f, AttackDistance);
	FCollisionQueryParams Params(SCENE_QUERY_STAT(BathhouseMeleeAttack), false);
	Params.AddIgnoredActor(AttackUser);
	Params.AddIgnoredActor(AttackWeapon);
	TArray<FHitResult> Hits;
	World->SweepMultiByChannel(
		Hits,
		Center,
		Center + CameraDirection * UE_KINDA_SMALL_NUMBER,
		FQuat::Identity,
		TraceChannel,
		FCollisionShape::MakeSphere(FMath::Max(1.0f, AttackRadius)),
		Params);

#if ENABLE_DRAW_DEBUG
	const bool bDrawMeleeDebug = CVarBathhouseDebugMeleeHit.GetValueOnGameThread() != 0;
	if (bDrawMeleeDebug)
	{
		DrawDebugDirectionalArrow(
			World,
			CameraOrigin,
			Center,
			12.0f,
			FColor::Cyan,
			false,
			MeleeDebugLifetimeSeconds,
			0,
			1.5f);
		DrawDebugSphere(
			World,
			Center,
			FMath::Max(1.0f, AttackRadius),
			24,
			Hits.IsEmpty() ? FColor::Red : FColor::Yellow,
			false,
			MeleeDebugLifetimeSeconds,
			0,
			1.5f);
	}
#endif

	TSet<TObjectPtr<AActor>> DamagedActors;
	for (const FHitResult& Hit : Hits)
	{
		AActor* Target = Hit.GetActor();
		if (!IsValid(Target) || Target == AttackUser || Target == AttackWeapon || DamagedActors.Contains(Target))
		{
		#if ENABLE_DRAW_DEBUG
			if (bDrawMeleeDebug && IsValid(Target))
			{
				const FVector HitLocation = Hit.ImpactPoint.IsNearlyZero() ? Hit.Location : Hit.ImpactPoint;
				DrawDebugPoint(World, HitLocation, 9.0f, FColor::Orange, false, MeleeDebugLifetimeSeconds);
				DrawDebugString(
					World,
					HitLocation,
					FString::Printf(TEXT("Raw/Duplicate: %s.%s"), *GetNameSafe(Target), *GetNameSafe(Hit.GetComponent())),
					nullptr,
					FColor::Orange,
					MeleeDebugLifetimeSeconds,
					true);
			}
		#endif
			continue;
		}
		UHealthComponent* Health = Target->FindComponentByClass<UHealthComponent>();
		if (!Health || !Health->IsHealthActive())
		{
		#if ENABLE_DRAW_DEBUG
			if (bDrawMeleeDebug)
			{
				const FVector HitLocation = Hit.ImpactPoint.IsNearlyZero() ? Hit.Location : Hit.ImpactPoint;
				DrawDebugPoint(World, HitLocation, 9.0f, FColor::Red, false, MeleeDebugLifetimeSeconds);
				DrawDebugString(
					World,
					HitLocation,
					FString::Printf(TEXT("No active Health: %s.%s"), *GetNameSafe(Target), *GetNameSafe(Hit.GetComponent())),
					nullptr,
					FColor::Red,
					MeleeDebugLifetimeSeconds,
					true);
			}
		#endif
			continue;
		}
		DamagedActors.Add(Target);
		FCombatDamageContext DamageContext;
		DamageContext.InstigatorActor = AttackUser;
		DamageContext.CauserActor = AttackWeapon;
		DamageContext.Damage = FMath::Max(0.0f, Damage);
		DamageContext.CameraOrigin = CameraOrigin;
		DamageContext.CameraDirection = CameraDirection;
		DamageContext.ImpulseStrength = FMath::Max(0.0f, ImpulseStrength);
		DamageContext.VerticalImpulse = VerticalImpulse;
		Health->ApplyDamage(DamageContext);
	#if ENABLE_DRAW_DEBUG
		if (bDrawMeleeDebug)
		{
			const FVector HitLocation = Hit.ImpactPoint.IsNearlyZero() ? Hit.Location : Hit.ImpactPoint;
			DrawDebugPoint(World, HitLocation, 12.0f, FColor::Green, false, MeleeDebugLifetimeSeconds);
			DrawDebugString(
				World,
				HitLocation,
				FString::Printf(TEXT("Damaged: %s | %.1f"), *GetNameSafe(Target), DamageContext.Damage),
				nullptr,
				FColor::Green,
				MeleeDebugLifetimeSeconds,
				true);
		}
	#endif
	}
	OnAttackHit.Broadcast();
}

void UMeleeAttackComponent::FinishAttack()
{
	if (!bAttacking)
	{
		return;
	}
	bAttacking = false;
	bHitCommitted = false;
	ElapsedSeconds = 0.0f;
	AttackUser = nullptr;
	AttackWeapon = nullptr;
	AttackCamera = nullptr;
	ActiveMotion = nullptr;
	SetComponentTickEnabled(false);
	OnAttackEnded.Broadcast();
}
