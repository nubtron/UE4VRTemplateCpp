// Fill out your copyright notice in the Description page of Project Settings.

using UnrealBuildTool;

public class VRTemplateCpp : ModuleRules
{
	public VRTemplateCpp(ReadOnlyTargetRules Target) : base(Target)
	{
		// MinFilesUsingPrecompiledHeaderOverride = 1;
        // bFasterWithoutUnity = true;
        // PCHUsage = PCHUsageMode.Default;
		
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "HeadMountedDisplay" });

		PrivateDependencyModuleNames.AddRange(new string[] {  });
		
		
		// Steam VR
		{
			PrivateDependencyModuleNames.Add("SteamVR");
			PrivateIncludePaths.Add("SteamVR/Classes");
		}

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });
		
		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
