#include "Service/DisplayCueComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Actor.h"
#include "Materials/MaterialInterface.h"
#include "Service/ServiceDisplaySettings.h"

UDisplayCueComponent::UDisplayCueComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UDisplayCueComponent::OnRegister()
{
	Super::OnRegister();
	EnsureChildren();
}

void UDisplayCueComponent::OnUnregister()
{
	HideAll();
	Super::OnUnregister();
}

void UDisplayCueComponent::OnComponentDestroyed(bool bDestroyingHierarchy)
{
	DestroyChildren();
	Super::OnComponentDestroyed(bDestroyingHierarchy);
}

void UDisplayCueComponent::EndPlay(const EEndPlayReason::Type Reason)
{
	HideAll();
	Super::EndPlay(Reason);
}

void UDisplayCueComponent::EnsureChildren()
{
	if (!GetOwner() || !GetWorld())
	{
		return;
	}
	auto Create = [this]()
	{
		auto* Child = NewObject<UStaticMeshComponent>(GetOwner(), NAME_None, RF_Transient);
		Child->SetupAttachment(this);
		Child->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Child->SetCanEverAffectNavigation(false);
		Child->SetGenerateOverlapEvents(false);
		Child->CastShadow = false;
		Child->SetVisibility(false);
		Child->RegisterComponent();
		return Child;
	};
	if (!IsValid(InsertPreview))
	{
		InsertPreview = Create();
	}
	if (!IsValid(TakeHighlightProxy))
	{
		TakeHighlightProxy = Create();
		TakeHighlightProxy->SetRenderInMainPass(false);
		TakeHighlightProxy->SetRenderInDepthPass(false);
		TakeHighlightProxy->SetRenderCustomDepth(true);
		TakeHighlightProxy->SetCustomDepthStencilValue(
			GetDefault<UServiceDisplaySettings>()->GetTakeHighlightStencilValue());
	}
}

void UDisplayCueComponent::DestroyChildren()
{
	for (UStaticMeshComponent* Child : {InsertPreview.Get(), TakeHighlightProxy.Get()})
	{
		if (IsValid(Child))
		{
			Child->DestroyComponent();
		}
	}
	InsertPreview = nullptr;
	TakeHighlightProxy = nullptr;
}

void UDisplayCueComponent::HideInsertPreview()
{
	if (InsertPreview)
	{
		InsertPreview->SetVisibility(false);
	}
}

void UDisplayCueComponent::HideTakeHighlight()
{
	if (TakeHighlightProxy)
	{
		TakeHighlightProxy->SetVisibility(false);
	}
}

void UDisplayCueComponent::HideAll()
{
	HideInsertPreview();
	HideTakeHighlight();
}

void UDisplayCueComponent::ShowInsertPreview(UStaticMesh* Mesh, const FTransform& Transform)
{
	HideInsertPreview();
	EnsureChildren();
	if (!Mesh || !InsertPreview)
	{
		return;
	}
	auto* Material = GetDefault<UServiceDisplaySettings>()->LoadInsertPreviewMaterial();
	if (!Material)
	{
		if (!bWarnedMissingPreviewMaterial)
		{
			bWarnedMissingPreviewMaterial = true;
			UE_LOG(LogTemp, Warning,
				   TEXT("ServiceDisplaySettings.InsertPreviewMaterial is not set; display insert preview is omitted."));
		}
		return;
	}
	InsertPreview->SetStaticMesh(Mesh);
	for (int32 Index = 0; Index < InsertPreview->GetNumMaterials(); ++Index)
	{
		InsertPreview->SetMaterial(Index, Material);
	}
	InsertPreview->SetRelativeTransform(Transform);
	InsertPreview->SetVisibility(true);
}

void UDisplayCueComponent::ShowTakeHighlight(UStaticMesh* Mesh, const FTransform& Transform)
{
	HideTakeHighlight();
	EnsureChildren();
	const auto* Settings = GetDefault<UServiceDisplaySettings>();
	if (!Mesh || !TakeHighlightProxy || !Settings->ShouldShowTakeHighlight())
	{
		return;
	}
	TakeHighlightProxy->SetStaticMesh(Mesh);
	TakeHighlightProxy->SetCustomDepthStencilValue(Settings->GetTakeHighlightStencilValue());
	TakeHighlightProxy->SetRelativeTransform(Transform);
	TakeHighlightProxy->SetVisibility(true);
}
