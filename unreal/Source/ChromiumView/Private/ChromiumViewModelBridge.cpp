// Copyright Epic Games, Inc. All Rights Reserved.

#include "ChromiumViewModelBridge.h"
#include "ChromiumViewWidget.h"
#include "SChromiumView.h"
#include "MVVMViewModelBase.h"
#include "FieldNotificationDeclaration.h"
#include "IFieldNotificationClassDescriptor.h"
#include "JsonObjectConverter.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "ChromiumViewModule.h"

UChromiumViewModelBridge::UChromiumViewModelBridge()
{
}

void UChromiumViewModelBridge::Initialize(UMVVMViewModelBase* InViewModel, UChromiumViewWidget* InViewWidget)
{
  if (bIsInitialized)
  {
    Deinitialize();
  }

  ViewModel = InViewModel;
  ViewWidget = InViewWidget;
  ExecuteJavascriptDelegate.Unbind();

  if (ViewModel && ViewWidget)
  {
    SubscribeToFieldChanges();
    bIsInitialized = true;
  }
}

void UChromiumViewModelBridge::InitializeWithDelegate(UMVVMViewModelBase* InViewModel, FOnExecuteJavascript InExecuteJavascript)
{
  if (bIsInitialized)
  {
    Deinitialize();
  }

  ViewModel = InViewModel;
  ViewWidget = nullptr;
  ExecuteJavascriptDelegate = InExecuteJavascript;

  if (ViewModel && ExecuteJavascriptDelegate.IsBound())
  {
    SubscribeToFieldChanges();
    bIsInitialized = true;
  }
}

void UChromiumViewModelBridge::Deinitialize()
{
  if (bIsInitialized)
  {
    UnsubscribeFromFieldChanges();
  }

  ViewModel = nullptr;
  ViewWidget = nullptr;
  ExecuteJavascriptDelegate.Unbind();
  bIsInitialized = false;
}

void UChromiumViewModelBridge::SubscribeToFieldChanges()
{
  if (!ViewModel)
  {
    return;
  }

  // Get the field notification descriptor to iterate all fields
  const UE::FieldNotification::IClassDescriptor& Descriptor = ViewModel->GetFieldNotificationDescriptor();

  // Subscribe to each field
  Descriptor.ForEachField(ViewModel->GetClass(), [this](UE::FieldNotification::FFieldId FieldId) -> bool
  {
    FDelegateHandle Handle = ViewModel->AddFieldValueChangedDelegate(
      FieldId,
      INotifyFieldValueChanged::FFieldValueChangedDelegate::CreateUObject(
        this, &UChromiumViewModelBridge::OnViewModelFieldChanged));

    FieldChangeDelegateHandles.Add(FieldId, Handle);
    return true; // Continue iteration
  });

#if WITH_EDITORONLY_DATA
  // AngelScript ViewModels can't emit UHT's static FieldNotify descriptor, so ForEachField
  // above misses their fields. Also subscribe to any UPROPERTY tagged with FieldNotify
  // metadata (set by the AngelScript class generator) by name. The delegate store is
  // name-keyed (FFieldId equality/hash are name-based), so a name broadcast from AngelScript
  // (FieldNotify::NotifyFieldChanged) reaches these handlers.
  for (TFieldIterator<FProperty> PropIt(ViewModel->GetClass()); PropIt; ++PropIt)
  {
    FProperty* Property = *PropIt;
    if (!Property->HasMetaData(TEXT("FieldNotify")))
    {
      continue;
    }
    UE::FieldNotification::FFieldId FieldId(Property->GetFName(), /*BitNumber*/ 0);
    if (FieldChangeDelegateHandles.Contains(FieldId))
    {
      continue; // already subscribed via the static descriptor
    }
    FDelegateHandle Handle = ViewModel->AddFieldValueChangedDelegate(
      FieldId,
      INotifyFieldValueChanged::FFieldValueChangedDelegate::CreateUObject(
        this, &UChromiumViewModelBridge::OnViewModelFieldChanged));
    FieldChangeDelegateHandles.Add(FieldId, Handle);
  }
#endif // WITH_EDITORONLY_DATA
}

void UChromiumViewModelBridge::UnsubscribeFromFieldChanges()
{
  if (!ViewModel)
  {
    return;
  }

  for (const auto& Pair : FieldChangeDelegateHandles)
  {
    ViewModel->RemoveFieldValueChangedDelegate(Pair.Key, Pair.Value);
  }

  FieldChangeDelegateHandles.Empty();
}

void UChromiumViewModelBridge::OnViewModelFieldChanged(UObject* Owner, UE::FieldNotification::FFieldId FieldId)
{
  if (!bIsInitialized || !ViewModel || Owner != ViewModel)
  {
    return;
  }

  FName FieldName = FieldId.GetName();
  FString JsonValue = FieldValueToJson(FieldId);

  // Broadcast the change through delegate
  OnFieldChanged.Broadcast(FieldName, JsonValue);

  // Send to JavaScript
  BroadcastFieldChangeToJS(FieldName, JsonValue);
}

FString UChromiumViewModelBridge::FieldValueToJson(UE::FieldNotification::FFieldId FieldId)
{
  if (!ViewModel)
  {
    return TEXT("null");
  }

  UClass* ViewModelClass = ViewModel->GetClass();
  FName FieldName = FieldId.GetName();

  // Find the property in the ViewModel
  FProperty* Property = ViewModelClass->FindPropertyByName(FieldName);
  return PropertyToJson(Property);
}

FString UChromiumViewModelBridge::PropertyToJson(FProperty* Property)
{
  if (!ViewModel || !Property)
  {
    return TEXT("null");
  }

  // Get the property value
  const void* PropertyValue = Property->ContainerPtrToValuePtr<void>(ViewModel);

  // Use UE's built-in property to JSON conversion - handles all types including arrays of structs
  TSharedPtr<FJsonValue> JsonValue = FJsonObjectConverter::UPropertyToJsonValue(Property, PropertyValue);
  if (!JsonValue.IsValid())
  {
    return TEXT("null");
  }

  // Serialize the JSON value to a string based on its type
  return JsonValueToString(JsonValue);
}

FString UChromiumViewModelBridge::JsonValueToString(const TSharedPtr<FJsonValue>& JsonValue)
{
  if (!JsonValue.IsValid())
  {
    return TEXT("null");
  }

  switch (JsonValue->Type)
  {
    case EJson::Null:
      return TEXT("null");

    case EJson::Boolean:
      return JsonValue->AsBool() ? TEXT("true") : TEXT("false");

    case EJson::Number:
      return FString::Printf(TEXT("%g"), JsonValue->AsNumber());

    case EJson::String:
    {
      // Properly escape and quote the string for JSON
      FString EscapedString = JsonValue->AsString();
      EscapedString = EscapedString.Replace(TEXT("\\"), TEXT("\\\\"));
      EscapedString = EscapedString.Replace(TEXT("\""), TEXT("\\\""));
      EscapedString = EscapedString.Replace(TEXT("\n"), TEXT("\\n"));
      EscapedString = EscapedString.Replace(TEXT("\r"), TEXT("\\r"));
      EscapedString = EscapedString.Replace(TEXT("\t"), TEXT("\\t"));
      return FString::Printf(TEXT("\"%s\""), *EscapedString);
    }

    case EJson::Array:
    {
      FString OutputString;
      TSharedRef<TJsonWriter<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>> Writer =
        TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&OutputString);
      FJsonSerializer::Serialize(JsonValue->AsArray(), Writer);
      Writer->Close();
      return OutputString;
    }

    case EJson::Object:
    {
      FString OutputString;
      TSharedRef<TJsonWriter<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>> Writer =
        TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&OutputString);
      FJsonSerializer::Serialize(JsonValue->AsObject().ToSharedRef(), Writer);
      Writer->Close();
      return OutputString;
    }

    default:
      return TEXT("null");
  }
}

void UChromiumViewModelBridge::BroadcastFieldChangeToJS(const FName& FieldName, const FString& JsonValue)
{
  // Build the JavaScript call to notify the View of the change
  FString JSCode = FString::Printf(
    TEXT("if (window.ChromiumView && window.ChromiumView._onFieldChanged) { window.ChromiumView._onFieldChanged('%s', %s); }"),
    *FieldName.ToString(),
    *JsonValue
  );

  // Execute via ViewWidget or delegate
  if (ViewWidget)
  {
    ViewWidget->ExecuteJavascript(JSCode);
  }
  else if (ExecuteJavascriptDelegate.IsBound())
  {
    ExecuteJavascriptDelegate.Execute(JSCode);
  }
}

void UChromiumViewModelBridge::HandleViewEvent(const FString& EventName, const FString& JsonPayload, int32 CallbackId)
{
  FName EventFName(*EventName);

  // MVVM command dispatch: invoke the matching UFUNCTION on the bound ViewModel (the
  // "command" the view fires via createSendEvent). The JSON payload's fields are
  // marshalled into the function's parameters by name (e.g. {"OptionId":"Self"} ->
  // void ChooseOption(FName OptionId)). Without this the view->ViewModel direction is
  // dead — events would only reach OnViewEvent listeners.
  if (ViewModel)
  {
    if (UFunction* Func = ViewModel->FindFunction(EventFName))
    {
      void* Params = FMemory::Malloc(FMath::Max<int32>(Func->ParmsSize, 1));
      Func->InitializeStruct(Params);

      if (!JsonPayload.IsEmpty())
      {
        TSharedPtr<FJsonObject> JsonObject;
        TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonPayload);
        if (FJsonSerializer::Deserialize(Reader, JsonObject) && JsonObject.IsValid())
        {
          // A UFunction is a UStruct whose child properties are its parameters, so the
          // standard struct converter maps payload fields onto the params by name.
          FJsonObjectConverter::JsonObjectToUStruct(JsonObject.ToSharedRef(), Func, Params, 0, 0);
        }
      }

      ViewModel->ProcessEvent(Func, Params);

      Func->DestroyStruct(Params);
      FMemory::Free(Params);
    }
    else
    {
      UE_LOG(LogChromiumView, Warning,
        TEXT("View event '%s' has no matching UFUNCTION on ViewModel '%s'"),
        *EventName, *GetNameSafe(ViewModel));
    }
  }

  OnViewEvent.Broadcast(EventFName, JsonPayload, CallbackId);
}

FString UChromiumViewModelBridge::GetViewModelStateJson()
{
  if (!ViewModel)
  {
    return TEXT("{}");
  }

  // Serialize the entire ViewModel to JSON
  TSharedRef<FJsonObject> JsonObject = MakeShared<FJsonObject>();
  UClass* ViewModelClass = ViewModel->GetClass();

  // Iterate through all properties
  for (TFieldIterator<FProperty> PropIt(ViewModelClass); PropIt; ++PropIt)
  {
    FProperty* Property = *PropIt;

    // Skip properties that aren't blueprint visible
    if (!Property->HasAnyPropertyFlags(CPF_BlueprintVisible))
    {
      continue;
    }

    FName FieldName = Property->GetFName();
    void* PropertyValue = Property->ContainerPtrToValuePtr<void>(ViewModel);

    // Add to JSON object based on type
    if (FBoolProperty* BoolProp = CastField<FBoolProperty>(Property))
    {
      JsonObject->SetBoolField(FieldName.ToString(), BoolProp->GetPropertyValue(PropertyValue));
    }
    else if (FIntProperty* IntProp = CastField<FIntProperty>(Property))
    {
      JsonObject->SetNumberField(FieldName.ToString(), IntProp->GetPropertyValue(PropertyValue));
    }
    else if (FFloatProperty* FloatProp = CastField<FFloatProperty>(Property))
    {
      JsonObject->SetNumberField(FieldName.ToString(), FloatProp->GetPropertyValue(PropertyValue));
    }
    else if (FDoubleProperty* DoubleProp = CastField<FDoubleProperty>(Property))
    {
      JsonObject->SetNumberField(FieldName.ToString(), DoubleProp->GetPropertyValue(PropertyValue));
    }
    else if (FStrProperty* StrProp = CastField<FStrProperty>(Property))
    {
      JsonObject->SetStringField(FieldName.ToString(), StrProp->GetPropertyValue(PropertyValue));
    }
    else if (FTextProperty* TextProp = CastField<FTextProperty>(Property))
    {
      JsonObject->SetStringField(FieldName.ToString(), TextProp->GetPropertyValue(PropertyValue).ToString());
    }
    else if (FNameProperty* NameProp = CastField<FNameProperty>(Property))
    {
      JsonObject->SetStringField(FieldName.ToString(), NameProp->GetPropertyValue(PropertyValue).ToString());
    }
    // Additional types can be added here
  }

  FString OutputString;
  TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
  FJsonSerializer::Serialize(JsonObject, Writer);

  return OutputString;
}

void UChromiumViewModelBridge::SetDesiredSize(float Width, float Height, int32 SizeMode)
{
  FChromiumViewDesiredSize DesiredSize;
  DesiredSize.Width = Width;
  DesiredSize.Height = Height;
  DesiredSize.SizeMode = static_cast<EChromiumViewSizeMode>(FMath::Clamp(SizeMode, 0, 2));

  OnDesiredSizeChanged.Broadcast(DesiredSize);
}

void UChromiumViewModelBridge::NotifyViewReady()
{
  OnViewReady.Broadcast();
}

void UChromiumViewModelBridge::SendCallbackResponse(int32 CallbackId, const FString& JsonResult, bool bIsError)
{
  // Send the callback response to JavaScript
  FString JSCode = FString::Printf(
    TEXT("if (window.ChromiumView && window.ChromiumView._resolveCallback) { window.ChromiumView._resolveCallback(%d, %s, %s); }"),
    CallbackId,
    *JsonResult,
    bIsError ? TEXT("true") : TEXT("false")
  );

  // Execute via ViewWidget or delegate
  if (ViewWidget)
  {
    ViewWidget->ExecuteJavascript(JSCode);
  }
  else if (ExecuteJavascriptDelegate.IsBound())
  {
    ExecuteJavascriptDelegate.Execute(JSCode);
  }
}

void UChromiumViewModelBridge::RequestFieldValue(const FString& FieldName)
{
  if (!ViewModel)
  {
    return;
  }

  // Check we have a way to execute JavaScript
  if (!ViewWidget && !ExecuteJavascriptDelegate.IsBound())
  {
    return;
  }

  // Find the property by name
  UClass* ViewModelClass = ViewModel->GetClass();
  FProperty* Property = ViewModelClass->FindPropertyByName(FName(*FieldName));

  if (!Property)
  {
    UE_LOG(LogChromiumView, Warning, TEXT("RequestFieldValue: Field '%s' not found on ViewModel"), *FieldName);
    return;
  }

  // Get the JSON value for this field
  FString JsonValue = PropertyToJson(Property);

  // Broadcast to JavaScript (reuse existing mechanism)
  BroadcastFieldChangeToJS(FName(*FieldName), JsonValue);
}
