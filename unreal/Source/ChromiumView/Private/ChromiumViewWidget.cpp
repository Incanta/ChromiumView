// Copyright Epic Games, Inc. All Rights Reserved.

#include "ChromiumViewWidget.h"
#include "SChromiumView.h"
#include "ChromiumViewModelBridge.h"
#include "ChromiumViewModule.h"
#include "ChromiumViewDeveloperSettings.h"
#include "ChromiumViewSettings.h"
#include "ChromiumViewBrowserPool.h"
#include "MVVMViewModelBase.h"
#include "Blueprint/GameViewportSubsystem.h"
#include "IWebBrowserWindow.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "HAL/FileManager.h"

#define LOCTEXT_NAMESPACE "ChromiumViewWidget"

UChromiumViewWidget::UChromiumViewWidget(const FObjectInitializer& ObjectInitializer)
  : Super(ObjectInitializer)
{
  ViewModelBridge = CreateDefaultSubobject<UChromiumViewModelBridge>(TEXT("ViewModelBridge"));

  // Initialize from project settings
  const UChromiumViewSettings* Settings = UChromiumViewSettings::Get();
  if (Settings)
  {
    bUseBrowserPool = Settings->bEnableBrowserPooling;
    ViewConfig.bSupportsTransparency = Settings->bDefaultSupportsTransparency;
    ViewConfig.FrameRate = Settings->DefaultFrameRate;
  }
}

void UChromiumViewWidget::ReleaseSlateResources(bool bReleaseChildren)
{
  // Release browser back to pool if we were using pooling
  if (bUseBrowserPool && !PooledBrowserUrl.IsEmpty())
  {
    if (UChromiumViewBrowserPool* Pool = UChromiumViewBrowserPool::Get())
    {
      Pool->ReleaseBrowser(PooledBrowserUrl);
      UE_LOG(LogChromiumView, Log, TEXT("Released pooled browser for URL: %s"), *PooledBrowserUrl);
    }
    PooledBrowserUrl.Empty();
    bIsUsingSharedBrowser = false;
  }

  Super::ReleaseSlateResources(bReleaseChildren);
  SlateWidget.Reset();
}

TSharedRef<SWidget> UChromiumViewWidget::RebuildWidget()
{
  // Use pending config if available, otherwise use current ViewConfig
  if (PendingViewConfig.IsSet())
  {
    ViewConfig = PendingViewConfig.GetValue();
    CurrentHtmlPath = ViewConfig.HtmlPath;
    PendingViewConfig.Reset();
    UE_LOG(LogChromiumView, Log, TEXT("RebuildWidget: Using pending config with HtmlPath: '%s'"), *ViewConfig.HtmlPath);
  }

  FString InitialUrl = TEXT("about:blank");
  if (!ViewConfig.HtmlPath.IsEmpty())
  {
    InitialUrl = ResolveViewUrl(ViewConfig.HtmlPath);
  }

  // Try to use browser pooling if enabled
  if (bUseBrowserPool && InitialUrl != TEXT("about:blank"))
  {
    if (UChromiumViewBrowserPool* Pool = UChromiumViewBrowserPool::Get())
    {
      bool bIsShared = false;
      SlateWidget = Pool->AcquireBrowser(
        InitialUrl,
        ViewConfig.bSupportsTransparency,
        ViewConfig.FrameRate,
        &bIsShared);

      if (SlateWidget.IsValid())
      {
        PooledBrowserUrl = InitialUrl;
        bIsUsingSharedBrowser = bIsShared;
        UE_LOG(LogChromiumView, Log, TEXT("Acquired %s browser from pool for URL: %s"),
          bIsShared ? TEXT("shared") : TEXT("new"), *InitialUrl);

        // Set up our callbacks on the pooled browser
        SlateWidget->SetOnLoadCompleted(FSimpleDelegate::CreateUObject(this, &UChromiumViewWidget::HandleLoadCompleted));
        SlateWidget->SetOnLoadError(FSimpleDelegate::CreateUObject(this, &UChromiumViewWidget::HandleLoadError));

        // If the browser is already loaded (reusing a pooled instance), trigger the callback
        if (SlateWidget->IsLoaded())
        {
          UE_LOG(LogChromiumView, Log, TEXT("Pooled browser already loaded, triggering HandleLoadCompleted"));
          HandleLoadCompleted();
        }
      }
    }
  }

  // Fall back to direct creation if pooling failed or is disabled
  if (!SlateWidget.IsValid())
  {
    UE_LOG(LogChromiumView, Log, TEXT("Creating new SChromiumView widget with InitialUrl: %s"), *InitialUrl);
    SlateWidget = SNew(SChromiumView)
      .InitialUrl(InitialUrl)
      .SupportsTransparency(ViewConfig.bSupportsTransparency)
      .BackgroundColor(ViewConfig.BackgroundColor)
      .FrameRate(ViewConfig.FrameRate)
      .OnLoadCompleted_UObject(this, &UChromiumViewWidget::HandleLoadCompleted)
      .OnLoadError_UObject(this, &UChromiumViewWidget::HandleLoadError);
  }

  // Bind the ViewModel bridge if we have a bound ViewModel
  if (BoundViewModel && SlateWidget.IsValid())
  {
    SlateWidget->BindUObject(TEXT("ChromiumViewBridge"), ViewModelBridge, true);
  }

  // Re-apply interactivity (covers both pooled and freshly-created Slate widgets).
  if (SlateWidget.IsValid())
  {
    SlateWidget->SetInteractive(bInteractive);
  }

  return SlateWidget.ToSharedRef();
}

void UChromiumViewWidget::SetInteractive(bool bInInteractive)
{
  bInteractive = bInInteractive;
  if (SlateWidget.IsValid())
  {
    SlateWidget->SetInteractive(bInteractive);
  }
}

void UChromiumViewWidget::SynchronizeProperties()
{
  Super::SynchronizeProperties();

  if (SlateWidget.IsValid())
  {
    // Update properties if needed
  }
}

#if WITH_EDITOR
const FText UChromiumViewWidget::GetPaletteCategory()
{
  return LOCTEXT("ChromiumView", "ChromiumView");
}
#endif

void UChromiumViewWidget::LoadView(const FString& HtmlPath)
{
  FChromiumViewConfig Config;
  Config.HtmlPath = HtmlPath;
  LoadViewWithConfig(Config);
}

void UChromiumViewWidget::LoadViewWithConfig(const FChromiumViewConfig& Config)
{
  ViewConfig = Config;
  CurrentHtmlPath = Config.HtmlPath;
  bIsViewReady = false;

  UE_LOG(LogChromiumView, Log, TEXT("LoadViewWithConfig called with HtmlPath: '%s'"), *Config.HtmlPath);

  if (!SlateWidget.IsValid())
  {
    // Widget isn't ready yet - store config for later and force rebuild
    UE_LOG(LogChromiumView, Log, TEXT("SlateWidget not ready, storing pending config"));
    PendingViewConfig = Config;

    // Try to trigger widget rebuild
    TakeWidget();
    return;
  }

  FString ViewUrl = ResolveViewUrl(Config.HtmlPath);
  UE_LOG(LogChromiumView, Log, TEXT("Loading view URL: %s"), *ViewUrl);
  SlateWidget->LoadURL(ViewUrl);
}

void UChromiumViewWidget::BindViewModel(UMVVMViewModelBase* InViewModel)
{
  if (BoundViewModel)
  {
    UnbindViewModel();
  }

  BoundViewModel = InViewModel;

  if (BoundViewModel && ViewModelBridge)
  {
    ViewModelBridge->Initialize(BoundViewModel, this);

    // Subscribe to bridge events
    ViewModelBridge->OnViewReady.AddDynamic(this, &UChromiumViewWidget::HandleViewReady);
    ViewModelBridge->OnDesiredSizeChanged.AddDynamic(this, &UChromiumViewWidget::HandleDesiredSizeChanged);
    ViewModelBridge->OnViewEvent.AddDynamic(this, &UChromiumViewWidget::HandleViewEvent);

    // Bind the bridge to the browser if it's already created
    if (SlateWidget.IsValid())
    {
      SlateWidget->BindUObject(TEXT("ChromiumViewBridge"), ViewModelBridge, true);
    }
  }
}

void UChromiumViewWidget::UnbindViewModel()
{
  if (ViewModelBridge)
  {
    ViewModelBridge->OnViewReady.RemoveAll(this);
    ViewModelBridge->OnDesiredSizeChanged.RemoveAll(this);
    ViewModelBridge->OnViewEvent.RemoveAll(this);
    ViewModelBridge->Deinitialize();

    if (SlateWidget.IsValid())
    {
      SlateWidget->UnbindUObject(TEXT("ChromiumViewBridge"), ViewModelBridge, true);
    }
  }

  BoundViewModel = nullptr;
}

UMVVMViewModelBase* UChromiumViewWidget::GetBoundViewModel() const
{
  return BoundViewModel;
}

void UChromiumViewWidget::ExecuteJavascript(const FString& ScriptText)
{
  if (SlateWidget.IsValid())
  {
    SlateWidget->ExecuteJavascript(ScriptText);
  }
}

void UChromiumViewWidget::SetSizeMode(EChromiumViewSizeMode NewSizeMode)
{
  CurrentDesiredSize.SizeMode = NewSizeMode;
  ViewConfig.InitialSizeMode = NewSizeMode;
  // Invalidate layout if needed
  InvalidateLayoutAndVolatility();
}

void UChromiumViewWidget::ReloadView()
{
  if (SlateWidget.IsValid())
  {
    bIsViewReady = false;
    SlateWidget->Reload();
  }
}

void UChromiumViewWidget::SetBrowserPoolingEnabled(bool bEnabled)
{
  bUseBrowserPool = bEnabled;
  // Note: This only affects new browser acquisitions, not the current browser
  UE_LOG(LogChromiumView, Log, TEXT("Browser pooling %s for widget"), bEnabled ? TEXT("enabled") : TEXT("disabled"));
}

void UChromiumViewWidget::InjectJavaScriptAPI()
{
  // Inject the ChromiumView JavaScript API
  // This is a fallback API for views that don't use the TypeScript library
  // If the TypeScript library has already set up window.ChromiumView, we preserve it
  FString JSCode = R"JS(
(function() {
  // If the TypeScript ChromiumView client already exists and has _onFieldChanged,
  // don't overwrite it - just log and return
  if (window.ChromiumView && typeof window.ChromiumView._onFieldChanged === 'function') {
    console.log('[ChromiumView] TypeScript API already initialized, skipping injection');
    return;
  }

  // Fallback ChromiumView API for vanilla JS views
  window.ChromiumView = window.ChromiumView || {};

  // Internal callback management
  var _callbacks = {};
  var _nextCallbackId = 1;

  // Event listeners
  var _fieldChangeListeners = {};
  var _readyListeners = [];
  var _isReady = false;

  /**
   * Register a callback for ViewModel field changes
   * @param {string} fieldName - The name of the field to watch
   * @param {function} callback - Callback function(newValue)
   */
  ChromiumView.onFieldChanged = function(fieldName, callback) {
    if (!_fieldChangeListeners[fieldName]) {
      _fieldChangeListeners[fieldName] = [];
    }
    _fieldChangeListeners[fieldName].push(callback);
  };

  /**
   * Remove a callback for ViewModel field changes
   * @param {string} fieldName - The name of the field
   * @param {function} callback - The callback to remove
   */
  ChromiumView.offFieldChanged = function(fieldName, callback) {
    if (_fieldChangeListeners[fieldName]) {
      var index = _fieldChangeListeners[fieldName].indexOf(callback);
      if (index > -1) {
        _fieldChangeListeners[fieldName].splice(index, 1);
      }
    }
  };

  /**
   * Send an event to the ViewModel
   * @param {string} eventName - The name of the event
   * @param {object} payload - The event payload
   * @returns {Promise} Promise that resolves with the response
   */
  ChromiumView.sendEvent = function(eventName, payload) {
    return new Promise(function(resolve, reject) {
      var callbackId = _nextCallbackId++;
      _callbacks[callbackId] = { resolve: resolve, reject: reject };

      var jsonPayload = JSON.stringify(payload || {});

      if (window.ue && window.ue.chromiumviewbridge) {
        window.ue.chromiumviewbridge.handleviewevent(eventName, jsonPayload, callbackId);
      }
    });
  };

  /**
   * Get the current ViewModel state
   * @returns {Promise<object>} Promise that resolves with the ViewModel state
   */
  ChromiumView.getViewModelState = function() {
    return new Promise(function(resolve, reject) {
      if (window.ue && window.ue.chromiumviewbridge) {
        window.ue.chromiumviewbridge.getviewmodelstatejson().then(function(jsonString) {
          try {
            resolve(JSON.parse(jsonString));
          } catch (e) {
            reject(e);
          }
        });
      } else {
        reject(new Error('ChromiumView bridge not available'));
      }
    });
  };

  /**
   * Set the desired size of the View
   * @param {number} width - Desired width in pixels
   * @param {number} height - Desired height in pixels
   * @param {number} sizeMode - 0: FullScreen, 1: DesiredSize, 2: Auto
   */
  ChromiumView.setDesiredSize = function(width, height, sizeMode) {
    if (window.ue && window.ue.chromiumviewbridge) {
      window.ue.chromiumviewbridge.setdesiredsize(width, height, sizeMode || 1);
    }
  };

  /**
   * Register a callback for when the View is ready
   * @param {function} callback - Callback function
   */
  ChromiumView.onReady = function(callback) {
    if (_isReady) {
      callback();
    } else {
      _readyListeners.push(callback);
    }
  };

  /**
   * Notify that the View is ready
   * Should be called by the View HTML when it's finished loading
   */
  ChromiumView.notifyReady = function() {
    _isReady = true;
    _readyListeners.forEach(function(cb) { cb(); });
    _readyListeners = [];

    if (window.ue && window.ue.chromiumviewbridge) {
      window.ue.chromiumviewbridge.notifyviewready();
    }
  };

  // Internal: Handle field changes from C++
  ChromiumView._onFieldChanged = function(fieldName, newValue) {
    if (_fieldChangeListeners[fieldName]) {
      _fieldChangeListeners[fieldName].forEach(function(callback) {
        try {
          callback(newValue);
        } catch (e) {
          console.error('Error in field change listener:', e);
        }
      });
    }
  };

  // Internal: Resolve callbacks from C++
  ChromiumView._resolveCallback = function(callbackId, result, isError) {
    var callback = _callbacks[callbackId];
    if (callback) {
      delete _callbacks[callbackId];
      if (isError) {
        callback.reject(result);
      } else {
        callback.resolve(result);
      }
    }
  };

  console.log('[ChromiumView] API initialized');
})();
)JS";

  ExecuteJavascript(JSCode);
}

void UChromiumViewWidget::HandleViewReady()
{
  bIsViewReady = true;
  OnViewReady.Broadcast();
}

void UChromiumViewWidget::HandleDesiredSizeChanged(FChromiumViewDesiredSize NewSize)
{
  CurrentDesiredSize = NewSize;
  OnDesiredSizeChanged.Broadcast(NewSize);
  InvalidateLayoutAndVolatility();
}

void UChromiumViewWidget::HandleViewEvent(FName EventName, const FString& JsonPayload, int32 CallbackId)
{
  OnViewEvent.Broadcast(EventName, JsonPayload, CallbackId);
}

void UChromiumViewWidget::HandleLoadCompleted()
{
  UE_LOG(LogChromiumView, Log, TEXT("HandleLoadCompleted - Page loaded successfully for: %s"), *CurrentHtmlPath);

  // Inject the JavaScript API when the page loads
  InjectJavaScriptAPI();

  // Send the initial ViewModel state if we have a bound ViewModel
  if (BoundViewModel && ViewModelBridge)
  {
    FString StateJson = ViewModelBridge->GetViewModelStateJson();
    UE_LOG(LogChromiumView, Verbose, TEXT("Sending initial ViewModel state: %s"), *StateJson);
    FString JSCode = FString::Printf(
      TEXT("if (window.ChromiumView && window.ChromiumView._initialState) { window.ChromiumView._initialState(%s); }"),
      *StateJson
    );
    ExecuteJavascript(JSCode);
  }
}

void UChromiumViewWidget::HandleLoadError()
{
  UE_LOG(LogChromiumView, Error, TEXT("HandleLoadError - Failed to load view: %s"), *CurrentHtmlPath);
  UE_LOG(LogChromiumView, Error, TEXT("This may be caused by:"));
  UE_LOG(LogChromiumView, Error, TEXT("  - File not found at the resolved path"));
  UE_LOG(LogChromiumView, Error, TEXT("  - CORS issues with file:// URLs and module scripts"));
  UE_LOG(LogChromiumView, Error, TEXT("  - JavaScript module loading failures"));
  UE_LOG(LogChromiumView, Error, TEXT("Check the browser console output above for more details"));

  // Try to get more info from the browser window if available
  if (SlateWidget.IsValid())
  {
    TSharedPtr<IWebBrowserWindow> BrowserWindow = SlateWidget->GetBrowserWindow();
    if (BrowserWindow.IsValid())
    {
      int32 ErrorCode = BrowserWindow->GetLoadError();
      FString CurrentUrl = BrowserWindow->GetUrl();
      UE_LOG(LogChromiumView, Error, TEXT("Browser error code: %d, Current URL: %s"), ErrorCode, *CurrentUrl);
    }
  }
}

FString UChromiumViewWidget::ResolveViewUrl(const FString& ViewPath) const
{
  UE_LOG(LogChromiumView, Log, TEXT("ResolveViewUrl called with ViewPath: '%s'"), *ViewPath);

  // Strip .html extension if provided (for backwards compatibility)
  FString CleanPath = ViewPath;
  if (CleanPath.EndsWith(TEXT(".html")))
  {
    CleanPath = CleanPath.LeftChop(5);
    UE_LOG(LogChromiumView, Verbose, TEXT("Stripped .html extension, CleanPath: '%s'"), *CleanPath);
  }

#if WITH_EDITOR
  const UChromiumViewDeveloperSettings* Settings = UChromiumViewDeveloperSettings::Get();
  UE_LOG(LogChromiumView, Verbose, TEXT("WITH_EDITOR=true, Settings=%s, ShouldUseDevServer=%s"),
    Settings ? TEXT("valid") : TEXT("null"),
    (Settings && Settings->ShouldUseDevServer()) ? TEXT("true") : TEXT("false"));

  if (Settings && Settings->ShouldUseDevServer())
  {
    // Use dev server URL (no .html extension for Vite/dev servers)
    FString DevServerUrl = Settings->GetDevServerURL();
    // Ensure no trailing slash on base URL
    if (DevServerUrl.EndsWith(TEXT("/")))
    {
      DevServerUrl = DevServerUrl.LeftChop(1);
    }
    // Ensure path starts with /
    FString DevPath = CleanPath;
    if (!DevPath.StartsWith(TEXT("/")))
    {
      DevPath = TEXT("/") + DevPath;
    }
    FString FullUrl = DevServerUrl + DevPath;
    UE_LOG(LogChromiumView, Log, TEXT("Using dev server URL: %s"), *FullUrl);
    return FullUrl;
  }
#else
  UE_LOG(LogChromiumView, Verbose, TEXT("WITH_EDITOR=false, using file:// URL"));
#endif

  // Use file:// URL with .html extension
  FString DiskPath = GetDiskPath(CleanPath + TEXT(".html"));
  UE_LOG(LogChromiumView, Log, TEXT("Resolved disk path: '%s'"), *DiskPath);

  if (!FPaths::FileExists(DiskPath))
  {
    UE_LOG(LogChromiumView, Error, TEXT("View file NOT FOUND: %s"), *DiskPath);
    UE_LOG(LogChromiumView, Error, TEXT("Project Dir: %s"), *FPaths::ProjectDir());
    UE_LOG(LogChromiumView, Error, TEXT("View Base Path: %s"), *FChromiumViewModule::GetViewBasePath());
  }
  else
  {
    UE_LOG(LogChromiumView, Log, TEXT("View file exists: %s"), *DiskPath);

    // Log file size for debugging
    int64 FileSize = IFileManager::Get().FileSize(*DiskPath);
    UE_LOG(LogChromiumView, Log, TEXT("View file size: %lld bytes"), FileSize);
  }

  FString FileUrl = FString::Printf(TEXT("file:///%s"), *DiskPath.Replace(TEXT("\\"), TEXT("/")));
  UE_LOG(LogChromiumView, Log, TEXT("Using file URL: %s"), *FileUrl);
  return FileUrl;
}

FString UChromiumViewWidget::GetDiskPath(const FString& ViewPath) const
{
  FString BasePath = FChromiumViewModule::GetViewBasePath();
  FString FullPath = FPaths::Combine(BasePath, ViewPath);
  FPaths::NormalizeFilename(FullPath);
  return FPaths::ConvertRelativePathToFull(FullPath);
}

void UChromiumViewWidget::AddToViewport(int32 ZOrder)
{
  if (UGameViewportSubsystem* Subsystem = UGameViewportSubsystem::Get(GetWorld()))
  {
    FGameViewportWidgetSlot ViewportSlot;
    ViewportSlot.ZOrder = ZOrder;
    Subsystem->AddWidget(this, ViewportSlot);
  }
}

bool UChromiumViewWidget::AddToPlayerScreen(ULocalPlayer* LocalPlayer, int32 ZOrder)
{
  if (!LocalPlayer)
  {
    UE_LOG(LogChromiumView, Warning, TEXT("AddToPlayerScreen called with null LocalPlayer"));
    return false;
  }

  if (UGameViewportSubsystem* Subsystem = UGameViewportSubsystem::Get(GetWorld()))
  {
    FGameViewportWidgetSlot ViewportSlot;
    ViewportSlot.ZOrder = ZOrder;
    Subsystem->AddWidgetForPlayer(this, LocalPlayer, ViewportSlot);
    return true;
  }
  return false;
}

void UChromiumViewWidget::RemoveFromViewport()
{
  if (UGameViewportSubsystem* Subsystem = UGameViewportSubsystem::Get(GetWorld()))
  {
    Subsystem->RemoveWidget(this);
  }
}

#undef LOCTEXT_NAMESPACE
