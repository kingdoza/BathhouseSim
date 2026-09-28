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
#include "Shop/ShopDeliveryBoxActor.h"
#include "Shop/ShopSettings.h"
#include "Shop/ShopUnboxingPlacement.h"

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
	if (!PlayerPawn || !Capsule || !World || Context.CameraDirection.IsNearlyZero())
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
	TArray<UFacilityPlacementDefinition*> Definitions;
	int64 TotalQuantity = 0;
	for (const FShopOrderLine& Line : Box.Contents)
	{
		FText DefinitionFailure;
		if (Line.ProductId.IsNone() || !IsValid(Line.PlacementDefinition.Get())
			|| Line.DisplayName.IsEmpty() || Line.Quantity <= 0
			|| Line.PlacementDefinition->LockerSlotCount != 0
			|| !Line.PlacementDefinition->FacilityTags.HasTag(TAG_Facility_Discardable)
			|| !Line.PlacementDefinition->ValidateRuntime(DefinitionFailure))
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
		for (int32 Index = 0; Index < Line.Quantity; ++Index)
		{
			Definitions.Add(Line.PlacementDefinition);
		}
	}
	const float HalfHeight = Capsule->GetScaledCapsuleHalfHeight();
	const FVector FootLocation = PlayerPawn->GetActorLocation() - FVector::UpVector * HalfHeight;
	const float ViewYaw = Context.CameraDirection.Rotation().Yaw;
	FRandomStream RandomStream(FMath::Rand());
	TArray<FTransform> SpawnTransforms;
	if (!FShopUnboxingPlacement::FindSpawnTransforms(
		*World,
		*PlayerPawn,
		*Capsule,
		Box,
		FootLocation,
		ViewYaw,
		Definitions,
		Settings->GetUnboxForwardDistanceCm(),
		RandomStream,
		Settings->GetUnboxOverlapDepthCm(),
		SpawnTransforms,
		OutFailureReason))
	{
		return false;
	}

	TArray<APlaceableFacilityItemActor*> SpawnedItems;
	SpawnedItems.Reserve(Definitions.Num());
	for (int32 Index = 0; Index < Definitions.Num(); ++Index)
	{
		APlaceableFacilityItemActor* Item = APlaceableFacilityItemActor::SpawnFreshItem(
			*World,
			*Definitions[Index],
			SpawnTransforms[Index],
			OutFailureReason);
		if (!Item)
		{
			for (APlaceableFacilityItemActor* Spawned : SpawnedItems)
			{
				if (IsValid(Spawned))
				{
					Spawned->Destroy();
				}
			}
			return false;
		}
		FText ActivationFailure;
		if (!Item->ActivateFreeWorld(SpawnTransforms[Index], ActivationFailure))
		{
			OutFailureReason = ActivationFailure;
			Item->Destroy();
			for (APlaceableFacilityItemActor* Spawned : SpawnedItems)
			{
				if (IsValid(Spawned))
				{
					Spawned->Destroy();
				}
			}
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
		for (APlaceableFacilityItemActor* Spawned : SpawnedItems)
		{
			if (IsValid(Spawned))
			{
				Spawned->Destroy();
			}
		}
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
