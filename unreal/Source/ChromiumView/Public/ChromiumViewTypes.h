// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Widgets/SCompoundWidget.h"
#include "SWebBrowserView.h"
#include "IWebBrowserWindow.h"

#include "ChromiumViewTypes.generated.h"

class UChromiumViewWidget;
class UMVVMViewModelBase;

/**
 * Enum for how the Chromium View should be sized
 */
UENUM(BlueprintType)
enum class EChromiumViewSizeMode : uint8
{
  /** The view fills the entire screen/parent widget */
  FullScreen UMETA(DisplayName = "Full Screen"),
  /** The view uses a specific size defined by the HTML content */
  DesiredSize UMETA(DisplayName = "Desired Size"),
  /** The view size is controlled by parent widget */
  Auto UMETA(DisplayName = "Auto")
};

/**
 * Information about a View's desired size, communicated from JS to C++
 */
USTRUCT(BlueprintType)
struct CHROMIUMVIEW_API FChromiumViewDesiredSize
{
  GENERATED_BODY()

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ChromiumView")
  float Width = 800.0f;

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ChromiumView")
  float Height = 600.0f;

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ChromiumView")
  EChromiumViewSizeMode SizeMode = EChromiumViewSizeMode::Auto;
};

/**
 * Configuration for creating a Chromium View
 */
USTRUCT(BlueprintType)
struct CHROMIUMVIEW_API FChromiumViewConfig
{
  GENERATED_BODY()

  /**
   * The path to the View (without .html extension).
   * Example: "sample-hud" or "ui/main-menu"
   *
   * When using disk: resolves to {ViewDirectory}/{Path}.html
   * When using dev server: resolves to {DevServerURL}/{Path}
   */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ChromiumView", meta = (DisplayName = "View Path"))
  FString HtmlPath;

  /** Initial size mode for the view */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ChromiumView")
  EChromiumViewSizeMode InitialSizeMode = EChromiumViewSizeMode::Auto;

  /** Whether the view supports transparency */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ChromiumView")
  bool bSupportsTransparency = true;

  /** Background color (used when bSupportsTransparency is false) */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ChromiumView")
  FColor BackgroundColor = FColor::White;

  /** Browser frame rate */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ChromiumView")
  int32 FrameRate = 60;
};

/**
 * Delegate for when a ViewModel field value changes (broadcast to JS)
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnChromiumViewFieldChanged, FName, FieldName, const FString&, JsonValue);

/**
 * Delegate for when the View sends an event to the ViewModel
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnChromiumViewEvent, FName, EventName, const FString&, JsonPayload, int32, CallbackId);

/**
 * Delegate for when the View's desired size changes
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnChromiumViewDesiredSizeChanged, FChromiumViewDesiredSize, DesiredSize);

/**
 * Delegate for when the View has finished loading
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnChromiumViewReady);
