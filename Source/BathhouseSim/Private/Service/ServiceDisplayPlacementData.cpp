#include "Service/ServiceDisplayPlacementData.h"

#include "Service/ServiceItemDefinition.h"

#define LOCTEXT_NAMESPACE "ServiceDisplayPlacementData"

FText UServiceDisplayPlacementData::GetContentsSummary() const
{
	TArray<TPair<const UServiceItemDefinition*, int32>> Totals;
	for (const auto& Snapshot : Spaces)
	{
		if (Snapshot.Kind && Snapshot.Count > 0)
		{
			auto* Entry = Totals.FindByPredicate(
				[&](const auto& Value)
				{
					return Value.Key == Snapshot.Kind;
				});
			if (Entry)
			{
				Entry->Value += Snapshot.Count;
			}
			else
			{
				Totals.Emplace(Snapshot.Kind, Snapshot.Count);
			}
		}
	}
	TArray<FString> Parts;
	for (const auto& Entry : Totals)
	{
		Parts.Add(FText::Format(bBottleSummary ? LOCTEXT("Bottles", "{0} {1}병") : LOCTEXT("Items", "{0} {1}개"),
								Entry.Key->DisplayName, Entry.Value)
					  .ToString());
	}
	return FText::FromString(FString::Join(Parts, TEXT(", ")));
}

#undef LOCTEXT_NAMESPACE
