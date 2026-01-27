// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class ChromiumViewBlueprint : ModuleRules
{
  public ChromiumViewBlueprint(ReadOnlyTargetRules Target) : base(Target)
  {
    PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

    PublicDependencyModuleNames.AddRange(
      new string[]
      {
        "Core",
        "CoreUObject",
        "Engine",
        "ChromiumView",
        "ModelViewViewModel",
        "ModelViewViewModelBlueprint",
      });

    PrivateDependencyModuleNames.AddRange(
      new string[]
      {
        "SlateCore",
        "Slate",
        "UMG",
      });
  }
}
