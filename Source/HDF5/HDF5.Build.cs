using System;
using System.IO;
using UnrealBuildTool;

public class HDF5 : ModuleRules
{
	public HDF5(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicIncludePaths.AddRange(
			new string[] {
				// ... add public include paths required here ...
			}
			);


		PrivateIncludePaths.AddRange(
			new string[] {
				// ... add other private include paths required here ...
				"HDF5/hdf5",
            }
			);


		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject"
				// ... add other public dependencies that you statically link with here ...
			}
			);


		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
			}
			);


		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
				// ... add any modules that your module loads dynamically here ...
			}
			);

        AddEngineThirdPartyPrivateStaticDependencies(Target,
			"zlib"
		);
        PublicSystemLibraries.Add("shlwapi.lib");
        //PublicDefinitions.Add("H5_HAVE_THREADSAFE");
        //PublicDefinitions.Add("H5_HAVE_THREADSAFE=1");
        //PublicDefinitions.Add("H5_HAVE_THREADSAFE_API");
        PublicDefinitions.Add("H5_HAVE_FILTER_DEFLATE");
        PublicDefinitions.Add("H5_HAVE_ZLIB_H");
        PublicDefinitions.Add("H5_HAVE_ZLIB_H");
    }
}
