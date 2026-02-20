using UnrealBuildTool;

public class MinimapEditor : ModuleRules
{
    public MinimapEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core"
            }
        );

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "CoreUObject",
                "Engine",
                "Slate",
                "SlateCore", 
                "EditorScriptingUtilities", 
                "Minimap",
                "DetailCustomizations",
                "InputCore",
                "UnrealEd",
                "ComponentVisualizers",
                "RenderCore",
            }
        );
    }
}