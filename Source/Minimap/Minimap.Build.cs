// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class Minimap : ModuleRules
{
	public Minimap(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core", 
				"UMG",
				"ZoneGraph",
			}
			);
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore",
				"Settings", 
				"AIModule",
				"GameplayTags",
				"NavigationSystem"
			}
			);
	}
}
