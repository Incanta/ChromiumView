/**
 * ChromiumView Preact Components
 *
 * Pre-built Preact components for common ChromiumView patterns.
 */

import {
  type ComponentType,
  type ComponentChildren,
  Component,
} from "preact";
import { useEffect } from "preact/hooks";
import { ChromiumViewSizeMode } from "@unreal/chromium-view";
import { ChromiumViewProvider, useChromiumViewContext } from "./context";
import { useField, useIsReady, useOnReady, useSetDesiredSize } from "./hooks";
import type { ChromiumViewProps, UseFieldOptions, WithFieldProps } from "./types";

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
export function ChromiumView({
  children,
  autoNotifyReady = true,
  width,
  height,
  sizeMode,
  onReady,
  class: className,
  style,
}: ChromiumViewProps) {
  return (
    <ChromiumViewProvider autoNotifyReady={autoNotifyReady} onReady={onReady}>
      <ChromiumViewInner
        width={width}
        height={height}
        sizeMode={sizeMode}
        class={className}
        style={style}
      >
        {children}
      </ChromiumViewInner>
    </ChromiumViewProvider>
  );
}

/**
 * Inner component that handles sizing after provider is available
 */
function ChromiumViewInner({
  children,
  width,
  height,
  sizeMode = ChromiumViewSizeMode.Auto,
  class: className,
  style,
}: Omit<ChromiumViewProps, "autoNotifyReady" | "onReady">) {
  const setDesiredSize = useSetDesiredSize();

  useEffect(() => {
    if (width !== undefined && height !== undefined) {
      setDesiredSize(width, height, sizeMode);
    }
  }, [width, height, sizeMode, setDesiredSize]);

  return (
    <div class={className} style={style}>
      {children}
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
export function WhenReady({
  children,
  fallback = null,
}: {
  children: ComponentChildren;
  fallback?: ComponentChildren;
}) {
  const isReady = useIsReady();

  return <>{isReady ? children : fallback}</>;
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
export function FieldValue<T = unknown>({
  fieldName,
  initialValue,
  transform,
  render,
  fallback = null,
}: {
  fieldName: string;
  initialValue?: T;
  transform?: (value: unknown) => T;
  render?: (value: T) => ComponentChildren;
  fallback?: ComponentChildren;
}) {
  const value = useField<T>(fieldName, { initialValue, transform });

  if (value === undefined) {
    return <>{fallback}</>;
  }

  if (render) {
    return <>{render(value)}</>;
  }

  return <>{String(value)}</>;
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
 * function HealthBar({ value, isLoading, label }: Props) {
 *   return (
 *     <div>
 *       <span>{label}</span>
 *       <div style={{ width: `${value}%` }} />
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
  options: UseFieldOptions<T> = {}
) {
  return function withFieldHOC(
    WrappedComponent: ComponentType<P>
  ): ComponentType<Omit<P, keyof WithFieldProps<T>>> {
    function WithFieldComponent(props: Omit<P, keyof WithFieldProps<T>>) {
      const value = useField<T>(fieldName, options);
      const isLoading = value === undefined;

      return (
        <WrappedComponent
          {...(props as P)}
          value={value as T}
          isLoading={isLoading}
        />
      );
    }

    WithFieldComponent.displayName = `withField(${
      (WrappedComponent as ComponentType<P> & { displayName?: string }).displayName ||
      WrappedComponent.name ||
      "Component"
    })`;

    return WithFieldComponent;
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
export function FieldEffect<T = unknown>({
  fieldName,
  onChanged,
}: {
  fieldName: string;
  onChanged: (value: T) => void;
}): null {
  const { api } = useChromiumViewContext();

  useEffect(() => {
    if (!api) return;

    const handler = (value: unknown) => {
      onChanged(value as T);
    };

    api.onFieldChanged(fieldName, handler);

    return () => {
      api.offFieldChanged(fieldName, handler);
    };
  }, [api, fieldName, onChanged]);

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
export function OnReady({
  callback,
}: {
  callback: () => void;
}): null {
  useOnReady(callback);
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
export function ChromiumViewGuard({
  children,
  fallback = null,
}: {
  children: ComponentChildren;
  fallback?: ComponentChildren;
}) {
  const isAvailable = typeof window !== "undefined" && !!window.ue?.chromiumviewbridge;

  return <>{isAvailable ? children : fallback}</>;
}

/**
 * Error boundary component for ChromiumView
 *
 * @example
 * ```tsx
 * function App() {
 *   return (
 *     <ChromiumViewErrorBoundary
 *       fallback={<div>Something went wrong</div>}
 *     >
 *       <ChromiumViewProvider>
 *         <GameHUD />
 *       </ChromiumViewProvider>
 *     </ChromiumViewErrorBoundary>
 *   );
 * }
 * ```
 */
interface ErrorBoundaryProps {
  children: ComponentChildren;
  fallback?: ComponentChildren;
  onError?: (error: Error) => void;
}

interface ErrorBoundaryState {
  hasError: boolean;
  error: Error | null;
}

export class ChromiumViewErrorBoundary extends Component<ErrorBoundaryProps, ErrorBoundaryState> {
  constructor(props: ErrorBoundaryProps) {
    super(props);
    this.state = { hasError: false, error: null };
  }

  static getDerivedStateFromError(error: Error): ErrorBoundaryState {
    return { hasError: true, error };
  }

  componentDidCatch(error: Error) {
    console.error("ChromiumView Error:", error);
    this.props.onError?.(error);
  }

  render() {
    if (this.state.hasError) {
      return this.props.fallback ?? (
        <div style={{ color: "red", padding: "20px" }}>
          <h2>ChromiumView Error</h2>
          <p>{this.state.error?.message}</p>
        </div>
      );
    }

    return this.props.children;
  }
}
