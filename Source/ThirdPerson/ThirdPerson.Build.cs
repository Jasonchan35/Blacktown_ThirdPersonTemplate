// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class ThirdPerson : ModuleRules
{
	public ThirdPerson(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PrivatePCHHeaderFile = "ThirdPerson_PCH.h";

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput"
		});

		if (Target.bBuildEditor == true) {
			PrivateDependencyModuleNames.AddRange(new string[] {
				"UnrealEd",
			});
		}
	}
}
