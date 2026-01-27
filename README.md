# ChromiumView Plugin

A plugin for Unreal Engine that implements the Model-View-ViewModel (MVVM) design pattern using the WebBrowser (Chromium) module as the View layer.

## Overview

ChromiumView allows you to create game UI using web technologies (HTML, CSS, JavaScript/TypeScript) while maintaining the benefits of Unreal Engine's MVVM pattern. It acts as a bridge between:

- **Model**: Your game data (managed by Unreal Engine)
- **ViewModel**: `UMVVMViewModelBase` classes (from the existing ModelViewViewModel plugin)
- **View**: HTML/JavaScript web pages rendered via Chromium

## Features

- **Web-based Views**: Create UI using HTML, CSS, and JavaScript
- **ViewModel Binding**: Automatic synchronization between ViewModel properties and JavaScript
- **Bidirectional Communication**: Views can send events to ViewModels and receive property updates
- **TypeScript Support**: Full TypeScript definitions for type-safe development
- **Flexible Sizing**: Views can be full-screen, use a desired size, or auto-size
- **Works in Editor and Packaged Builds**: Views are loaded from the project's `View` directory

## Installation

1. Enable the plugin in your project's `.uproject` file:

```json
{
  "Plugins": [
    {
      "Name": "ChromiumView",
      "Enabled": true
    },
    {
      "Name": "ModelViewViewModel",
      "Enabled": true
    }
  ]
}
```

2. Create a `View` directory in your project's root folder to store HTML files.

## Quick Start

### 1. Create a ViewModel (C++)

```cpp
#pragma once

#include "MVVMViewModelBase.h"
#include "MyGameViewModel.generated.h"

UCLASS(BlueprintType)
class UMyGameViewModel : public UMVVMViewModelBase
{
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintReadWrite, FieldNotify, Category = "Game")
    FString PlayerName;

    UPROPERTY(BlueprintReadWrite, FieldNotify, Category = "Game")
    int32 Score;

    UPROPERTY(BlueprintReadWrite, FieldNotify, Category = "Game")
    float Health;

    UFUNCTION(BlueprintCallable, Category = "Game")
    void OnPlayerAction(const FString& Action);
};
```

### 2. Create an HTML View

Create `View/game-hud.html` in your project:

```html
<!DOCTYPE html>
<html>
<head>
    <title>Game HUD</title>
    <style>
        body { font-family: Arial; color: white; background: transparent; }
        .score { font-size: 24px; }
    </style>
</head>
<body>
    <div class="hud">
        <div class="player-name" id="player-name">Player</div>
        <div class="score">Score: <span id="score">0</span></div>
        <button id="action-btn">Take Action</button>
    </div>

    <script>
        // Wait for ChromiumView to be available
        if (window.ChromiumView) {
            const cv = window.ChromiumView;

            // Listen for ViewModel changes
            cv.onFieldChanged("PlayerName", (name) => {
                document.getElementById("player-name").textContent = name;
            });

            cv.onFieldChanged("Score", (score) => {
                document.getElementById("score").textContent = score;
            });

            // Send events to ViewModel
            document.getElementById("action-btn").onclick = () => {
                cv.sendEvent("OnPlayerAction", { action: "attack" });
            };

            // Notify the View is ready
            cv.notifyReady();
        }
    </script>
</body>
</html>
```

### 3. Use in Blueprint or C++

**Blueprint:**
1. Create a `ChromiumViewWidget`
2. Call `LoadView("game-hud.html")`
3. Create your ViewModel and call `BindViewModel`
4. Add the widget to your viewport

**C++:**
```cpp
// Create the widget
UChromiumViewWidget* ViewWidget = CreateWidget<UChromiumViewWidget>(GetWorld());

// Load the HTML view
ViewWidget->LoadView(TEXT("game-hud.html"));

// Create and bind the ViewModel
UMyGameViewModel* ViewModel = NewObject<UMyGameViewModel>();
ViewWidget->BindViewModel(ViewModel);

// Add to viewport
ViewWidget->AddToViewport();
```

## TypeScript Development

The plugin includes TypeScript definitions for type-safe View development.

### Setup

```bash
cd Engine/Plugins/ChromiumView/TypeScript
npm install
npm run build
```

### Usage

```typescript
import { createChromiumView, ChromiumViewSizeMode } from "@unreal/chromium-view";

const cv = createChromiumView();

// Type-safe field changes
cv.onFieldChanged<number>("Score", (score) => {
    console.log("New score:", score);
});

// Type-safe events
interface PlayerAction {
    action: string;
    target?: string;
}

cv.sendEvent<PlayerAction>("OnPlayerAction", {
    action: "attack",
    target: "enemy_1"
});

cv.notifyReady();
```

## API Reference

### UChromiumViewWidget

| Method | Description |
|--------|-------------|
| `LoadView(HtmlPath)` | Load an HTML view from the View directory |
| `LoadViewWithConfig(Config)` | Load a view with configuration options |
| `BindViewModel(ViewModel)` | Bind a ViewModel to the View |
| `UnbindViewModel()` | Unbind the current ViewModel |
| `ExecuteJavascript(Script)` | Execute JavaScript in the View |
| `ReloadView()` | Reload the current View |
| `SetSizeMode(Mode)` | Set the size mode of the View |

### JavaScript API (window.ChromiumView)

| Method | Description |
|--------|-------------|
| `onFieldChanged(fieldName, callback)` | Listen for ViewModel property changes |
| `offFieldChanged(fieldName, callback)` | Remove a field change listener |
| `sendEvent(eventName, payload)` | Send an event to the ViewModel |
| `getViewModelState()` | Get the current ViewModel state |
| `setDesiredSize(width, height, mode)` | Set the desired size of the View |
| `onReady(callback)` | Register a callback for when ready |
| `notifyReady()` | Notify that the View is ready |

## File Structure

```
YourProject/
├── View/                          # Your HTML views go here
│   ├── main-menu.html
│   ├── game-hud.html
│   └── assets/
│       ├── styles.css
│       └── app.js
└── Content/
    └── ...
```

## Configuration

### FChromiumViewConfig

| Property | Type | Default | Description |
|----------|------|---------|-------------|
| `HtmlPath` | FString | "" | Relative path to HTML file |
| `InitialSizeMode` | EChromiumViewSizeMode | Auto | Initial size mode |
| `bSupportsTransparency` | bool | true | Enable transparency |
| `BackgroundColor` | FColor | White | Background color |
| `FrameRate` | int32 | 60 | Browser frame rate |

### Size Modes

| Mode | Description |
|------|-------------|
| `FullScreen` | View fills the entire parent |
| `DesiredSize` | View uses dimensions set by `setDesiredSize()` |
| `Auto` | Parent widget controls size |

## Best Practices

1. **Always call `notifyReady()`** after your View has finished initializing
2. **Use TypeScript** for better type safety and IDE support
3. **Keep Views lightweight** - complex logic should be in ViewModels
4. **Handle initial state** - call `getViewModelState()` when loading
5. **Clean up listeners** when Views are destroyed

## Limitations

- Views are loaded from files, not packed in UFS
- JavaScript execution is async - use callbacks/promises appropriately
- Debug tools may be limited in packaged builds

## Dependencies

- ModelViewViewModel plugin (enabled automatically)
- WebBrowser module (part of Unreal Engine)

## License

See LICENSE file for details.
