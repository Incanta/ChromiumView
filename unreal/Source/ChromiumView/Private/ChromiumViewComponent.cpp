// Copyright Epic Games, Inc. All Rights Reserved.

#include "ChromiumViewComponent.h"
#include "SChromiumView.h"
#include "ChromiumViewModelBridge.h"
#include "ChromiumViewDeveloperSettings.h"
#include "MVVMViewModelBase.h"
#include "Slate/WidgetRenderer.h"
#include "Widgets/SVirtualWindow.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "PhysicsEngine/BodySetup.h"
#include "PhysicsEngine/BoxElem.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/Engine.h"
#include "PrimitiveViewRelevance.h"
#include "PrimitiveSceneProxy.h"
#include "MaterialShared.h"
#include "Materials/Material.h"
#include "Materials/MaterialRenderProxy.h"
#include "DynamicMeshBuilder.h"
#include "SceneManagement.h"
#include "Input/HittestGrid.h"
#include "Misc/Paths.h"
#include "ChromiumViewModule.h"

/** Scene proxy for rendering the ChromiumView in 3D */
class FChromiumView3DSceneProxy final : public FPrimitiveSceneProxy
{
public:
  SIZE_T GetTypeHash() const override
  {
    static size_t UniquePointer;
    return reinterpret_cast<size_t>(&UniquePointer);
  }

  FChromiumView3DSceneProxy(UChromiumViewComponent* InComponent, ISlate3DRenderer& InRenderer)
    : FPrimitiveSceneProxy(InComponent)
    , Pivot(InComponent->GetPivot())
    , Renderer(InRenderer)
    , RenderTarget(InComponent->GetRenderTarget())
    , MaterialInstance(InComponent->GetMaterialInstance())
    , BodySetup(InComponent->GetBodySetup())
  {
    bWillEverBeLit = false;

    if (MaterialInstance)
    {
      MaterialRelevance = MaterialInstance->GetRelevance_Concurrent(GetScene().GetShaderPlatform());
    }
  }

  virtual void GetDynamicMeshElements(const TArray<const FSceneView*>& Views, const FSceneViewFamily& ViewFamily, uint32 VisibilityMap, FMeshElementCollector& Collector) const override
  {
#if WITH_EDITOR
    const bool bWireframe = AllowDebugViewmodes() && ViewFamily.EngineShowFlags.Wireframe;

    auto WireframeMaterialInstance = new FColoredMaterialRenderProxy(
      GEngine->WireframeMaterial ? GEngine->WireframeMaterial->GetRenderProxy() : nullptr,
      FLinearColor(0, 0.5f, 1.f)
    );

    Collector.RegisterOneFrameMaterialProxy(WireframeMaterialInstance);

    FMaterialRenderProxy* ParentMaterialProxy = nullptr;
    if (bWireframe)
    {
      ParentMaterialProxy = WireframeMaterialInstance;
    }
    else if (MaterialInstance != nullptr)
    {
      ParentMaterialProxy = MaterialInstance->GetRenderProxy();
    }
#else
    FMaterialRenderProxy* ParentMaterialProxy = MaterialInstance ? MaterialInstance->GetRenderProxy() : nullptr;
#endif

    const FMatrix& ViewportLocalToWorld = GetLocalToWorld();

    FMatrix PreviousLocalToWorld;
    if (!GetScene().GetPreviousLocalToWorld(GetPrimitiveSceneInfo(), PreviousLocalToWorld))
    {
      PreviousLocalToWorld = GetLocalToWorld();
    }

    if (RenderTarget)
    {
      FTextureResource* TextureResource = RenderTarget->GetResource();
      if (TextureResource)
      {
        float U = -RenderTarget->SizeX * static_cast<float>(Pivot.X);
        float V = -RenderTarget->SizeY * static_cast<float>(Pivot.Y);
        float UL = RenderTarget->SizeX * (1.0f - static_cast<float>(Pivot.X));
        float VL = RenderTarget->SizeY * (1.0f - static_cast<float>(Pivot.Y));

        int32 VertexIndices[4];

        for (int32 ViewIndex = 0; ViewIndex < Views.Num(); ViewIndex++)
        {
          FDynamicMeshBuilder MeshBuilder(Views[ViewIndex]->GetFeatureLevel());

          if (VisibilityMap & (1 << ViewIndex))
          {
            VertexIndices[0] = MeshBuilder.AddVertex(-FVector3f(0, U, V), FVector2f(0, 0), FVector3f(0, -1, 0), FVector3f(0, 0, -1), FVector3f(1, 0, 0), FColor::White);
            VertexIndices[1] = MeshBuilder.AddVertex(-FVector3f(0, U, VL), FVector2f(0, 1), FVector3f(0, -1, 0), FVector3f(0, 0, -1), FVector3f(1, 0, 0), FColor::White);
            VertexIndices[2] = MeshBuilder.AddVertex(-FVector3f(0, UL, VL), FVector2f(1, 1), FVector3f(0, -1, 0), FVector3f(0, 0, -1), FVector3f(1, 0, 0), FColor::White);
            VertexIndices[3] = MeshBuilder.AddVertex(-FVector3f(0, UL, V), FVector2f(1, 0), FVector3f(0, -1, 0), FVector3f(0, 0, -1), FVector3f(1, 0, 0), FColor::White);

            MeshBuilder.AddTriangle(VertexIndices[0], VertexIndices[1], VertexIndices[2]);
            MeshBuilder.AddTriangle(VertexIndices[0], VertexIndices[2], VertexIndices[3]);

            FDynamicMeshBuilderSettings Settings;
            Settings.bDisableBackfaceCulling = false;
            Settings.bReceivesDecals = true;
            Settings.bUseSelectionOutline = true;
            MeshBuilder.GetMesh(ViewportLocalToWorld, PreviousLocalToWorld, ParentMaterialProxy, SDPG_World, Settings, nullptr, ViewIndex, Collector, FHitProxyId());
          }
        }
      }
    }
  }

  virtual FPrimitiveViewRelevance GetViewRelevance(const FSceneView* View) const override
  {
    bool bVisible = true;

    FPrimitiveViewRelevance Result;
    MaterialRelevance.SetPrimitiveViewRelevance(Result);

    Result.bDrawRelevance = IsShown(View) && bVisible && View->Family->EngineShowFlags.WidgetComponents;
    Result.bDynamicRelevance = true;
    Result.bRenderCustomDepth = ShouldRenderCustomDepth();
    Result.bRenderInMainPass = ShouldRenderInMainPass();
    Result.bUsesLightingChannels = GetLightingChannelMask() != GetDefaultLightingChannelMask();
    Result.bShadowRelevance = IsShadowCast(View);
    Result.bTranslucentSelfShadow = bCastVolumetricTranslucentShadow;
    Result.bEditorPrimitiveRelevance = false;
    Result.bVelocityRelevance = DrawsVelocity() && Result.bOpaque && Result.bRenderInMainPass;

    return Result;
  }

  virtual void GetLightRelevance(const FLightSceneProxy* LightSceneProxy, bool& bDynamic, bool& bRelevant, bool& bLightMapped, bool& bShadowMapped) const override
  {
    bDynamic = false;
    bRelevant = false;
    bLightMapped = false;
    bShadowMapped = false;
  }

  virtual void OnTransformChanged(FRHICommandListBase& RHICmdList) override
  {
    Origin = GetLocalToWorld().GetOrigin();
  }

  virtual bool CanBeOccluded() const override
  {
    return !MaterialRelevance.bDisableDepthTest;
  }

  virtual uint32 GetMemoryFootprint(void) const override { return (sizeof(*this) + GetAllocatedSize()); }
  SIZE_T GetAllocatedSize(void) const { return FPrimitiveSceneProxy::GetAllocatedSize(); }

private:
  FVector Origin;
  FVector2D Pivot;
  ISlate3DRenderer& Renderer;
  UTextureRenderTarget2D* RenderTarget;
  UMaterialInstanceDynamic* MaterialInstance;
  FMaterialRelevance MaterialRelevance;
  UBodySetup* BodySetup;
};

UChromiumViewComponent::UChromiumViewComponent(const FObjectInitializer& PCIP)
  : Super(PCIP)
  , DrawSize(FIntPoint(500, 500))
  , Pivot(FVector2D(0.5f, 0.5f))
  , BackgroundColor(FLinearColor::Transparent)
  , TintColorAndOpacity(FLinearColor::White)
  , bIsTwoSided(false)
  , bTickWhenOffscreen(false)
  , CurrentDrawSize(FIntPoint(0, 0))
  , WidgetRenderer(nullptr)
{
  PrimaryComponentTick.bCanEverTick = true;
  bTickInEditor = true;

  SetRelativeRotation(FRotator::ZeroRotator);

  BodyInstance.SetCollisionProfileName(FName(TEXT("UI")));

  // Load translucent materials
  static ConstructorHelpers::FObjectFinder<UMaterialInterface> TranslucentMaterial_Finder(TEXT("/Engine/EngineMaterials/Widget3DPassThrough_Translucent"));
  static ConstructorHelpers::FObjectFinder<UMaterialInterface> TranslucentMaterial_OneSided_Finder(TEXT("/Engine/EngineMaterials/Widget3DPassThrough_Translucent_OneSided"));
  TranslucentMaterial = TranslucentMaterial_Finder.Object;
  TranslucentMaterial_OneSided = TranslucentMaterial_OneSided_Finder.Object;
}

void UChromiumViewComponent::BeginPlay()
{
  Super::BeginPlay();

  CurrentDrawSize = DrawSize;
  InitView();
  UpdateBodySetup(true);
  RecreatePhysicsState();
}

void UChromiumViewComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
  ReleaseResources();
  Super::EndPlay(EndPlayReason);
}

void UChromiumViewComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
  Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

  UpdateView();

  if (bRedrawRequested || !bTickWhenOffscreen || WasRecentlyRendered())
  {
    DrawViewToRenderTarget(DeltaTime);
  }
}

void UChromiumViewComponent::OnRegister()
{
  CurrentDrawSize = DrawSize;
  Super::OnRegister();

#if !UE_SERVER
  if (!IsRunningDedicatedServer())
  {
    if (!WidgetRenderer)
    {
      WidgetRenderer = new FWidgetRenderer(false, true);
    }
  }
#endif
}

void UChromiumViewComponent::OnUnregister()
{
  ReleaseResources();
  Super::OnUnregister();
}

FPrimitiveSceneProxy* UChromiumViewComponent::CreateSceneProxy()
{
  if (WidgetRenderer && ChromiumViewWidget.IsValid())
  {
    if (ISlate3DRenderer* SlateRenderer = WidgetRenderer->GetSlateRenderer())
    {
      RequestRedraw();
      return new FChromiumView3DSceneProxy(this, *SlateRenderer);
    }
  }

  return nullptr;
}

FBoxSphereBounds UChromiumViewComponent::CalcBounds(const FTransform& LocalToWorld) const
{
  const FVector Origin = FVector(.5f,
    -(CurrentDrawSize.X * 0.5f) + (CurrentDrawSize.X * Pivot.X),
    -(CurrentDrawSize.Y * 0.5f) + (CurrentDrawSize.Y * Pivot.Y));

  const FVector BoxExtent = FVector(1.f, CurrentDrawSize.X / 2.0f, CurrentDrawSize.Y / 2.0f);

  FBoxSphereBounds NewBounds(FBox(Origin - BoxExtent, Origin + BoxExtent));
  NewBounds = NewBounds.TransformBy(LocalToWorld);

  NewBounds.BoxExtent *= BoundsScale;
  NewBounds.SphereRadius *= BoundsScale;

  return NewBounds;
}

UBodySetup* UChromiumViewComponent::GetBodySetup()
{
  UpdateBodySetup();
  return BodySetup;
}

FCollisionShape UChromiumViewComponent::GetCollisionShape(float Inflation) const
{
  FVector BoxHalfExtent = (FVector(0.01f, CurrentDrawSize.X * 0.5f, CurrentDrawSize.Y * 0.5f) * GetComponentTransform().GetScale3D()) + Inflation;
  if (Inflation < 0.0f)
  {
    BoxHalfExtent = BoxHalfExtent.ComponentMax(FVector::ZeroVector);
  }
  return FCollisionShape::MakeBox(BoxHalfExtent);
}

UMaterialInterface* UChromiumViewComponent::GetMaterial(int32 MaterialIndex) const
{
  if (OverrideMaterials.IsValidIndex(MaterialIndex) && OverrideMaterials[MaterialIndex])
  {
    return OverrideMaterials[MaterialIndex];
  }

  return bIsTwoSided ? TranslucentMaterial : TranslucentMaterial_OneSided;
}

void UChromiumViewComponent::SetMaterial(int32 ElementIndex, UMaterialInterface* Material)
{
  Super::SetMaterial(ElementIndex, Material);
  UpdateMaterialInstance();
}

int32 UChromiumViewComponent::GetNumMaterials() const
{
  return 1;
}

void UChromiumViewComponent::GetUsedMaterials(TArray<UMaterialInterface*>& OutMaterials, bool bGetDebugMaterials) const
{
  if (MaterialInstance)
  {
    OutMaterials.Add(MaterialInstance);
  }
}

#if WITH_EDITOR
void UChromiumViewComponent::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
  Super::PostEditChangeProperty(PropertyChangedEvent);

  FName PropertyName = (PropertyChangedEvent.Property != nullptr) ? PropertyChangedEvent.Property->GetFName() : NAME_None;

  if (PropertyName == GET_MEMBER_NAME_CHECKED(UChromiumViewComponent, DrawSize))
  {
    CurrentDrawSize = DrawSize;
    UpdateBodySetup(true);
    MarkRenderStateDirty();
  }
  else if (PropertyName == GET_MEMBER_NAME_CHECKED(UChromiumViewComponent, bIsTwoSided))
  {
    UpdateMaterialInstance();
  }
}
#endif

void UChromiumViewComponent::InitView()
{
  if (ChromiumViewWidget.IsValid())
  {
    return;
  }

  // Create the ChromiumView slate widget
  ChromiumViewWidget = SNew(SChromiumView)
    .SupportsTransparency(true)
    .BackgroundColor(FColor::Transparent)
    .FrameRate(60)
    .OnLoadCompleted(FSimpleDelegate::CreateUObject(this, &UChromiumViewComponent::HandleLoadCompleted));

  // Create the virtual window to host the widget
  SlateWindow = SNew(SVirtualWindow).Size(FVector2D(CurrentDrawSize.X, CurrentDrawSize.Y));
  SlateWindow->SetContent(ChromiumViewWidget.ToSharedRef());

  // Create the ViewModel bridge
  ViewModelBridge = NewObject<UChromiumViewModelBridge>(this);

  // Bind the bridge to JavaScript
  ChromiumViewWidget->BindUObject(TEXT("chromiumviewbridge"), ViewModelBridge, true);

  // Bind bridge delegates
  ViewModelBridge->OnViewReady.AddDynamic(this, &UChromiumViewComponent::HandleViewReady);
  ViewModelBridge->OnViewEvent.AddDynamic(this, &UChromiumViewComponent::HandleViewEvent);

  UpdateMaterialInstance();
  UpdateRenderTarget(CurrentDrawSize);

  // Load the view if configured
  if (!ViewConfig.HtmlPath.IsEmpty())
  {
    LoadViewWithConfig(ViewConfig);
  }
}

void UChromiumViewComponent::ReleaseResources()
{
  if (ViewModelBridge)
  {
    ViewModelBridge->Deinitialize();
    ViewModelBridge->OnViewReady.RemoveAll(this);
    ViewModelBridge->OnViewEvent.RemoveAll(this);
    ViewModelBridge = nullptr;
  }

  if (WidgetRenderer)
  {
    BeginCleanup(WidgetRenderer);
    WidgetRenderer = nullptr;
  }

  SlateWindow.Reset();
  ChromiumViewWidget.Reset();

  if (RenderTarget)
  {
    RenderTarget = nullptr;
  }

  if (MaterialInstance)
  {
    MaterialInstance = nullptr;
  }

  BoundViewModel = nullptr;
  bIsViewReady = false;
}

void UChromiumViewComponent::UpdateView()
{
  if (SlateWindow.IsValid() && CurrentDrawSize != DrawSize)
  {
    CurrentDrawSize = DrawSize;
    SlateWindow->Resize(FVector2D(CurrentDrawSize.X, CurrentDrawSize.Y));
    UpdateRenderTarget(CurrentDrawSize);
    UpdateBodySetup(true);
    MarkRenderStateDirty();
  }
}

void UChromiumViewComponent::UpdateRenderTarget(FIntPoint DesiredRenderTargetSize)
{
  bool bClearTarget = false;

  if (DesiredRenderTargetSize.X <= 0 || DesiredRenderTargetSize.Y <= 0)
  {
    return;
  }

  if (RenderTarget == nullptr)
  {
    RenderTarget = NewObject<UTextureRenderTarget2D>(this);
    RenderTarget->ClearColor = FLinearColor::Transparent;
    RenderTarget->InitCustomFormat(DesiredRenderTargetSize.X, DesiredRenderTargetSize.Y, PF_B8G8R8A8, false);
    bClearTarget = true;
  }
  else if (RenderTarget->SizeX != DesiredRenderTargetSize.X || RenderTarget->SizeY != DesiredRenderTargetSize.Y)
  {
    RenderTarget->ResizeTarget(DesiredRenderTargetSize.X, DesiredRenderTargetSize.Y);
    bClearTarget = true;
  }

  if (bClearTarget)
  {
    RenderTarget->UpdateResourceImmediate(false);
  }

  UpdateMaterialInstanceParameters();
}

void UChromiumViewComponent::UpdateBodySetup(bool bDrawSizeChanged)
{
  if (!BodySetup || bDrawSizeChanged)
  {
    if (!BodySetup)
    {
      BodySetup = NewObject<UBodySetup>(this, NAME_None, RF_Transient);
      BodySetup->CollisionTraceFlag = CTF_UseSimpleAsComplex;
    }

    BodySetup->AggGeom.BoxElems.Reset();

    FKBoxElem BoxElem;
    BoxElem.Center = FVector(0.0f, 0.0f, 0.0f);
    BoxElem.X = 0.01f;
    BoxElem.Y = CurrentDrawSize.X;
    BoxElem.Z = CurrentDrawSize.Y;
    BodySetup->AggGeom.BoxElems.Add(BoxElem);
  }
}

void UChromiumViewComponent::UpdateMaterialInstance()
{
  MaterialInstance = nullptr;

  UMaterialInterface* BaseMaterial = GetMaterial(0);
  MaterialInstance = UMaterialInstanceDynamic::Create(BaseMaterial, this);
  UpdateMaterialInstanceParameters();

  MarkRenderStateDirty();
}

void UChromiumViewComponent::UpdateMaterialInstanceParameters()
{
  if (MaterialInstance && RenderTarget)
  {
    MaterialInstance->SetTextureParameterValue(TEXT("SlateUI"), RenderTarget);
    MaterialInstance->SetVectorParameterValue(TEXT("TintColorAndOpacity"), TintColorAndOpacity);
    MaterialInstance->SetScalarParameterValue(TEXT("OpacityFromTexture"), 1.0f);
  }
}

void UChromiumViewComponent::DrawViewToRenderTarget(float DeltaTime)
{
  if (!WidgetRenderer || !SlateWindow.IsValid() || !RenderTarget)
  {
    return;
  }

  WidgetRenderer->DrawWindow(
    RenderTarget,
    SlateWindow->GetHittestGrid(),
    SlateWindow.ToSharedRef(),
    1.0f,
    FVector2D(CurrentDrawSize.X, CurrentDrawSize.Y),
    DeltaTime
  );

  bRedrawRequested = false;
}

void UChromiumViewComponent::LoadView(const FString& HtmlPath)
{
  FChromiumViewConfig Config;
  Config.HtmlPath = HtmlPath;
  LoadViewWithConfig(Config);
}

void UChromiumViewComponent::LoadViewWithConfig(const FChromiumViewConfig& Config)
{
  if (!ChromiumViewWidget.IsValid())
  {
    InitView();
  }

  ViewConfig = Config;
  CurrentHtmlPath = Config.HtmlPath;

  FString Url = ResolveViewUrl(Config.HtmlPath);

  bIsViewReady = false;
  ChromiumViewWidget->LoadURL(Url);
}

void UChromiumViewComponent::BindViewModel(UMVVMViewModelBase* InViewModel)
{
  if (BoundViewModel == InViewModel)
  {
    return;
  }

  UnbindViewModel();

  BoundViewModel = InViewModel;

  if (BoundViewModel && ViewModelBridge && ChromiumViewWidget.IsValid())
  {
    // Initialize with a delegate that executes JavaScript via our ChromiumViewWidget
    ViewModelBridge->InitializeWithDelegate(
      BoundViewModel,
      FOnExecuteJavascript::CreateLambda([this](const FString& ScriptText)
      {
        if (ChromiumViewWidget.IsValid())
        {
          ChromiumViewWidget->ExecuteJavascript(ScriptText);
        }
      })
    );
  }
}

void UChromiumViewComponent::UnbindViewModel()
{
  if (BoundViewModel && ViewModelBridge)
  {
    ViewModelBridge->Deinitialize();
  }
  BoundViewModel = nullptr;
}

UMVVMViewModelBase* UChromiumViewComponent::GetBoundViewModel() const
{
  return BoundViewModel;
}

void UChromiumViewComponent::ExecuteJavascript(const FString& ScriptText)
{
  if (ChromiumViewWidget.IsValid())
  {
    ChromiumViewWidget->ExecuteJavascript(ScriptText);
  }
}

UTextureRenderTarget2D* UChromiumViewComponent::GetRenderTarget() const
{
  return RenderTarget;
}

UMaterialInstanceDynamic* UChromiumViewComponent::GetMaterialInstance() const
{
  return MaterialInstance;
}

FVector2D UChromiumViewComponent::GetDrawSize() const
{
  return FVector2D(DrawSize.X, DrawSize.Y);
}

void UChromiumViewComponent::SetDrawSize(FVector2D Size)
{
  DrawSize = FIntPoint(FMath::RoundToInt(Size.X), FMath::RoundToInt(Size.Y));
}

void UChromiumViewComponent::RequestRedraw()
{
  bRedrawRequested = true;
}

void UChromiumViewComponent::SetTwoSided(bool bWantTwoSided)
{
  if (bIsTwoSided != bWantTwoSided)
  {
    bIsTwoSided = bWantTwoSided;
    UpdateMaterialInstance();
  }
}

void UChromiumViewComponent::InjectJavaScriptAPI()
{
  if (!ChromiumViewWidget.IsValid())
  {
    return;
  }

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

  ChromiumView.onFieldChanged = function(fieldName, callback) {
    if (!_fieldChangeListeners[fieldName]) {
      _fieldChangeListeners[fieldName] = [];
    }
    _fieldChangeListeners[fieldName].push(callback);
  };

  ChromiumView.offFieldChanged = function(fieldName, callback) {
    if (_fieldChangeListeners[fieldName]) {
      var index = _fieldChangeListeners[fieldName].indexOf(callback);
      if (index > -1) {
        _fieldChangeListeners[fieldName].splice(index, 1);
      }
    }
  };

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

  ChromiumView.getViewModelState = function() {
    return new Promise(function(resolve, reject) {
      if (window.ue && window.ue.chromiumviewbridge) {
        window.ue.chromiumviewbridge.getviewmodelstatejson().then(function(jsonString) {
          try { resolve(JSON.parse(jsonString)); } catch (e) { reject(e); }
        });
      } else {
        reject(new Error('ChromiumView bridge not available'));
      }
    });
  };

  ChromiumView.setDesiredSize = function(width, height, sizeMode) {
    if (window.ue && window.ue.chromiumviewbridge) {
      window.ue.chromiumviewbridge.setdesiredsize(width, height, sizeMode || 1);
    }
  };

  ChromiumView.onReady = function(callback) {
    if (_isReady) { callback(); } else { _readyListeners.push(callback); }
  };

  ChromiumView.notifyReady = function() {
    _isReady = true;
    _readyListeners.forEach(function(cb) { cb(); });
    _readyListeners = [];
    if (window.ue && window.ue.chromiumviewbridge) {
      window.ue.chromiumviewbridge.notifyviewready();
    }
  };

  ChromiumView._onFieldChanged = function(fieldName, newValue) {
    if (_fieldChangeListeners[fieldName]) {
      _fieldChangeListeners[fieldName].forEach(function(callback) {
        try { callback(newValue); } catch (e) { console.error('Error in field change listener:', e); }
      });
    }
  };

  ChromiumView._resolveCallback = function(callbackId, result, isError) {
    var callback = _callbacks[callbackId];
    if (callback) {
      delete _callbacks[callbackId];
      if (isError) { callback.reject(result); } else { callback.resolve(result); }
    }
  };

  console.log('[ChromiumView] API initialized');
})();
)JS";

  ChromiumViewWidget->ExecuteJavascript(JSCode);
}

void UChromiumViewComponent::HandleViewReady()
{
  bIsViewReady = true;
  OnViewReady.Broadcast();
}

void UChromiumViewComponent::HandleViewEvent(FName EventName, const FString& JsonPayload, int32 CallbackId)
{
  OnViewEvent.Broadcast(EventName, JsonPayload, CallbackId);
}

void UChromiumViewComponent::HandleLoadCompleted()
{
  InjectJavaScriptAPI();
}

FString UChromiumViewComponent::ResolveViewUrl(const FString& ViewPath) const
{
  // Strip .html extension if provided (for backwards compatibility)
  FString CleanPath = ViewPath;
  if (CleanPath.EndsWith(TEXT(".html")))
  {
    CleanPath = CleanPath.LeftChop(5);
  }

#if WITH_EDITOR
  const UChromiumViewDeveloperSettings* Settings = UChromiumViewDeveloperSettings::Get();
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
    UE_LOG(LogChromiumView, Log, TEXT("[Component] Using dev server: %s"), *FullUrl);
    return FullUrl;
  }
#endif

  // Use file:// URL with .html extension
  FString DiskPath = GetDiskPath(CleanPath + TEXT(".html"));
  if (!FPaths::FileExists(DiskPath))
  {
    UE_LOG(LogChromiumView, Warning, TEXT("[Component] View file not found: %s"), *DiskPath);
  }
  FString FileUrl = FString::Printf(TEXT("file:///%s"), *DiskPath.Replace(TEXT("\\"), TEXT("/")));
  UE_LOG(LogChromiumView, Log, TEXT("[Component] Using file URL: %s"), *FileUrl);
  return FileUrl;
}

FString UChromiumViewComponent::GetDiskPath(const FString& ViewPath) const
{
  FString ProjectDir = FPaths::ProjectDir();
  FString ViewDir = FPaths::Combine(ProjectDir, TEXT("ViewSource/dist"));

  FString FullPath = FPaths::Combine(ViewDir, ViewPath);

  if (!ViewPath.Contains(TEXT(".")))
  {
    FullPath += TEXT(".html");
  }

  FPaths::NormalizeFilename(FullPath);
  return FullPath;
}
