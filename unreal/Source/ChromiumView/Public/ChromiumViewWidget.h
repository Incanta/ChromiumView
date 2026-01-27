// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/Widget.h"
#include "Widgets/SCompoundWidget.h"
#include "ChromiumViewTypes.h"

#include "ChromiumViewWidget.generated.h"

class SChromiumView;
class UMVVMViewModelBase;
class UChromiumViewModelBridge;
class ULocalPlayer;

/**
 * A UMG Widget that hosts a Chromium-based web view as a View in MVVM.
 * This widget loads HTML files from the project's View directory and
 * provides bidirectional communication with ViewModels.
 */
UCLASS(BlueprintType, meta = (DisplayName = "Chromium View"))
class CHROMIUMVIEW_API UChromiumViewWidget : public UWidget
{
  GENERATED_BODY()

public:
  UChromiumViewWidget(const FObjectInitializer& ObjectInitializer);

  //~ Begin UWidget Interface
  virtual void ReleaseSlateResources(bool bReleaseChildren) override;
  virtual TSharedRef<SWidget> RebuildWidget() override;
  virtual void SynchronizeProperties() override;
#if WITH_EDITOR
  virtual const FText GetPaletteCategory() override;
#endif
  //~ End UWidget Interface

  /**
   * Load an HTML View from the project's View directory.
   * @param HtmlPath Relative path to the HTML file from the View directory
   */
  UFUNCTION(BlueprintCallable, Category = "ChromiumView")
  void LoadView(const FString& HtmlPath);

  /**
   * Load an HTML View with a specific configuration.
   * @param Config Configuration for the view
   */
  UFUNCTION(BlueprintCallable, Category = "ChromiumView")
  void LoadViewWithConfig(const FChromiumViewConfig& Config);

  /**
   * Bind a ViewModel to this View.
   * @param InViewModel The ViewModel to bind
   */
  UFUNCTION(BlueprintCallable, Category = "ChromiumView|ViewModel")
  void BindViewModel(UMVVMViewModelBase* InViewModel);

  /**
   * Unbind the current ViewModel.
   */
  UFUNCTION(BlueprintCallable, Category = "ChromiumView|ViewModel")
  void UnbindViewModel();

  /**
   * Get the currently bound ViewModel.
   */
  UFUNCTION(BlueprintPure, Category = "ChromiumView|ViewModel")
  UMVVMViewModelBase* GetBoundViewModel() const;

  /**
   * Execute JavaScript code in the View.
   * @param ScriptText The JavaScript code to execute
   */
  UFUNCTION(BlueprintCallable, Category = "ChromiumView")
  void ExecuteJavascript(const FString& ScriptText);

  /**
   * Check if the View is loaded and ready.
   */
  UFUNCTION(BlueprintPure, Category = "ChromiumView")
  bool IsViewReady() const { return bIsViewReady; }

  /**
   * Get the current desired size of the View.
   */
  UFUNCTION(BlueprintPure, Category = "ChromiumView")
  FChromiumViewDesiredSize GetDesiredViewSize() const { return CurrentDesiredSize; }

  /**
   * Set the size mode of the View.
   */
  UFUNCTION(BlueprintCallable, Category = "ChromiumView")
  void SetSizeMode(EChromiumViewSizeMode NewSizeMode);

  /**
   * Reload the current View.
   */
  UFUNCTION(BlueprintCallable, Category = "ChromiumView")
  void ReloadView();

  /**
   * Enable or disable browser pooling for this widget.
   * When enabled, multiple widgets loading the same URL will share a browser instance.
   * @param bEnabled Whether to enable pooling
   */
  UFUNCTION(BlueprintCallable, Category = "ChromiumView|Memory")
  void SetBrowserPoolingEnabled(bool bEnabled);

  /**
   * Check if browser pooling is enabled for this widget.
   */
  UFUNCTION(BlueprintPure, Category = "ChromiumView|Memory")
  bool IsBrowserPoolingEnabled() const { return bUseBrowserPool; }

  /**
   * Check if this widget is currently sharing a browser with other widgets.
   */
  UFUNCTION(BlueprintPure, Category = "ChromiumView|Memory")
  bool IsUsingSharedBrowser() const { return bIsUsingSharedBrowser; }

  /**
   * Adds the widget to the game's viewport and fills the entire screen, unless SetDesiredSizeInViewport is called.
   * @param ZOrder The higher the number, the more on top this widget will be.
   */
  UFUNCTION(BlueprintCallable, BlueprintCosmetic, Category = "ChromiumView|Viewport", meta = (AdvancedDisplay = "ZOrder"))
  void AddToViewport(int32 ZOrder = 0);

  /**
   * Adds the widget to the game's viewport in a section dedicated to the player.
   * This is valuable in a split screen game where you need to only show a widget over a player's portion of the viewport.
   * @param LocalPlayer The local player to add the widget for
   * @param ZOrder The higher the number, the more on top this widget will be.
   */
  UFUNCTION(BlueprintCallable, BlueprintCosmetic, Category = "ChromiumView|Viewport", meta = (AdvancedDisplay = "ZOrder"))
  bool AddToPlayerScreen(ULocalPlayer* LocalPlayer, int32 ZOrder = 0);

  /**
   * Removes the widget from the viewport.
   */
  UFUNCTION(BlueprintCallable, BlueprintCosmetic, Category = "ChromiumView|Viewport")
  void RemoveFromViewport();

  /** Delegate fired when the View is ready */
  UPROPERTY(BlueprintAssignable, Category = "ChromiumView")
  FOnChromiumViewReady OnViewReady;

  /** Delegate fired when the View's desired size changes */
  UPROPERTY(BlueprintAssignable, Category = "ChromiumView")
  FOnChromiumViewDesiredSizeChanged OnDesiredSizeChanged;

  /** Delegate fired when the View sends an event */
  UPROPERTY(BlueprintAssignable, Category = "ChromiumView")
  FOnChromiumViewEvent OnViewEvent;

  /** Configuration for the View */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ChromiumView")
  FChromiumViewConfig ViewConfig;

  /** Get the ViewModel bridge */
  UChromiumViewModelBridge* GetViewModelBridge() const { return ViewModelBridge; }

protected:
  /** Inject the ChromiumView JavaScript API into the View */
  void InjectJavaScriptAPI();

  /** Handle View ready notification from the bridge */
  UFUNCTION()
  void HandleViewReady();

  /** Handle desired size change from the bridge */
  UFUNCTION()
  void HandleDesiredSizeChanged(FChromiumViewDesiredSize NewSize);

  /** Handle View event from the bridge */
  UFUNCTION()
  void HandleViewEvent(FName EventName, const FString& JsonPayload, int32 CallbackId);

  /** Handle load completed */
  void HandleLoadCompleted();

  /** Handle load error */
  void HandleLoadError();

  /**
   * Resolve a View path to a full URL.
   * Uses dev server URL if enabled in editor, otherwise uses file:// URL.
   * @param ViewPath The view path without extension (e.g., "sample-hud")
   * @return The full URL to load
   */
  FString ResolveViewUrl(const FString& ViewPath) const;

  /**
   * Get the full file path for a View on disk.
   * @param ViewPath The view path without extension
   * @return The full file path with .html extension
   */
  FString GetDiskPath(const FString& ViewPath) const;

private:
  TSharedPtr<SChromiumView> SlateWidget;

  UPROPERTY()
  TObjectPtr<UChromiumViewModelBridge> ViewModelBridge;

  UPROPERTY()
  TObjectPtr<UMVVMViewModelBase> BoundViewModel;

  FChromiumViewDesiredSize CurrentDesiredSize;
  bool bIsViewReady = false;
  FString CurrentHtmlPath;

  /** URL used to acquire the browser from the pool */
  FString PooledBrowserUrl;

  /** Whether to use browser pooling (enabled by default) */
  bool bUseBrowserPool = true;

  /** Whether this widget is currently using a shared browser instance */
  bool bIsUsingSharedBrowser = false;

  /** Pending view config to load after widget is built */
  TOptional<FChromiumViewConfig> PendingViewConfig;
};
