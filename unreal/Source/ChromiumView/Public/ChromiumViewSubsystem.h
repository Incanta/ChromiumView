// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "ChromiumViewTypes.h"

#include "ChromiumViewSubsystem.generated.h"

class UChromiumViewWidget;
class UMVVMViewModelBase;

/**
 * Subsystem for managing ChromiumView widgets throughout the game.
 * Provides convenient functions for creating and managing Views.
 */
UCLASS()
class CHROMIUMVIEW_API UChromiumViewSubsystem : public UGameInstanceSubsystem
{
  GENERATED_BODY()

public:
  // USubsystem implementation
  virtual void Initialize(FSubsystemCollectionBase& Collection) override;
  virtual void Deinitialize() override;

  /**
   * Create a ChromiumView widget and optionally add it to the viewport.
   * @param HtmlPath Relative path to HTML file in the View directory
   * @param bAddToViewport Whether to add the widget to the viewport immediately
   * @param ZOrder Z-order for the widget (higher = on top)
   * @return The created widget, or nullptr on failure
   */
  UFUNCTION(BlueprintCallable, Category = "ChromiumView")
  UChromiumViewWidget* CreateView(
    const FString& HtmlPath,
    bool bAddToViewport = true,
    int32 ZOrder = 0);

  /**
   * Create a ChromiumView widget with configuration.
   * @param Config View configuration
   * @param bAddToViewport Whether to add the widget to the viewport immediately
   * @param ZOrder Z-order for the widget (higher = on top)
   * @return The created widget, or nullptr on failure
   */
  UFUNCTION(BlueprintCallable, Category = "ChromiumView")
  UChromiumViewWidget* CreateViewWithConfig(
    const FChromiumViewConfig& Config,
    bool bAddToViewport = true,
    int32 ZOrder = 0);

  /**
   * Create a ChromiumView and bind it to a ViewModel.
   * @param HtmlPath Relative path to HTML file in the View directory
   * @param ViewModel The ViewModel to bind
   * @param bAddToViewport Whether to add the widget to the viewport immediately
   * @param ZOrder Z-order for the widget (higher = on top)
   * @return The created widget, or nullptr on failure
   */
  UFUNCTION(BlueprintCallable, Category = "ChromiumView")
  UChromiumViewWidget* CreateViewWithViewModel(
    const FString& HtmlPath,
    UMVVMViewModelBase* ViewModel,
    bool bAddToViewport = true,
    int32 ZOrder = 0);

  /**
   * Get all active ChromiumView widgets.
   * @return Array of active widgets
   */
  UFUNCTION(BlueprintPure, Category = "ChromiumView")
  TArray<UChromiumViewWidget*> GetActiveViews() const;

  /**
   * Find a ChromiumView widget by its HTML path.
   * @param HtmlPath The HTML path to search for
   * @return The widget, or nullptr if not found
   */
  UFUNCTION(BlueprintPure, Category = "ChromiumView")
  UChromiumViewWidget* FindViewByPath(const FString& HtmlPath) const;

  /**
   * Remove a ChromiumView widget from the viewport and destroy it.
   * @param Widget The widget to remove
   */
  UFUNCTION(BlueprintCallable, Category = "ChromiumView")
  void RemoveView(UChromiumViewWidget* Widget);

  /**
   * Remove all ChromiumView widgets.
   */
  UFUNCTION(BlueprintCallable, Category = "ChromiumView")
  void RemoveAllViews();

  /**
   * Check if any Views are currently active.
   * @return True if there are active Views
   */
  UFUNCTION(BlueprintPure, Category = "ChromiumView")
  bool HasActiveViews() const;

  /**
   * Get the View base path.
   * @return The full path to the View directory
   */
  UFUNCTION(BlueprintPure, Category = "ChromiumView")
  FString GetViewBasePath() const;

private:
  UPROPERTY()
  TArray<TObjectPtr<UChromiumViewWidget>> ActiveWidgets;

  void RegisterWidget(UChromiumViewWidget* Widget);
  void UnregisterWidget(UChromiumViewWidget* Widget);
};
