// Copyright Epic Games, Inc. All Rights Reserved.

#include "SChromiumView.h"
#include "SWebBrowserView.h"
#include "IWebBrowserWindow.h"
#include "Widgets/Layout/SBox.h"
#include "ChromiumViewModule.h"

void SChromiumView::Construct(const FArguments& InArgs)
{
  OnLoadCompleted = InArgs._OnLoadCompleted;
  OnLoadError = InArgs._OnLoadError;

  UE_LOG(LogChromiumView, Log, TEXT("SChromiumView::Construct - InitialUrl: %s"), *InArgs._InitialUrl);

  // Disable hit testing so this widget doesn't capture cursor/input
  SetVisibility(EVisibility::HitTestInvisible);

  ChildSlot
  [
    SAssignNew(WebBrowserView, SWebBrowserView)
      .InitialURL(InArgs._InitialUrl)
      .SupportsTransparency(InArgs._SupportsTransparency)
      .BackgroundColor(InArgs._BackgroundColor)
      .BrowserFrameRate(InArgs._FrameRate)
      .ShowErrorMessage(true)
      .OnLoadCompleted_Raw(this, &SChromiumView::HandleLoadCompleted)
      .OnLoadError_Raw(this, &SChromiumView::HandleLoadError)
      .OnConsoleMessage_Raw(this, &SChromiumView::HandleConsoleMessage)
  ];

  UE_LOG(LogChromiumView, Log, TEXT("SChromiumView::Construct - WebBrowserView created: %s"),
    WebBrowserView.IsValid() ? TEXT("yes") : TEXT("no"));
}

void SChromiumView::LoadURL(const FString& Url)
{
  if (WebBrowserView.IsValid())
  {
    WebBrowserView->LoadURL(Url);
  }
}

void SChromiumView::LoadString(const FString& Contents, const FString& DummyUrl)
{
  if (WebBrowserView.IsValid())
  {
    WebBrowserView->LoadString(Contents, DummyUrl);
  }
}

void SChromiumView::ExecuteJavascript(const FString& ScriptText)
{
  if (WebBrowserView.IsValid())
  {
    WebBrowserView->ExecuteJavascript(ScriptText);
  }
}

void SChromiumView::BindUObject(const FString& Name, UObject* Object, bool bIsPermanent)
{
  if (WebBrowserView.IsValid())
  {
    WebBrowserView->BindUObject(Name, Object, bIsPermanent);
  }
}

void SChromiumView::UnbindUObject(const FString& Name, UObject* Object, bool bIsPermanent)
{
  if (WebBrowserView.IsValid())
  {
    WebBrowserView->UnbindUObject(Name, Object, bIsPermanent);
  }
}

bool SChromiumView::IsLoaded() const
{
  return WebBrowserView.IsValid() && WebBrowserView->IsLoaded();
}

bool SChromiumView::IsInitialized() const
{
  return WebBrowserView.IsValid() && WebBrowserView->IsInitialized();
}

void SChromiumView::Reload()
{
  if (WebBrowserView.IsValid())
  {
    WebBrowserView->Reload();
  }
}

TSharedPtr<IWebBrowserWindow> SChromiumView::GetBrowserWindow() const
{
  if (WebBrowserView.IsValid())
  {
    return WebBrowserView->GetBrowserWindow();
  }
  return nullptr;
}

void SChromiumView::HandleLoadCompleted()
{
  UE_LOG(LogChromiumView, Log, TEXT("SChromiumView::HandleLoadCompleted called, delegate bound: %s"),
    OnLoadCompleted.IsBound() ? TEXT("yes") : TEXT("no"));
  OnLoadCompleted.ExecuteIfBound();
}

void SChromiumView::HandleLoadError()
{
  UE_LOG(LogChromiumView, Log, TEXT("SChromiumView::HandleLoadError called, delegate bound: %s"),
    OnLoadError.IsBound() ? TEXT("yes") : TEXT("no"));
  OnLoadError.ExecuteIfBound();
}

void SChromiumView::HandleConsoleMessage(const FString& Message, const FString& Source, int32 Line, EWebBrowserConsoleLogSeverity Severity)
{
  // Log console messages from the browser for debugging
  // Include additional context for debugging file:// URL issues
  FString SeverityStr;
  switch (Severity)
  {
    case EWebBrowserConsoleLogSeverity::Error:
    case EWebBrowserConsoleLogSeverity::Fatal:
      SeverityStr = TEXT("ERROR");
      UE_LOG(LogChromiumView, Error, TEXT("[Console] %s: %s (%s:%d)"), *SeverityStr, *Message, *Source, Line);

      // Check for common file:// URL issues
      if (Message.Contains(TEXT("CORS")) || Message.Contains(TEXT("cross-origin")))
      {
        UE_LOG(LogChromiumView, Error, TEXT("CORS error detected. This is common with file:// URLs and module scripts."));
        UE_LOG(LogChromiumView, Error, TEXT("Consider using the dev server or ensure CEF has --allow-file-access-from-files enabled."));
      }
      if (Message.Contains(TEXT("Failed to load")) || Message.Contains(TEXT("net::ERR")))
      {
        UE_LOG(LogChromiumView, Error, TEXT("Resource load failure. Check if all referenced files exist at the expected paths."));
      }
      break;
    case EWebBrowserConsoleLogSeverity::Warning:
      SeverityStr = TEXT("WARN");
      UE_LOG(LogChromiumView, Warning, TEXT("[Console] %s: %s (%s:%d)"), *SeverityStr, *Message, *Source, Line);
      break;
    default:
      SeverityStr = TEXT("LOG");
      UE_LOG(LogChromiumView, Log, TEXT("[Console] %s: %s (%s:%d)"), *SeverityStr, *Message, *Source, Line);
      break;
  }
}
