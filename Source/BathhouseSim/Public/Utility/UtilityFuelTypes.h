#pragma once

#include "CoreMinimal.h"
#include "UtilityFuelTypes.generated.h"

UENUM(BlueprintType)
enum class EUtilityFuelKind : uint8
{
	None = 0,
	Coal = 1,
	DryIce = 2
};

inline bool IsSupportedUtilityFuelKind(const EUtilityFuelKind Kind)
{
	return Kind == EUtilityFuelKind::Coal || Kind == EUtilityFuelKind::DryIce;
}

inline FText GetUtilityFuelKindDisplayName(const EUtilityFuelKind Kind)
{
	switch (Kind)
	{
	case EUtilityFuelKind::Coal:
		return NSLOCTEXT("UtilityFuel", "CoalDisplayName", "석탄");
	case EUtilityFuelKind::DryIce:
		return NSLOCTEXT("UtilityFuel", "DryIceDisplayName", "드라이아이스");
	default:
		return FText::GetEmpty();
	}
}

UENUM(BlueprintType)
enum class EUtilityFuelFailure : uint8
{
	None = 0,
	InvalidLoad = 1,
	NoShovel = 2,
	ShovelAlreadyLoaded = 3,
	ShovelEmpty = 4,
	WrongFuel = 5,
	InvalidTarget = 6,
	NotInstalled = 7,
	RecoveryInProgress = 8,
	CapacityFull = 9,
	TransactionBusy = 10,
	InvalidAuthoring = 11
};

USTRUCT(BlueprintType)
struct BATHHOUSESIM_API FUtilityFuelLoad
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Utility Fuel")
	EUtilityFuelKind Kind = EUtilityFuelKind::None;

	UPROPERTY(BlueprintReadOnly, Category = "Utility Fuel")
	float Points = 0.0f;

	bool IsEmpty() const
	{
		return Kind == EUtilityFuelKind::None && Points == 0.0f;
	}

	bool IsValid() const
	{
		if (!FMath::IsFinite(Points) || Points < 0.0f)
		{
			return false;
		}
		return IsEmpty() || (IsSupportedUtilityFuelKind(Kind) && Points > 0.0f);
	}
};

USTRUCT(BlueprintType)
struct BATHHOUSESIM_API FUtilityOperationSnapshot
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Utility Operation")
	float RemainingPoints = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Utility Operation")
	float MaximumPoints = 100.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Utility Operation")
	float DecayPointsPerSecond = 1.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Utility Operation")
	bool bPlacedClockActive = false;

	UPROPERTY(BlueprintReadOnly, Category = "Utility Operation")
	bool bIsOperating = false;

	UPROPERTY(BlueprintReadOnly, Category = "Utility Operation")
	int64 Revision = 0;
};

USTRUCT(BlueprintType)
struct BATHHOUSESIM_API FUtilityFuelResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Utility Fuel")
	bool bSucceeded = false;

	UPROPERTY(BlueprintReadOnly, Category = "Utility Fuel")
	EUtilityFuelFailure Failure = EUtilityFuelFailure::None;

	UPROPERTY(BlueprintReadOnly, Category = "Utility Fuel")
	float TransferredPoints = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Utility Fuel")
	FText FailureReason;

	static FUtilityFuelResult Succeeded(const float InTransferredPoints)
	{
		FUtilityFuelResult Result;
		Result.bSucceeded = true;
		Result.TransferredPoints = InTransferredPoints;
		return Result;
	}

	static FUtilityFuelResult Failed(const EUtilityFuelFailure InFailure, const FText& InReason)
	{
		FUtilityFuelResult Result;
		Result.Failure = InFailure;
		Result.FailureReason = InReason;
		return Result;
	}
};
