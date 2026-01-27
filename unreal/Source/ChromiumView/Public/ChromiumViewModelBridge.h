// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "MVVMViewModelBase.h"
#include "ChromiumViewTypes.h"
#include "WebJSFunction.h"
#include "Dom/JsonValue.h"

#include "ChromiumViewModelBridge.generated.h"

class UChromiumViewWidget;

/** Delegate for executing JavaScript in the view */
DECLARE_DELEGATE_OneParam(FOnExecuteJavascript, const FString& /* ScriptText */);

/**
 * Bridge class that connects a ViewModel to a Chromium View.
 * Handles bidirectional communication between C++ ViewModels and JavaScript.
 */
UCLASS(BlueprintType)
class CHROMIUMVIEW_API UChromiumViewModelBridge : public UObject
{
  GENERATED_BODY()

public:
  UChromiumViewModelBridge();

  /** Initialize the bridge with a ViewModel and UChromiumViewWidget */
  UFUNCTION(BlueprintCallable, Category = "ChromiumView|ViewModel")
  void Initialize(UMVVMViewModelBase* InViewModel, UChromiumViewWidget* InViewWidget);

  /** Initialize the bridge with a ViewModel and a JavaScript execution delegate (for UChromiumViewComponent) */
  void InitializeWithDelegate(UMVVMViewModelBase* InViewModel, FOnExecuteJavascript InExecuteJavascript);

  /** Cleanup the bridge */
  UFUNCTION(BlueprintCallable, Category = "ChromiumView|ViewModel")
  void Deinitialize();

  /** Get the bound ViewModel */
  UFUNCTION(BlueprintPure, Category = "ChromiumView|ViewModel")
  UMVVMViewModelBase* GetViewModel() const { return ViewModel; }

  /** Check if the bridge is initialized */
  UFUNCTION(BlueprintPure, Category = "ChromiumView|ViewModel")
  bool IsInitialized() const { return bIsInitialized; }

  /**
   * Called from JavaScript when the View sends an event to the ViewModel.
   * @param EventName The name of the event
   * @param JsonPayload JSON-encoded payload data
   * @param CallbackId Optional callback ID for async responses
   */
  UFUNCTION(BlueprintCallable, Category = "ChromiumView|ViewModel")
  void HandleViewEvent(const FString& EventName, const FString& JsonPayload, int32 CallbackId);

  /**
   * Called from JavaScript when the View requests the current ViewModel state.
   */
  UFUNCTION(BlueprintCallable, Category = "ChromiumView|ViewModel")
  FString GetViewModelStateJson();

  /**
   * Called from JavaScript when a field listener is registered.
   * Immediately sends the current value of the field back to JavaScript.
   * This ensures new listeners (e.g., after HMR) get the current value.
   * @param FieldName The name of the field to request
   */
  UFUNCTION(BlueprintCallable, Category = "ChromiumView|ViewModel")
  void RequestFieldValue(const FString& FieldName);

  /**
   * Called from JavaScript when the View reports its desired size.
   */
  UFUNCTION(BlueprintCallable, Category = "ChromiumView|ViewModel")
  void SetDesiredSize(float Width, float Height, int32 SizeMode);

  /**
   * Called from JavaScript when the View has finished loading and is ready.
   */
  UFUNCTION(BlueprintCallable, Category = "ChromiumView|ViewModel")
  void NotifyViewReady();

  /**
   * Send a response back to JavaScript for an async callback.
   * @param CallbackId The callback ID provided by the JS caller
   * @param JsonResult JSON-encoded result data
   * @param bIsError Whether this is an error response
   */
  UFUNCTION(BlueprintCallable, Category = "ChromiumView|ViewModel")
  void SendCallbackResponse(int32 CallbackId, const FString& JsonResult, bool bIsError);

  /** Delegate fired when a ViewModel field value changes */
  UPROPERTY(BlueprintAssignable, Category = "ChromiumView|ViewModel")
  FOnChromiumViewFieldChanged OnFieldChanged;

  /** Delegate fired when the View sends an event */
  UPROPERTY(BlueprintAssignable, Category = "ChromiumView|ViewModel")
  FOnChromiumViewEvent OnViewEvent;

  /** Delegate fired when the View's desired size changes */
  UPROPERTY(BlueprintAssignable, Category = "ChromiumView|ViewModel")
  FOnChromiumViewDesiredSizeChanged OnDesiredSizeChanged;

  /** Delegate fired when the View is ready */
  UPROPERTY(BlueprintAssignable, Category = "ChromiumView|ViewModel")
  FOnChromiumViewReady OnViewReady;

protected:
  /** Subscribe to ViewModel field change notifications */
  void SubscribeToFieldChanges();

  /** Unsubscribe from ViewModel field change notifications */
  void UnsubscribeFromFieldChanges();

  /** Handler for ViewModel field changes */
  void OnViewModelFieldChanged(UObject* Owner, UE::FieldNotification::FFieldId FieldId);

  /** Convert a field value to JSON string */
  FString FieldValueToJson(UE::FieldNotification::FFieldId FieldId);

  /** Convert a property value to JSON string */
  FString PropertyToJson(FProperty* Property);

  /** Convert a FJsonValue to a JSON string */
  static FString JsonValueToString(const TSharedPtr<FJsonValue>& JsonValue);

  /** Broadcast field change to JavaScript */
  void BroadcastFieldChangeToJS(const FName& FieldName, const FString& JsonValue);

private:
  UPROPERTY()
  TObjectPtr<UMVVMViewModelBase> ViewModel;

  UPROPERTY()
  TObjectPtr<UChromiumViewWidget> ViewWidget;

  /** Delegate for executing JavaScript (used when ViewWidget is null) */
  FOnExecuteJavascript ExecuteJavascriptDelegate;

  bool bIsInitialized = false;

  /** Delegate handles for field change subscriptions */
  TMap<UE::FieldNotification::FFieldId, FDelegateHandle> FieldChangeDelegateHandles;
};
