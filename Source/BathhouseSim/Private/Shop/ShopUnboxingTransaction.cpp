#include "Shop/ShopUnboxingTransaction.h"

#include "Templates/UnrealTemplate.h"

#include "Components/CapsuleComponent.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "Interaction/HeldEquipmentUsable.h"
#include "Interaction/PlayerCarryComponent.h"
#include "Interaction/PlayerInteractionComponent.h"
#include "Placement/FacilityPlacementTypes.h"
#include "Placement/FacilityPlacementDefinition.h"
#include "Placement/PlaceableFacilityItemActor.h"
#include "Service/ItemBoxActor.h"
#include "Service/ServiceItemDefinition.h"
#include "Shop/ShopDeliveryBoxActor.h"
#include "Shop/ShopProductRules.h"
#include "Shop/ShopSettings.h"
#include "Shop/ShopUnboxingPlacement.h"
#include "Shop/ShopUnboxingTuning.h"

#define LOCTEXT_NAMESPACE "ShopUnboxingTransaction"

bool FShopUnboxingTransaction::Open(
	AShopDeliveryBoxActor& Box,
	const FHeldEquipmentUseContext& Context,
	FText& OutFailureReason)
{
	if (Box.bOpening || Context.Equipment != &Box || !Context.User
		|| !Context.CarryComponent || Context.CarryComponent->GetOwner() != Context.User
		|| Context.CarryComponent->GetHeldObject() != &Box
		|| !Context.InteractionComponent
		|| Context.InteractionComponent->GetOwner() != Context.User
		|| Context.InteractionComponent->IsInteractionSuppressed()
		|| !Box.bContentsInitialized || Box.Lifecycle != AShopDeliveryBoxActor::ELifecycle::Held)
	{
		OutFailureReason = LOCTEXT("InvalidOpenState", "배송 상자를 열 수 있는 상태가 아닙니다.");
		return false;
	}
	TGuardValue<bool> OpenGuard(Box.bOpening, true);
	APawn* PlayerPawn = Cast<APawn>(Context.User);
	UCapsuleComponent* Capsule = PlayerPawn ? PlayerPawn->FindComponentByClass<UCapsuleComponent>() : nullptr;
	UWorld* World = Box.GetWorld();
	if (!PlayerPawn || !Capsule || !World || Context.CameraDirection.IsNearlyZero()
		|| Context.CameraOrigin.ContainsNaN())
	{
		OutFailureReason = LOCTEXT("MissingUnboxContext", "상자 개봉 위치를 계산할 플레이어 정보가 없습니다.");
		return false;
	}

	const UShopSettings* Settings = GetDefault<UShopSettings>();
	if (!Settings)
	{
		OutFailureReason = LOCTEXT("MissingUnboxSettings", "상점 설정을 확인할 수 없습니다.");
		return false;
	}
	// Every unboxed unit is either a facility item or one full item box.
	struct FUnboxUnit
	{
		UFacilityPlacementDefinition* Facility = nullptr;
		UServiceItemDefinition* BoxKind = nullptr;
	};
	TArray<FUnboxUnit> Units;
	TArray<FShopUnboxItemShape> Shapes;
	TSubclassOf<AItemBoxActor> ItemBoxClass;
	int64 TotalQuantity = 0;
	for (const FShopOrderLine& Line : Box.Contents)
	{
		FText DefinitionFailure;
		if (!FShopProductRules::ValidateOrderLine(Line, DefinitionFailure))
		{
			OutFailureReason = LOCTEXT("InvalidBoxContents", "배송 상자의 주문 내용이 올바르지 않습니다.");
			return false;
		}
		TotalQuantity += Line.Quantity;
		if (TotalQuantity > Settings->GetCartTotalQuantityLimit())
		{
			OutFailureReason = LOCTEXT("ExcessiveBoxContents", "배송 상자 상품 수량이 허용 한도를 초과했습니다.");
			return false;
		}
		FShopUnboxItemShape LineShape;
		if (Line.ItemBoxDefinition)
		{
			if (!ItemBoxClass)
			{
				ItemBoxClass = Settings->LoadItemBoxClass();
			}
			if (!ItemBoxClass)
			{
				OutFailureReason = LOCTEXT("MissingItemBoxClass", "품목 박스 클래스가 설정되지 않았습니다.");
				return false;
			}
			if (!FShopUnboxItemShape::FromItemBoxClass(ItemBoxClass, LineShape, OutFailureReason))
			{
				return false;
			}
		}
		else if (!FShopUnboxItemShape::FromFacilityDefinition(*Line.PlacementDefinition, LineShape, OutFailureReason))
		{
			return false;
		}
		for (int32 Index = 0; Index < Line.Quantity; ++Index)
		{
			FUnboxUnit& Unit = Units.AddDefaulted_GetRef();
			Unit.Facility = Line.PlacementDefinition;
			Unit.BoxKind = Line.ItemBoxDefinition;
			Shapes.Add(LineShape);
		}
	}
	const float HalfHeight = Capsule->GetScaledCapsuleHalfHeight();
	FShopUnboxingPlacementRequest PlacementRequest;
	PlacementRequest.CameraOrigin = Context.CameraOrigin;
	PlacementRequest.CameraDirection = Context.CameraDirection.GetSafeNormal();
	PlacementRequest.FootLocation = PlayerPawn->GetActorLocation() - FVector::UpVector * HalfHeight;
	const FShopUnboxingTuning Tuning = FShopUnboxingTuning::FromSettings(*Settings);
	FRandomStream RandomStream(FMath::Rand());
	TArray<FTransform> SpawnTransforms;
	if (!FShopUnboxingPlacement::FindSpawnTransforms(
		*World,
		*PlayerPawn,
		*Capsule,
		Box,
		Shapes,
		PlacementRequest,
		Tuning,
		RandomStream,
		SpawnTransforms,
		OutFailureReason))
	{
		return false;
	}

	TArray<AActor*> SpawnedItems;
	SpawnedItems.Reserve(Units.Num());
	auto DestroySpawned = [&SpawnedItems]()
	{
		for (AActor* Spawned : SpawnedItems)
		{
			if (IsValid(Spawned))
			{
				Spawned->Destroy();
			}
		}
	};
	for (int32 Index = 0; Index < Units.Num(); ++Index)
	{
		FText ActivationFailure;
		if (Units[Index].BoxKind)
		{
			AItemBoxActor* ItemBox = AItemBoxActor::SpawnFilledBox(
				*World,
				ItemBoxClass,
				Units[Index].BoxKind,
				Units[Index].BoxKind->BoxCapacity,
				SpawnTransforms[Index],
				OutFailureReason);
			if (!ItemBox)
			{
				DestroySpawned();
				return false;
			}
			if (!ItemBox->ActivateFreeWorld(SpawnTransforms[Index], ActivationFailure))
			{
				OutFailureReason = ActivationFailure;
				ItemBox->Destroy();
				DestroySpawned();
				return false;
			}
			SpawnedItems.Add(ItemBox);
			continue;
		}
		APlaceableFacilityItemActor* Item = APlaceableFacilityItemActor::SpawnFreshItem(
			*World,
			*Units[Index].Facility,
			SpawnTransforms[Index],
			OutFailureReason);
		if (!Item)
		{
			DestroySpawned();
			return false;
		}
		if (!Item->ActivateFreeWorld(SpawnTransforms[Index], ActivationFailure))
		{
			OutFailureReason = ActivationFailure;
			Item->Destroy();
			DestroySpawned();
			return false;
		}
		SpawnedItems.Add(Item);
	}

	UPlayerCarryComponent* Carry = Context.CarryComponent;
	const bool bConsumed = Carry->CommitConsumeHeldObject(&Box, [&Box, Carry]()
	{
		return Box.Lifecycle == AShopDeliveryBoxActor::ELifecycle::Held
			&& Box.Carrier.Get() == Carry
			&& Box.bContentsInitialized;
	});
	if (!bConsumed)
	{
		DestroySpawned();
		OutFailureReason = LOCTEXT("BoxConsumeFailed", "상자 소지 상태가 변경되어 개봉을 취소했습니다.");
		return false;
	}

	Box.Carrier.Reset();
	Box.Contents.Reset();
	Box.bContentsInitialized = false;
	Box.Lifecycle = AShopDeliveryBoxActor::ELifecycle::Consumed;
	Box.Destroy();
	return true;
}

#undef LOCTEXT_NAMESPACE
