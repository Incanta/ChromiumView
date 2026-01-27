# ChromiumView SolidJS Bindings

SolidJS primitives and components for building web-based Views that communicate with Unreal Engine ViewModels using the ChromiumView plugin.

## Installation

```bash
npm install @unreal/chromium-view-solid @unreal/chromium-view solid-js
```

## Quick Start

```tsx
import { render } from "solid-js/web";
import {
  ChromiumView,
  createField,
  createSendEvent,
} from "@unreal/chromium-view-solid";

function GameHUD() {
  // Automatically updates when ViewModel.Health changes
  const health = createField<number>("Health", { initialValue: 100 });
  const score = createField<number>("Score", { initialValue: 0 });
  const playerName = createField<string>("PlayerName", { initialValue: "Player" });

  // Send events to the ViewModel
  const { send: attack, isLoading } = createSendEvent<{ action: string }>("PlayerAction");

  return (
    <div class="hud">
      <h1>{playerName()}</h1>
      <div>Health: {health()}</div>
      <div>Score: {score()}</div>
      <button onClick={() => attack({ action: "attack" })} disabled={isLoading()}>
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

render(() => <App />, document.getElementById("root")!);
```

## Primitives

SolidJS uses fine-grained reactivity with signals. All primitives return accessors (functions) that you call to get the current value.

### `createField<T>(fieldName, options?)`

Subscribe to a single ViewModel field. The component re-renders when the field changes.

```tsx
const health = createField<number>("Health", { initialValue: 100 });
const name = createField<string>("PlayerName");

// Access values by calling the accessor
return <div>Health: {health()}</div>;

// With transform
const healthPercent = createField<number>("Health", {
  initialValue: 100,
  transform: (value) => Math.round((value as number) / 100 * 100),
});
```

**Options:**
- `initialValue?: T` - Initial value before ViewModel data arrives
- `transform?: (value: unknown) => T` - Transform function for the value
- `skipInitialFetch?: boolean` - Skip fetching initial state from ViewModel

### `createFields<T>(fieldNames, options?)`

Subscribe to multiple fields at once.

```tsx
const stats = createFields<{
  Health: number;
  Mana: number;
  Stamina: number;
}>(["Health", "Mana", "Stamina"], {
  Health: { initialValue: 100 },
  Mana: { initialValue: 100 },
  Stamina: { initialValue: 100 },
});

return <div>Health: {stats().Health}</div>;
```

### `createViewModel<T>(options?)`

Subscribe to the entire ViewModel state.

```tsx
interface GameState {
  PlayerName: string;
  Score: number;
  Health: number;
}

const state = createViewModel<GameState>({
  fields: ["PlayerName", "Score", "Health"],
  fetchInitialState: true,
});

return <div>Player: {state()?.PlayerName}</div>;
```

### `createSendEvent<TPayload, TResponse>(eventName, options?)`

Send events to the ViewModel with loading and error states.

```tsx
const { send, isLoading, error, data } = createSendEvent<
  { action: string },
  { success: boolean }
>("PlayerAction", {
  onSuccess: (result) => console.log("Success:", result),
  onError: (error) => console.error("Error:", error),
});

const handleClick = async () => {
  const result = await send({ action: "attack" });
};

return (
  <button onClick={handleClick} disabled={isLoading()}>
    {isLoading() ? "..." : "Attack"}
  </button>
);
```

**Options:**
- `throwOnError?: boolean` - Throw errors instead of returning them
- `onSuccess?: (result) => void` - Success callback
- `onError?: (error) => void` - Error callback

### `createSendEventCallback()`

Get a simple sendEvent function without state management.

```tsx
const sendEvent = createSendEventCallback();

const handleClick = () => {
  sendEvent("PlayerAction", { action: "jump" });
};
```

### `createIsReady()`

Check if the ChromiumView is ready.

```tsx
const isReady = createIsReady();

return (
  <Show when={isReady()} fallback={<div>Loading...</div>}>
    <GameHUD />
  </Show>
);
```

### `createFieldEffect(fieldName, callback)`

Run a side effect when a field changes.

```tsx
createFieldEffect("Health", (health: number) => {
  if (health < 20) {
    playLowHealthWarning();
  }
});
```

### `createOnReady(callback)`

Run a callback once when the View becomes ready.

```tsx
createOnReady(() => {
  console.log("View is ready!");
  initializeGame();
});
```

### `createSetDesiredSize()`

Get a function to set the View's desired size.

```tsx
const setSize = createSetDesiredSize();

onMount(() => {
  setSize(1920, 1080);
});
```

### `createChromiumViewApi()`

Get direct access to the ChromiumView API.

```tsx
const api = createChromiumViewApi();

createEffect(() => {
  const currentApi = api();
  if (!currentApi) return;
  // Direct API access for advanced use cases
});
```

## Components

### `<ChromiumView>`

Main wrapper component that sets up the provider and handles sizing.

```tsx
<ChromiumView
  width={1920}
  height={1080}
  autoNotifyReady
  onReady={() => console.log("Ready!")}
>
  <GameHUD />
</ChromiumView>
```

**Props:**
- `width?: number` - Desired width
- `height?: number` - Desired height
- `sizeMode?: ChromiumViewSizeMode` - Size mode
- `autoNotifyReady?: boolean` - Auto notify ready (default: true)
- `onReady?: () => void` - Ready callback
- `class?: string` - CSS class
- `style?: JSX.CSSProperties` - Inline styles

### `<ChromiumViewProvider>`

Context provider for manual setup.

```tsx
<ChromiumViewProvider autoNotifyReady onReady={() => console.log("Ready!")}>
  <GameHUD />
</ChromiumViewProvider>
```

### `<WhenReady>`

Conditionally render based on ready state.

```tsx
<ChromiumViewProvider>
  <WhenReady fallback={<LoadingScreen />}>
    <GameHUD />
  </WhenReady>
</ChromiumViewProvider>
```

### `<FieldValue>`

Display a field value directly.

```tsx
<div>
  Score: <FieldValue fieldName="Score" initialValue={0} />
</div>

// With custom render
<FieldValue
  fieldName="Health"
  initialValue={100}
  render={(health) => <HealthBar value={health} />}
/>
```

### `<FieldEffect>`

Run side effects when fields change.

```tsx
<FieldEffect
  fieldName="Health"
  onChanged={(health) => {
    if (health < 20) playLowHealthSound();
  }}
/>
```

### `<OnReady>`

Run a callback when ready.

```tsx
<OnReady callback={() => initializeGame()} />
```

### `<ChromiumViewGuard>`

Guard content based on availability.

```tsx
<ChromiumViewGuard fallback={<div>Please run in Unreal Engine</div>}>
  <ChromiumViewProvider>
    <GameHUD />
  </ChromiumViewProvider>
</ChromiumViewGuard>
```

### `<ChromiumViewErrorBoundary>`

Catch and handle errors.

```tsx
<ChromiumViewErrorBoundary
  fallback={(error, reset) => (
    <div>
      Error: {error.message}
      <button onClick={reset}>Retry</button>
    </div>
  )}
>
  <ChromiumViewProvider>
    <GameHUD />
  </ChromiumViewProvider>
</ChromiumViewErrorBoundary>
```

## Higher-Order Components

### `withField`

Inject a field value as props.

```tsx
interface Props extends WithFieldProps<number> {
  label: string;
}

function HealthBar(props: Props) {
  return (
    <div>
      <span>{props.label}</span>
      <div style={{ width: `${props.value}%` }} />
    </div>
  );
}

const ConnectedHealthBar = withField<number, Props>("Health", {
  initialValue: 100,
})(HealthBar);

// Usage
<ConnectedHealthBar label="HP" />
```

## Context

### `useChromiumViewContext()`

Access the full context value.

```tsx
const { api, isReady, sendEvent, getViewModelState } = useChromiumViewContext();

// Note: api and isReady are accessors, call them to get values
const currentApi = api();
const ready = isReady();
```

### `useChromiumViewAvailable()`

Check availability outside of provider.

```tsx
const isAvailable = useChromiumViewAvailable();

return (
  <Show
    when={isAvailable()}
    fallback={<div>Not in Unreal Engine</div>}
  >
    <ChromiumViewProvider>
      <GameHUD />
    </ChromiumViewProvider>
  </Show>
);
```

## TypeScript

Full TypeScript support is included. Define your ViewModel types:

```tsx
interface GameViewModel {
  PlayerName: string;
  Health: number;
  Mana: number;
  Score: number;
  Inventory: string[];
}

// Type-safe field access
const health = createField<GameViewModel["Health"]>("Health", {
  initialValue: 100,
});

// Type-safe events
interface AttackPayload {
  targetId: string;
  damage: number;
}

interface AttackResponse {
  success: boolean;
  newHealth: number;
}

const { send } = createSendEvent<AttackPayload, AttackResponse>("Attack");
```

## Differences from React Bindings

SolidJS uses signals instead of useState, which means:

1. **Values are accessors**: Call them as functions to get values
   ```tsx
   // React
   const health = useField("Health");
   return <div>{health}</div>;

   // Solid
   const health = createField("Health");
   return <div>{health()}</div>;
   ```

2. **No dependencies arrays**: SolidJS automatically tracks reactive dependencies
   ```tsx
   // React
   useEffect(() => {
     // effect
   }, [dependency]);

   // Solid
   createEffect(() => {
     // effect - dependencies are automatically tracked
   });
   ```

3. **Fine-grained updates**: Only the specific DOM nodes that use a signal update, not entire components

4. **Primitives instead of hooks**: We use `createX` naming convention instead of `useX` to align with SolidJS conventions
