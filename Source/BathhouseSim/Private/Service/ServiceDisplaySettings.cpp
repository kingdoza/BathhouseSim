#include "Service/ServiceDisplaySettings.h"

#include "Materials/MaterialInterface.h"

UMaterialInterface* UServiceDisplaySettings::LoadInsertPreviewMaterial() const
{
	return InsertPreviewMaterial.LoadSynchronous();
}
