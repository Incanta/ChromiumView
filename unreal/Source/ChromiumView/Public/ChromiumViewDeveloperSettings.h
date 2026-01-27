// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "ChromiumViewDeveloperSettings.generated.h"

/**
 * Developer settings for ChromiumView plugin.
 * These settings are per-user and not checked into source control.
 */
UCLASS(config = EditorPerProjectUserSettings, meta = (DisplayName = "Chromium View"))
class CHROMIUMVIEW_API UChromiumViewDeveloperSettings : public UDeveloperSettings
{
  GENERATED_BODY()

public:
  UChromiumViewDeveloperSettings();

  /**
   * Get the singleton instance of the developer settings.
   */
  static const UChromiumViewDeveloperSettings* Get();

#if WITH_EDITORONLY_DATA
  /**
   * Whether to use a development server (e.g., Vite) for loading Views.
   * When enabled, Views will be loaded from the dev server URL instead of disk.
   * This allows for hot module reloading during development.
   */
  UPROPERTY(config, EditAnywhere, Category = "Development Server", meta = (DisplayName = "Use Development Server"))
  bool bUseDevServer = false;

  /**
   * The base URL of the development server.
   * Example: http://localhost:3000
   */
  UPROPERTY(config, EditAnywhere, Category = "Development Server", meta = (DisplayName = "Dev Server URL", EditCondition = "bUseDevServer"))
  FString DevServerURL = TEXT("http://localhost:3000");
#endif

  //~ Begin UDeveloperSettings Interface
  virtual FName GetCategoryName() const override { return FName(TEXT("Plugins")); }
#if WITH_EDITOR
  virtual FText GetSectionText() const override { return NSLOCTEXT("ChromiumView", "ChromiumViewSettingsName", "Chromium View"); }
  virtual FText GetSectionDescription() const override { return NSLOCTEXT("ChromiumView", "ChromiumViewSettingsDescription", "Configure ChromiumView development settings"); }
#endif
  //~ End UDeveloperSettings Interface

  /**
   * Check if the dev server should be used.
   * Always returns false in non-editor builds.
   */
  bool ShouldUseDevServer() const;

  /**
   * Get the dev server URL.
   * Returns empty string in non-editor builds.
   */
  FString GetDevServerURL() const;
};
