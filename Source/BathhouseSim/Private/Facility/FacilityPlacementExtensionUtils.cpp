#include "Facility/FacilityPlacementExtensionUtils.h"
#include "Facility/FacilityPlacementExtension.h"
#include "Facility/BathhouseFacilityPlacementInstanceData.h"
#include "GameFramework/Actor.h"
#if WITH_EDITOR
#include "Engine/BlueprintGeneratedClass.h"
#include "Engine/SCS_Node.h"
#include "Engine/SimpleConstructionScript.h"
#endif

#define LOCTEXT_NAMESPACE "FacilityPlacementExtensionUtils"

namespace FacilityPlacementExtensionUtils
{
	void CollectAuthoringComponents(const AActor& Owner, TArray<UActorComponent*>& Out)
	{
		Out.Reset();
		Owner.GetComponents(Out);
#if WITH_EDITOR
		if (Owner.IsTemplate())
		{
			for (const UClass* Class = Owner.GetClass(); Class; Class = Class->GetSuperClass())
			{
				if (const auto* Generated = Cast<UBlueprintGeneratedClass>(Class))
				{
					if (Generated->SimpleConstructionScript)
					{
						for (const USCS_Node* Node : Generated->SimpleConstructionScript->GetAllNodes())
						{
							if (Node && Node->ComponentTemplate)
							{
								Out.AddUnique(Node->ComponentTemplate);
							}
						}
					}
				}
			}
		}
#endif
	}

	bool ValidateAuthoring(const AActor& Owner, FText& Failure)
	{
		TArray<UActorComponent*> Components;
		CollectAuthoringComponents(Owner, Components);
		TSet<FName> Keys;
		for (const UActorComponent* Component : Components)
		{
			if (const auto* Extension = Cast<IFacilityPlacementExtension>(Component))
			{
				const FName Key = Extension->GetPlacementExtensionKey();
				if (Key.IsNone() || Keys.Contains(Key))
				{
					Failure = LOCTEXT("InvalidKey", "설비 확장 key는 비어 있거나 중복될 수 없습니다.");
					return false;
				}
				Keys.Add(Key);
				if (!Extension->ValidateExtensionAuthoring(Components, Failure))
				{
					return false;
				}
			}
		}
		Failure = FText::GetEmpty();
		return true;
	}

	bool Export(const AActor& Owner, UBathhouseFacilityPlacementInstanceData& Data, FText& Failure)
	{
		if (!ValidateAuthoring(Owner, Failure))
		{
			return false;
		}
		Data.Extensions.Reset();
		TInlineComponentArray<UActorComponent*> Components;
		Owner.GetComponents(Components);
		for (const UActorComponent* Component : Components)
		{
			if (const auto* Extension = Cast<IFacilityPlacementExtension>(Component))
			{
				if (!Extension->ValidatePlacementExtension(nullptr, Failure))
				{
					return false;
				}
				auto* Exported = Extension->ExportPlacementExtension(&Data);
				if (!Exported || Exported->Key != Extension->GetPlacementExtensionKey() ||
					!Extension->ValidatePlacementExtension(Exported, Failure))
				{
					return false;
				}
				Data.Extensions.Add(Exported);
			}
		}
		return true;
	}

	bool Import(AActor& Owner, const UBathhouseFacilityPlacementInstanceData* Data, FText& Failure)
	{
		if (!ValidateAuthoring(Owner, Failure))
		{
			return false;
		}
		TInlineComponentArray<UActorComponent*> Components;
		Owner.GetComponents(Components);
		TMap<FName, IFacilityPlacementExtension*> Extensions;
		for (UActorComponent* Component : Components)
		{
			if (auto* Extension = Cast<IFacilityPlacementExtension>(Component))
			{
				Extensions.Add(Extension->GetPlacementExtensionKey(), Extension);
			}
		}
		TMap<FName, const UFacilityPlacementExtensionData*> Payloads;
		if (Data)
		{
			for (const UFacilityPlacementExtensionData* Payload : Data->Extensions)
			{
				if (!Payload || Payload->Key.IsNone() || Payloads.Contains(Payload->Key) ||
					!Extensions.Contains(Payload->Key))
				{
					Failure = LOCTEXT("PayloadKey", "설비 확장 데이터 key가 올바르지 않습니다.");
					return false;
				}
				Payloads.Add(Payload->Key, Payload);
			}
			if (Payloads.Num() != Extensions.Num())
			{
				Failure = LOCTEXT("MissingData", "설비 확장 데이터가 누락됐습니다.");
				return false;
			}
		}
		for (const auto& Pair : Extensions)
		{
			if (!Pair.Value->ValidatePlacementExtension(Data ? Payloads.FindRef(Pair.Key) : nullptr, Failure))
			{
				return false;
			}
		}
		for (const auto& Pair : Extensions)
		{
			Pair.Value->ApplyPlacementExtension(Data ? Payloads.FindRef(Pair.Key) : nullptr);
		}
		Failure = FText::GetEmpty();
		return true;
	}
} // namespace FacilityPlacementExtensionUtils

#undef LOCTEXT_NAMESPACE
