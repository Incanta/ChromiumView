// Copyright Epic Games, Inc. All Rights Reserved.

#include "ChromiumViewSubsystem.h"
#include "ChromiumViewWidget.h"
#include "ChromiumViewModule.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/GameViewportSubsystem.h"
#include "Components/Widget.h"

void UChromiumViewSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
  Super::Initialize(Collection);
  UE_LOG(LogChromiumView, Log, TEXT("Subsystem initialized"));
}

void UChromiumViewSubsystem::Deinitialize()
{
  RemoveAllViews();
  Super::Deinitialize();
  UE_LOG(LogChromiumView, Log, TEXT("Subsystem deinitialized"));
}

UChromiumViewWidget* UChromiumViewSubsystem::CreateView(
  const FString& HtmlPath,
  bool bAddToViewport,
  int32 ZOrder)
{
  FChromiumViewConfig Config;
  Config.HtmlPath = HtmlPath;
  return CreateViewWithConfig(Config, bAddToViewport, ZOrder);
}

UChromiumViewWidget* UChromiumViewSubsystem::CreateViewWithConfig(
  const FChromiumViewConfig& Config,
  bool bAddToViewport,
  int32 ZOrder)
{
  UGameInstance* GameInstance = GetGameInstance();
  if (!GameInstance)
  {
    UE_LOG(LogChromiumView, Error, TEXT("No GameInstance available"));
    return nullptr;
  }

  // Create the widget
  UChromiumViewWidget* Widget = NewObject<UChromiumViewWidget>(GameInstance, UChromiumViewWidget::StaticClass());
  if (!Widget)
  {
    UE_LOG(LogChromiumView, Error, TEXT("Failed to create widget"));
    return nullptr;
  }

  Widget->ViewConfig = Config;

  // Load the view if path is specified
  if (!Config.HtmlPath.IsEmpty())
  {
    Widget->LoadViewWithConfig(Config);
  }

  // Add to viewport if requested
  if (bAddToViewport)
  {
    FGameViewportWidgetSlot Slot;
    Slot.ZOrder = ZOrder;
    UGameViewportSubsystem::Get()->AddWidget(Widget, Slot);
  }

  RegisterWidget(Widget);

  UE_LOG(LogChromiumView, Log, TEXT("Created view: %s"), *Config.HtmlPath);
  return Widget;
}

UChromiumViewWidget* UChromiumViewSubsystem::CreateViewWithViewModel(
  const FString& HtmlPath,
  UMVVMViewModelBase* ViewModel,
  bool bAddToViewport,
  int32 ZOrder)
{
  UChromiumViewWidget* Widget = CreateView(HtmlPath, bAddToViewport, ZOrder);
  if (Widget && ViewModel)
  {
    Widget->BindViewModel(ViewModel);
  }
  return Widget;
}

TArray<UChromiumViewWidget*> UChromiumViewSubsystem::GetActiveViews() const
{
  TArray<UChromiumViewWidget*> Result;
  for (const TObjectPtr<UChromiumViewWidget>& Widget : ActiveWidgets)
  {
    if (Widget)
    {
      Result.Add(Widget);
    }
  }
  return Result;
}

UChromiumViewWidget* UChromiumViewSubsystem::FindViewByPath(const FString& HtmlPath) const
{
  for (const TObjectPtr<UChromiumViewWidget>& Widget : ActiveWidgets)
  {
    if (Widget && Widget->ViewConfig.HtmlPath == HtmlPath)
    {
      return Widget;
    }
  }
  return nullptr;
}

void UChromiumViewSubsystem::RemoveView(UChromiumViewWidget* Widget)
{
  if (!Widget)
  {
    return;
  }

  UnregisterWidget(Widget);

  // Drop the view's (dev-server) connection before it's destroyed so CEF doesn't block for
  // minutes closing a live socket during teardown (the PIE-stop hang).
  Widget->PrepareForTeardown();
  Widget->UnbindViewModel();
  Widget->RemoveFromParent();

  UE_LOG(LogChromiumView, Log, TEXT("Removed view: %s"), *Widget->ViewConfig.HtmlPath);
}

void UChromiumViewSubsystem::RemoveAllViews()
{
  TArray<TObjectPtr<UChromiumViewWidget>> WidgetsCopy = ActiveWidgets;
  for (const TObjectPtr<UChromiumViewWidget>& Widget : WidgetsCopy)
  {
    if (Widget)
    {
      RemoveView(Widget);
    }
  }
  ActiveWidgets.Empty();
}

bool UChromiumViewSubsystem::HasActiveViews() const
{
  for (const TObjectPtr<UChromiumViewWidget>& Widget : ActiveWidgets)
  {
    if (Widget)
    {
      return true;
    }
  }
  return false;
}

FString UChromiumViewSubsystem::GetViewBasePath() const
{
  return FChromiumViewModule::GetViewBasePath();
}

void UChromiumViewSubsystem::RegisterWidget(UChromiumViewWidget* Widget)
{
  if (Widget && !ActiveWidgets.Contains(Widget))
  {
    ActiveWidgets.Add(Widget);
  }
}

void UChromiumViewSubsystem::UnregisterWidget(UChromiumViewWidget* Widget)
{
  ActiveWidgets.Remove(Widget);
}
