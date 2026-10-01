#include "Combat/WrenchRepairSession.h"
#include "Combat/WrenchRepairable.h"
#include "GameFramework/Actor.h"

bool FWrenchRepairSession::IsRepairTarget(AActor* Actor)
{
	const auto* Repairable = IsValid(Actor) ? Cast<IWrenchRepairable>(Actor) : nullptr;
	return Repairable && Repairable->IsWrenchRepairRequired();
}

bool FWrenchRepairSession::Begin(AActor* Actor)
{
	Reset();
	if (!IsRepairTarget(Actor))
	{
		return false;
	}
	Target = Actor;
	return true;
}

float FWrenchRepairSession::GetProgress(AActor* FocusActor) const
{
	auto* Repairable =
		Target.Get() == FocusActor && IsRepairTarget(FocusActor) ? Cast<IWrenchRepairable>(FocusActor) : nullptr;
	return Repairable
			   ? FMath::Clamp(Elapsed / FMath::Max(UE_SMALL_NUMBER, Repairable->GetWrenchRepairSeconds()), 0.f, 1.f)
			   : 0.f;
}

FHeldEquipmentUseUpdate FWrenchRepairSession::Update(AActor* FocusActor, float Delta)
{
	FHeldEquipmentUseUpdate Result;
	if (Target.Get() != FocusActor || !IsRepairTarget(FocusActor))
	{
		Reset();
		return Result;
	}
	auto* Repairable = Cast<IWrenchRepairable>(FocusActor);
	if (!FMath::IsFinite(Delta) || Delta < 0)
	{
		Reset();
		return Result;
	}
	Elapsed += Delta;
	Result.Progress = GetProgress(FocusActor);
	Result.State = EPlayerHoldInteractionState::Running;
	if (Elapsed >= Repairable->GetWrenchRepairSeconds())
	{
		Result.State = Repairable->CommitWrenchRepair(Result.FailureReason) ? EPlayerHoldInteractionState::Succeeded
																			: EPlayerHoldInteractionState::Failed;
		Reset();
	}
	return Result;
}
