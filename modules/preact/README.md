# ChromiumView Preact Bindings

Preact hooks and components for building web-based Views that communicate with Unreal Engine ViewModels using the ChromiumView plugin.

## Installation

```bash
npm install @unreal/chromium-view-preact @unreal/chromium-view preact
```

## Quick Start

```tsx
import { render } from "preact";
import {
  ChromiumView,
  useField,
  useSendEvent,
} from "@unreal/chromium-view-preact";

function GameHUD() {
  // Automatically updates when ViewModel.Health changes
  const health = useField<number>("Health", { initialValue: 100 });
  const score = useField<number>("Score", { initialValue: 0 });
  const playerName = useField<string>("PlayerName", { initialValue: "Player" });

  // Send events to the ViewModel
  const { send: attack, isLoading } = useSendEvent<{ action: string }>("PlayerAction");

  return (
    <div class="hud">
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

render(<App />, document.getElementById("root")!);
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

### `useIsReady()`

Check if the ChromiumView is ready.

```tsx
const isReady = useIsReady();

if (!isReady) {
  return <div>Loading...</div>;
}

return <GameHUD />;
```

### `useFieldEffect(fieldName, callback)`

Run a side effect when a field changes.

```tsx
useFieldEffect("Health", (health: number) => {
  if (health < 20) {
    playLowHealthWarning();
  }
});
```

### `useOnReady(callback)`

Run a callback once when the View becomes ready.

```tsx
useOnReady(() => {
  console.log("View is ready!");
  initializeGame();
});
```

### `useSetDesiredSize()`

Get a function to set the View's desired size.

```tsx
const setSize = useSetDesiredSize();

useEffect(() => {
  setSize(1920, 1080);
}, [setSize]);
```

### `useChromiumViewApi()`

Get direct access to the ChromiumView API.

```tsx
const api = useChromiumViewApi();

useEffect(() => {
  if (!api) return;
  // Direct API access for advanced use cases
}, [api]);
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
  fallback={<div>Something went wrong</div>}
  onError={(error) => console.error(error)}
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

## Context

### `useChromiumViewContext()`

Access the full context value.

```tsx
const { api, isReady, sendEvent, getViewModelState } = useChromiumViewContext();
```

### `useChromiumViewAvailable()`

Check availability outside of provider.

```tsx
const isAvailable = useChromiumViewAvailable();

if (!isAvailable) {
  return <div>Not in Unreal Engine</div>;
}

return (
  <ChromiumViewProvider>
    <GameHUD />
  </ChromiumViewProvider>
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
const health = useField<GameViewModel["Health"]>("Health", {
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

const { send } = useSendEvent<AttackPayload, AttackResponse>("Attack");
```

## Differences from React

Preact is designed to be a drop-in replacement for React with a much smaller footprint (~3KB). The API is nearly identical to the React bindings:

1. **Same hook names**: `useField`, `useSendEvent`, etc.
2. **Same component names**: `ChromiumView`, `WhenReady`, etc.
3. **Minor differences**:
   - Use `class` instead of `className` for CSS classes
   - Import from `preact` and `preact/hooks` instead of `react`
   - Slightly different error boundary API

## Migrating from React

If you have an existing React-based ChromiumView project:

1. Change your imports:
   ```tsx
   // Before
   import { useField } from "@unreal/chromium-view-react";

   // After
   import { useField } from "@unreal/chromium-view-preact";
   ```

2. Update `className` to `class`:
   ```tsx
   // Before
   <div className="health-bar">

   // After
   <div class="health-bar">
   ```

3. Update your bundler config if needed (most bundlers support Preact out of the box)
