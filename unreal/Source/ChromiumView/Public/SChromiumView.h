// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "ChromiumViewTypes.h"

class SWebBrowserView;
class IWebBrowserWindow;
class UChromiumViewModelBridge;

/**
 * Slate widget that wraps SWebBrowserView for use in the ChromiumView MVVM system.
 */
class CHROMIUMVIEW_API SChromiumView : public SCompoundWidget
{
public:
  SLATE_BEGIN_ARGS(SChromiumView)
    : _InitialUrl(TEXT("about:blank"))
    , _SupportsTransparency(true)
    , _BackgroundColor(FColor::White)
    , _FrameRate(60)
  { }
    SLATE_ARGUMENT(FString, InitialUrl)
    SLATE_ARGUMENT(bool, SupportsTransparency)
    SLATE_ARGUMENT(FColor, BackgroundColor)
    SLATE_ARGUMENT(int32, FrameRate)
    SLATE_EVENT(FSimpleDelegate, OnLoadCompleted)
    SLATE_EVENT(FSimpleDelegate, OnLoadError)
  SLATE_END_ARGS()

  /** Construct this widget */
  void Construct(const FArguments& InArgs);

  /** Load a URL */
  void LoadURL(const FString& Url);

  /** Load HTML content as a string */
  void LoadString(const FString& Contents, const FString& DummyUrl);

  /** Execute JavaScript */
  void ExecuteJavascript(const FString& ScriptText);

  /** Bind a UObject to be accessible from JavaScript */
  void BindUObject(const FString& Name, UObject* Object, bool bIsPermanent = true);

  /** Unbind a UObject from JavaScript */
  void UnbindUObject(const FString& Name, UObject* Object, bool bIsPermanent = true);

  /** Check if the browser is loaded */
  bool IsLoaded() const;

  /** Check if the browser is initialized */
  bool IsInitialized() const;

  /** Reload the current page */
  void Reload();

  /** Get the browser window */
  TSharedPtr<IWebBrowserWindow> GetBrowserWindow() const;

  /** Get the underlying SWebBrowserView */
  TSharedPtr<SWebBrowserView> GetWebBrowserView() const { return WebBrowserView; }

  /** Set the OnLoadCompleted delegate (for use when reusing pooled browsers) */
  void SetOnLoadCompleted(const FSimpleDelegate& InDelegate) { OnLoadCompleted = InDelegate; }

  /** Set the OnLoadError delegate (for use when reusing pooled browsers) */
  void SetOnLoadError(const FSimpleDelegate& InDelegate) { OnLoadError = InDelegate; }

  /**
   * Toggle interactivity. Passive (default): HitTestInvisible, so the view is a
   * pass-through overlay (e.g. a HUD) that never steals cursor/mouse-look. Interactive:
   * SelfHitTestInvisible, so this wrapper stays transparent to hit-testing but the child
   * SWebBrowserView becomes hit-testable and CEF receives mouse/keyboard (clickable menus).
   */
  void SetInteractive(bool bInInteractive);

  //~ Begin SWidget Interface - input handling gated on bInteractive (the child
  //~ SWebBrowserView handles events natively once this wrapper is SelfHitTestInvisible;
  //~ these stubs only matter for the passive case where they stay unhandled).
  virtual bool SupportsKeyboardFocus() const override { return bInteractive; }
  virtual FReply OnFocusReceived(const FGeometry& MyGeometry, const FFocusEvent& InFocusEvent) override { return FReply::Unhandled(); }
  virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override { return FReply::Unhandled(); }
  virtual FReply OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override { return FReply::Unhandled(); }
  virtual FReply OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override { return FReply::Unhandled(); }
  virtual FReply OnMouseWheel(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override { return FReply::Unhandled(); }
  virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override { return FReply::Unhandled(); }
  virtual FReply OnKeyUp(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override { return FReply::Unhandled(); }
  virtual FReply OnKeyChar(const FGeometry& MyGeometry, const FCharacterEvent& InCharacterEvent) override { return FReply::Unhandled(); }
  virtual FCursorReply OnCursorQuery(const FGeometry& MyGeometry, const FPointerEvent& CursorEvent) const override { return FCursorReply::Unhandled(); }
  virtual bool IsInteractable() const override { return bInteractive; }
  //~ End SWidget Interface

private:
  /** Handle document load completed */
  void HandleLoadCompleted();

  /** Handle document load error */
  void HandleLoadError();

  /** Handle console messages from the browser */
  void HandleConsoleMessage(const FString& Message, const FString& Source, int32 Line, EWebBrowserConsoleLogSeverity Severity);

private:
  TSharedPtr<SWebBrowserView> WebBrowserView;
  FSimpleDelegate OnLoadCompleted;
  FSimpleDelegate OnLoadError;

  /** Whether this view accepts cursor/mouse/keyboard input (see SetInteractive). */
  bool bInteractive = false;
};
