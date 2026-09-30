#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Blueprint/WidgetBlueprintGeneratedClass.h"
#include "Blueprint/WidgetTree.h"
#include "Engine/Blueprint.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "Engine/SCS_Node.h"
#include "Engine/SimpleConstructionScript.h"
#include "Facility/BathhouseFacilityActor.h"
#include "Facility/BathhouseFacilitySlotComponent.h"
#include "Misc/CommandLine.h"
#include "Misc/PackageName.h"
#include "Misc/Parse.h"
#include "Placement/FacilityPlacementDefinition.h"
#include "Shop/ShopCatalog.h"
#include "UI/InteractionPromptWidget.h"
#include "UObject/UObjectHash.h"

namespace
{
struct FServiceLoadSpec
{
	const TCHAR* Name;
	const TCHAR* OriginalPackage;
	const TCHAR* CopyPackage;
};

const FServiceLoadSpec ServiceLoadSpecs[] = {
	{
		TEXT("DA_ShopCatalog"),
		TEXT("/Game/Bathhouse/Data/Shop/DA_ShopCatalog"),
		TEXT("/Game/Developers/MigrationCheck/DA_ShopCatalog")
	},
	{
		TEXT("WBP_InteractionPrompt"),
		TEXT("/Game/Bathhouse/UI/WBP_InteractionPrompt"),
		TEXT("/Game/Developers/MigrationCheck/WBP_InteractionPrompt")
	},
	{
		TEXT("BP_Shower"),
		TEXT("/Game/Bathhouse/Blueprints/Facility/BP_Shower"),
		TEXT("/Game/Developers/MigrationCheck/BP_Shower")
	}
};

UBlueprint* FindServiceBlueprint(UPackage* Package)
{
	UBlueprint* Found = nullptr;
	if (Package)
	{
		ForEachObjectWithOuter(Package, [&Found](UObject* Object)
		{
			if (UBlueprint* Blueprint = Cast<UBlueprint>(Object))
			{
				Found = Blueprint;
			}
		}, EGetObjectsFlags::None);
	}
	return Found;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBathhouseServiceBlueprintLoadTest,
	"BathhouseSim.Service.BlueprintLoad",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBathhouseServiceBlueprintLoadTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FString RequestedPath;
	FParse::Value(FCommandLine::Get(), TEXT("BathhouseServiceLoadPath="), RequestedPath);
	TArray<const FServiceLoadSpec*> Selected;
	if (RequestedPath.IsEmpty())
	{
		for (const FServiceLoadSpec& Spec : ServiceLoadSpecs)
		{
			Selected.Add(&Spec);
		}
	}
	else
	{
		if (!FPackageName::IsValidLongPackageName(RequestedPath))
		{
			AddError(FString::Printf(TEXT("Invalid service load package path: %s"), *RequestedPath));
			return false;
		}
		for (const FServiceLoadSpec& Spec : ServiceLoadSpecs)
		{
			if (RequestedPath.Equals(Spec.OriginalPackage, ESearchCase::IgnoreCase)
				|| RequestedPath.Equals(Spec.CopyPackage, ESearchCase::IgnoreCase))
			{
				Selected.Add(&Spec);
				break;
			}
		}
		if (Selected.IsEmpty())
		{
			AddError(FString::Printf(TEXT("Unsupported service load path: %s"), *RequestedPath));
			return false;
		}
	}

	const TCHAR* RequiredPromptWidgets[] = {
		TEXT("PromptRoot"), TEXT("TargetNameText"), TEXT("ActionNameText"), TEXT("FailureReasonText"),
		TEXT("SecondaryActionNameText"), TEXT("SecondaryFailureReasonText"), TEXT("InteractionProgressBar"),
		TEXT("EquipmentActionNameText"), TEXT("EquipmentFailureReasonText"), TEXT("EquipmentProgressBar"),
		TEXT("PlacementActionNameText"), TEXT("PlacementFailureReasonText"),
		TEXT("RecoveryActionNameText"), TEXT("RecoveryFailureReasonText"), TEXT("RecoveryProgressBar")
	};

	for (const FServiceLoadSpec* Spec : Selected)
	{
		const FString PackageToLoad = RequestedPath.IsEmpty() ? FString(Spec->OriginalPackage) : RequestedPath;
		UPackage* Package = LoadPackage(nullptr, *PackageToLoad, LOAD_None);
		if (!TestNotNull(FString::Printf(TEXT("%s package loads"), Spec->Name), Package))
		{
			return false;
		}
		if (FCString::Strcmp(Spec->Name, TEXT("DA_ShopCatalog")) == 0)
		{
			const UShopCatalog* Catalog = FindObject<UShopCatalog>(Package, Spec->Name);
			if (!TestNotNull(TEXT("Catalog object loads"), Catalog))
			{
				return false;
			}
			TestEqual(TEXT("Catalog keeps its existing product count"), Catalog->Products.Num(), 7);
			for (const FShopProductEntry& Product : Catalog->Products)
			{
				TestFalse(FString::Printf(TEXT("%s keeps a product id"), *Product.ProductId.ToString()), Product.ProductId.IsNone());
				TestTrue(FString::Printf(TEXT("%s keeps its facility definition"), *Product.ProductId.ToString()),
					IsValid(Product.PlacementDefinition));
				TestTrue(FString::Printf(TEXT("%s has no item box definition yet"), *Product.ProductId.ToString()),
					Product.ItemBoxDefinition == nullptr);
				TestTrue(FString::Printf(TEXT("%s keeps a positive price"), *Product.ProductId.ToString()), Product.Price > 0);
			}
			continue;
		}
		UBlueprint* Blueprint = FindServiceBlueprint(Package);
		if (!TestNotNull(FString::Printf(TEXT("%s Blueprint object loads"), Spec->Name), Blueprint)
			|| !TestNotNull(FString::Printf(TEXT("%s generated class loads"), Spec->Name),
				Blueprint ? Blueprint->GeneratedClass.Get() : nullptr))
		{
			return false;
		}
		if (FCString::Strcmp(Spec->Name, TEXT("WBP_InteractionPrompt")) == 0)
		{
			TestTrue(TEXT("WBP keeps its native parent"),
				Blueprint->GeneratedClass->GetSuperClass() == UInteractionPromptWidget::StaticClass());
			const UWidgetBlueprintGeneratedClass* WidgetClass = Cast<UWidgetBlueprintGeneratedClass>(Blueprint->GeneratedClass);
			if (!TestNotNull(TEXT("Widget generated class loads"), WidgetClass)
				|| !TestNotNull(TEXT("Widget tree archetype loads"), WidgetClass->GetWidgetTreeArchetype()))
			{
				return false;
			}
			for (const TCHAR* WidgetName : RequiredPromptWidgets)
			{
				TestTrue(FString::Printf(TEXT("WBP retains required widget %s"), WidgetName),
					WidgetClass->GetWidgetTreeArchetype()->FindWidget(FName(WidgetName)) != nullptr);
			}
		}
		else
		{
			TestTrue(TEXT("BP_Shower keeps a facility native parent"),
				Blueprint->GeneratedClass->IsChildOf(ABathhouseFacilityActor::StaticClass()));
			const ABathhouseFacilityActor* CDO = Cast<ABathhouseFacilityActor>(Blueprint->GeneratedClass->GetDefaultObject());
			TestNotNull(TEXT("BP_Shower CDO loads"), CDO);
			TestEqual(TEXT("BP_Shower keeps its Shower facility type"),
				CDO ? CDO->GetFacilityType() : EBathhouseFacilityType::Bath, EBathhouseFacilityType::Shower);
			bool bHasSlot = CDO && CDO->FindComponentByClass<UBathhouseFacilitySlotComponent>() != nullptr;
			for (const UClass* Class = Blueprint->GeneratedClass; Class && !bHasSlot; Class = Class->GetSuperClass())
			{
				const UBlueprintGeneratedClass* GeneratedClass = Cast<UBlueprintGeneratedClass>(Class);
				if (!GeneratedClass || !GeneratedClass->SimpleConstructionScript)
				{
					continue;
				}
				for (const USCS_Node* Node : GeneratedClass->SimpleConstructionScript->GetAllNodes())
				{
					if (Node && Node->ComponentClass
						&& Node->ComponentClass->IsChildOf(UBathhouseFacilitySlotComponent::StaticClass()))
					{
						bHasSlot = true;
						break;
					}
				}
			}
			TestTrue(TEXT("BP_Shower keeps its facility slot"), bHasSlot);
		}
	}
	return true;
}

#endif
