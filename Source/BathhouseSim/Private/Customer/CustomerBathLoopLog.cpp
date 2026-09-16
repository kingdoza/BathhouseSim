#include "Customer/CustomerBathLoopLog.h"

DEFINE_LOG_CATEGORY(LogBathhouseCustomerBath);

const TCHAR* LexToString(const ECustomerBathLoopReason Reason)
{
	switch (Reason)
	{
	case ECustomerBathLoopReason::NoBathCandidate: return TEXT("NoBathCandidate");
	case ECustomerBathLoopReason::WaterBelowThreshold: return TEXT("WaterBelowThreshold");
	case ECustomerBathLoopReason::NoAvailableSlot: return TEXT("NoAvailableSlot");
	case ECustomerBathLoopReason::ReservationLost: return TEXT("ReservationLost");
	case ECustomerBathLoopReason::NavigationFailed: return TEXT("NavigationFailed");
	case ECustomerBathLoopReason::EntryValidationFailed: return TEXT("EntryValidationFailed");
	case ECustomerBathLoopReason::DwellCompleted: return TEXT("DwellCompleted");
	case ECustomerBathLoopReason::BathStayExpired: return TEXT("BathStayExpired");
	case ECustomerBathLoopReason::SearchExpired: return TEXT("SearchExpired");
	case ECustomerBathLoopReason::KnockdownInterrupted: return TEXT("KnockdownInterrupted");
	case ECustomerBathLoopReason::StateTreeExited: return TEXT("StateTreeExited");
	case ECustomerBathLoopReason::TechnicalFailure: return TEXT("TechnicalFailure");
	default: return TEXT("TechnicalFailure");
	}
}
