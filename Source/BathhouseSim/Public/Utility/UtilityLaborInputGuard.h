#pragma once

class AActor;
class FText;

namespace UtilityLaborInputGuard
{
	bool ValidateOwnerInput(AActor* User, FText& OutFailureReason);
}
