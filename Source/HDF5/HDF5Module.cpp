#include "HDF5Module.h"

#define LOCTEXT_NAMESPACE "FHDF5Module"
DEFINE_LOG_CATEGORY(LogHDF5);

void FHDF5Module::StartupModule()
{
}

void FHDF5Module::ShutdownModule()
{
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FHDF5Module, HDF5)