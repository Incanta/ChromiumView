# ChromiumView React Bindings

React hooks and components for building web-based Views that communicate with Unreal Engine ViewModels using the ChromiumView plugin.

## Installation

```bash
npm install @unreal/chromium-view-react @unreal/chromium-view react
```

## Quick Start

```tsx
import React from "react";
import { createRoot } from "react-dom/client";
import {
  ChromiumView,
  useField,
  useSendEvent,
} from "@unreal/chromium-view-react";

function GameHUD() {
  // Automatically updates when ViewModel.Health changes
  const health = useField<number>("Health", { initialValue: 100 });
  const score = useField<number>("Score", { initialValue: 0 });
  const playerName = useField<string>("PlayerName", { initialValue: "Player" });

  // Send events to the ViewModel
  const { send: attack, isLoading } = useSendEvent<{ action: string }>("PlayerAction");

  return (
    <div className="hud">
      <h1>{playerName}</h1>
      <div>Health: {health}</div>
      <div>Score: {score}</div>
      <button onClick={() => attack({ action: "attack" })} disabled={isLoading}>
        Attack
      </button>
    </div>
  );
}

function App() {
  return (
    <ChromiumView
      width={800}
      height={600}
      autoNotifyReady
      onReady={() => console.log("View is ready!")}
    >
      <GameHUD />
    </ChromiumView>
  );
}

createRoot(document.getElementById("root")!).render(<App />);
```

## Hooks

### `useField<T>(fieldName, options?)`

Subscribe to a single ViewModel field. The component re-renders when the field changes.

```tsx
const health = useField<number>("Health", { initialValue: 100 });
const name = useField<string>("PlayerName");

// With transform
const healthPercent = useField<number>("Health", {
  initialValue: 100,
  transform: (value) => Math.round((value as number) / 100 * 100),
});
```

**Options:**
- `initialValue?: T` - Initial value before ViewModel data arrives
- `transform?: (value: unknown) => T` - Transform function for the value
- `skipInitialFetch?: boolean` - Skip fetching initial state from ViewModel

### `useFields<T>(fieldNames, options?)`

Subscribe to multiple fields at once.

```tsx
const stats = useFields<{
  Health: number;
  Mana: number;
  Stamina: number;
}>(["Health", "Mana", "Stamina"], {
  Health: { initialValue: 100 },
  Mana: { initialValue: 100 },
  Stamina: { initialValue: 100 },
});

return <div>Health: {stats.Health}</div>;
```

### `useViewModel<T>(options?)`

Subscribe to the entire ViewModel state.

```tsx
interface GameState {
  PlayerName: string;
  Score: number;
  Health: number;
}

const state = useViewModel<GameState>({
  fields: ["PlayerName", "Score", "Health"],
  fetchInitialState: true,
});
```

### `useSendEvent<TPayload, TResponse>(eventName, options?)`

Send events to the ViewModel with loading and error states.

```tsx
const { send, isLoading, error, data } = useSendEvent<
  { action: string },
  { success: boolean }
>("PlayerAction", {
  onSuccess: (result) => console.log("Success:", result),
  onError: (error) => console.error("Error:", error),
});

const handleClick = async () => {
  const result = await send({ action: "attack" });
};
```

**Options:**
- `throwOnError?: boolean` - Throw errors instead of returning them
- `onSuccess?: (result) => void` - Success callback
- `onError?: (error) => void` - Error callback

### `useSendEventCallback()`

Get a simple sendEvent function without state management.

```tsx
const sendEvent = useSendEventCallback();

const handleClick = () => {
  sendEvent("PlayerAction", { action: "jump" });
};
```

### `useFieldEffect<T>(fieldName, callback)`

Run a side effect when a field changes.

```tsx
useFieldEffect<number>("Health", (health) => {
  if (health < 20) {
    playLowHealthWarning();
  }
});
```

### `useOnReady(callback)`

Run a callback once when the View becomes ready.

```tsx
useOnReady(() => {
  console.log("View initialized!");
  initializeGame();
});
```

### `useIsReady()`

Check if the View is ready.

```tsx
const isReady = useIsReady();

if (!isReady) {
  return <LoadingScreen />;
}
```

### `useSetDesiredSize()`

Get a function to set the View's desired size.

```tsx
const setSize = useSetDesiredSize();

useEffect(() => {
  setSize(1920, 1080, ChromiumViewSizeMode.DesiredSize);
}, [setSize]);
```

### `useChromiumViewApi()`

Get direct access to the ChromiumView API.

```tsx
const api = useChromiumViewApi();

// For advanced use cases
api?.onFieldChanged("CustomField", handleChange);
```

## Components

### `<ChromiumView>`

Main wrapper component that sets up the provider and handles sizing.

```tsx
<ChromiumView
  width={1920}
  height={1080}
  sizeMode={ChromiumViewSizeMode.DesiredSize}
  autoNotifyReady
  onReady={() => console.log("Ready!")}
  className="game-hud"
  style={{ position: "absolute" }}
>
  <GameContent />
</ChromiumView>
```

### `<ChromiumViewProvider>`

Context provider for manual setup.

```tsx
<ChromiumViewProvider
  autoNotifyReady={false}
  onReady={() => console.log("Ready!")}
  onError={(error) => console.error(error)}
>
  <GameContent />
</ChromiumViewProvider>
```

### `<WhenReady>`

Conditionally render content when the View is ready.

```tsx
<ChromiumViewProvider>
  <WhenReady fallback={<LoadingScreen />}>
    <GameHUD />
  </WhenReady>
</ChromiumViewProvider>
```

### `<FieldValue>`

Display a field value with automatic updates.

```tsx
<div>
  Score: <FieldValue fieldName="Score" initialValue={0} />
</div>

// With custom rendering
<FieldValue
  fieldName="Health"
  initialValue={100}
  render={(value) => (
    <div className="health-bar" style={{ width: `${value}%` }} />
  )}
/>
```

### `<FieldEffect>`

Run effects when fields change (declarative alternative to `useFieldEffect`).

```tsx
<FieldEffect
  fieldName="Health"
  onChanged={(health) => {
    if (health < 20) playLowHealthSound();
  }}
/>
```

### `<OnReady>`

Run a callback when ready (declarative alternative to `useOnReady`).

```tsx
<OnReady callback={() => initializeGame()} />
```

### `<ChromiumViewGuard>`

Show fallback content when not running in Unreal Engine.

```tsx
<ChromiumViewGuard fallback={<div>Please run in Unreal Engine</div>}>
  <ChromiumViewProvider>
    <GameHUD />
  </ChromiumViewProvider>
</ChromiumViewGuard>
```

### `<ChromiumViewErrorBoundary>`

Error boundary for ChromiumView errors.

```tsx
<ChromiumViewErrorBoundary
  fallback={<div>Something went wrong</div>}
  onError={(error, info) => logError(error, info)}
>
  <ChromiumViewProvider>
    <GameHUD />
  </ChromiumViewProvider>
</ChromiumViewErrorBoundary>
```

## Higher-Order Components

### `withField<T, P>(fieldName, options)`

HOC to inject a field value as props.

```tsx
interface Props extends WithFieldProps<number> {
  label: string;
}

function HealthBar({ value, isLoading, label }: Props) {
  return (
    <div>
      <span>{label}</span>
      <div style={{ width: `${value}%` }} />
    </div>
  );
}

const ConnectedHealthBar = withField<number, Props>("Health", {
  initialValue: 100,
})(HealthBar);

// Usage
<ConnectedHealthBar label="HP" />
```

## TypeScript Support

All hooks and components are fully typed. Define your ViewModel interface for best results:

```tsx
interface GameViewModel {
  PlayerName: string;
  Score: number;
  Health: number;
  Mana: number;
  Stamina: number;
}

// Type-safe field access
const health = useField<GameViewModel["Health"]>("Health");

// Type-safe state
const state = useViewModel<GameViewModel>({
  fields: ["PlayerName", "Score", "Health"],
});
```

## Best Practices

1. **Use the `ChromiumView` component** for simple setups
2. **Provide initial values** to prevent undefined states
3. **Use `WhenReady`** to avoid rendering before data is available
4. **Use `useSendEvent`** for actions with loading/error states
5. **Use `useFieldEffect`** for side effects instead of `useEffect` + `useField`

## Example: Complete Game HUD

```tsx
import React from "react";
import {
  ChromiumView,
  WhenReady,
  useField,
  useSendEvent,
  useFieldEffect,
  FieldEffect,
} from "@unreal/chromium-view-react";

function HealthBar() {
  const health = useField<number>("Health", { initialValue: 100 });
  const maxHealth = useField<number>("MaxHealth", { initialValue: 100 });

  const percent = maxHealth ? (health! / maxHealth) * 100 : 0;

  return (
    <div className="health-bar">
      <div className="fill" style={{ width: `${percent}%` }} />
      <span>{health} / {maxHealth}</span>
    </div>
  );
}

function ActionButtons() {
  const { send: attack, isLoading: attacking } = useSendEvent("PlayerAction");
  const { send: defend, isLoading: defending } = useSendEvent("PlayerAction");

  return (
    <div className="actions">
      <button onClick={() => attack({ action: "attack" })} disabled={attacking}>
        {attacking ? "..." : "Attack"}
      </button>
      <button onClick={() => defend({ action: "defend" })} disabled={defending}>
        {defending ? "..." : "Defend"}
      </button>
    </div>
  );
}

function SoundEffects() {
  useFieldEffect<number>("Health", (health) => {
    if (health < 20) {
      // Play low health warning
    }
  });

  return null;
}

function GameHUD() {
  return (
    <div className="game-hud">
      <HealthBar />
      <ActionButtons />
      <SoundEffects />
    </div>
  );
}

export default function App() {
  return (
    <ChromiumView
      width={800}
      height={600}
      autoNotifyReady
      onReady={() => console.log("HUD Ready!")}
    >
      <WhenReady fallback={<div>Loading...</div>}>
        <GameHUD />
      </WhenReady>
    </ChromiumView>
  );
}
```
