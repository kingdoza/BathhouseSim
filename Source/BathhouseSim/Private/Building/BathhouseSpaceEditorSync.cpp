#include "Building/BathhouseSpaceEditorSync.h"

#if WITH_EDITOR

#include "Building/BathhouseSpaceActor.h"
#include "Containers/Ticker.h"
#include "Engine/World.h"
#include "EngineUtils.h"

namespace
{
	TMap<TWeakObjectPtr<UWorld>, FTSTicker::FDelegateHandle>& PendingWorlds()
	{
		static TMap<TWeakObjectPtr<UWorld>, FTSTicker::FDelegateHandle> Pending;
		return Pending;
	}
}

void FBathhouseSpaceEditorSync::RequestRebuild(UWorld* World)
{
	if (!World || World->WorldType != EWorldType::Editor)
	{
		return;
	}
	const TWeakObjectPtr<UWorld> Key(World);
	if (PendingWorlds().Contains(Key))
	{
		return;
	}
	PendingWorlds().Add(Key, FTSTicker::GetCoreTicker().AddTicker(
		FTickerDelegate::CreateLambda([Key](float)
		{
			PendingWorlds().Remove(Key);
			if (UWorld* TickWorld = Key.Get())
			{
				RebuildNow(TickWorld);
			}
			return false;
		}),
		0.0f));
}

void FBathhouseSpaceEditorSync::RebuildNow(UWorld* World)
{
	if (!World)
	{
		return;
	}
	for (TActorIterator<ABathhouseSpaceActor> It(World); It; ++It)
	{
		if (IsValid(*It))
		{
			It->RebuildShell();
		}
	}
}

bool FBathhouseSpaceEditorSync::HasPendingRequest(const UWorld* World)
{
	return World && PendingWorlds().Contains(TWeakObjectPtr<UWorld>(const_cast<UWorld*>(World)));
}

#endif
