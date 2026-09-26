#if WITH_EDITOR && WITH_DEV_AUTOMATION_TESTS

#include "Computer/BathhouseComputerActor.h"
#include "Components/ActorComponent.h"
#include "Components/SceneComponent.h"
#include "Components/WidgetComponent.h"
#include "Engine/Blueprint.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Misc/AutomationTest.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/PackageName.h"
#include "UObject/UObjectHash.h"
#include "UObject/UnrealType.h"

namespace
{
UBlueprint* FindComputerBlueprint(UPackage* Package)
{
	if (!Package)
	{
		return nullptr;
	}

	UBlueprint* Result = nullptr;
	ForEachObjectWithOuter(Package, [&Result](UObject* Object)
	{
		if (UBlueprint* Blueprint = Cast<UBlueprint>(Object))
		{
			Result = Blueprint;
		}
	}, EGetObjectsFlags::None);
	return Result;
}

bool HasNamedSceneComponent(const AActor* Actor, const FName ComponentName)
{
	if (!Actor)
	{
		return false;
	}

	TArray<UActorComponent*> Components;
	Actor->GetComponents(Components);
	for (const UActorComponent* Component : Components)
	{
		if (Component && Component->GetFName() == ComponentName && Component->IsA<USceneComponent>())
		{
			return true;
		}
	}
	return false;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FComputerBlueprintLoadAutomationTest,
	"BathhouseSim.Computer.BlueprintLoad",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FComputerBlueprintLoadAutomationTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FString PackagePath;
	FParse::Value(FCommandLine::Get(), TEXT("BathhouseComputerLoadPath="), PackagePath);
	if (PackagePath.IsEmpty())
	{
		PackagePath = TEXT("/Game/Bathhouse/Blueprints/Computer/BP_BathhouseComputer");
	}
	if (!FPackageName::IsValidLongPackageName(PackagePath))
	{
		AddError(FString::Printf(TEXT("Invalid computer Blueprint package path: %s"), *PackagePath));
		return false;
	}

	UPackage* Package = LoadPackage(nullptr, *PackagePath, LOAD_None);
	UBlueprint* Blueprint = FindComputerBlueprint(Package);
	if (!TestNotNull(TEXT("Computer Blueprint package loads"), Package)
		|| !TestNotNull(TEXT("Computer Blueprint object loads"), Blueprint)
		|| !TestNotNull(TEXT("Computer Blueprint generated class loads"), Blueprint ? Blueprint->GeneratedClass.Get() : nullptr))
	{
		return false;
	}

	TestTrue(TEXT("Computer Blueprint keeps ABathhouseComputerActor as its native parent"),
		Blueprint->GeneratedClass->GetSuperClass() == ABathhouseComputerActor::StaticClass());
	ABathhouseComputerActor* ComputerCDO = Cast<ABathhouseComputerActor>(Blueprint->GeneratedClass->GetDefaultObject());
	if (!TestNotNull(TEXT("Computer Blueprint CDO loads"), ComputerCDO))
	{
		return false;
	}
	TestTrue(TEXT("Computer Blueprint CDO contains FocusExitPoint"),
		HasNamedSceneComponent(ComputerCDO, TEXT("FocusExitPoint")));
#if WITH_EDITORONLY_DATA
	TestTrue(TEXT("Editor-only FocusExitArrow is attached to the Blueprint CDO"),
		HasNamedSceneComponent(ComputerCDO, TEXT("FocusExitArrow")));
#endif
	TestEqual(TEXT("Computer Blueprint keeps the 100 cm search radius"),
		ComputerCDO->GetFocusExitSearchRadiusCm(), 100.0f);
	TestTrue(TEXT("Computer Blueprint keeps its ScreenWidget class"),
		ComputerCDO->GetScreenWidget() && IsValid(ComputerCDO->GetScreenWidget()->GetWidgetClass()));
	TestEqual(TEXT("Computer Blueprint retains the 0.35 second focus-in blend"),
		ComputerCDO->GetFocusBlendInSeconds(), 0.35f);
	TestEqual(TEXT("Computer Blueprint retains the 0.25 second focus-out blend"),
		ComputerCDO->GetFocusBlendOutSeconds(), 0.25f);

	const FObjectPropertyBase* ManagedZoneProperty = FindFProperty<FObjectPropertyBase>(
		ABathhouseComputerActor::StaticClass(), TEXT("ManagedBathPlacementZone"));
	TestNotNull(TEXT("Computer native class retains its ManagedBathPlacementZone property"), ManagedZoneProperty);
	int32 ComputerInstanceCount = 0;
	if (GEngine)
	{
		for (const FWorldContext& WorldContext : GEngine->GetWorldContexts())
		{
			UWorld* World = WorldContext.World();
			if (!World)
			{
				continue;
			}
			for (TActorIterator<ABathhouseComputerActor> It(World); It; ++It)
			{
				if (!It->GetClass()->IsChildOf(Blueprint->GeneratedClass))
				{
					continue;
				}
				++ComputerInstanceCount;
				TestTrue(TEXT("Loaded computer instance contains FocusExitPoint"),
					HasNamedSceneComponent(*It, TEXT("FocusExitPoint")));
				if (ManagedZoneProperty)
				{
					TestTrue(TEXT("Loaded computer instance retains ManagedBathPlacementZone"),
						IsValid(ManagedZoneProperty->GetObjectPropertyValue_InContainer(It.operator->())));
				}
			}
		}
	}
	AddInfo(FString::Printf(TEXT("Found %d loaded BP_BathhouseComputer world instance(s)."), ComputerInstanceCount));
	if (FParse::Param(FCommandLine::Get(), TEXT("BathhouseComputerRequireWorldInstance")))
	{
		TestTrue(TEXT("The requested map loaded a BP_BathhouseComputer instance"), ComputerInstanceCount > 0);
	}
	return true;
}

#endif
