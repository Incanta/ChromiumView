// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class ChromiumView : ModuleRules
{
  public ChromiumView(ReadOnlyTargetRules Target) : base(Target)
  {
    PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

    PublicDependencyModuleNames.AddRange(
      new string[]
      {
        "Core",
        "CoreUObject",
        "Engine",
        "FieldNotification",
        "ModelViewViewModel",
        "WebBrowser",
        "DeveloperSettings",
      });

    PrivateDependencyModuleNames.AddRange(
      new string[]
      {
        "SlateCore",
        "Slate",
        "UMG",
        "InputCore",
        "Json",
        "JsonUtilities",
        "RenderCore",
      });

    if (Target.bBuildEditor)
    {
      PrivateDependencyModuleNames.AddRange(
      new string[]
      {
        "UnrealEd",
      });
    }
  }
}
