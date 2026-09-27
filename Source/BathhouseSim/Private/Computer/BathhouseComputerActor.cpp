#include "Computer/BathhouseComputerActor.h"
#include "Computer/ComputerScreenContext.h"
#include "Blueprint/UserWidget.h"
#include "GameFramework/Pawn.h"

#include "Camera/CameraComponent.h"
#include "Components/ArrowComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "Computer/PlayerComputerUseComponent.h"
#include "Interaction/PlayerCarryComponent.h"
#include "Facility/BathWaterOperationsSubsystem.h"
#include "Placement/FacilityPlacementZoneActor.h"


DEFINE_LOG_CATEGORY(LogBathhouseComputer);

#define LOCTEXT_NAMESPACE "BathhouseComputerActor"

ABathhouseComputerActor::ABathhouseComputerActor()
{
	PrimaryActorTick.bCanEverTick = false;

	ComputerMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ComputerMesh"));
	SetRootComponent(ComputerMesh);
	ComputerMesh->SetCollisionProfileName(TEXT("BlockAllDynamic"));

	ScreenWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("ScreenWidget"));
	ScreenWidget->SetupAttachment(ComputerMesh);
	ScreenWidget->SetWidgetSpace(EWidgetSpace::World);
	ScreenWidget->SetDrawAtDesiredSize(true);

	FocusCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FocusCamera"));
	FocusCamera->SetupAttachment(ComputerMesh);

	FocusExitPoint = CreateDefaultSubobject<USceneComponent>(TEXT("FocusExitPoint"));
	FocusExitPoint->SetupAttachment(RootComponent);
#if WITH_EDITORONLY_DATA
	FocusExitArrow = CreateEditorOnlyDefaultSubobject<UArrowComponent>(TEXT("FocusExitArrow"));
	if (FocusExitArrow)
	{
		FocusExitArrow->SetupAttachment(FocusExitPoint);
	}
#endif
}

void ABathhouseComputerActor::BeginPlay()
{
	Super::BeginPlay();
	if (ScreenWidget)
	{
		ScreenWidget->InitWidget();
		if (IComputerScreenContextReceiver* Receiver =
			Cast<IComputerScreenContextReceiver>(ScreenWidget->GetUserWidgetObject()))
		{
			FComputerScreenContext ScreenContext;
			ScreenContext.Computer = this;
			ScreenContext.Operations = GetWorld()
				? GetWorld()->GetSubsystem<UBathWaterOperationsSubsystem>()
				: nullptr;
			ScreenContext.ManagedZone = ManagedBathPlacementZone;
			Receiver->InitializeComputerScreen(ScreenContext);
		}
	}
}

void ABathhouseComputerActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UPlayerComputerUseComponent* PlayerComputerUse = CurrentUser.Get();
	CurrentUser.Reset();
	if (PlayerComputerUse)
	{
		PlayerComputerUse->HandleComputerUnavailable(this);
	}

	Super::EndPlay(EndPlayReason);
}

FPlayerInteractionQuery ABathhouseComputerActor::QueryInteraction(const FPlayerInteractionContext& Context) const
{
	FPlayerInteractionQuery Query;
	Query.bVisible = true;
	Query.TargetName = LOCTEXT("ComputerTarget", "컴퓨터");
	Query.ActionName = LOCTEXT("UseComputer", "컴퓨터 사용");

	if (!Context.CarryComponent)
	{
		Query.FailureReason = LOCTEXT("MissingCarry", "소지 상태를 확인할 수 없습니다.");
		return Query;
	}
	if (!Context.CarryComponent->IsHandEmpty())
	{
		Query.FailureReason = LOCTEXT("HandsOccupied", "손에 든 물건을 내려놓아야 합니다");
		return Query;
	}

	const UPlayerComputerUseComponent* PlayerComputerUse = ResolvePlayerComputerUse(Context);
	if (!PlayerComputerUse)
	{
		Query.FailureReason = LOCTEXT("MissingPlayerComputerUse", "컴퓨터 사용 상태를 확인할 수 없습니다.");
		return Query;
	}
	if (!IsScreenReady())
	{
		Query.FailureReason = LOCTEXT("ScreenUnavailable", "컴퓨터 화면을 사용할 수 없습니다.");
		return Query;
	}
	if (CurrentUser.IsValid() && CurrentUser.Get() != PlayerComputerUse)
	{
		Query.FailureReason = LOCTEXT("ComputerOccupied", "다른 사용자가 컴퓨터를 사용 중입니다.");
		return Query;
	}

	Query.bCanInteract = true;
	return Query;
}

FPlayerInteractionResult ABathhouseComputerActor::ExecuteInteraction(const FPlayerInteractionContext& Context)
{
	const FPlayerInteractionQuery Query = QueryInteraction(Context);
	UPlayerComputerUseComponent* PlayerComputerUse = ResolvePlayerComputerUse(Context);
	if (!Query.bCanInteract || !PlayerComputerUse)
	{
		return FPlayerInteractionResult::Failed(Query.FailureReason);
	}
	const bool bReservationAlreadyOwned = IsReservedBy(PlayerComputerUse);
	if (!TryReserveFor(PlayerComputerUse))
	{
		return FPlayerInteractionResult::Failed(LOCTEXT("ReservationFailed", "다른 사용자가 컴퓨터를 사용 중입니다."));
	}
	if (!PlayerComputerUse->BeginComputerUse(this))
	{
		if (!bReservationAlreadyOwned)
		{
			ReleaseReservation(PlayerComputerUse);
		}
		return FPlayerInteractionResult::Failed(LOCTEXT("FocusFailed", "컴퓨터 사용을 시작할 수 없습니다."));
	}
	if (ScreenWidget)
	{
		if (IComputerScreenContextReceiver* Receiver =
			Cast<IComputerScreenContextReceiver>(ScreenWidget->GetUserWidgetObject()))
		{
			APawn* UserPawn = Cast<APawn>(Context.Interactor);
			Receiver->NotifyComputerUserChanged(UserPawn ? UserPawn->GetPlayerState() : nullptr);
		}
	}

	return FPlayerInteractionResult::Succeeded();
}

bool ABathhouseComputerActor::TryReserveFor(UPlayerComputerUseComponent* PlayerComputerUse)
{
	if (!PlayerComputerUse || (CurrentUser.IsValid() && CurrentUser.Get() != PlayerComputerUse))
	{
		return false;
	}

	CurrentUser = PlayerComputerUse;
	return true;
}

void ABathhouseComputerActor::ReleaseReservation(UPlayerComputerUseComponent* PlayerComputerUse)
{
	if (PlayerComputerUse && CurrentUser.Get() == PlayerComputerUse)
	{
		CurrentUser.Reset();
	}
}

bool ABathhouseComputerActor::IsReservedBy(const UPlayerComputerUseComponent* PlayerComputerUse) const
{
	return PlayerComputerUse && CurrentUser.Get() == PlayerComputerUse;
}

FTransform ABathhouseComputerActor::GetFocusExitFootTransform() const
{
	const FTransform PointTransform = FocusExitPoint
		? FocusExitPoint->GetComponentTransform()
		: GetActorTransform();
	const FRotator PointRotation = PointTransform.Rotator();
	return FTransform(
		FRotator(PointRotation.Pitch, PointRotation.Yaw, 0.0f),
		PointTransform.GetLocation());
}

float ABathhouseComputerActor::GetFocusExitSearchRadiusCm() const
{
	if (!FMath::IsFinite(FocusExitSearchRadiusCm) || FocusExitSearchRadiusCm < 0.0f)
	{
		UE_LOG(LogBathhouseComputer, Warning,
			TEXT("%s has invalid FocusExitSearchRadiusCm (%g); treating it as 0 cm."),
			*GetPathName(),
			FocusExitSearchRadiusCm);
		return 0.0f;
	}
	return FocusExitSearchRadiusCm;
}

bool ABathhouseComputerActor::IsScreenReady() const
{
	return IsValid(ScreenWidget)
		&& IsValid(FocusCamera)
		&& IsValid(ScreenWidget->GetUserWidgetObject());
}

UPlayerComputerUseComponent* ABathhouseComputerActor::ResolvePlayerComputerUse(
	const FPlayerInteractionContext& Context) const
{
	return Context.Interactor
		? Context.Interactor->FindComponentByClass<UPlayerComputerUseComponent>()
		: nullptr;
}

#undef LOCTEXT_NAMESPACE
