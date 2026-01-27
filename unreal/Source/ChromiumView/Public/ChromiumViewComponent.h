// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/MeshComponent.h"
#include "ChromiumViewTypes.h"

#include "ChromiumViewComponent.generated.h"

class FWidgetRenderer;
class SChromiumView;
class SVirtualWindow;
class UBodySetup;
class UMaterialInstanceDynamic;
class UTextureRenderTarget2D;
class UMVVMViewModelBase;
class UChromiumViewModelBridge;

/**
 * A component that renders a ChromiumView (web browser) in 3D world space.
 * Similar to UWidgetComponent but specifically for ChromiumView web content.
 *
 * The web content is rendered to a texture which is then displayed on a mesh
 * in the world, allowing for in-world UI panels, interactive screens, etc.
 */
UCLASS(Blueprintable, ClassGroup = "UserInterface", hidecategories = (Object, Activation, "Components|Activation", Sockets, Base, Lighting, LOD, Mesh), editinlinenew, meta = (BlueprintSpawnableComponent))
class CHROMIUMVIEW_API UChromiumViewComponent : public UMeshComponent
{
  GENERATED_UCLASS_BODY()

public:
  //~ Begin UActorComponent Interface
  virtual void BeginPlay() override;
  virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
  virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
  //~ End UActorComponent Interface

  //~ Begin UPrimitiveComponent Interface
  virtual FPrimitiveSceneProxy* CreateSceneProxy() override;
  virtual FBoxSphereBounds CalcBounds(const FTransform& LocalToWorld) const override;
  virtual UBodySetup* GetBodySetup() override;
  virtual FCollisionShape GetCollisionShape(float Inflation) const override;
  virtual void OnRegister() override;
  virtual void OnUnregister() override;
  virtual UMaterialInterface* GetMaterial(int32 MaterialIndex) const override;
  virtual void SetMaterial(int32 ElementIndex, UMaterialInterface* Material) override;
  virtual int32 GetNumMaterials() const override;
  virtual void GetUsedMaterials(TArray<UMaterialInterface*>& OutMaterials, bool bGetDebugMaterials = false) const override;
  //~ End UPrimitiveComponent Interface

#if WITH_EDITOR
  virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

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

  /** Returns the render target to which the web content is rendered */
  UFUNCTION(BlueprintCallable, Category = "ChromiumView")
  UTextureRenderTarget2D* GetRenderTarget() const;

  /** Returns the dynamic material instance used to render the web content */
  UFUNCTION(BlueprintCallable, Category = "ChromiumView")
  UMaterialInstanceDynamic* GetMaterialInstance() const;

  /** Returns the draw size of the component in world units */
  UFUNCTION(BlueprintCallable, Category = "ChromiumView")
  FVector2D GetDrawSize() const;

  /** Sets the draw size of the component in world units */
  UFUNCTION(BlueprintCallable, Category = "ChromiumView")
  void SetDrawSize(FVector2D Size);

  /** Requests that the view be redrawn */
  UFUNCTION(BlueprintCallable, Category = "ChromiumView")
  void RequestRedraw();

  /** Gets whether the component is two-sided */
  UFUNCTION(BlueprintCallable, Category = "Rendering")
  bool GetTwoSided() const { return bIsTwoSided; }

  /** Sets whether the component is two-sided */
  UFUNCTION(BlueprintCallable, Category = "Rendering")
  void SetTwoSided(bool bWantTwoSided);

  /** Returns the pivot point where the view is rendered about the origin */
  UFUNCTION(BlueprintCallable, Category = "ChromiumView")
  FVector2D GetPivot() const { return Pivot; }

  /** Sets the pivot point */
  UFUNCTION(BlueprintCallable, Category = "ChromiumView")
  void SetPivot(const FVector2D& InPivot) { Pivot = InPivot; }

  /** Delegate fired when the View is ready */
  UPROPERTY(BlueprintAssignable, Category = "ChromiumView")
  FOnChromiumViewReady OnViewReady;

  /** Delegate fired when the View sends an event */
  UPROPERTY(BlueprintAssignable, Category = "ChromiumView")
  FOnChromiumViewEvent OnViewEvent;

  /** Get the ViewModel bridge */
  UChromiumViewModelBridge* GetViewModelBridge() const { return ViewModelBridge; }

protected:
  /** Initialize the ChromiumView */
  virtual void InitView();

  /** Release resources associated with the view */
  virtual void ReleaseResources();

  /** Update the view render */
  virtual void UpdateView();

  /** Ensure the render target is initialized and updates it if needed */
  virtual void UpdateRenderTarget(FIntPoint DesiredRenderTargetSize);

  /** Update the body setup for collision */
  void UpdateBodySetup(bool bDrawSizeChanged = false);

  /** Update the material instance */
  void UpdateMaterialInstance();

  /** Update material instance parameters */
  void UpdateMaterialInstanceParameters();

  /** Draw the ChromiumView to the render target */
  virtual void DrawViewToRenderTarget(float DeltaTime);

  /** Inject the ChromiumView JavaScript API into the View */
  void InjectJavaScriptAPI();

  /** Handle View ready notification from the bridge */
  UFUNCTION()
  void HandleViewReady();

  /** Handle View event from the bridge */
  UFUNCTION()
  void HandleViewEvent(FName EventName, const FString& JsonPayload, int32 CallbackId);

  /** Handle load completed */
  void HandleLoadCompleted();

  /**
   * Resolve a View path to a full URL.
   * @param ViewPath The view path without extension
   * @return The full URL to load
   */
  FString ResolveViewUrl(const FString& ViewPath) const;

  /**
   * Get the full file path for a View on disk.
   * @param ViewPath The view path without extension
   * @return The full file path with .html extension
   */
  FString GetDiskPath(const FString& ViewPath) const;

protected:
  /** The size of the displayed quad in world units */
  UPROPERTY(EditAnywhere, Category = "ChromiumView")
  FIntPoint DrawSize;

  /** The Alignment/Pivot point that the view is placed at relative to the position */
  UPROPERTY(EditAnywhere, Category = "ChromiumView")
  FVector2D Pivot;

  /** Configuration for the View */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ChromiumView")
  FChromiumViewConfig ViewConfig;

  /** The background color of the component */
  UPROPERTY(EditAnywhere, Category = "Rendering")
  FLinearColor BackgroundColor;

  /** Tint color and opacity for this component */
  UPROPERTY(EditAnywhere, Category = "Rendering")
  FLinearColor TintColorAndOpacity;

  /** Is the component visible from behind? */
  UPROPERTY(EditAnywhere, Category = "Rendering")
  bool bIsTwoSided;

  /** Should the component tick when offscreen? */
  UPROPERTY(EditAnywhere, Category = "Animation")
  bool bTickWhenOffscreen;

  /** The body setup of the displayed quad */
  UPROPERTY(Transient, DuplicateTransient)
  TObjectPtr<UBodySetup> BodySetup;

  /** The material instance for translucent rendering */
  UPROPERTY()
  TObjectPtr<UMaterialInterface> TranslucentMaterial;

  /** The material instance for translucent, one-sided rendering */
  UPROPERTY()
  TObjectPtr<UMaterialInterface> TranslucentMaterial_OneSided;

  /** The target to which the web content is rendered */
  UPROPERTY(Transient, DuplicateTransient)
  TObjectPtr<UTextureRenderTarget2D> RenderTarget;

  /** The dynamic instance of the material that the render target is attached to */
  UPROPERTY(Transient, DuplicateTransient)
  TObjectPtr<UMaterialInstanceDynamic> MaterialInstance;

  /** The actual draw size (may differ from DrawSize based on content) */
  UPROPERTY(Transient, DuplicateTransient)
  FIntPoint CurrentDrawSize;

private:
  /** The slate window that contains the ChromiumView content */
  TSharedPtr<SVirtualWindow> SlateWindow;

  /** The ChromiumView slate widget */
  TSharedPtr<SChromiumView> ChromiumViewWidget;

  /** Helper class for drawing to a render target */
  FWidgetRenderer* WidgetRenderer;

  UPROPERTY()
  TObjectPtr<UChromiumViewModelBridge> ViewModelBridge;

  UPROPERTY()
  TObjectPtr<UMVVMViewModelBase> BoundViewModel;

  bool bIsViewReady = false;
  bool bRedrawRequested = true;
  FString CurrentHtmlPath;
};
