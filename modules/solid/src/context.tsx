/**
 * ChromiumView SolidJS Context
 *
 * Provides SolidJS context for accessing the ChromiumView API
 * throughout your component tree.
 */

import {
  createContext,
  useContext,
  createSignal,
  onMount,
  type JSX,
} from "solid-js";
import {
  createChromiumView,
  isChromiumViewAvailable,
  getChromiumView,
  ChromiumViewSizeMode,
} from "@incanta/ue-view-base";
import type {
  ChromiumViewAPI,
  ViewModelState,
  ViewEventPayload,
} from "@incanta/ue-view-base";
import type {
  ChromiumViewProviderProps,
  ChromiumViewContextValue,
} from "./types";

/**
 * SolidJS context for ChromiumView
 */
const ChromiumViewContext = createContext<ChromiumViewContextValue>();

/**
 * Provider component that initializes and provides the ChromiumView API
 *
 * @example
 * ```tsx
 * function App() {
 *   return (
 *     <ChromiumViewProvider autoNotifyReady onReady={() => console.log("Ready!")}>
 *       <GameHUD />
 *     </ChromiumViewProvider>
 *   );
 * }
 * ```
 */
export function ChromiumViewProvider(props: ChromiumViewProviderProps): JSX.Element {
  const [isReady, setIsReady] = createSignal(false);
  const [api, setApi] = createSignal<ChromiumViewAPI | null>(props.api ?? null);
  let initialized = false;

  // Initialize the ChromiumView API
  onMount(() => {
    if (initialized) return;
    initialized = true;

    try {
      // Use provided API or create/get the global one
      if (props.api) {
        setApi(props.api);
      } else {
        // Try to get existing API first
        let chromiumViewApi = getChromiumView();
        if (!chromiumViewApi) {
          // Create new client if none exists
          const client = createChromiumView();
          chromiumViewApi = client;
        }
        setApi(chromiumViewApi);
      }
    } catch (error) {
      props.onError?.(error instanceof Error ? error : new Error(String(error)));
    }
  });

  // Handle ready state
  onMount(() => {
    const currentApi = api();
    if (!currentApi) return;

    currentApi.onReady(() => {
      setIsReady(true);
      props.onReady?.();
    });

    // Auto notify ready if enabled
    if (props.autoNotifyReady !== false) {
      // Use requestAnimationFrame to ensure SolidJS has finished rendering
      requestAnimationFrame(() => {
        currentApi.notifyReady();
      });
    }
  });

  // Context methods
  const sendEvent = async <TPayload = ViewEventPayload, TResponse = unknown>(
    eventName: string,
    payload?: TPayload
  ): Promise<TResponse> => {
    const currentApi = api();
    if (!currentApi) {
      throw new Error("ChromiumView API not available");
    }
    return currentApi.sendEvent<TPayload, TResponse>(eventName, payload);
  };

  const getViewModelState = async <T = ViewModelState>(): Promise<T> => {
    const currentApi = api();
    if (!currentApi) {
      throw new Error("ChromiumView API not available");
    }
    return currentApi.getViewModelState<T>();
  };

  const setDesiredSize = (
    width: number,
    height: number,
    sizeMode?: ChromiumViewSizeMode
  ): void => {
    api()?.setDesiredSize(width, height, sizeMode);
  };

  const notifyReady = (): void => {
    api()?.notifyReady();
  };

  const contextValue: ChromiumViewContextValue = {
    api,
    isReady,
    isAvailable: () => isChromiumViewAvailable(),
    sendEvent,
    getViewModelState,
    setDesiredSize,
    notifyReady,
  };

  return (
    <ChromiumViewContext.Provider value={contextValue}>
      {props.children}
    </ChromiumViewContext.Provider>
  );
}

/**
 * Hook to access the ChromiumView context
 *
 * @throws Error if used outside of ChromiumViewProvider
 *
 * @example
 * ```tsx
 * function MyComponent() {
 *   const { sendEvent, isReady } = useChromiumViewContext();
 *
 *   const handleClick = async () => {
 *     await sendEvent("buttonClicked", { buttonId: "action" });
 *   };
 *
 *   return <button onClick={handleClick} disabled={!isReady()}>Click me</button>;
 * }
 * ```
 */
export function useChromiumViewContext(): ChromiumViewContextValue {
  const context = useContext(ChromiumViewContext);
  if (!context) {
    throw new Error(
      "useChromiumViewContext must be used within a ChromiumViewProvider"
    );
  }
  return context;
}

/**
 * Check if ChromiumView is available (can be used outside provider)
 *
 * @example
 * ```tsx
 * function MyComponent() {
 *   const isAvailable = useChromiumViewAvailable();
 *
 *   if (!isAvailable()) {
 *     return <div>Running outside of Unreal Engine</div>;
 *   }
 *
 *   return <ChromiumViewProvider>...</ChromiumViewProvider>;
 * }
 * ```
 */
export function useChromiumViewAvailable() {
  return () => isChromiumViewAvailable();
}
