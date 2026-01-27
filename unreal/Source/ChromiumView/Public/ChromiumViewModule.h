// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "HAL/IConsoleManager.h"

DECLARE_LOG_CATEGORY_EXTERN(LogChromiumView, Log, All);

class FChromiumViewModule : public IModuleInterface
{
public:
  /** IModuleInterface implementation */
  virtual void StartupModule() override;
  virtual void ShutdownModule() override;

  /** Gets the base path for View files */
  static FString GetViewBasePath();

  /** Log memory statistics for ChromiumView/CEF */
  static void LogMemoryStats();

  /** Log detailed CEF process information */
  static void LogCEFProcessInfo();

  /** Log debug information for troubleshooting view loading */
  static void LogDebugInfo();

private:
  /** Register console commands */
  void RegisterConsoleCommands();

  /** Unregister console commands */
  void UnregisterConsoleCommands();

  /** Console command handles */
  TArray<IConsoleObject*> ConsoleCommands;
};
