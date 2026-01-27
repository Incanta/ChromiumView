// Copyright Epic Games, Inc. All Rights Reserved.

#include "ChromiumViewDeveloperSettings.h"

UChromiumViewDeveloperSettings::UChromiumViewDeveloperSettings()
{
}

const UChromiumViewDeveloperSettings* UChromiumViewDeveloperSettings::Get()
{
  return GetDefault<UChromiumViewDeveloperSettings>();
}

bool UChromiumViewDeveloperSettings::ShouldUseDevServer() const
{
#if WITH_EDITORONLY_DATA
  return bUseDevServer;
#else
  return false;
#endif
}

FString UChromiumViewDeveloperSettings::GetDevServerURL() const
{
#if WITH_EDITORONLY_DATA
  return DevServerURL;
#else
  return FString();
#endif
}
