#include "UI/BathWaterManagementScreenWidget.h"
#include "Facility/BathWaterOperationsSubsystem.h"

#include "Facility/BathhouseBathFacilityActor.h"
#include "Engine/World.h"
#include "Placement/FacilityPlacementZoneActor.h"
#include "UI/BathWaterCapacitySummaryWidget.h"
#include "UI/BathWaterDetailWidget.h"
#include "UI/BathWaterMapWidget.h"

void UBathWaterManagementScreenWidget::InitializeComputerScreen(const FComputerScreenContext& Context)
{
	InitializeManagementContext(Context.Operations.Get(), Context.ManagedZone.Get());
}

void UBathWaterManagementScreenWidget::InitializeManagementContext(
	UBathWaterOperationsSubsystem* InOperations,
	AFacilityPlacementZoneActor* InZone)
{
	UnbindContext();
	Operations = InOperations;
	PlacementZone = InZone;
	SelectedBath.Reset();
	BindContext();
	RefreshSnapshot();
}

void UBathWaterManagementScreenWidget::NativeConstruct()
{
	Super::NativeConstruct();
	BindContext();
	if (BathMap)
	{
		BathMap->OnSelectionChanged.RemoveAll(this);
		BathMap->OnSelectionChanged.AddUObject(this, &UBathWaterManagementScreenWidget::HandleSelectionChanged);
		BathMap->SetPlacementZone(PlacementZone.Get());
	}
	if (BathDetail)
	{
		BathDetail->SetOperationsContext(Operations.Get());
	}
	RefreshSnapshot();
}

void UBathWaterManagementScreenWidget::NativeDestruct()
{
	if (BathMap)
	{
		BathMap->OnSelectionChanged.RemoveAll(this);
	}
	UnbindContext();
	SelectedBath.Reset();
	Super::NativeDestruct();
}

void UBathWaterManagementScreenWidget::NativeTick(const FGeometry& MyGeometry, const float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	(void)InDeltaTime;
	RefreshSnapshot();
}

void UBathWaterManagementScreenWidget::BindContext()
{
	if (Operations.IsValid() && !OperationsChangedHandle.IsValid())
	{
		OperationsChangedHandle = Operations->OnOperationsChanged.AddUObject(
			this, &UBathWaterManagementScreenWidget::HandleOperationsChanged);
	}
	if (BathMap) BathMap->SetPlacementZone(PlacementZone.Get());
	if (BathDetail) BathDetail->SetOperationsContext(Operations.Get());
}

void UBathWaterManagementScreenWidget::UnbindContext()
{
	if (Operations.IsValid() && OperationsChangedHandle.IsValid())
	{
		Operations->OnOperationsChanged.Remove(OperationsChangedHandle);
	}
	OperationsChangedHandle.Reset();
}

void UBathWaterManagementScreenWidget::HandleOperationsChanged()
{
	RefreshSnapshot();
}

void UBathWaterManagementScreenWidget::HandleSelectionChanged(ABathhouseBathFacilityActor* Bath)
{
	SelectedBath = Bath;
	RefreshSnapshot();
}

void UBathWaterManagementScreenWidget::RefreshSnapshot()
{
	const UWorld* World = GetWorld();
	if (!World || World->bIsTearingDown)
	{
		return;
	}
	if (!Operations.IsValid() || !PlacementZone.IsValid())
	{
		SelectedBath.Reset();
		if (BathMap) BathMap->ClearTiles();
		if (BathDetail) BathDetail->ApplyBathSnapshot(nullptr);
		return;
	}
	const FBathWaterOperationsSnapshot Snapshot = Operations->GetSnapshot();
	const FBathWaterBathSnapshot* SelectedSnapshot = nullptr;
	for (const FBathWaterBathSnapshot& BathSnapshot : Snapshot.Baths)
	{
		if (BathSnapshot.BathActor == SelectedBath)
		{
			SelectedSnapshot = &BathSnapshot;
			break;
		}
	}
	if (SelectedBath.IsValid() && !SelectedSnapshot)
	{
		SelectedBath.Reset();
	}
	if (CapacitySummary) CapacitySummary->ApplyCapacitySnapshot(Snapshot);
	if (BathMap) BathMap->ApplySnapshot(Snapshot, SelectedBath.Get());
	if (BathDetail) BathDetail->ApplyBathSnapshot(SelectedSnapshot);
}
