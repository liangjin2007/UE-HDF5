# UE-HDF5

A UE HDF5 plugin based on .

I wrote this plugin to support read in and write out 3d motion data. However, for it's simplicity, it can be modified to support any other purpose.

Can work on UE5.6, not working on UE5.7.

This code incorporates components from [hdf5] (https://github.com/HDFGroup/hdf5/tree/hdf5_2.0.0/),  which is licensed under [3-clause BSD License](https://github.com/HDFGroup/hdf5/blob/hdf5_2.0.0/LICENSE).  

All derivative works must comply with the terms of the original license.

## Usage

Take my 3d motion case as an example:

- Read HDF5
```
#include "CoreMinimal.h"
#include "HDF5Motion.h"

FHDF5Motion HDF5Motion;

HDF5Motion.ReadFile(TEXT("test.hdf5"));

// The hdf5 data
const TMap<FString, TArray<TArray<float>>>& MotionObject = HDF5Motion.MotionObject;

// Do something
float fps = HDF5Motion.GetFPS(); // return 30.0, actually we can write out or read in fps from MotionObject object, by add an item, e.g., (TEXT("FPS"), {{60.f}}) 
```

- Write HDF5
```
#include "CoreMinimal.h"
#include "HDF5Motion.h"

FHDF5Motion HDF5Motion;
TMap<FString, TArray<TArray<float>>>& MotionObject = HDF5Motion.MotionObject;

// Setup any data to MotionObject

HDF5Motion.WriteFile("test.hdf5");

```


