#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ShopDeliveryPointActor.generated.h"

class UArrowComponent;
class UBillboardComponent;
class UPrimitiveComponent;
class USceneComponent;
class UShopOrderSubsystem;

UCLASS(Blueprintable)
class BATHHOUSESIM_API AShopDeliveryPointActor : public AActor
{
	GENERATED_BODY()

public:
	AShopDeliveryPointActor();
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	bool FindDropTransform(
		const FVector& BoxHalfExtent,
		const UPrimitiveComponent& BoxCollisionTemplate,
		FTransform& OutTransform) const;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shop|Delivery", meta = (ClampMin = "1.0"))
	float MaxSearchHeightCm = 1000.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Shop|Delivery", meta = (ClampMin = "0.0"))
	float DropGapCm = 10.0f;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shop")
	TObjectPtr<USceneComponent> SceneRoot;

#if WITH_EDITORONLY_DATA
	UPROPERTY()
	TObjectPtr<UBillboardComponent> EditorBillboard;

	UPROPERTY()
	TObjectPtr<UArrowComponent> EditorArrow;
#endif
};
