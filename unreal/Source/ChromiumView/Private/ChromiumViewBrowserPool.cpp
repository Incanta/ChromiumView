// Copyright Epic Games, Inc. All Rights Reserved.

#include "ChromiumViewBrowserPool.h"
#include "ChromiumViewSettings.h"
#include "SChromiumView.h"
#include "Widgets/SCompoundWidget.h"
#include "HAL/PlatformTime.h"

DEFINE_LOG_CATEGORY_STATIC(LogChromiumViewPool, Log, All);

TWeakObjectPtr<UChromiumViewBrowserPool> UChromiumViewBrowserPool::Instance;

UChromiumViewBrowserPool::UChromiumViewBrowserPool()
{
  // Initialize from project settings
  const UChromiumViewSettings* Settings = UChromiumViewSettings::Get();
  if (Settings)
  {
    bPoolingEnabled = Settings->bEnableBrowserPooling;
    MaxPooledBrowsers = Settings->MaxPooledBrowsers;
  }
  else
  {
    bPoolingEnabled = true;
    MaxPooledBrowsers = 10;
  }
}

UChromiumViewBrowserPool* UChromiumViewBrowserPool::Get()
{
  if (!Instance.IsValid())
  {
    Instance = NewObject<UChromiumViewBrowserPool>(GetTransientPackage(), NAME_None, RF_MarkAsRootSet);
  }
  return Instance.Get();
}

TSharedPtr<SChromiumView> UChromiumViewBrowserPool::AcquireBrowser(
  const FString& Url,
  bool bSupportsTransparency,
  int32 FrameRate,
  bool* OutIsShared)
{
  if (OutIsShared)
  {
    *OutIsShared = false;
  }

  // If pooling is disabled, always create a new browser
  if (!bPoolingEnabled)
  {
    return CreateBrowser(Url, bSupportsTransparency, FrameRate);
  }

  // Check if we have a pooled browser for this URL
  FPooledBrowserEntry* ExistingEntry = PooledBrowsers.Find(Url);
  if (ExistingEntry && ExistingEntry->SlateWidget.IsValid())
  {
    // Check if parameters match
    if (ExistingEntry->bSupportsTransparency == bSupportsTransparency)
    {
      ExistingEntry->RefCount++;
      ExistingEntry->LastAccessTime = FPlatformTime::Seconds();

      if (OutIsShared)
      {
        *OutIsShared = ExistingEntry->RefCount > 1;
      }

      UE_LOG(LogChromiumViewPool, Log, TEXT("Reusing pooled browser for URL: %s (RefCount: %d)"), *Url, ExistingEntry->RefCount);
      return ExistingEntry->SlateWidget;
    }
  }

  // Create a new browser
  TSharedPtr<SChromiumView> NewBrowser = CreateBrowser(Url, bSupportsTransparency, FrameRate);
  if (!NewBrowser.IsValid())
  {
    return nullptr;
  }

  // Add to pool
  FPooledBrowserEntry& NewEntry = PooledBrowsers.Add(Url);
  NewEntry.Url = Url;
  NewEntry.SlateWidget = NewBrowser;
  NewEntry.RefCount = 1;
  NewEntry.bSupportsTransparency = bSupportsTransparency;
  NewEntry.FrameRate = FrameRate;
  NewEntry.LastAccessTime = FPlatformTime::Seconds();

  UE_LOG(LogChromiumViewPool, Log, TEXT("Created new pooled browser for URL: %s"), *Url);

  // Check if we need to evict old browsers
  EvictLRUBrowsers();

  return NewBrowser;
}

void UChromiumViewBrowserPool::ReleaseBrowser(const FString& Url)
{
  FPooledBrowserEntry* Entry = PooledBrowsers.Find(Url);
  if (Entry)
  {
    Entry->RefCount--;
    UE_LOG(LogChromiumViewPool, Log, TEXT("Released browser for URL: %s (RefCount: %d)"), *Url, Entry->RefCount);

    // Don't immediately destroy - keep in pool for potential reuse
    // The LRU eviction will clean up unused browsers when needed
    if (Entry->RefCount <= 0)
    {
      Entry->RefCount = 0;
      // Mark last access time for LRU
      Entry->LastAccessTime = FPlatformTime::Seconds();
    }
  }
}

TSharedPtr<SChromiumView> UChromiumViewBrowserPool::CreateBrowser(
  const FString& Url,
  bool bSupportsTransparency,
  int32 FrameRate)
{
  TSharedPtr<SChromiumView> NewBrowser = SNew(SChromiumView)
    .InitialUrl(Url)
    .SupportsTransparency(bSupportsTransparency)
    .BackgroundColor(bSupportsTransparency ? FColor::Transparent : FColor::White)
    .FrameRate(FrameRate);

  return NewBrowser;
}

void UChromiumViewBrowserPool::EvictLRUBrowsers()
{
  // Count active browsers (with refcount > 0)
  int32 ActiveCount = 0;
  for (const auto& Pair : PooledBrowsers)
  {
    if (Pair.Value.RefCount > 0)
    {
      ActiveCount++;
    }
  }

  // If we're over capacity, evict unused browsers
  if (PooledBrowsers.Num() > MaxPooledBrowsers)
  {
    // Find browsers with RefCount == 0 and sort by last access time
    TArray<TPair<FString, double>> UnusedBrowsers;
    for (const auto& Pair : PooledBrowsers)
    {
      if (Pair.Value.RefCount <= 0)
      {
        UnusedBrowsers.Add(TPair<FString, double>(Pair.Key, Pair.Value.LastAccessTime));
      }
    }

    // Sort by last access time (oldest first)
    UnusedBrowsers.Sort([](const TPair<FString, double>& A, const TPair<FString, double>& B) {
      return A.Value < B.Value;
    });

    // Evict oldest unused browsers until we're under capacity
    int32 ToEvict = PooledBrowsers.Num() - MaxPooledBrowsers;
    for (int32 i = 0; i < ToEvict && i < UnusedBrowsers.Num(); i++)
    {
      UE_LOG(LogChromiumViewPool, Log, TEXT("Evicting unused browser: %s"), *UnusedBrowsers[i].Key);
      PooledBrowsers.Remove(UnusedBrowsers[i].Key);
    }
  }
}

FChromiumViewMemoryStats UChromiumViewBrowserPool::GetMemoryStats() const
{
  FChromiumViewMemoryStats Stats;

  Stats.PooledBrowserCount = PooledBrowsers.Num();
  Stats.UniqueUrlCount = PooledBrowsers.Num();

  int32 TotalConsumers = 0;
  int32 ActiveBrowsers = 0;

  for (const auto& Pair : PooledBrowsers)
  {
    TotalConsumers += FMath::Max(Pair.Value.RefCount, 1);
    if (Pair.Value.RefCount > 0)
    {
      ActiveBrowsers++;
    }
  }

  Stats.ActiveBrowserCount = ActiveBrowsers;
  Stats.TotalPoolConsumers = TotalConsumers;

  // Calculate memory savings
  // Without pooling, we'd have TotalConsumers browsers
  // With pooling, we have PooledBrowserCount browsers
  int32 BrowsersSaved = TotalConsumers - Stats.PooledBrowserCount;
  Stats.EstimatedMemorySavedBytes = FMath::Max(0, BrowsersSaved) * EstimatedBrowserMemoryBytes;
  Stats.EstimatedMemoryPerBrowserBytes = EstimatedBrowserMemoryBytes;

  // Estimate render target memory
  // This is harder to measure precisely, but we can estimate based on typical sizes
  Stats.RenderTargetMemoryBytes = Stats.PooledBrowserCount * (1920 * 1080 * 4); // Assume 1080p RGBA

  return Stats;
}

void UChromiumViewBrowserPool::LogMemoryStats() const
{
  FChromiumViewMemoryStats Stats = GetMemoryStats();

  UE_LOG(LogChromiumViewPool, Display, TEXT("========================================"));
  UE_LOG(LogChromiumViewPool, Display, TEXT("ChromiumView Browser Pool Memory Stats"));
  UE_LOG(LogChromiumViewPool, Display, TEXT("========================================"));
  UE_LOG(LogChromiumViewPool, Display, TEXT("Pooling Enabled: %s"), bPoolingEnabled ? TEXT("Yes") : TEXT("No"));
  UE_LOG(LogChromiumViewPool, Display, TEXT("Active Browser Instances: %d"), Stats.ActiveBrowserCount);
  UE_LOG(LogChromiumViewPool, Display, TEXT("Total Pooled Browsers: %d"), Stats.PooledBrowserCount);
  UE_LOG(LogChromiumViewPool, Display, TEXT("Unique URLs: %d"), Stats.UniqueUrlCount);
  UE_LOG(LogChromiumViewPool, Display, TEXT("Total Consumers: %d"), Stats.TotalPoolConsumers);
  UE_LOG(LogChromiumViewPool, Display, TEXT("Estimated Memory per Browser: %.2f MB"), Stats.EstimatedMemoryPerBrowserBytes / (1024.0 * 1024.0));
  UE_LOG(LogChromiumViewPool, Display, TEXT("Estimated Memory Saved: %.2f MB"), Stats.EstimatedMemorySavedBytes / (1024.0 * 1024.0));
  UE_LOG(LogChromiumViewPool, Display, TEXT("Render Target Memory: %.2f MB"), Stats.RenderTargetMemoryBytes / (1024.0 * 1024.0));
  UE_LOG(LogChromiumViewPool, Display, TEXT("----------------------------------------"));

  // Log individual browser details
  for (const auto& Pair : PooledBrowsers)
  {
    UE_LOG(LogChromiumViewPool, Display, TEXT(" [%s] RefCount: %d, Transparency: %s, FPS: %d"),
      *Pair.Key,
      Pair.Value.RefCount,
      Pair.Value.bSupportsTransparency ? TEXT("Yes") : TEXT("No"),
      Pair.Value.FrameRate);
  }
  UE_LOG(LogChromiumViewPool, Display, TEXT("========================================"));
}

void UChromiumViewBrowserPool::ClearUnusedBrowsers()
{
  TArray<FString> ToRemove;
  for (const auto& Pair : PooledBrowsers)
  {
    if (Pair.Value.RefCount <= 0)
    {
      ToRemove.Add(Pair.Key);
    }
  }

  for (const FString& Url : ToRemove)
  {
    UE_LOG(LogChromiumViewPool, Log, TEXT("Clearing unused browser: %s"), *Url);
    PooledBrowsers.Remove(Url);
  }

  UE_LOG(LogChromiumViewPool, Log, TEXT("Cleared %d unused browsers"), ToRemove.Num());
}

bool UChromiumViewBrowserPool::HasPooledBrowser(const FString& Url) const
{
  return PooledBrowsers.Contains(Url);
}

int32 UChromiumViewBrowserPool::GetConsumerCount(const FString& Url) const
{
  const FPooledBrowserEntry* Entry = PooledBrowsers.Find(Url);
  return Entry ? Entry->RefCount : 0;
}

void UChromiumViewBrowserPool::SetPoolingEnabled(bool bEnabled)
{
  bPoolingEnabled = bEnabled;
  UE_LOG(LogChromiumViewPool, Log, TEXT("Browser pooling %s"), bEnabled ? TEXT("enabled") : TEXT("disabled"));

  if (!bEnabled)
  {
    // Clear unused browsers when disabling pooling
    ClearUnusedBrowsers();
  }
}
