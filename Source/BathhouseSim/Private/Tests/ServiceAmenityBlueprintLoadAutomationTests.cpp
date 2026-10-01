#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Engine/Blueprint.h"
#include "UObject/UObjectHash.h"
#include "UObject/UnrealType.h"
#include "Character/FirstPersonCharacter.h"
#include "Service/PlayerScrubFocusComponent.h"
#include "UI/BathhouseHUD.h"
#include "Combat/MonkeyWrenchActor.h"
#include "Combat/MeleeAttackComponent.h"
#include "Facility/BathhouseFacilityActor.h"
#include "Facility/BathhouseFacilitySlotComponent.h"
#include "Facility/FacilityPlacementExtensionUtils.h"
#include "Service/ServiceDisplayManagerComponent.h"
#include "Service/DisplaySpaceComponent.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FServiceAmenityBlueprintLoadTest, "BathhouseSim.Service.Amenity.BlueprintLoad",
								 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FServiceAmenityBlueprintLoadTest::RunTest(const FString&)
{
	const bool Copies = FParse::Param(FCommandLine::Get(), TEXT("ServiceAmenityCopies"));

	struct FSpec
	{
		const TCHAR* Name;
		const TCHAR* Folder;
		UClass* Parent;
	};

	const FSpec Specs[] = {
		{TEXT("BP_FirstPersonCharacter"), TEXT("/Game/FirstPersonCharacter"), AFirstPersonCharacter::StaticClass()},
		{TEXT("BP_BathhouseHUD"), TEXT("/Game/Bathhouse/Blueprints/Game"), ABathhouseHUD::StaticClass()},
		{TEXT("BP_MonkeyWrench"), TEXT("/Game/Bathhouse/Blueprints/Combat"), AMonkeyWrenchActor::StaticClass()},
		{TEXT("BP_Shower"), TEXT("/Game/Bathhouse/Blueprints/Facility"), ABathhouseFacilityActor::StaticClass()}};
	for (const auto& Spec : Specs)
	{
		const FString Path =
			FString::Printf(TEXT("%s/%s"), Copies ? TEXT("/Game/Developers/MigrationCheck") : Spec.Folder, Spec.Name);
		auto* Package = LoadPackage(nullptr, *Path, LOAD_None);
		if (!TestNotNull(Path, Package))
		{
			return false;
		}
		UBlueprint* BP = nullptr;
		ForEachObjectWithOuter(Package,
							   [&](UObject* Object)
							   {
								   if (auto* B = Cast<UBlueprint>(Object))
								   {
									   BP = B;
								   }
							   });
		if (!TestNotNull(TEXT("Blueprint"), BP) || !TestNotNull(TEXT("Generated class"), BP->GeneratedClass.Get()))
		{
			return false;
		}
		TestEqual(TEXT("Native parent preserved"), BP->GeneratedClass->GetSuperClass(), Spec.Parent);
		auto* CDO = Cast<AActor>(BP->GeneratedClass->GetDefaultObject());
		if (!TestNotNull(TEXT("CDO"), CDO))
		{
			return false;
		}
		TArray<UActorComponent*> Components;
		FacilityPlacementExtensionUtils::CollectAuthoringComponents(*CDO, Components);
		const auto Count = [&](UClass* Type)
		{
			return Components
				.FilterByPredicate(
					[&](auto* C)
					{
						return C && C->IsA(Type);
					})
				.Num();
		};
		if (Spec.Parent == AFirstPersonCharacter::StaticClass())
		{
			TestEqual(TEXT("New PlayerScrubFocus component exactly once"),
					  Count(UPlayerScrubFocusComponent::StaticClass()), 1);
			for (const auto* Name :
				 {TEXT("FirstPersonCamera"), TEXT("HeldKeyAnchor"), TEXT("PlayerCarry"), TEXT("PlayerInteraction"),
				  TEXT("HeldEquipmentMotion"), TEXT("PlayerEquipmentUse"), TEXT("PlayerHeldTargetUse"),
				  TEXT("PlayerFacilityPlacement"), TEXT("ComputerWidgetInteraction"), TEXT("PlayerComputerUse"),
				  TEXT("PlayerScrubFocus")})
			{
				TestNotNull(Name, CDO->GetDefaultSubobjectByName(Name));
			}
		}
		else if (Spec.Parent == ABathhouseHUD::StaticClass())
		{
			TestNotNull(TEXT("New HUD widget class property"),
						FindFProperty<FClassProperty>(CDO->GetClass(), TEXT("ScrubFocusHudWidgetClass")));
		}
		else if (Spec.Parent == AMonkeyWrenchActor::StaticClass())
		{
			TestNotNull(TEXT("Wrench mesh retained"), CDO->GetDefaultSubobjectByName(TEXT("WorldMesh")));
			TestEqual(TEXT("Melee attack retained"), Count(UMeleeAttackComponent::StaticClass()), 1);
		}
		else
		{
			TestEqual(TEXT("Shower slots retained"), Count(UBathhouseFacilitySlotComponent::StaticClass()), 2);
			TestEqual(TEXT("Shower manager retained"), Count(UServiceDisplayManagerComponent::StaticClass()), 1);
			TestEqual(TEXT("Shower spaces retained"), Count(UDisplaySpaceComponent::StaticClass()), 2);
			TestNotNull(TEXT("SceneRoot retained"), CDO->GetDefaultSubobjectByName(TEXT("SceneRoot")));
			TestNotNull(TEXT("FacilityPlacement retained"), CDO->GetDefaultSubobjectByName(TEXT("FacilityPlacement")));
		}
		TestFalse(TEXT("Read-only load leaves package clean"), Package->IsDirty());
		AddInfo(Path);
	}
	return true;
}
#endif
