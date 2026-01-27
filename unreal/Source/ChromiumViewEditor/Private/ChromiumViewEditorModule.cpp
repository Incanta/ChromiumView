// Copyright Epic Games, Inc. All Rights Reserved.

#include "ChromiumViewEditorModule.h"

#define LOCTEXT_NAMESPACE "FChromiumViewEditorModule"

void FChromiumViewEditorModule::StartupModule()
{
  // This code will execute after your module is loaded into memory
}

void FChromiumViewEditorModule::ShutdownModule()
{
  // This function may be called during shutdown to clean up your module.
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FChromiumViewEditorModule, ChromiumViewEditor)
