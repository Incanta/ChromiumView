/**
 * ChromiumView Preact Context
 *
 * Provides Preact context for accessing the ChromiumView API
 * throughout your component tree.
 */

import {
  createContext,
} from "preact";
import {
  useContext,
  useState,
  useEffect,
  useCallback,
  useMemo,
  useRef,
} from "preact/hooks";
import {
  createChromiumView,
  isChromiumViewAvailable,
  getChromiumView,
  ChromiumViewSizeMode,
} from "@unreal/chromium-view";
import type {
  ChromiumViewAPI,
  ViewModelState,
  ViewEventPayload,
} from "@unreal/chromium-view";
import type {
  ChromiumViewProviderProps,
  ChromiumViewContextValue,
} from "./types";

/**
 * Preact context for ChromiumView
 */
const ChromiumViewContext = createContext<ChromiumViewContextValue | null>(null);

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
export function ChromiumViewProvider({
  children,
  api: providedApi,
  autoNotifyReady = true,
  onReady,
  onError,
}: ChromiumViewProviderProps) {
  const [isReady, setIsReady] = useState(false);
  const [api, setApi] = useState<ChromiumViewAPI | null>(providedApi ?? null);
  const initializedRef = useRef(false);

  // Initialize the ChromiumView API
  useEffect(() => {
    if (initializedRef.current) return;
    initializedRef.current = true;

    try {
      // Use provided API or create/get the global one
      if (providedApi) {
        setApi(providedApi);
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
      onError?.(error instanceof Error ? error : new Error(String(error)));
    }
  }, [providedApi, onError]);

  // Handle ready state
  useEffect(() => {
    if (!api) return;

    api.onReady(() => {
      setIsReady(true);
      onReady?.();
    });

    // Auto notify ready if enabled
    if (autoNotifyReady) {
      // Use requestAnimationFrame to ensure Preact has finished rendering
      requestAnimationFrame(() => {
        api.notifyReady();
      });
    }
  }, [api, autoNotifyReady, onReady]);

  // Memoized context methods
  const sendEvent = useCallback(
    async <TPayload = ViewEventPayload, TResponse = unknown>(
      eventName: string,
      payload?: TPayload
    ): Promise<TResponse> => {
      if (!api) {
        throw new Error("ChromiumView API not available");
      }
      return api.sendEvent<TPayload, TResponse>(eventName, payload);
    },
    [api]
  );

  const getViewModelState = useCallback(
    async <T = ViewModelState>(): Promise<T> => {
      if (!api) {
        throw new Error("ChromiumView API not available");
      }
      return api.getViewModelState<T>();
    },
    [api]
  );

  const setDesiredSize = useCallback(
    (width: number, height: number, sizeMode?: ChromiumViewSizeMode): void => {
      api?.setDesiredSize(width, height, sizeMode);
    },
    [api]
  );

  const notifyReady = useCallback((): void => {
    api?.notifyReady();
  }, [api]);

  const contextValue = useMemo<ChromiumViewContextValue>(
    () => ({
      api,
      isReady,
      isAvailable: isChromiumViewAvailable(),
      sendEvent,
      getViewModelState,
      setDesiredSize,
      notifyReady,
    }),
    [api, isReady, sendEvent, getViewModelState, setDesiredSize, notifyReady]
  );

  return (
    <ChromiumViewContext.Provider value={contextValue}>
      {children}
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
 *   return <button onClick={handleClick} disabled={!isReady}>Click me</button>;
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
 * Hook to check if ChromiumView is available (can be used outside provider)
 *
 * @example
 * ```tsx
 * function MyComponent() {
 *   const isAvailable = useChromiumViewAvailable();
 *
 *   if (!isAvailable) {
 *     return <div>Running outside of Unreal Engine</div>;
 *   }
 *
 *   return <ChromiumViewProvider>...</ChromiumViewProvider>;
 * }
 * ```
 */
export function useChromiumViewAvailable(): boolean {
  return isChromiumViewAvailable();
}
