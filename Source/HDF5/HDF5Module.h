#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class FHDF5Module : public IModuleInterface
{

public:
	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
DECLARE_LOG_CATEGORY_EXTERN(LogHDF5, Log, All);