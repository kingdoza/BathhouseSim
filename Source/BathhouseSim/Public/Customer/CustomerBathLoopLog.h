#pragma once

#include "CoreMinimal.h"

DECLARE_LOG_CATEGORY_EXTERN(LogBathhouseCustomerBath, Log, All);

enum class ECustomerBathLoopLogLevel : uint8
{
	Verbose,
	Log,
	Warning
};

enum class ECustomerBathLoopReason : uint8
{
	NoBathCandidate,
	WaterBelowThreshold,
	NoAvailableSlot,
	ReservationLost,
	NavigationFailed,
	EntryValidationFailed,
	DwellCompleted,
	BathStayExpired,
	SearchExpired,
	KnockdownInterrupted,
	StateTreeExited,
	TechnicalFailure
};

BATHHOUSESIM_API const TCHAR* LexToString(ECustomerBathLoopReason Reason);
