# ChromiumView TypeScript Bindings

TypeScript definitions and utilities for building web-based Views that communicate with Unreal Engine ViewModels using the ChromiumView MVVM plugin.

## Installation

```bash
npm install @unreal/chromium-view
```

Or copy the `dist` folder to your project.

## Quick Start

### Basic Usage

```typescript
import { createChromiumView, ChromiumViewSizeMode } from "@unreal/chromium-view";

// Create the ChromiumView client
const chromiumView = createChromiumView();

// Listen for ViewModel field changes
chromiumView.onFieldChanged("playerHealth", (newHealth: number) => {
  document.getElementById("health-bar")!.style.width = `${newHealth}%`;
});

chromiumView.onFieldChanged("playerName", (name: string) => {
  document.getElementById("player-name")!.textContent = name;
});

// Send events to the ViewModel
document.getElementById("action-button")!.addEventListener("click", async () => {
  const result = await chromiumView.sendEvent("buttonClicked", {
    buttonId: "action",
    timestamp: Date.now(),
  });
  console.log("Event result:", result);
});

// Set the desired size of the view
chromiumView.setDesiredSize(800, 600, ChromiumViewSizeMode.DesiredSize);

// Notify that the View is ready
chromiumView.notifyReady();
```

### Using Decorators (TypeScript)

```typescript
import {
  ChromiumViewBase,
  BindField,
  OnFieldChange,
} from "@unreal/chromium-view";

class GameHUD extends ChromiumViewBase {
  // Automatically updates when ViewModel.PlayerHealth changes
  @BindField("PlayerHealth")
  health: number = 100;

  @BindField("PlayerName")
  name: string = "";

  // Called when ViewModel.Score changes
  @OnFieldChange("Score")
  onScoreChanged(newScore: number) {
    this.updateScoreDisplay(newScore);
  }

  private updateScoreDisplay(score: number) {
    document.getElementById("score")!.textContent = `Score: ${score}`;
  }

  async onButtonClick() {
    await this.sendEvent("UIButtonClicked", { buttonId: "start" });
  }
}

const hud = new GameHUD();
```

## API Reference

### ChromiumViewAPI

The main API for communicating with Unreal Engine ViewModels.

#### Methods

- `onFieldChanged<T>(fieldName: string, callback: (newValue: T) => void): void`
  - Register a callback for when a ViewModel field changes

- `offFieldChanged<T>(fieldName: string, callback: (newValue: T) => void): void`
  - Remove a field change callback

- `sendEvent<TPayload, TResponse>(eventName: string, payload?: TPayload): Promise<TResponse>`
  - Send an event to the ViewModel

- `getViewModelState<T>(): Promise<T>`
  - Get the current ViewModel state

- `setDesiredSize(width: number, height: number, sizeMode?: ChromiumViewSizeMode): void`
  - Set the desired size of the View

- `onReady(callback: () => void): void`
  - Register a callback for when the View is ready

- `notifyReady(): void`
  - Notify Unreal Engine that the View has finished loading

### ChromiumViewSizeMode

```typescript
enum ChromiumViewSizeMode {
  FullScreen = 0,   // View fills the parent
  DesiredSize = 1,  // View uses specified dimensions
  Auto = 2,         // Parent controls size
}
```

### Decorators

- `@BindField(fieldName: string)` - Bind a property to a ViewModel field
- `@OnFieldChange(fieldName: string)` - Mark a method as a field change handler

### Utility Functions

- `createChromiumView(): ChromiumViewClient` - Create a new ChromiumView client
- `getChromiumView(): ChromiumViewAPI | undefined` - Get the global API instance
- `isChromiumViewAvailable(): boolean` - Check if running in ChromiumView context
- `initializeBindings(instance: object): void` - Initialize decorators on an instance

## ViewModel Communication

### Field Changes (ViewModel → View)

When a ViewModel field changes in Unreal Engine, the ChromiumView bridge automatically notifies the JavaScript side. Register listeners using `onFieldChanged`:

```typescript
chromiumView.onFieldChanged("FieldName", (newValue) => {
  // Handle the change
});
```

### Events (View → ViewModel)

Send events from JavaScript to the ViewModel using `sendEvent`:

```typescript
// Fire-and-forget
chromiumView.sendEvent("EventName", { data: "value" });

// With async response
const response = await chromiumView.sendEvent("QueryEvent", { query: "test" });
```

Handle these events in your Unreal Engine Blueprint or C++ code using the `OnViewEvent` delegate on `UChromiumViewWidget`.

## View Sizing

Views can control their size by calling `setDesiredSize`:

```typescript
// Set a fixed size
chromiumView.setDesiredSize(1920, 1080, ChromiumViewSizeMode.DesiredSize);

// Request full screen
chromiumView.setDesiredSize(0, 0, ChromiumViewSizeMode.FullScreen);
```

## Best Practices

1. **Always call `notifyReady()`** - This tells Unreal Engine that your View is loaded and ready to receive data.

2. **Handle initial state** - Use `getViewModelState()` to get the current state when your View loads.

3. **Clean up listeners** - Remove field change listeners when they're no longer needed.

4. **Use TypeScript** - The type definitions help catch errors and provide better IDE support.

## Building

```bash
npm install
npm run build
```

The compiled JavaScript and type definitions will be in the `dist` folder.
