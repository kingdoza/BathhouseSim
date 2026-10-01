#pragma once
#include "Service/MassageChairActor.h"
#include "Service/TelevisionActor.h"
#include "Service/ScrubTableActor.h"
#include "UI/ScrubFocusHudWidget.h"
#include "ServiceAmenityAutomationTestProbe.generated.h"
UCLASS(Transient, NotBlueprintable)

class AServiceAmenityChairProbe : public AMassageChairActor
{
	GENERATED_BODY()
public:

	AServiceAmenityChairProbe();
	virtual void OnConstruction(const FTransform& Transform) override;

	void ConfigureUse(float Seconds, float Chance)
	{
		UseSeconds = Seconds;
		BreakChancePercent = Chance;
	}
};
UCLASS(Transient, NotBlueprintable)

class AServiceAmenityBenchProbe : public ABathhouseFacilityActor
{
	GENERATED_BODY()
public:

	AServiceAmenityBenchProbe();
	virtual void OnConstruction(const FTransform& Transform) override;
};
UCLASS(Transient, NotBlueprintable)

class AServiceAmenityTableProbe : public AScrubTableActor
{
	GENERATED_BODY()
public:

	AServiceAmenityTableProbe();
	virtual void OnConstruction(const FTransform& Transform) override;

	void ConfigureScrub(float Distance, float Wait, float BlendIn = 0, float BlendOut = 0)
	{
		RequiredRubDistanceCm = Distance;
		WaitLimitSeconds = Wait;
		FocusBlendInSeconds = BlendIn;
		FocusBlendOutSeconds = BlendOut;
	}

	void ClearCashClass()
	{
		CashOfferClass = nullptr;
	}

	USceneComponent* GetExitPoint() const
	{
		return ScrubExitPoint;
	}

	USceneComponent* GetStandPoint() const
	{
		return CashStandPoint;
	}
};
UCLASS(Transient, NotBlueprintable)

class UServiceAmenityHudProbe : public UScrubFocusHudWidget
{
	GENERATED_BODY()
public:

	void BuildForTest();

	void PollForTest()
	{
		NativeTick(GetCachedGeometry(), 0);
	}

	float GetRatioForTest() const;
	FString GetWaitForTest() const;
};
