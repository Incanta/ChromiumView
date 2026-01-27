/**
 * ChromiumView React Hooks
 *
 * Custom React hooks for interacting with Unreal Engine ViewModels.
 */

import { useState, useEffect, useCallback, useRef, useMemo } from "react";
import type { ViewModelState, ViewEventPayload } from "@unreal/chromium-view";
import { useChromiumViewContext } from "./context";
import type {
  UseFieldOptions,
  UseViewModelOptions,
  UseSendEventOptions,
  UseSendEventResult,
} from "./types";

/**
 * Hook to subscribe to a single ViewModel field
 *
 * Automatically updates the component when the field value changes.
 *
 * @param fieldName - The name of the ViewModel field to subscribe to
 * @param options - Configuration options
 * @returns The current field value
 *
 * @example
 * ```tsx
 * function HealthBar() {
 *   const health = useField<number>("Health", { initialValue: 100 });
 *
 *   return (
 *     <div className="health-bar">
 *       <div style={{ width: `${health}%` }} />
 *     </div>
 *   );
 * }
 * ```
 */
export function useField<T = unknown>(
  fieldName: string,
  options: UseFieldOptions<T> = {}
): T | undefined {
  const { initialValue, transform, skipInitialFetch = false } = options;
  const { api, isReady, getViewModelState } = useChromiumViewContext();
  const [value, setValue] = useState<T | undefined>(initialValue);
  const transformRef = useRef(transform);

  // Keep transform ref up to date
  transformRef.current = transform;

  // Fetch initial value from ViewModel state
  useEffect(() => {
    if (skipInitialFetch || !isReady) return;

    getViewModelState<Record<string, unknown>>()
      .then((state) => {
        if (fieldName in state) {
          const rawValue = state[fieldName];
          const newValue = transformRef.current
            ? transformRef.current(rawValue)
            : (rawValue as T);
          setValue(newValue);
        }
      })
      .catch((error) => {
        console.error(`Failed to fetch initial state for field "${fieldName}":`, error);
      });
  }, [fieldName, isReady, getViewModelState, skipInitialFetch]);

  // Subscribe to field changes
  useEffect(() => {
    if (!api) {
      console.log(`[useField] No API available for field "${fieldName}"`);
      return;
    }

    console.log(`[useField] Subscribing to field "${fieldName}"`);

    const handleChange = (newValue: unknown) => {
      console.log(`[useField] Received change for "${fieldName}":`, newValue);
      const transformedValue = transformRef.current
        ? transformRef.current(newValue)
        : (newValue as T);
      setValue(transformedValue);
    };

    api.onFieldChanged(fieldName, handleChange);

    return () => {
      console.log(`[useField] Unsubscribing from field "${fieldName}"`);
      api.offFieldChanged(fieldName, handleChange);
    };
  }, [api, fieldName]);

  return value;
}

/**
 * Hook to subscribe to multiple ViewModel fields
 *
 * @param fieldNames - Array of field names to subscribe to
 * @param options - Configuration options per field
 * @returns Object with field values keyed by field name
 *
 * @example
 * ```tsx
 * function PlayerStats() {
 *   const stats = useFields(["Health", "Mana", "Stamina"], {
 *     Health: { initialValue: 100 },
 *     Mana: { initialValue: 100 },
 *     Stamina: { initialValue: 100 },
 *   });
 *
 *   return (
 *     <div>
 *       <div>Health: {stats.Health}</div>
 *       <div>Mana: {stats.Mana}</div>
 *       <div>Stamina: {stats.Stamina}</div>
 *     </div>
 *   );
 * }
 * ```
 */
export function useFields<T extends Record<string, unknown>>(
  fieldNames: (keyof T)[],
  options: Partial<Record<keyof T, UseFieldOptions<T[keyof T]>>> = {}
): Partial<T> {
  const { api, isReady, getViewModelState } = useChromiumViewContext();
  const [values, setValues] = useState<Partial<T>>(() => {
    const initial: Partial<T> = {};
    for (const fieldName of fieldNames) {
      const fieldOptions = options[fieldName];
      if (fieldOptions?.initialValue !== undefined) {
        initial[fieldName] = fieldOptions.initialValue;
      }
    }
    return initial;
  });
  const optionsRef = useRef(options);

  // Keep options ref up to date
  optionsRef.current = options;

  // Fetch initial values
  useEffect(() => {
    if (!isReady) return;

    getViewModelState<Record<string, unknown>>()
      .then((state) => {
        setValues((prev) => {
          const next = { ...prev };
          for (const fieldName of fieldNames) {
            const key = fieldName as string;
            if (key in state) {
              const fieldOptions = optionsRef.current[fieldName];
              const rawValue = state[key];
              const value = fieldOptions?.transform
                ? fieldOptions.transform(rawValue)
                : rawValue;
              (next as Record<string, unknown>)[key] = value;
            }
          }
          return next;
        });
      })
      .catch((error) => {
        console.error("Failed to fetch initial ViewModel state:", error);
      });
  }, [fieldNames, isReady, getViewModelState]);

  // Subscribe to field changes
  useEffect(() => {
    if (!api) return;

    const handlers: Array<{ field: string; handler: (value: unknown) => void }> = [];

    for (const fieldName of fieldNames) {
      const key = fieldName as string;
      const handler = (newValue: unknown) => {
        const fieldOptions = optionsRef.current[fieldName];
        const value = fieldOptions?.transform
          ? fieldOptions.transform(newValue)
          : newValue;

        setValues((prev) => ({
          ...prev,
          [key]: value,
        }));
      };

      api.onFieldChanged(key, handler);
      handlers.push({ field: key, handler });
    }

    return () => {
      for (const { field, handler } of handlers) {
        api.offFieldChanged(field, handler);
      }
    };
  }, [api, fieldNames]);

  return values;
}

/**
 * Hook to subscribe to the entire ViewModel state
 *
 * @param options - Configuration options
 * @returns The current ViewModel state
 *
 * @example
 * ```tsx
 * interface GameState {
 *   PlayerName: string;
 *   Score: number;
 *   Health: number;
 * }
 *
 * function GameHUD() {
 *   const state = useViewModel<GameState>({ fetchInitialState: true });
 *
 *   return (
 *     <div>
 *       <div>Player: {state?.PlayerName}</div>
 *       <div>Score: {state?.Score}</div>
 *     </div>
 *   );
 * }
 * ```
 */
export function useViewModel<T extends ViewModelState = ViewModelState>(
  options: UseViewModelOptions<T> = {}
): T | null {
  const { fields, fetchInitialState = true } = options;
  const { api, isReady, getViewModelState } = useChromiumViewContext();
  const [state, setState] = useState<T | null>(null);

  // Fetch initial state
  useEffect(() => {
    if (!fetchInitialState || !isReady) return;

    getViewModelState<T>()
      .then(setState)
      .catch((error) => {
        console.error("Failed to fetch ViewModel state:", error);
      });
  }, [fetchInitialState, isReady, getViewModelState]);

  // Subscribe to field changes
  useEffect(() => {
    if (!api) return;

    const fieldsToWatch = fields ?? [];
    const handlers: Array<{ field: string; handler: (value: unknown) => void }> = [];

    // If no specific fields, we need a different strategy
    // Since we can't know all fields, we'll update on any field change
    if (fieldsToWatch.length === 0) {
      // This is a limitation - we can't auto-subscribe to all fields
      // Users should specify fields or manually trigger updates
      return;
    }

    for (const fieldName of fieldsToWatch) {
      const key = fieldName as string;
      const handler = (newValue: unknown) => {
        setState((prev) => {
          if (!prev) return prev;
          return {
            ...prev,
            [key]: newValue,
          };
        });
      };

      api.onFieldChanged(key, handler);
      handlers.push({ field: key, handler });
    }

    return () => {
      for (const { field, handler } of handlers) {
        api.offFieldChanged(field, handler);
      }
    };
  }, [api, fields]);

  return state;
}

/**
 * Hook to send events to the ViewModel
 *
 * Provides loading and error states for the event.
 *
 * @param eventName - The name of the event to send
 * @param options - Configuration options
 * @returns Object with send function and state
 *
 * @example
 * ```tsx
 * function ActionButton() {
 *   const { send, isLoading, error } = useSendEvent<{ action: string }, void>(
 *     "PlayerAction",
 *     { onSuccess: () => console.log("Action sent!") }
 *   );
 *
 *   return (
 *     <button onClick={() => send({ action: "attack" })} disabled={isLoading}>
 *       {isLoading ? "..." : "Attack"}
 *     </button>
 *   );
 * }
 * ```
 */
export function useSendEvent<TPayload = ViewEventPayload, TResponse = unknown>(
  eventName: string,
  options: UseSendEventOptions = {}
): UseSendEventResult<TPayload, TResponse> {
  const { throwOnError = false, onSuccess, onError } = options;
  const { sendEvent } = useChromiumViewContext();

  const [isLoading, setIsLoading] = useState(false);
  const [error, setError] = useState<Error | null>(null);
  const [data, setData] = useState<TResponse | undefined>(undefined);

  const send = useCallback(
    async (payload?: TPayload): Promise<TResponse | undefined> => {
      setIsLoading(true);
      setError(null);

      try {
        const result = await sendEvent<TPayload, TResponse>(eventName, payload);
        setData(result);
        onSuccess?.(result);
        return result;
      } catch (err) {
        const error = err instanceof Error ? err : new Error(String(err));
        setError(error);
        onError?.(error);
        if (throwOnError) {
          throw error;
        }
        return undefined;
      } finally {
        setIsLoading(false);
      }
    },
    [eventName, sendEvent, throwOnError, onSuccess, onError]
  );

  const reset = useCallback(() => {
    setIsLoading(false);
    setError(null);
    setData(undefined);
  }, []);

  return useMemo(
    () => ({ send, isLoading, error, data, reset }),
    [send, isLoading, error, data, reset]
  );
}

/**
 * Hook to get a stable sendEvent function
 *
 * Simpler alternative to useSendEvent when you don't need loading/error states.
 *
 * @returns Function to send events
 *
 * @example
 * ```tsx
 * function ActionButton() {
 *   const sendEvent = useSendEventCallback();
 *
 *   return (
 *     <button onClick={() => sendEvent("PlayerAction", { action: "jump" })}>
 *       Jump
 *     </button>
 *   );
 * }
 * ```
 */
export function useSendEventCallback(): <TPayload = ViewEventPayload, TResponse = unknown>(
  eventName: string,
  payload?: TPayload
) => Promise<TResponse> {
  const { sendEvent } = useChromiumViewContext();
  return sendEvent;
}

/**
 * Hook to check if the ChromiumView is ready
 *
 * @returns Whether the View is ready
 *
 * @example
 * ```tsx
 * function LoadingScreen() {
 *   const isReady = useIsReady();
 *
 *   if (!isReady) {
 *     return <div>Loading...</div>;
 *   }
 *
 *   return <GameHUD />;
 * }
 * ```
 */
export function useIsReady(): boolean {
  const { isReady } = useChromiumViewContext();
  return isReady;
}

/**
 * Hook to get the ChromiumView API instance
 *
 * @returns The ChromiumView API or null if not available
 *
 * @example
 * ```tsx
 * function AdvancedComponent() {
 *   const api = useChromiumViewApi();
 *
 *   useEffect(() => {
 *     if (!api) return;
 *     // Direct API access for advanced use cases
 *   }, [api]);
 * }
 * ```
 */
export function useChromiumViewApi() {
  const { api } = useChromiumViewContext();
  return api;
}

/**
 * Hook to set the desired size of the View
 *
 * @returns Function to set the desired size
 *
 * @example
 * ```tsx
 * function ResizableView() {
 *   const setSize = useSetDesiredSize();
 *
 *   useEffect(() => {
 *     setSize(1920, 1080);
 *   }, [setSize]);
 * }
 * ```
 */
export function useSetDesiredSize() {
  const { setDesiredSize } = useChromiumViewContext();
  return setDesiredSize;
}

/**
 * Hook that runs a callback when a field changes
 *
 * @param fieldName - The name of the field to watch
 * @param callback - Callback to run when the field changes
 *
 * @example
 * ```tsx
 * function HealthMonitor() {
 *   useFieldEffect("Health", (health) => {
 *     if (health < 20) {
 *       playLowHealthWarning();
 *     }
 *   });
 *
 *   return null;
 * }
 * ```
 */
export function useFieldEffect<T = unknown>(
  fieldName: string,
  callback: (value: T) => void
): void {
  const { api } = useChromiumViewContext();
  const callbackRef = useRef(callback);

  // Keep callback ref up to date
  callbackRef.current = callback;

  useEffect(() => {
    if (!api) return;

    const handler = (value: unknown) => {
      callbackRef.current(value as T);
    };

    api.onFieldChanged(fieldName, handler);

    return () => {
      api.offFieldChanged(fieldName, handler);
    };
  }, [api, fieldName]);
}

/**
 * Hook that runs a callback once when the View becomes ready
 *
 * @param callback - Callback to run when ready
 *
 * @example
 * ```tsx
 * function InitComponent() {
 *   useOnReady(() => {
 *     console.log("View is ready!");
 *     initializeGame();
 *   });
 *
 *   return <div>Game</div>;
 * }
 * ```
 */
export function useOnReady(callback: () => void): void {
  const { api, isReady } = useChromiumViewContext();
  const hasCalledRef = useRef(false);
  const callbackRef = useRef(callback);

  // Keep callback ref up to date
  callbackRef.current = callback;

  useEffect(() => {
    if (isReady && !hasCalledRef.current) {
      hasCalledRef.current = true;
      callbackRef.current();
    }
  }, [isReady]);

  useEffect(() => {
    if (!api || hasCalledRef.current) return;

    api.onReady(() => {
      if (!hasCalledRef.current) {
        hasCalledRef.current = true;
        callbackRef.current();
      }
    });
  }, [api]);
}
