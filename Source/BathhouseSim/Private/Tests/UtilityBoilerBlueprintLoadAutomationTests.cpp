#if WITH_EDITOR && WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"

#include "Components/ActorComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Blueprint.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "Engine/Engine.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Kismet2/CompilerResultsLog.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "PackageTools.h"
#include "UObject/SavePackage.h"
#include "UObject/UObjectHash.h"
#include "Utility/BathWaterBoilerFacilityActor.h"
#include "Utility/BathWaterCirculatorFacilityActor.h"
#include "Utility/BathWaterCoolerFacilityActor.h"
#include "Utility/BathWaterFuelUtilityFacilityActor.h"
#include "Utility/BathWaterLaborUtilityFacilityActor.h"
#include "Facility/BathWaterUtilityFacilityActor.h"
#include "Utility/UtilityFuelDoorComponent.h"
#include "Utility/UtilityFuelIntakeComponent.h"
#include "Utility/UtilityFuelIntakeVolumeComponent.h"
#include "Utility/UtilityGaugeComponent.h"
#include "Utility/UtilityLeverLaborComponent.h"
#include "Utility/UtilityLeverOperatingVolumeComponent.h"
#include "Utility/UtilityOperationComponent.h"

namespace
{
struct FUtilityBlueprintLoadSpec
{
	const TCHAR* Name;
	UClass* NativeParent;
	UClass* TransitionalParent;
	enum class EKind : uint8 { Boiler, Cooler, Circulator } Kind;
};

const TCHAR* GetKindLabel(const FUtilityBlueprintLoadSpec& Spec)
{
	switch (Spec.Kind)
	{
	case FUtilityBlueprintLoadSpec::EKind::Boiler: return TEXT("Boiler");
	case FUtilityBlueprintLoadSpec::EKind::Cooler: return TEXT("Cooler");
	case FUtilityBlueprintLoadSpec::EKind::Circulator: return TEXT("Circulator");
	default: return TEXT("Utility");
	}
}

UBlueprint* FindBlueprint(UPackage* Package)
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

int32 CountDirectGaugeFaceObjects(const UObject* CDO)
{
	if (!CDO)
	{
		return 0;
	}
	TArray<UObject*> Children;
	GetObjectsWithOuter(CDO, Children, EGetObjectsFlags::None);
	int32 Count = 0;
	for (const UObject* Child : Children)
	{
		if (Child && Child->GetOuter() == CDO && Child->GetFName() == TEXT("GaugeFace"))
		{
			++Count;
		}
	}
	return Count;
}

bool HasNativeGaugeFace()
{
	const ABathWaterLaborUtilityFacilityActor* NativeCDO =
		ABathWaterLaborUtilityFacilityActor::StaticClass()->GetDefaultObject<ABathWaterLaborUtilityFacilityActor>();
	TArray<UActorComponent*> Components;
	NativeCDO->GetComponents(Components);
	for (const UActorComponent* Component : Components)
	{
		if (Component && Component->GetFName() == TEXT("GaugeFace"))
		{
			return true;
		}
	}
	return false;
}

bool VerifyBlueprintContract(FAutomationTestBase& Test, const FString& PackagePath,
	const FUtilityBlueprintLoadSpec& Spec, UBlueprint* Blueprint, const bool bAfterResave)
{
	if (!Test.TestNotNull(FString::Printf(TEXT("%s Blueprint asset exists"), GetKindLabel(Spec)), Blueprint)
		|| !Test.TestNotNull(FString::Printf(TEXT("%s generated class exists"), GetKindLabel(Spec)), Blueprint ? Blueprint->GeneratedClass.Get() : nullptr))
	{
		return false;
	}

	UClass* ActualParent = Blueprint->GeneratedClass->GetSuperClass();
	const bool bCurrentNativeParent = ActualParent == Spec.NativeParent;
	const bool bKnownPreMigrationParent = Spec.TransitionalParent && ActualParent == Spec.TransitionalParent;
	Test.TestTrue(FString::Printf(TEXT("%s generated class has its expected native parent (actual: %s)"),
		GetKindLabel(Spec), ActualParent ? *ActualParent->GetName() : TEXT("None")),
		bCurrentNativeParent || bKnownPreMigrationParent);
	if (bKnownPreMigrationParent)
	{
		Test.AddWarning(FString::Printf(TEXT("%s remains on its pre-reparent native parent; authored migration is outside this C++ load check."),
			GetKindLabel(Spec)));
	}

	Test.TestFalse(TEXT("Labor native hierarchy has no GaugeFace component"), HasNativeGaugeFace());
	AActor* CDO = Cast<AActor>(Blueprint->GeneratedClass->GetDefaultObject());
	if (!Test.TestNotNull(FString::Printf(TEXT("%s Blueprint CDO exists"), GetKindLabel(Spec)), CDO))
	{
		return false;
	}
	const int32 GaugeFaceRemnants = CountDirectGaugeFaceObjects(CDO);
	if (bAfterResave)
	{
		Test.TestEqual(TEXT("Resaved and reloaded copy has no CDO-owned GaugeFace remnants"), GaugeFaceRemnants, 0);
	}
	else if (GaugeFaceRemnants > 0)
	{
		Test.AddWarning(FString::Printf(TEXT("%s has %d CDO-owned GaugeFace object(s) before migration resave."),
			GetKindLabel(Spec), GaugeFaceRemnants));
	}

	if (bCurrentNativeParent)
	{
		ABathWaterLaborUtilityFacilityActor* LaborCDO = Cast<ABathWaterLaborUtilityFacilityActor>(CDO);
		Test.TestTrue(FString::Printf(TEXT("%s CDO contains shared Operation and Gauge subobjects"), GetKindLabel(Spec)),
			LaborCDO && LaborCDO->GetOperation() && LaborCDO->GetGaugeNeedlePivot()
			&& LaborCDO->GetGaugeNeedleMesh() && LaborCDO->GetGaugePresentation());

		if (Spec.Kind == FUtilityBlueprintLoadSpec::EKind::Boiler)
		{
			ABathWaterBoilerFacilityActor* BoilerCDO = Cast<ABathWaterBoilerFacilityActor>(CDO);
			Test.TestTrue(TEXT("Boiler CDO contains legacy FuelIntake, intake volume, fuel door, and presentation"),
				BoilerCDO && BoilerCDO->GetFuelIntake()
				&& BoilerCDO->GetFuelIntake()->IsA<UUtilityFuelIntakeComponent>()
				&& BoilerCDO->GetFuelIntake()->IsA<UStaticMeshComponent>()
				&& BoilerCDO->GetFuelIntakeVolume()
				&& BoilerCDO->GetFuelDoorMesh() && BoilerCDO->GetFuelDoorPresentation());
		}
		else if (Spec.Kind == FUtilityBlueprintLoadSpec::EKind::Cooler)
		{
			ABathWaterFuelUtilityFacilityActor* FuelCDO = Cast<ABathWaterFuelUtilityFacilityActor>(CDO);
			Test.TestTrue(TEXT("Cooler CDO contains intake volume, fuel door, and presentation"),
				FuelCDO && FuelCDO->GetFuelIntakeVolume()
				&& FuelCDO->GetFuelDoorMesh() && FuelCDO->GetFuelDoorPresentation());
		}
		else
		{
			ABathWaterCirculatorFacilityActor* CirculatorCDO = Cast<ABathWaterCirculatorFacilityActor>(CDO);
			Test.TestTrue(TEXT("Circulator CDO contains operating volume, lever pivot, mesh, and labor component"),
				CirculatorCDO && CirculatorCDO->GetLeverOperatingVolume()
				&& CirculatorCDO->GetLeverPivot() && CirculatorCDO->GetLeverMesh()
				&& CirculatorCDO->GetLeverLabor());
		}
	}
	else
	{
		Test.AddWarning(FString::Printf(TEXT("%s required native subobjects will be checked after its authorized Blueprint reparent."),
			GetKindLabel(Spec)));
	}

	int32 InstanceCount = 0;
	if (GEngine)
	{
		for (const FWorldContext& WorldContext : GEngine->GetWorldContexts())
		{
			UWorld* World = WorldContext.World();
			if (!World || World->WorldType == EWorldType::EditorPreview || World->WorldType == EWorldType::Inactive)
			{
				continue;
			}
			for (TActorIterator<AActor> It(World); It; ++It)
			{
				AActor* Instance = *It;
				if (!Instance->IsA(Blueprint->GeneratedClass))
				{
					continue;
				}
				++InstanceCount;
				if (!bCurrentNativeParent)
				{
					continue;
				}

				ABathWaterLaborUtilityFacilityActor* LaborInstance = Cast<ABathWaterLaborUtilityFacilityActor>(Instance);
				Test.TestTrue(FString::Printf(TEXT("%s loaded instance retains Operation and Gauge subobjects"), GetKindLabel(Spec)),
					LaborInstance && LaborInstance->GetOperation() && LaborInstance->GetGaugeNeedlePivot()
					&& LaborInstance->GetGaugeNeedleMesh() && LaborInstance->GetGaugePresentation());
				if (Spec.Kind == FUtilityBlueprintLoadSpec::EKind::Boiler)
				{
					ABathWaterBoilerFacilityActor* BoilerInstance = Cast<ABathWaterBoilerFacilityActor>(Instance);
					Test.TestTrue(TEXT("Loaded boiler instance retains compatible FuelIntake, volume, door, and presentation components"),
						BoilerInstance && IsValid(BoilerInstance->GetFuelIntake())
						&& BoilerInstance->GetFuelIntake()->IsA<UStaticMeshComponent>()
						&& IsValid(BoilerInstance->GetFuelIntakeVolume())
						&& BoilerInstance->GetFuelIntakeVolume()->IsA<UUtilityFuelIntakeVolumeComponent>()
						&& IsValid(BoilerInstance->GetFuelDoorMesh())
						&& IsValid(BoilerInstance->GetFuelDoorPresentation()));
				}
				else if (Spec.Kind == FUtilityBlueprintLoadSpec::EKind::Cooler)
				{
					ABathWaterFuelUtilityFacilityActor* FuelInstance = Cast<ABathWaterFuelUtilityFacilityActor>(Instance);
					Test.TestTrue(TEXT("Loaded cooler instance retains intake volume, door, and presentation components"),
						FuelInstance && IsValid(FuelInstance->GetFuelIntakeVolume())
						&& IsValid(FuelInstance->GetFuelDoorMesh())
						&& IsValid(FuelInstance->GetFuelDoorPresentation()));
				}
				else
				{
					ABathWaterCirculatorFacilityActor* CirculatorInstance = Cast<ABathWaterCirculatorFacilityActor>(Instance);
					Test.TestTrue(TEXT("Loaded circulator instance retains operating volume, lever pivot, mesh, and labor component"),
						CirculatorInstance && IsValid(CirculatorInstance->GetLeverOperatingVolume())
						&& IsValid(CirculatorInstance->GetLeverPivot()) && IsValid(CirculatorInstance->GetLeverMesh())
						&& IsValid(CirculatorInstance->GetLeverLabor()));
				}
			}
		}
	}
	if (InstanceCount == 0)
	{
		Test.AddWarning(FString::Printf(TEXT("No %s instance is loaded in the active world; Blueprint CDO compatibility was checked."),
			GetKindLabel(Spec)));
	}
	return true;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUtilityBoilerBlueprintLoadAutomationTest,
	"BathhouseSim.Utility.Labor.BoilerBlueprintLoad",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUtilityBoilerBlueprintLoadAutomationTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	const FUtilityBlueprintLoadSpec Specs[] =
	{
		{ TEXT("BP_Boiler"), ABathWaterBoilerFacilityActor::StaticClass(), nullptr, FUtilityBlueprintLoadSpec::EKind::Boiler },
		{ TEXT("BP_Cooler"), ABathWaterCoolerFacilityActor::StaticClass(), ABathWaterUtilityFacilityActor::StaticClass(), FUtilityBlueprintLoadSpec::EKind::Cooler },
		{ TEXT("BP_Circulator"), ABathWaterCirculatorFacilityActor::StaticClass(), ABathWaterUtilityFacilityActor::StaticClass(), FUtilityBlueprintLoadSpec::EKind::Circulator }
	};
	TMap<FString, const FUtilityBlueprintLoadSpec*> SpecsByName;
	for (const FUtilityBlueprintLoadSpec& Spec : Specs)
	{
		SpecsByName.Add(Spec.Name, &Spec);
	}

	FString RequestedPath;
	FParse::Value(FCommandLine::Get(), TEXT("BathhouseBoilerLoadPath="), RequestedPath);
	TArray<FString> PackagePaths;
	if (!RequestedPath.IsEmpty())
	{
		PackagePaths.Add(RequestedPath);
	}
	else
	{
		PackagePaths = {
			TEXT("/Game/Bathhouse/Blueprints/Facility/BP_Boiler"),
			TEXT("/Game/Bathhouse/Blueprints/Facility/BP_Cooler"),
			TEXT("/Game/Bathhouse/Blueprints/Facility/BP_Circulator")
		};
	}

	const bool bResaveCopy = FParse::Param(FCommandLine::Get(), TEXT("BathhouseLoadCheckResave"));
	bool bAllSucceeded = true;
	for (const FString& PackagePath : PackagePaths)
	{
		if (!FPackageName::IsValidLongPackageName(PackagePath))
		{
			AddError(FString::Printf(TEXT("Invalid utility Blueprint package path: %s"), *PackagePath));
			bAllSucceeded = false;
			continue;
		}
		if (bResaveCopy && !PackagePath.StartsWith(TEXT("/Game/Developers/MigrationCheck/"), ESearchCase::IgnoreCase))
		{
			AddError(FString::Printf(TEXT("Refusing Compile/Save/reload outside /Game/Developers/MigrationCheck/: %s"), *PackagePath));
			bAllSucceeded = false;
			continue;
		}

		const FString AssetName = FPackageName::GetShortName(PackagePath);
		const FUtilityBlueprintLoadSpec* const* SpecPointer = SpecsByName.Find(AssetName);
		if (!SpecPointer && AssetName == TEXT("BP_Boiler_LoadCheck"))
		{
			SpecPointer = SpecsByName.Find(TEXT("BP_Boiler"));
		}
		if (!SpecPointer)
		{
			AddError(FString::Printf(TEXT("No utility Blueprint contract is defined for package: %s"), *PackagePath));
			bAllSucceeded = false;
			continue;
		}
		const FUtilityBlueprintLoadSpec& Spec = **SpecPointer;
		UPackage* Package = LoadPackage(nullptr, *PackagePath, LOAD_None);
		UBlueprint* Blueprint = FindBlueprint(Package);
		if (!TestNotNull(FString::Printf(TEXT("%s package loads: %s"), GetKindLabel(Spec), *PackagePath), Package)
			|| !TestNotNull(FString::Printf(TEXT("%s Blueprint object loads"), GetKindLabel(Spec)), Blueprint))
		{
			bAllSucceeded = false;
			continue;
		}
		bAllSucceeded &= VerifyBlueprintContract(*this, PackagePath, Spec, Blueprint, false);

		if (!bResaveCopy)
		{
			continue;
		}

		FCompilerResultsLog CompileResults;
		FKismetEditorUtilities::CompileBlueprint(Blueprint, EBlueprintCompileOptions::None, &CompileResults);
		if (!TestEqual(FString::Printf(TEXT("%s migration copy compiles without errors"), GetKindLabel(Spec)),
			CompileResults.NumErrors, 0))
		{
			FString CompilerMessages;
			for (const TSharedRef<FTokenizedMessage>& Message : CompileResults.Messages)
			{
				CompilerMessages += Message->ToText().ToString() + LINE_TERMINATOR;
			}
			AddError(CompilerMessages);
			bAllSucceeded = false;
			continue;
		}

		const FString Filename = FPackageName::LongPackageNameToFilename(PackagePath, FPackageName::GetAssetPackageExtension());
		FSavePackageArgs SaveArgs;
		SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
		SaveArgs.bSlowTask = false;
		if (!TestTrue(FString::Printf(TEXT("%s migration copy saves to %s"), GetKindLabel(Spec), *Filename),
			UPackage::SavePackage(Package, Blueprint, *Filename, SaveArgs)))
		{
			bAllSucceeded = false;
			continue;
		}

		FText ReloadFailure;
		TArray<UPackage*> PackagesToReload{ Package };
		const bool bReloaded = UPackageTools::ReloadPackages(
			PackagesToReload, ReloadFailure, EReloadPackagesInteractionMode::AssumePositive);
		if (!TestTrue(FString::Printf(TEXT("%s migration copy reloads after save (%s)"),
			GetKindLabel(Spec), *ReloadFailure.ToString()), bReloaded))
		{
			bAllSucceeded = false;
			continue;
		}

		UPackage* ReloadedPackage = LoadPackage(nullptr, *PackagePath, LOAD_None);
		UBlueprint* ReloadedBlueprint = FindBlueprint(ReloadedPackage);
		if (!TestNotNull(TEXT("Saved migration copy reloads as a Blueprint"), ReloadedBlueprint))
		{
			bAllSucceeded = false;
			continue;
		}
		bAllSucceeded &= VerifyBlueprintContract(*this, PackagePath, Spec, ReloadedBlueprint, true);
	}
	return bAllSucceeded;
}
#endif