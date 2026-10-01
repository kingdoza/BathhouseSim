#pragma once
#include "Interaction/HeldEquipmentUsable.h"

class FWrenchRepairSession
{
public:

	static bool IsRepairTarget(AActor* Actor);
	bool Begin(AActor* Actor);
	FHeldEquipmentUseUpdate Update(AActor* FocusActor, float Delta);

	void Reset()
	{
		Target.Reset();
		Elapsed = 0;
	}

	bool IsActive() const
	{
		return !Target.IsExplicitlyNull();
	}

	float GetProgress(AActor* FocusActor) const;

private:

	TWeakObjectPtr<AActor> Target;
	float Elapsed = 0;
};
