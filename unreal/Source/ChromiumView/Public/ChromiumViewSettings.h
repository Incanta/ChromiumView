// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "ChromiumViewSettings.generated.h"

/**
 * Project settings for ChromiumView plugin.
 * These settings are stored in DefaultGame.ini and should be checked into source control.
 */
UCLASS(config = Game, defaultconfig, meta = (DisplayName = "Chromium View"))
class CHROMIUMVIEW_API UChromiumViewSettings : public UDeveloperSettings
{
  GENERATED_BODY()

public:
  UChromiumViewSettings();

  /**
   * Get the singleton instance of the project settings.
   */
  static const UChromiumViewSettings* Get();

  //~ Begin UDeveloperSettings Interface
  virtual FName GetCategoryName() const override { return FName(TEXT("Plugins")); }
#if WITH_EDITOR
  virtual FText GetSectionText() const override { return NSLOCTEXT("ChromiumView", "ChromiumViewProjectSettingsName", "Chromium View"); }
  virtual FText GetSectionDescription() const override { return NSLOCTEXT("ChromiumView", "ChromiumViewProjectSettingsDescription", "Configure project-wide ChromiumView settings"); }
#endif
  //~ End UDeveloperSettings Interface

  /**
   * Whether to enable browser pooling.
   * When enabled, multiple ChromiumView widgets loading the same URL will share a browser instance,
   * reducing memory usage. Disable this if you need each widget to have its own browser instance.
   */
  UPROPERTY(config, EditAnywhere, Category = "Browser Pool", meta = (DisplayName = "Enable Browser Pooling"))
  bool bEnableBrowserPooling = true;

  /**
   * Maximum number of browsers to keep in the pool.
   * Browsers beyond this limit will be evicted using LRU (Least Recently Used) policy.
   */
  UPROPERTY(config, EditAnywhere, Category = "Browser Pool", meta = (DisplayName = "Max Pooled Browsers", EditCondition = "bEnableBrowserPooling", ClampMin = "1", ClampMax = "50"))
  int32 MaxPooledBrowsers = 10;

  /**
   * Default frame rate for browser rendering.
   * Higher values provide smoother animations but use more CPU.
   */
  UPROPERTY(config, EditAnywhere, Category = "Rendering", meta = (DisplayName = "Default Frame Rate", ClampMin = "1", ClampMax = "120"))
  int32 DefaultFrameRate = 60;

  /**
   * Whether to enable transparency by default for new ChromiumView widgets.
   */
  UPROPERTY(config, EditAnywhere, Category = "Rendering", meta = (DisplayName = "Enable Transparency By Default"))
  bool bDefaultSupportsTransparency = true;
};
