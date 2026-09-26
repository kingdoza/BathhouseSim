#include "Utility/UtilityFuelDoorComponent.h"

#include "Components/SceneComponent.h"
#include "Engine/World.h"
#include "Utility/UtilityPivotRotation.h"

#if WITH_EDITOR
#include "UObject/ObjectSaveContext.h"
#endif

#define LOCTEXT_NAMESPACE "UtilityFuelDoorComponent"

UUtilityFuelDoorComponent::UUtilityFuelDoorComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	PivotRotation = new FUtilityPivotRotation();
}

UUtilityFuelDoorComponent::~UUtilityFuelDoorComponent()
{
	delete PivotRotation;
	PivotRotation = nullptr;
}

void UUtilityFuelDoorComponent::BeginPlay()
{
	Super::BeginPlay();
	ApplyClosedImmediately();
}

void UUtilityFuelDoorComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ApplyClosedImmediately();
	if (HasBegunPlay())
	{
		Super::EndPlay(EndPlayReason);
	}
}

void UUtilityFuelDoorComponent::TickComponent(
	const float DeltaTime,
	const ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	RecalculateTargetAlpha();
	if (OpenAlpha == TargetOpenAlpha)
	{
		SetComponentTickEnabled(false);
		return;
	}

	const float Duration = TargetOpenAlpha > OpenAlpha ? OpenSeconds : CloseSeconds;
	if (!FMath::IsFinite(Duration) || Duration <= 0.0f)
	{
		OpenAlpha = TargetOpenAlpha;
	}
	else
	{
		const float Step = FMath::Max(0.0f, DeltaTime) / Duration;
		const float Remaining = FMath::Abs(TargetOpenAlpha - OpenAlpha);
		if (Step >= Remaining)
		{
			OpenAlpha = TargetOpenAlpha;
		}
		else
		{
			OpenAlpha += FMath::Sign(TargetOpenAlpha - OpenAlpha) * Step;
		}
	}

	OpenAlpha = FMath::Clamp(OpenAlpha, 0.0f, 1.0f);
	ApplyCurrentPose();
	if (OpenAlpha == TargetOpenAlpha)
	{
		SetComponentTickEnabled(false);
	}
}

void UUtilityFuelDoorComponent::Configure(USceneComponent* InPivot)
{
	if (Pivot == InPivot)
	{
		return;
	}
	Pivot = InPivot;
	PivotRotation->Forget();
	SourceInsertability.Reset();
	OpenAlpha = 0.0f;
	TargetOpenAlpha = 0.0f;
	bEditorPreviewPose = false;
	SetComponentTickEnabled(false);
}

void UUtilityFuelDoorComponent::SetSourceInsertable(const UObject* Source, const bool bInsertable)
{
	if (!IsValid(Source))
	{
		return;
	}
	const TWeakObjectPtr<UObject> WeakSource(const_cast<UObject*>(Source));
	SourceInsertability.FindOrAdd(WeakSource) = bInsertable;
	RecalculateTargetAlpha();
}

void UUtilityFuelDoorComponent::RemoveSource(const UObject* Source)
{
	if (Source)
	{
		SourceInsertability.Remove(TWeakObjectPtr<UObject>(const_cast<UObject*>(Source)));
	}
	RecalculateTargetAlpha();
}

void UUtilityFuelDoorComponent::ApplyClosedImmediately()
{
	SourceInsertability.Reset();
	OpenAlpha = 0.0f;
	TargetOpenAlpha = 0.0f;
	bEditorPreviewPose = false;
	SetComponentTickEnabled(false);
	if (PivotRotation)
	{
		PivotRotation->CaptureBaseline(Pivot);
		PivotRotation->Reset(Pivot);
	}
}

bool UUtilityFuelDoorComponent::HasValidAuthoring(FText& OutFailureReason) const
{
	if (!IsValid(Pivot)
		|| !FMath::IsFinite(LocalRotationAxis.X) || !FMath::IsFinite(LocalRotationAxis.Y)
		|| !FMath::IsFinite(LocalRotationAxis.Z) || LocalRotationAxis.IsNearlyZero()
		|| !FMath::IsFinite(OpenAngleDegrees) || OpenAngleDegrees == 0.0f
		|| !FMath::IsFinite(OpenSeconds) || OpenSeconds < 0.0f
		|| !FMath::IsFinite(CloseSeconds) || CloseSeconds < 0.0f)
	{
		OutFailureReason = LOCTEXT(
			"InvalidFuelDoorAuthoring",
			"투입구 문 pivot, 회전축, 열림 각도 또는 전환 시간이 올바르지 않습니다.");
		return false;
	}
	OutFailureReason = FText::GetEmpty();
	return true;
}

void UUtilityFuelDoorComponent::PreviewOpenPose()
{
	if (!IsEditorPreviewWorld())
	{
		return;
	}
	PivotRotation->CaptureBaseline(Pivot);
	bEditorPreviewPose = true;
	OpenAlpha = 1.0f;
	TargetOpenAlpha = 1.0f;
	SetComponentTickEnabled(false);
	ApplyCurrentPose();
}

void UUtilityFuelDoorComponent::RestoreClosedPose()
{
	if (IsEditorPreviewWorld())
	{
		ApplyClosedImmediately();
	}
}

#if WITH_EDITOR
void UUtilityFuelDoorComponent::PreSave(const FObjectPreSaveContext SaveContext)
{
	if (bEditorPreviewPose && IsEditorPreviewWorld())
	{
		ApplyClosedImmediately();
	}
	Super::PreSave(SaveContext);
}
#endif

bool UUtilityFuelDoorComponent::IsEditorPreviewWorld() const
{
	const UWorld* World = GetWorld();
	return World && (World->WorldType == EWorldType::Editor || World->WorldType == EWorldType::EditorPreview);
}

void UUtilityFuelDoorComponent::RecalculateTargetAlpha()
{
	bool bAnySourceInsertable = false;
	for (auto It = SourceInsertability.CreateIterator(); It; ++It)
	{
		if (!It.Key().IsValid())
		{
			It.RemoveCurrent();
			continue;
		}
		bAnySourceInsertable |= It.Value();
	}

	TargetOpenAlpha = bAnySourceInsertable ? 1.0f : 0.0f;
	if (OpenAlpha == TargetOpenAlpha)
	{
		SetComponentTickEnabled(false);
		return;
	}

	const float Duration = TargetOpenAlpha > OpenAlpha ? OpenSeconds : CloseSeconds;
	if (!FMath::IsFinite(Duration) || Duration <= 0.0f)
	{
		OpenAlpha = TargetOpenAlpha;
		ApplyCurrentPose();
		SetComponentTickEnabled(false);
		return;
	}
	SetComponentTickEnabled(true);
}

void UUtilityFuelDoorComponent::ApplyCurrentPose()
{
	if (PivotRotation)
	{
		PivotRotation->Apply(Pivot, LocalRotationAxis, OpenAngleDegrees * OpenAlpha);
	}
}

#undef LOCTEXT_NAMESPACE
