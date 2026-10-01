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
#include "Facility/FacilityPlacementExtensionUtils.h"
#include "Service/DisplaySpaceComponent.h"
#include "Service/DrinkFridgeActor.h"
#include "Service/ItemBoxActor.h"
#include "Service/ServiceItemDefinition.h"
#include "Service/ServiceDisplayManagerComponent.h"
#include "Towel/TowelProcessingMachineActor.h"
#include "Towel/TowelInventoryComponent.h"
#include "Towel/Presentation/TowelPileVisualComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Interaction/Presentation/OpeningPresentationComponent.h"
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
	{TEXT("BP_DrinkFridge"),TEXT("/Game/Bathhouse/Blueprints/Service/BP_DrinkFridge"),TEXT("/Game/Developers/MigrationCheck/BP_DrinkFridge")},
	{TEXT("BP_Washer"),TEXT("/Game/Bathhouse/Blueprints/Towel/BP_Washer"),TEXT("/Game/Developers/MigrationCheck/BP_Washer")},
	{TEXT("BP_Dryer"),TEXT("/Game/Bathhouse/Blueprints/Towel/BP_Dryer"),TEXT("/Game/Developers/MigrationCheck/BP_Dryer")},
	{TEXT("BP_ItemBox"),TEXT("/Game/Bathhouse/Blueprints/Service/BP_ItemBox"),TEXT("/Game/Developers/MigrationCheck/BP_ItemBox")},
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
			TestEqual(TEXT("Catalog includes the authored unit-four products"), Catalog->Products.Num(), 20);
			for (const TCHAR* ProductId : {TEXT("MassageChair"), TEXT("RestBench"), TEXT("Television"), TEXT("ScrubTable")})
			{
				TestTrue(FString::Printf(TEXT("SVC4-001 catalog includes %s"), ProductId),
					Catalog->Products.ContainsByPredicate([ProductId](const FShopProductEntry& Product)
					{
						return Product.ProductId == FName(ProductId);
					}));
			}
			for (const FShopProductEntry& Product : Catalog->Products)
			{
				TestFalse(FString::Printf(TEXT("%s keeps a product id"), *Product.ProductId.ToString()), Product.ProductId.IsNone());
				TestTrue(FString::Printf(TEXT("%s keeps exactly one definition"), *Product.ProductId.ToString()),
					IsValid(Product.PlacementDefinition) != IsValid(Product.ItemBoxDefinition));
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
			const FName Name(Spec->Name);
			const bool bMachine=Name==TEXT("BP_Washer") || Name==TEXT("BP_Dryer");
			UClass* Expected=Name==TEXT("BP_ItemBox") ? AItemBoxActor::StaticClass()
				: bMachine ? ATowelProcessingMachineActor::StaticClass()
				: Name==TEXT("BP_DrinkFridge") ? ADrinkFridgeActor::StaticClass() : ABathhouseFacilityActor::StaticClass();
			TestTrue(TEXT("Blueprint keeps its native parent"),Blueprint->GeneratedClass->GetSuperClass()==Expected);
			AActor* CDO=Cast<AActor>(Blueprint->GeneratedClass->GetDefaultObject());
			if (!TestNotNull(TEXT("Blueprint CDO loads"),CDO)) return false;
			TArray<UActorComponent*> Components;
			FacilityPlacementExtensionUtils::CollectAuthoringComponents(*CDO,Components);
			auto CountType=[&](UClass* Type)
			{
				int32 Count=0;
				for (auto* Component : Components) if (Component && Component->IsA(Type)) ++Count;
				return Count;
			};
			if (bMachine)
			{
				TestEqual(TEXT("Existing machine inventory retained"),CountType(UTowelInventoryComponent::StaticClass()),1);
				TestEqual(TEXT("Existing machine visual retained"),CountType(UTowelPileVisualComponent::StaticClass()),1);
				TestNotNull(TEXT("New LidPivot native subobject"),CDO->GetDefaultSubobjectByName(TEXT("LidPivot")));
				TestNotNull(TEXT("New LidMesh native subobject"),CDO->GetDefaultSubobjectByName(TEXT("LidMesh")));
				TestNotNull(TEXT("New LidPresentation native subobject"),CDO->GetDefaultSubobjectByName(TEXT("LidPresentation")));
			}
			else if (Name==TEXT("BP_ItemBox"))
			{
				TestNotNull(TEXT("Existing BoxMesh native subobject retained"),CDO->GetDefaultSubobjectByName(TEXT("BoxMesh")));
				TestEqual(TEXT("Existing ContentsVisual retained"),CountType(UInstancedStaticMeshComponent::StaticClass()),1);
			}
			else
			{
				const auto* Facility=Cast<ABathhouseFacilityActor>(CDO);
				const bool bFridge=Name==TEXT("BP_DrinkFridge");
				TestEqual(TEXT("Existing facility type retained"),Facility->GetFacilityType(),
					bFridge ? EBathhouseFacilityType::DrinkFridge : EBathhouseFacilityType::Shower);
				TestEqual(TEXT("Existing customer slots retained"),CountType(UBathhouseFacilitySlotComponent::StaticClass()),bFridge ? 1 : 2);
				if (bFridge)
				{
					TestEqual(TEXT("New native display manager"),CountType(UServiceDisplayManagerComponent::StaticClass()),1);
					TestEqual(TEXT("Four existing SCS display spaces retained"),CountType(UDisplaySpaceComponent::StaticClass()),4);
				}
			}
			TestFalse(TEXT("Load gate does not dirty package"),Package->IsDirty());
		}
	}
	return true;
}

#endif
