// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ChromiumViewTypes.h"
#include "ChromiumViewBrowserPool.h"

#include "ChromiumViewBlueprintLibrary.generated.h"

class UChromiumViewWidget;
class UMVVMViewModelBase;

/**
 * Blueprint function library for ChromiumView operations.
 */
UCLASS()
class CHROMIUMVIEW_API UChromiumViewBlueprintLibrary : public UBlueprintFunctionLibrary
{
  GENERATED_BODY()

public:
  /**
   * Create a ChromiumView widget and load a View from HTML.
   * @param WorldContextObject World context
   * @param HtmlPath Relative path to HTML file in the View directory
   * @param OwningPlayer The player controller that owns this widget
   * @return The created ChromiumView widget
   */
  UFUNCTION(BlueprintCallable, Category = "ChromiumView", meta = (WorldContext = "WorldContextObject"))
  static UChromiumViewWidget* CreateChromiumView(
    UObject* WorldContextObject,
    const FString& HtmlPath,
    APlayerController* OwningPlayer = nullptr);

  /**
   * Create a ChromiumView widget with configuration.
   * @param WorldContextObject World context
   * @param Config View configuration
   * @param OwningPlayer The player controller that owns this widget
   * @return The created ChromiumView widget
   */
  UFUNCTION(BlueprintCallable, Category = "ChromiumView", meta = (WorldContext = "WorldContextObject"))
  static UChromiumViewWidget* CreateChromiumViewWithConfig(
    UObject* WorldContextObject,
    const FChromiumViewConfig& Config,
    APlayerController* OwningPlayer = nullptr);

  /**
   * Get the View base path (project View directory).
   * @return The full path to the View directory
   */
  UFUNCTION(BlueprintPure, Category = "ChromiumView")
  static FString GetViewBasePath();

  /**
   * Check if a View HTML file exists.
   * @param HtmlPath Relative path to HTML file in the View directory
   * @return True if the file exists
   */
  UFUNCTION(BlueprintPure, Category = "ChromiumView")
  static bool DoesViewExist(const FString& HtmlPath);

  /**
   * Get all available View HTML files.
   * @return Array of relative paths to HTML files
   */
  UFUNCTION(BlueprintCallable, Category = "ChromiumView")
  static TArray<FString> GetAvailableViews();

  /**
   * Send a callback response to a View.
   * @param View The ChromiumView widget
   * @param CallbackId The callback ID from the View event
   * @param JsonResult JSON-encoded result
   * @param bIsError Whether this is an error response
   */
  UFUNCTION(BlueprintCallable, Category = "ChromiumView")
  static void SendViewCallbackResponse(
    UChromiumViewWidget* View,
    int32 CallbackId,
    const FString& JsonResult,
    bool bIsError = false);

  // ============================
  // Memory Management Functions
  // ============================

  /**
   * Get memory statistics for the ChromiumView browser pool.
   * @return Memory statistics struct
   */
  UFUNCTION(BlueprintCallable, Category = "ChromiumView|Memory")
  static FChromiumViewMemoryStats GetBrowserPoolMemoryStats();

  /**
   * Log detailed memory statistics to the output log.
   * This includes browser pool stats and CEF process information.
   */
  UFUNCTION(BlueprintCallable, Category = "ChromiumView|Memory")
  static void LogMemoryStatistics();

  /**
   * Enable or disable browser pooling globally.
   * When enabled, multiple ChromiumView widgets loading the same URL will share a browser instance.
   * @param bEnabled Whether to enable pooling
   */
  UFUNCTION(BlueprintCallable, Category = "ChromiumView|Memory")
  static void SetGlobalBrowserPoolingEnabled(bool bEnabled);

  /**
   * Check if browser pooling is globally enabled.
   * @return True if pooling is enabled
   */
  UFUNCTION(BlueprintPure, Category = "ChromiumView|Memory")
  static bool IsGlobalBrowserPoolingEnabled();

  /**
   * Clear all unused browsers from the pool.
   * This frees memory from browsers that are not currently being displayed.
   */
  UFUNCTION(BlueprintCallable, Category = "ChromiumView|Memory")
  static void ClearUnusedBrowsers();

  /**
   * Get the number of active browser instances.
   * @return Number of active browsers
   */
  UFUNCTION(BlueprintPure, Category = "ChromiumView|Memory")
  static int32 GetActiveBrowserCount();

  /**
   * Get the estimated memory used by the browser pool.
   * @return Estimated memory in bytes
   */
  UFUNCTION(BlueprintPure, Category = "ChromiumView|Memory")
  static int64 GetEstimatedBrowserPoolMemory();

  /**
   * Get the estimated memory saved by browser pooling.
   * @return Estimated memory saved in bytes
   */
  UFUNCTION(BlueprintPure, Category = "ChromiumView|Memory")
  static int64 GetEstimatedMemorySaved();
};
