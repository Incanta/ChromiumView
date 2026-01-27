// Copyright Epic Games, Inc. All Rights Reserved.

#include "ChromiumViewModule.h"
#include "ChromiumViewBrowserPool.h"
#include "ChromiumViewDeveloperSettings.h"
#include "Misc/Paths.h"
#include "HAL/PlatformProcess.h"
#include "HAL/PlatformMemory.h"
#include "HAL/FileManager.h"
#include "WebBrowserModule.h"
#include "IWebBrowserSingleton.h"
#include "Modules/ModuleManager.h"

#if PLATFORM_WINDOWS
#include "Windows/AllowWindowsPlatformTypes.h"
#include <Windows.h>
#include <Psapi.h>
#include "Windows/HideWindowsPlatformTypes.h"
#endif

#define LOCTEXT_NAMESPACE "FChromiumViewModule"

DEFINE_LOG_CATEGORY(LogChromiumView);

void FChromiumViewModule::StartupModule()
{
  RegisterConsoleCommands();
  UE_LOG(LogChromiumView, Log, TEXT("ChromiumView module started"));

  if (FModuleManager::Get().IsModuleLoaded("WebBrowser") == false)
  {
    FModuleManager::Get().LoadModule("WebBrowser");
  }
}

void FChromiumViewModule::ShutdownModule()
{
  UnregisterConsoleCommands();
  UE_LOG(LogChromiumView, Log, TEXT("ChromiumView module shutdown"));
}

FString FChromiumViewModule::GetViewBasePath()
{
  // Get the path to the View directory
  // In packaged builds, files are staged relative to the executable directory
  // In editor, they're in the project directory

  FString ViewPath;

#if WITH_EDITOR
  // In editor, use the project directory
  ViewPath = FPaths::Combine(FPaths::ProjectDir(), TEXT("View"));
#else
  // In packaged builds, check multiple possible locations
  // 1. First try relative to the executable (common for staged non-UFS files)
  FString ExeDir = FPaths::GetPath(FPlatformProcess::ExecutablePath());
  ViewPath = FPaths::Combine(ExeDir, TEXT("View"));

  if (!FPaths::DirectoryExists(ViewPath))
  {
    // 2. Try relative to ProjectDir (which points to the Content directory's parent in packaged builds)
    ViewPath = FPaths::Combine(FPaths::ProjectDir(), TEXT("View"));
  }

  if (!FPaths::DirectoryExists(ViewPath))
  {
    // 3. Try one level up from executable (for some packaging configurations)
    ViewPath = FPaths::Combine(ExeDir, TEXT("../View"));
    FPaths::CollapseRelativeDirectories(ViewPath);
  }

  if (!FPaths::DirectoryExists(ViewPath))
  {
    // 4. Try in the same directory as the .pak file location
    ViewPath = FPaths::Combine(FPaths::ProjectContentDir(), TEXT("../View"));
    FPaths::CollapseRelativeDirectories(ViewPath);
  }

  // Log where we're looking for debugging
  UE_LOG(LogChromiumView, Log, TEXT("Packaged build - searching for View directory"));
  UE_LOG(LogChromiumView, Log, TEXT("Executable path: %s"), FPlatformProcess::ExecutablePath());
  UE_LOG(LogChromiumView, Log, TEXT("ProjectDir: %s"), *FPaths::ProjectDir());
  UE_LOG(LogChromiumView, Log, TEXT("Final ViewPath: %s (exists: %s)"),
    *ViewPath, FPaths::DirectoryExists(ViewPath) ? TEXT("yes") : TEXT("no"));
#endif

  return ViewPath;
}

void FChromiumViewModule::RegisterConsoleCommands()
{
  // Memory stats command
  ConsoleCommands.Add(IConsoleManager::Get().RegisterConsoleCommand(
    TEXT("ChromiumView.MemoryStats"),
    TEXT("Log memory statistics for ChromiumView browser pool"),
    FConsoleCommandDelegate::CreateStatic(&FChromiumViewModule::LogMemoryStats),
    ECVF_Default
  ));

  // CEF process info command
  ConsoleCommands.Add(IConsoleManager::Get().RegisterConsoleCommand(
    TEXT("ChromiumView.CEFInfo"),
    TEXT("Log CEF process and memory information"),
    FConsoleCommandDelegate::CreateStatic(&FChromiumViewModule::LogCEFProcessInfo),
    ECVF_Default
  ));

  // Debug info command
  ConsoleCommands.Add(IConsoleManager::Get().RegisterConsoleCommand(
    TEXT("ChromiumView.DebugInfo"),
    TEXT("Log debug information for troubleshooting view loading issues"),
    FConsoleCommandDelegate::CreateStatic(&FChromiumViewModule::LogDebugInfo),
    ECVF_Default
  ));

  // Enable/disable pooling
  ConsoleCommands.Add(IConsoleManager::Get().RegisterConsoleCommand(
    TEXT("ChromiumView.EnablePooling"),
    TEXT("Enable browser pooling for memory optimization"),
    FConsoleCommandWithArgsDelegate::CreateLambda([](const TArray<FString>& Args) {
      if (UChromiumViewBrowserPool* Pool = UChromiumViewBrowserPool::Get())
      {
        bool bEnable = true;
        if (Args.Num() > 0)
        {
          bEnable = Args[0].ToBool();
        }
        Pool->SetPoolingEnabled(bEnable);
      }
    }),
    ECVF_Default
  ));

  // Clear unused browsers
  ConsoleCommands.Add(IConsoleManager::Get().RegisterConsoleCommand(
    TEXT("ChromiumView.ClearUnused"),
    TEXT("Clear unused pooled browser instances"),
    FConsoleCommandDelegate::CreateLambda([]() {
      if (UChromiumViewBrowserPool* Pool = UChromiumViewBrowserPool::Get())
      {
        Pool->ClearUnusedBrowsers();
      }
    }),
    ECVF_Default
  ));

  UE_LOG(LogChromiumView, Log, TEXT("Registered console commands: ChromiumView.MemoryStats, ChromiumView.CEFInfo, ChromiumView.DebugInfo, ChromiumView.EnablePooling, ChromiumView.ClearUnused"));
}

void FChromiumViewModule::UnregisterConsoleCommands()
{
  for (IConsoleObject* Command : ConsoleCommands)
  {
    IConsoleManager::Get().UnregisterConsoleObject(Command);
  }
  ConsoleCommands.Empty();
}

void FChromiumViewModule::LogMemoryStats()
{
  UE_LOG(LogChromiumView, Display, TEXT(""));
  UE_LOG(LogChromiumView, Display, TEXT("=========================================="));
  UE_LOG(LogChromiumView, Display, TEXT("  ChromiumView Memory Statistics Report"));
  UE_LOG(LogChromiumView, Display, TEXT("=========================================="));

  // Get platform memory stats
  FPlatformMemoryStats MemStats = FPlatformMemory::GetStats();

  UE_LOG(LogChromiumView, Display, TEXT(""));
  UE_LOG(LogChromiumView, Display, TEXT("=== Platform Memory ==="));
  UE_LOG(LogChromiumView, Display, TEXT("Physical Memory Used: %.2f MB"), MemStats.UsedPhysical / (1024.0 * 1024.0));
  UE_LOG(LogChromiumView, Display, TEXT("Physical Memory Peak: %.2f MB"), MemStats.PeakUsedPhysical / (1024.0 * 1024.0));
  UE_LOG(LogChromiumView, Display, TEXT("Virtual Memory Used: %.2f MB"), MemStats.UsedVirtual / (1024.0 * 1024.0));
  UE_LOG(LogChromiumView, Display, TEXT("Virtual Memory Peak: %.2f MB"), MemStats.PeakUsedVirtual / (1024.0 * 1024.0));

  // Get browser pool stats
  if (UChromiumViewBrowserPool* Pool = UChromiumViewBrowserPool::Get())
  {
    UE_LOG(LogChromiumView, Display, TEXT(""));
    Pool->LogMemoryStats();
  }

  UE_LOG(LogChromiumView, Display, TEXT(""));
  UE_LOG(LogChromiumView, Display, TEXT("=========================================="));
}

void FChromiumViewModule::LogCEFProcessInfo()
{
  UE_LOG(LogChromiumView, Display, TEXT(""));
  UE_LOG(LogChromiumView, Display, TEXT("=========================================="));
  UE_LOG(LogChromiumView, Display, TEXT("  CEF Process Information"));
  UE_LOG(LogChromiumView, Display, TEXT("=========================================="));

#if PLATFORM_WINDOWS
  // Get current process memory info
  PROCESS_MEMORY_COUNTERS_EX ProcMemCounters;
  if (GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*)&ProcMemCounters, sizeof(ProcMemCounters)))
  {
    UE_LOG(LogChromiumView, Display, TEXT(""));
    UE_LOG(LogChromiumView, Display, TEXT("=== Main Process Memory ==="));
    UE_LOG(LogChromiumView, Display, TEXT("Working Set Size: %.2f MB"), ProcMemCounters.WorkingSetSize / (1024.0 * 1024.0));
    UE_LOG(LogChromiumView, Display, TEXT("Peak Working Set: %.2f MB"), ProcMemCounters.PeakWorkingSetSize / (1024.0 * 1024.0));
    UE_LOG(LogChromiumView, Display, TEXT("Private Usage: %.2f MB"), ProcMemCounters.PrivateUsage / (1024.0 * 1024.0));
    UE_LOG(LogChromiumView, Display, TEXT("Page File Usage: %.2f MB"), ProcMemCounters.PagefileUsage / (1024.0 * 1024.0));
  }

  // Find and report CEF subprocess memory
  UE_LOG(LogChromiumView, Display, TEXT(""));
  UE_LOG(LogChromiumView, Display, TEXT("=== CEF Subprocess Memory (EpicWebHelper) ==="));

  // Enumerate all processes to find EpicWebHelper
  DWORD ProcessIds[1024];
  DWORD BytesReturned;
  if (EnumProcesses(ProcessIds, sizeof(ProcessIds), &BytesReturned))
  {
    DWORD NumProcesses = BytesReturned / sizeof(DWORD);
    int32 CefProcessCount = 0;
    SIZE_T TotalCefMemory = 0;

    for (DWORD i = 0; i < NumProcesses; i++)
    {
      if (ProcessIds[i] != 0)
      {
        HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, false, ProcessIds[i]);
        if (hProcess)
        {
          TCHAR ProcessName[MAX_PATH];
          if (GetModuleBaseName(hProcess, nullptr, ProcessName, MAX_PATH))
          {
            FString Name(ProcessName);
            if (Name.Contains(TEXT("EpicWebHelper")))
            {
              PROCESS_MEMORY_COUNTERS_EX Counters;
              if (GetProcessMemoryInfo(hProcess, (PROCESS_MEMORY_COUNTERS*)&Counters, sizeof(Counters)))
              {
                CefProcessCount++;
                TotalCefMemory += Counters.WorkingSetSize;
                UE_LOG(LogChromiumView, Display, TEXT(" [%d] PID %d: %.2f MB (Private: %.2f MB)"),
                  CefProcessCount,
                  ProcessIds[i],
                  Counters.WorkingSetSize / (1024.0 * 1024.0),
                  Counters.PrivateUsage / (1024.0 * 1024.0));
              }
            }
          }
          CloseHandle(hProcess);
        }
      }
    }

    if (CefProcessCount > 0)
    {
      UE_LOG(LogChromiumView, Display, TEXT(""));
      UE_LOG(LogChromiumView, Display, TEXT("Total CEF Processes: %d"), CefProcessCount);
      UE_LOG(LogChromiumView, Display, TEXT("Total CEF Memory: %.2f MB"), TotalCefMemory / (1024.0 * 1024.0));
    }
    else
    {
      UE_LOG(LogChromiumView, Display, TEXT("No EpicWebHelper processes found"));
    }
  }
#else
  UE_LOG(LogChromiumView, Display, TEXT("CEF process enumeration only available on Windows"));
#endif

  // WebBrowser module info
  if (IWebBrowserModule::IsAvailable())
  {
    IWebBrowserSingleton* Singleton = IWebBrowserModule::Get().GetSingleton();
    if (Singleton)
    {
      UE_LOG(LogChromiumView, Display, TEXT(""));
      UE_LOG(LogChromiumView, Display, TEXT("=== WebBrowser Module ==="));
      UE_LOG(LogChromiumView, Display, TEXT("WebBrowser Singleton: Active"));
      UE_LOG(LogChromiumView, Display, TEXT("Is Shutting Down: %s"), Singleton->IsShuttingDown() ? TEXT("Yes") : TEXT("No"));
      UE_LOG(LogChromiumView, Display, TEXT("Cache Directory: %s"), *Singleton->ApplicationCacheDir());
    }
  }

  UE_LOG(LogChromiumView, Display, TEXT(""));
  UE_LOG(LogChromiumView, Display, TEXT("=========================================="));
}

void FChromiumViewModule::LogDebugInfo()
{
  UE_LOG(LogChromiumView, Display, TEXT(""));
  UE_LOG(LogChromiumView, Display, TEXT("=========================================="));
  UE_LOG(LogChromiumView, Display, TEXT("  ChromiumView Debug Information"));
  UE_LOG(LogChromiumView, Display, TEXT("=========================================="));

  // Path information
  UE_LOG(LogChromiumView, Display, TEXT(""));
  UE_LOG(LogChromiumView, Display, TEXT("=== Path Configuration ==="));
  UE_LOG(LogChromiumView, Display, TEXT("Project Directory: %s"), *FPaths::ProjectDir());
  UE_LOG(LogChromiumView, Display, TEXT("View Base Path: %s"), *GetViewBasePath());

  FString ViewBasePath = GetViewBasePath();
  if (FPaths::DirectoryExists(ViewBasePath))
  {
    UE_LOG(LogChromiumView, Display, TEXT("View Directory: EXISTS"));

    // List files in the View directory
    TArray<FString> HtmlFiles;
    IFileManager::Get().FindFilesRecursive(HtmlFiles, *ViewBasePath, TEXT("*.html"), true, false);

    UE_LOG(LogChromiumView, Display, TEXT(""));
    UE_LOG(LogChromiumView, Display, TEXT("=== Available View Files ==="));
    if (HtmlFiles.Num() > 0)
    {
      for (const FString& File : HtmlFiles)
      {
        FString RelativePath = File;
        FPaths::MakePathRelativeTo(RelativePath, *ViewBasePath);
        int64 FileSize = IFileManager::Get().FileSize(*File);
        UE_LOG(LogChromiumView, Display, TEXT(" %s (%lld bytes)"), *RelativePath, FileSize);
      }
    }
    else
    {
      UE_LOG(LogChromiumView, Display, TEXT(" No HTML files found"));
    }
  }
  else
  {
    UE_LOG(LogChromiumView, Warning, TEXT("View Directory: NOT FOUND - %s"), *ViewBasePath);
  }

#if WITH_EDITOR
  // Developer settings
  UE_LOG(LogChromiumView, Display, TEXT(""));
  UE_LOG(LogChromiumView, Display, TEXT("=== Developer Settings ==="));
  const UChromiumViewDeveloperSettings* Settings = UChromiumViewDeveloperSettings::Get();
  if (Settings)
  {
    UE_LOG(LogChromiumView, Display, TEXT("Use Dev Server: %s"), Settings->ShouldUseDevServer() ? TEXT("Yes") : TEXT("No"));
    UE_LOG(LogChromiumView, Display, TEXT("Dev Server URL: %s"), *Settings->GetDevServerURL());
  }
  else
  {
    UE_LOG(LogChromiumView, Warning, TEXT("Developer settings not available"));
  }
#endif

  // CEF configuration
  UE_LOG(LogChromiumView, Display, TEXT(""));
  UE_LOG(LogChromiumView, Display, TEXT("=== CEF Configuration Notes ==="));
  UE_LOG(LogChromiumView, Display, TEXT("file:// URL support: Requires --allow-file-access-from-files (enabled by default)"));
  UE_LOG(LogChromiumView, Display, TEXT("ES Modules with file://: Works if crossorigin attribute is NOT present in HTML"));
  UE_LOG(LogChromiumView, Display, TEXT(""));
  UE_LOG(LogChromiumView, Display, TEXT("=== Common Issues ==="));
  UE_LOG(LogChromiumView, Display, TEXT("1. CORS errors: Remove 'crossorigin' attribute from script/link tags"));
  UE_LOG(LogChromiumView, Display, TEXT("2. Blank page: Check browser console output in UE log"));
  UE_LOG(LogChromiumView, Display, TEXT("3. File not found: Verify file path with this command output"));
  UE_LOG(LogChromiumView, Display, TEXT("4. Module load failure: Ensure all referenced JS files exist"));

  UE_LOG(LogChromiumView, Display, TEXT(""));
  UE_LOG(LogChromiumView, Display, TEXT("=========================================="));
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FChromiumViewModule, ChromiumView)
