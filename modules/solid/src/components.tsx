/**
 * ChromiumView SolidJS Components
 *
 * Pre-built SolidJS components for common ChromiumView patterns.
 */

import {
  type JSX,
  type Component,
  Show,
  ErrorBoundary as SolidErrorBoundary,
  createEffect,
  splitProps,
} from "solid-js";
import { ChromiumViewSizeMode } from "@unreal/chromium-view";
import { ChromiumViewProvider, useChromiumViewContext } from "./context";
import {
  createField,
  createIsReady,
  createOnReady,
  createSetDesiredSize,
} from "./primitives";
import type { ChromiumViewProps, CreateFieldOptions, WithFieldProps } from "./types";

/**
 * Main ChromiumView component that sets up the provider and handles sizing
 *
 * @example
 * ```tsx
 * function App() {
 *   return (
 *     <ChromiumView
 *       width={1920}
 *       height={1080}
 *       autoNotifyReady
 *       onReady={() => console.log("Ready!")}
 *     >
 *       <GameHUD />
 *     </ChromiumView>
 *   );
 * }
 * ```
 */
export function ChromiumView(props: ChromiumViewProps): JSX.Element {
  const [local, rest] = splitProps(props, [
    "children",
    "autoNotifyReady",
    "onReady",
  ]);

  return (
    <ChromiumViewProvider autoNotifyReady={local.autoNotifyReady} onReady={local.onReady}>
      <ChromiumViewInner {...rest}>
        {local.children}
      </ChromiumViewInner>
    </ChromiumViewProvider>
  );
}

/**
 * Inner component that handles sizing after provider is available
 */
function ChromiumViewInner(
  props: Omit<ChromiumViewProps, "autoNotifyReady" | "onReady">
): JSX.Element {
  const setDesiredSize = createSetDesiredSize();

  createEffect(() => {
    if (props.width !== undefined && props.height !== undefined) {
      setDesiredSize(
        props.width,
        props.height,
        props.sizeMode ?? ChromiumViewSizeMode.Auto
      );
    }
  });

  return (
    <div class={props.class} style={props.style}>
      {props.children}
    </div>
  );
}

/**
 * Component that only renders its children when the View is ready
 *
 * @example
 * ```tsx
 * function App() {
 *   return (
 *     <ChromiumViewProvider>
 *       <WhenReady fallback={<LoadingScreen />}>
 *         <GameHUD />
 *       </WhenReady>
 *     </ChromiumViewProvider>
 *   );
 * }
 * ```
 */
export function WhenReady(props: {
  children: JSX.Element;
  fallback?: JSX.Element;
}): JSX.Element {
  const isReady = createIsReady();

  return (
    <Show when={isReady()} fallback={props.fallback}>
      {props.children}
    </Show>
  );
}

/**
 * Component that displays a field value with automatic updates
 *
 * @example
 * ```tsx
 * function ScoreDisplay() {
 *   return (
 *     <div>
 *       Score: <FieldValue fieldName="Score" initialValue={0} />
 *     </div>
 *   );
 * }
 * ```
 */
export function FieldValue<T = unknown>(props: {
  fieldName: string;
  initialValue?: T;
  transform?: (value: unknown) => T;
  render?: (value: T) => JSX.Element;
  fallback?: JSX.Element;
}): JSX.Element {
  const value = createField<T>(props.fieldName, {
    initialValue: props.initialValue,
    transform: props.transform,
  });

  return (
    <Show when={value() !== undefined} fallback={props.fallback}>
      {props.render ? props.render(value()!) : String(value())}
    </Show>
  );
}

/**
 * Higher-order component that injects a field value as a prop
 *
 * @example
 * ```tsx
 * interface Props extends WithFieldProps<number> {
 *   label: string;
 * }
 *
 * function HealthBar(props: Props) {
 *   return (
 *     <div>
 *       <span>{props.label}</span>
 *       <div style={{ width: `${props.value}%` }} />
 *     </div>
 *   );
 * }
 *
 * const ConnectedHealthBar = withField<number, Props>("Health", {
 *   initialValue: 100,
 * })(HealthBar);
 *
 * // Usage: <ConnectedHealthBar label="HP" />
 * ```
 */
export function withField<T, P extends WithFieldProps<T>>(
  fieldName: string,
  options: CreateFieldOptions<T> = {}
) {
  return function withFieldHOC(
    WrappedComponent: Component<P>
  ): Component<Omit<P, keyof WithFieldProps<T>>> {
    return function WithFieldComponent(props: Omit<P, keyof WithFieldProps<T>>) {
      const value = createField<T>(fieldName, options);
      const isLoading = () => value() === undefined;

      return (
        <WrappedComponent
          {...(props as P)}
          value={value() as T}
          isLoading={isLoading()}
        />
      );
    };
  };
}

/**
 * Component that runs an effect when a field changes
 *
 * @example
 * ```tsx
 * function GameEffects() {
 *   return (
 *     <>
 *       <FieldEffect
 *         fieldName="Health"
 *         onChanged={(health) => {
 *           if (health < 20) playLowHealthSound();
 *         }}
 *       />
 *       <FieldEffect
 *         fieldName="Score"
 *         onChanged={(score) => {
 *           if (score % 1000 === 0) playBonusSound();
 *         }}
 *       />
 *     </>
 *   );
 * }
 * ```
 */
export function FieldEffect<T = unknown>(props: {
  fieldName: string;
  onChanged: (value: T) => void;
}): null {
  const { api } = useChromiumViewContext();

  createEffect(() => {
    const currentApi = api();
    if (!currentApi) return;

    const handler = (value: unknown) => {
      props.onChanged(value as T);
    };

    currentApi.onFieldChanged(props.fieldName, handler);

    // Note: cleanup is handled by SolidJS tracking
  });

  return null;
}

/**
 * Component that runs a callback once when the View is ready
 *
 * @example
 * ```tsx
 * function App() {
 *   return (
 *     <ChromiumViewProvider>
 *       <OnReady callback={() => initializeGame()} />
 *       <GameHUD />
 *     </ChromiumViewProvider>
 *   );
 * }
 * ```
 */
export function OnReady(props: {
  callback: () => void;
}): null {
  createOnReady(props.callback);
  return null;
}

/**
 * Component that displays different content based on ChromiumView availability
 *
 * @example
 * ```tsx
 * function App() {
 *   return (
 *     <ChromiumViewGuard
 *       fallback={<div>Please run in Unreal Engine</div>}
 *     >
 *       <ChromiumViewProvider>
 *         <GameHUD />
 *       </ChromiumViewProvider>
 *     </ChromiumViewGuard>
 *   );
 * }
 * ```
 */
export function ChromiumViewGuard(props: {
  children: JSX.Element;
  fallback?: JSX.Element;
}): JSX.Element {
  const isAvailable = typeof window !== "undefined" && !!window.ue?.chromiumviewbridge;

  return (
    <Show when={isAvailable} fallback={props.fallback}>
      {props.children}
    </Show>
  );
}

/**
 * Error boundary component for ChromiumView
 *
 * @example
 * ```tsx
 * function App() {
 *   return (
 *     <ChromiumViewErrorBoundary
 *       fallback={(error) => <div>Error: {error.message}</div>}
 *     >
 *       <ChromiumViewProvider>
 *         <GameHUD />
 *       </ChromiumViewProvider>
 *     </ChromiumViewErrorBoundary>
 *   );
 * }
 * ```
 */
export function ChromiumViewErrorBoundary(props: {
  children: JSX.Element;
  fallback?: (error: Error, reset: () => void) => JSX.Element;
  onError?: (error: Error) => void;
}): JSX.Element {
  const defaultFallback = (error: Error) => (
    <div style={{ color: "red", padding: "20px" }}>
      <h2>ChromiumView Error</h2>
      <p>{error.message}</p>
    </div>
  );

  return (
    <SolidErrorBoundary
      fallback={(error, reset) => {
        props.onError?.(error);
        return props.fallback ? props.fallback(error, reset) : defaultFallback(error);
      }}
    >
      {props.children}
    </SolidErrorBoundary>
  );
}
