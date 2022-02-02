
using UnrealBuildTool;

public class VRM4UEditor : ModuleRules
{
	public VRM4UEditor(ReadOnlyTargetRules Target) : base(Target)
	{
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PrivateDependencyModuleNames.AddRange(
			new string[] {
				"Core",
				"CoreUObject",
                "InputCore",
                "EditorStyle",
                "ApplicationCore",
                "Engine",
				"Json",
				"UnrealEd",
                "Slate",
                "SlateCore",
                "MainFrame",
				"Settings",
                "ProceduralMeshComponent",
                "RenderCore",

                "MovieSceneCapture",
                "RHI",
                //"ShaderCore",
                "Renderer",

				"VRM4U",
			});

		PrivateIncludePathModuleNames.AddRange(
			new string[] {
				"AssetTools",
				"AssetRegistry",
            });

		DynamicallyLoadedModuleNames.AddRange(
			new string[] {
				"AssetTools",
				"AssetRegistry"
			});

        PrivateIncludePaths.AddRange(
        new string[] {
			// Relative to Engine\Plugins\Runtime\Oculus\OculusVR\Source
			//"../Runtime/Renderer/Private",
        });
    }
}
