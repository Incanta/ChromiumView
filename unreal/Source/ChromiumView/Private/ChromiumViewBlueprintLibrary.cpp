// Copyright Epic Games, Inc. All Rights Reserved.

#include "ChromiumViewBlueprintLibrary.h"
#include "ChromiumViewWidget.h"
#include "ChromiumViewModule.h"
#include "ChromiumViewModelBridge.h"
#include "ChromiumViewBrowserPool.h"
#include "Blueprint/UserWidget.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"

UChromiumViewWidget* UChromiumViewBlueprintLibrary::CreateChromiumView(
  UObject* WorldContextObject,
  const FString& HtmlPath,
  APlayerController* OwningPlayer)
{
  FChromiumViewConfig Config;
  Config.HtmlPath = HtmlPath;
  return CreateChromiumViewWithConfig(WorldContextObject, Config, OwningPlayer);
}

UChromiumViewWidget* UChromiumViewBlueprintLibrary::CreateChromiumViewWithConfig(
  UObject* WorldContextObject,
  const FChromiumViewConfig& Config,
  APlayerController* OwningPlayer)
{
  if (!WorldContextObject)
  {
    return nullptr;
  }

  UWorld* World = WorldContextObject->GetWorld();
  if (!World)
  {
    return nullptr;
  }

  // Create the widget
  UChromiumViewWidget* Widget = NewObject<UChromiumViewWidget>(WorldContextObject, UChromiumViewWidget::StaticClass());
  if (Widget)
  {
    Widget->ViewConfig = Config;

    // If we have an HTML path, load it
    if (!Config.HtmlPath.IsEmpty())
    {
      Widget->LoadViewWithConfig(Config);
    }
  }

  return Widget;
}

FString UChromiumViewBlueprintLibrary::GetViewBasePath()
{
  return FChromiumViewModule::GetViewBasePath();
}

bool UChromiumViewBlueprintLibrary::DoesViewExist(const FString& HtmlPath)
{
  FString BasePath = FChromiumViewModule::GetViewBasePath();
  FString FullPath = FPaths::Combine(BasePath, HtmlPath);
  FPaths::NormalizeFilename(FullPath);
  return FPaths::FileExists(FullPath);
}

TArray<FString> UChromiumViewBlueprintLibrary::GetAvailableViews()
{
  TArray<FString> HtmlFiles;
  FString BasePath = FChromiumViewModule::GetViewBasePath();

  if (FPaths::DirectoryExists(BasePath))
  {
    IFileManager& FileManager = IFileManager::Get();
    FileManager.FindFilesRecursive(HtmlFiles, *BasePath, TEXT("*.html"), true, false);

    // Convert to relative paths
    for (FString& FilePath : HtmlFiles)
    {
      FPaths::MakePathRelativeTo(FilePath, *BasePath);
    }
  }

  return HtmlFiles;
}

void UChromiumViewBlueprintLibrary::SendViewCallbackResponse(
  UChromiumViewWidget* View,
  int32 CallbackId,
  const FString& JsonResult,
  bool bIsError)
{
  if (View && View->GetViewModelBridge())
  {
    View->GetViewModelBridge()->SendCallbackResponse(CallbackId, JsonResult, bIsError);
  }
}

// ============================
// Memory Management Functions
// ============================

FChromiumViewMemoryStats UChromiumViewBlueprintLibrary::GetBrowserPoolMemoryStats()
{
  if (UChromiumViewBrowserPool* Pool = UChromiumViewBrowserPool::Get())
  {
    return Pool->GetMemoryStats();
  }
  return FChromiumViewMemoryStats();
}

void UChromiumViewBlueprintLibrary::LogMemoryStatistics()
{
  FChromiumViewModule::LogMemoryStats();
}

void UChromiumViewBlueprintLibrary::SetGlobalBrowserPoolingEnabled(bool bEnabled)
{
  if (UChromiumViewBrowserPool* Pool = UChromiumViewBrowserPool::Get())
  {
    Pool->SetPoolingEnabled(bEnabled);
  }
}

bool UChromiumViewBlueprintLibrary::IsGlobalBrowserPoolingEnabled()
{
  if (UChromiumViewBrowserPool* Pool = UChromiumViewBrowserPool::Get())
  {
    return Pool->IsPoolingEnabled();
  }
  return false;
}

void UChromiumViewBlueprintLibrary::ClearUnusedBrowsers()
{
  if (UChromiumViewBrowserPool* Pool = UChromiumViewBrowserPool::Get())
  {
    Pool->ClearUnusedBrowsers();
  }
}

int32 UChromiumViewBlueprintLibrary::GetActiveBrowserCount()
{
  if (UChromiumViewBrowserPool* Pool = UChromiumViewBrowserPool::Get())
  {
    return Pool->GetMemoryStats().ActiveBrowserCount;
  }
  return 0;
}

int64 UChromiumViewBlueprintLibrary::GetEstimatedBrowserPoolMemory()
{
  if (UChromiumViewBrowserPool* Pool = UChromiumViewBrowserPool::Get())
  {
    FChromiumViewMemoryStats Stats = Pool->GetMemoryStats();
    return Stats.PooledBrowserCount * Stats.EstimatedMemoryPerBrowserBytes;
  }
  return 0;
}

int64 UChromiumViewBlueprintLibrary::GetEstimatedMemorySaved()
{
  if (UChromiumViewBrowserPool* Pool = UChromiumViewBrowserPool::Get())
  {
    return Pool->GetMemoryStats().EstimatedMemorySavedBytes;
  }
  return 0;
}
