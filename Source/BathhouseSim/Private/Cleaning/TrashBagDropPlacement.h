#pragma once
#include "CoreMinimal.h"
class APawn;
class ATrashBagActor;

struct FTrashBagDropPlacement
{
	static bool Find(UWorld& World, APawn* UserPawn, const TArray<AActor*>& IgnoredActors,
					 TSubclassOf<ATrashBagActor> BagClass, float ForwardDistance, float MinForwardDistance,
					 FTransform& OutTransform);
};
