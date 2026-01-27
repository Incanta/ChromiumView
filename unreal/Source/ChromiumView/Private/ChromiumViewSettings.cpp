// Copyright Epic Games, Inc. All Rights Reserved.

#include "ChromiumViewSettings.h"

UChromiumViewSettings::UChromiumViewSettings()
{
  CategoryName = FName(TEXT("Plugins"));
}

const UChromiumViewSettings* UChromiumViewSettings::Get()
{
  return GetDefault<UChromiumViewSettings>();
}
