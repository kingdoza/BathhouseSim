#pragma once

#include "CoreMinimal.h"
#include "Placement/FacilityPlacementDefinition.h"
#include "Shop/ShopUnboxItemShape.h"

namespace ShopUnboxTest
{
/** Converts facility definitions into unbox shapes; a definition that cannot provide one yields an empty query. */
inline TArray<FShopUnboxItemShape> MakeShapes(const TArray<UFacilityPlacementDefinition*>& Definitions)
{
	TArray<FShopUnboxItemShape> Shapes;
	Shapes.Reserve(Definitions.Num());
	for (UFacilityPlacementDefinition* Definition : Definitions)
	{
		FShopUnboxItemShape& Shape = Shapes.AddDefaulted_GetRef();
		FText Failure;
		if (!IsValid(Definition) || !FShopUnboxItemShape::FromFacilityDefinition(*Definition, Shape, Failure))
		{
			Shape = FShopUnboxItemShape();
		}
	}
	return Shapes;
}
}
