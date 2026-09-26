#include "Utility/UtilityLaborInputGuard.h"

#include "Character/FirstPersonCharacter.h"
#include "Computer/PlayerComputerUseComponent.h"
#include "Placement/PlayerFacilityPlacementComponent.h"

#define LOCTEXT_NAMESPACE "UtilityLaborInputGuard"

bool UtilityLaborInputGuard::ValidateOwnerInput(AActor* User, FText& OutFailureReason)
{
	const AFirstPersonCharacter* Character = Cast<AFirstPersonCharacter>(User);
	if (Character)
	{
		if (const UPlayerComputerUseComponent* Computer = Character->GetPlayerComputerUse();
			Computer && Computer->IsCapturingInput())
		{
			OutFailureReason = LOCTEXT("ComputerOwnsInput", "컴퓨터 조작 중에는 설비를 사용할 수 없습니다.");
			return false;
		}
		if (const UPlayerFacilityPlacementComponent* Placement = Character->GetPlayerFacilityPlacement();
			Placement && Placement->IsPlacementActive())
		{
			OutFailureReason = LOCTEXT("PlacementOwnsInput", "설비 배치 중에는 설비를 사용할 수 없습니다.");
			return false;
		}
	}
	OutFailureReason = FText::GetEmpty();
	return true;
}

#undef LOCTEXT_NAMESPACE
