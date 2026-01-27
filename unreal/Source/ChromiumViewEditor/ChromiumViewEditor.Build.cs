// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class ChromiumViewEditor : ModuleRules
{
  public ChromiumViewEditor(ReadOnlyTargetRules Target) : base(Target)
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
        "ModelViewViewModelEditor",
      });

    PrivateDependencyModuleNames.AddRange(
      new string[]
      {
        "SlateCore",
        "Slate",
        "UMG",
        "UnrealEd",
        "AssetTools",
        "PropertyEditor",
        "EditorFramework",
        "EditorWidgets",
      });
  }
}
