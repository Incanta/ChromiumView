// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Widgets/SCompoundWidget.h"
#include "Engine/TextureRenderTarget2D.h"
#include "IWebBrowserWindow.h"

#include "ChromiumViewBrowserPool.generated.h"

class SChromiumView;
class SWebBrowserView;
class UChromiumViewModelBridge;

/**
 * Statistics about browser pool memory usage
 */
USTRUCT(BlueprintType)
struct CHROMIUMVIEW_API FChromiumViewMemoryStats
{
  GENERATED_BODY()

  /** Total number of active browser instances */
  UPROPERTY(BlueprintReadOnly, Category = "ChromiumView|Memory")
  int32 ActiveBrowserCount = 0;

  /** Number of pooled (shared) browser instances */
  UPROPERTY(BlueprintReadOnly, Category = "ChromiumView|Memory")
  int32 PooledBrowserCount = 0;

  /** Number of unique URLs being served */
  UPROPERTY(BlueprintReadOnly, Category = "ChromiumView|Memory")
  int32 UniqueUrlCount = 0;

  /** Total consumers using pooled browsers */
  UPROPERTY(BlueprintReadOnly, Category = "ChromiumView|Memory")
  int32 TotalPoolConsumers = 0;

  /** Estimated memory saved by pooling (in bytes) */
  UPROPERTY(BlueprintReadOnly, Category = "ChromiumView|Memory")
  int64 EstimatedMemorySavedBytes = 0;

  /** Estimated memory per browser instance (in bytes) */
  UPROPERTY(BlueprintReadOnly, Category = "ChromiumView|Memory")
  int64 EstimatedMemoryPerBrowserBytes = 0;

  /** Total render target memory (in bytes) */
  UPROPERTY(BlueprintReadOnly, Category = "ChromiumView|Memory")
  int64 RenderTargetMemoryBytes = 0;
};

/**
 * Represents a pooled browser instance that can be shared by multiple consumers
 */
USTRUCT()
struct FPooledBrowserEntry
{
  GENERATED_BODY()

  /** The URL this browser is displaying */
  FString Url;

  /** The shared Slate widget for this browser */
  TSharedPtr<SChromiumView> SlateWidget;

  /** Reference count - how many consumers are using this browser */
  int32 RefCount = 0;

  /** Whether this browser supports transparency */
  bool bSupportsTransparency = true;

  /** Last access time for LRU eviction */
  double LastAccessTime = 0.0;

  /** Frame rate for this browser */
  int32 FrameRate = 60;
};

/**
 * Manages a pool of shared browser instances to reduce memory usage.
 * When multiple ChromiumView instances load the same URL, they can share
 * a single underlying CEF browser instance.
 */
UCLASS()
class CHROMIUMVIEW_API UChromiumViewBrowserPool : public UObject
{
  GENERATED_BODY()

public:
  UChromiumViewBrowserPool();

  /** Get the singleton instance of the browser pool */
  static UChromiumViewBrowserPool* Get();

  /**
   * Acquire a browser instance for a given URL.
   * If a pooled instance exists, it will be reused. Otherwise a new one is created.
   * @param Url The URL to load
   * @param bSupportsTransparency Whether the browser should support transparency
   * @param FrameRate The desired frame rate
   * @param OutIsShared Set to true if this is a shared instance
   * @return The browser Slate widget, or nullptr on failure
   */
  TSharedPtr<SChromiumView> AcquireBrowser(
    const FString& Url,
    bool bSupportsTransparency = true,
    int32 FrameRate = 60,
    bool* OutIsShared = nullptr);

  /**
   * Release a browser instance back to the pool.
   * @param Url The URL of the browser to release
   */
  void ReleaseBrowser(const FString& Url);

  /**
   * Get memory statistics for the browser pool
   */
  UFUNCTION(BlueprintCallable, Category = "ChromiumView|Memory")
  FChromiumViewMemoryStats GetMemoryStats() const;

  /**
   * Log detailed memory statistics to the console
   */
  UFUNCTION(BlueprintCallable, Category = "ChromiumView|Memory")
  void LogMemoryStats() const;

  /**
   * Clear all pooled browsers that have no active consumers
   */
  UFUNCTION(BlueprintCallable, Category = "ChromiumView|Memory")
  void ClearUnusedBrowsers();

  /**
   * Check if a URL has a pooled browser
   */
  UFUNCTION(BlueprintPure, Category = "ChromiumView|Memory")
  bool HasPooledBrowser(const FString& Url) const;

  /**
   * Get the number of consumers for a pooled URL
   */
  UFUNCTION(BlueprintPure, Category = "ChromiumView|Memory")
  int32 GetConsumerCount(const FString& Url) const;

  /**
   * Enable or disable browser pooling
   */
  UFUNCTION(BlueprintCallable, Category = "ChromiumView|Memory")
  void SetPoolingEnabled(bool bEnabled);

  /** Whether pooling is currently enabled */
  UFUNCTION(BlueprintPure, Category = "ChromiumView|Memory")
  bool IsPoolingEnabled() const { return bPoolingEnabled; }

  /** Estimated memory per CEF browser instance in bytes */
  static constexpr int64 EstimatedBrowserMemoryBytes = 50 * 1024 * 1024; // ~50MB per browser

protected:
  /** Map of URL to pooled browser entry */
  TMap<FString, FPooledBrowserEntry> PooledBrowsers;

  /** Whether pooling is enabled */
  bool bPoolingEnabled = true;

  /** Maximum number of pooled browsers to keep alive */
  int32 MaxPooledBrowsers = 10;

  /** Singleton instance */
  static TWeakObjectPtr<UChromiumViewBrowserPool> Instance;

  /** Create a new browser instance */
  TSharedPtr<SChromiumView> CreateBrowser(
    const FString& Url,
    bool bSupportsTransparency,
    int32 FrameRate);

  /** Evict least recently used browsers if over capacity */
  void EvictLRUBrowsers();
};