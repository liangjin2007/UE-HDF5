#pragma once

#include "CoreMinimal.h"

struct HDF5_API FHDF5Motion 
{
public:

	bool ReadFile(const FString& InHDF5File);
	bool WriteFile(const FString& OutFile);

public:
	TMap<FString, TArray<TArray<float>>> MotionObject;

	int		FrameCount = 0;

	float GetTotalLength() const;

	int32 GetTotalFrames() const;

	float GetFPS() const;

	float GetTime(int32 FrameIndex) const;

	int32 GetFrameIndex(float Time) const;
};
