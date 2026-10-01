#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "ComputerWheelScrollTestProbe.generated.h"

// Counts UMG dynamic delegate callbacks for the computer wheel scroll automation test.
UCLASS(Transient, NotBlueprintable)
class UBathhouseComputerWheelProbe : public UObject
{
	GENERATED_BODY()

public:
	UFUNCTION()
	void HandleClicked() { ++ClickCount; }

	UFUNCTION()
	void HandleSliderChanged(float Value)
	{
		(void)Value;
		++SliderChangeCount;
	}

	int32 ClickCount = 0;
	int32 SliderChangeCount = 0;
};
