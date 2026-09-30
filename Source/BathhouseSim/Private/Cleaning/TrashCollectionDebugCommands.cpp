#if !UE_BUILD_SHIPPING
#include "Cleaning/TrashCollectionZoneActor.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "HAL/IConsoleManager.h"

namespace
{
	FAutoConsoleCommandWithWorldAndArgs GTrashCollectNow(
		TEXT("bathhouse.Debug.TrashCollection.CollectNow"),
		TEXT("Collect all free-world discardable items in every trash collection zone."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda(
			[](const TArray<FString>& Args, UWorld* World)
			{
				if (!World)
				{
					return;
				}
				for (TActorIterator<ATrashCollectionZoneActor> It(World); It; ++It)
				{
					It->CollectNow();
				}
			}));
} // namespace
#endif
