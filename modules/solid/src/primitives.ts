/**
 * ChromiumView SolidJS Primitives
 *
 * Reactive primitives for interacting with Unreal Engine ViewModels.
 */

import {
  createSignal,
  createEffect,
  onCleanup,
  type Accessor,
} from "solid-js";
import type { ViewModelState, ViewEventPayload } from "@unreal/chromium-view";
import { useChromiumViewContext } from "./context";
import type {
  CreateFieldOptions,
  CreateViewModelOptions,
  CreateSendEventOptions,
  CreateSendEventResult,
} from "./types";

/**
 * Primitive to subscribe to a single ViewModel field
 *
 * Automatically updates the signal when the field value changes.
 *
 * @param fieldName - The name of the ViewModel field to subscribe to
 * @param options - Configuration options
 * @returns An accessor for the current field value
 *
 * @example
 * ```tsx
 * function HealthBar() {
 *   const health = createField<number>("Health", { initialValue: 100 });
 *
 *   return (
 *     <div class="health-bar">
 *       <div style={{ width: `${health()}%` }} />
 *     </div>
 *   );
 * }
 * ```
 */
export function createField<T = unknown>(
  fieldName: string,
  options: CreateFieldOptions<T> = {}
): Accessor<T | undefined> {
  const { initialValue, transform, skipInitialFetch = false } = options;
  const { api, isReady, getViewModelState } = useChromiumViewContext();
  const [value, setValue] = createSignal<T | undefined>(initialValue);

  // Fetch initial value from ViewModel state
  createEffect(() => {
    if (skipInitialFetch || !isReady()) return;

    getViewModelState<Record<string, unknown>>()
      .then((state) => {
        if (fieldName in state) {
          const rawValue = state[fieldName];
          const newValue = transform ? transform(rawValue) : (rawValue as T);
          setValue(() => newValue);
        }
      })
      .catch((error) => {
        console.error(`Failed to fetch initial state for field "${fieldName}":`, error);
      });
  });

  // Subscribe to field changes
  createEffect(() => {
    const currentApi = api();
    if (!currentApi) {
      console.log(`[createField] No API available for field "${fieldName}"`);
      return;
    }

    console.log(`[createField] Subscribing to field "${fieldName}"`);

    const handleChange = (newValue: unknown) => {
      console.log(`[createField] Received change for "${fieldName}":`, newValue);
      const transformedValue = transform ? transform(newValue) : (newValue as T);
      setValue(() => transformedValue);
    };

    currentApi.onFieldChanged(fieldName, handleChange);

    onCleanup(() => {
      console.log(`[createField] Unsubscribing from field "${fieldName}"`);
      currentApi.offFieldChanged(fieldName, handleChange);
    });
  });

  return value;
}

/**
 * Primitive to subscribe to multiple ViewModel fields
 *
 * @param fieldNames - Array of field names to subscribe to
 * @param options - Configuration options per field
 * @returns An accessor for an object with field values keyed by field name
 *
 * @example
 * ```tsx
 * function PlayerStats() {
 *   const stats = createFields(["Health", "Mana", "Stamina"], {
 *     Health: { initialValue: 100 },
 *     Mana: { initialValue: 100 },
 *     Stamina: { initialValue: 100 },
 *   });
 *
 *   return (
 *     <div>
 *       <div>Health: {stats().Health}</div>
 *       <div>Mana: {stats().Mana}</div>
 *       <div>Stamina: {stats().Stamina}</div>
 *     </div>
 *   );
 * }
 * ```
 */
export function createFields<T extends Record<string, unknown>>(
  fieldNames: (keyof T)[],
  options: Partial<Record<keyof T, CreateFieldOptions<T[keyof T]>>> = {}
): Accessor<Partial<T>> {
  const { api, isReady, getViewModelState } = useChromiumViewContext();

  const getInitialValues = (): Partial<T> => {
    const initial: Partial<T> = {};
    for (const fieldName of fieldNames) {
      const fieldOptions = options[fieldName];
      if (fieldOptions?.initialValue !== undefined) {
        initial[fieldName] = fieldOptions.initialValue;
      }
    }
    return initial;
  };

  const [values, setValues] = createSignal<Partial<T>>(getInitialValues());

  // Fetch initial values
  createEffect(() => {
    if (!isReady()) return;

    getViewModelState<Record<string, unknown>>()
      .then((state) => {
        setValues((prev) => {
          const next = { ...prev };
          for (const fieldName of fieldNames) {
            const key = fieldName as string;
            if (key in state) {
              const fieldOptions = options[fieldName];
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
  });

  // Subscribe to field changes
  createEffect(() => {
    const currentApi = api();
    if (!currentApi) return;

    const handlers: Array<{ field: string; handler: (value: unknown) => void }> = [];

    for (const fieldName of fieldNames) {
      const key = fieldName as string;
      const handler = (newValue: unknown) => {
        const fieldOptions = options[fieldName];
        const value = fieldOptions?.transform
          ? fieldOptions.transform(newValue)
          : newValue;

        setValues((prev) => ({
          ...prev,
          [key]: value,
        }));
      };

      currentApi.onFieldChanged(key, handler);
      handlers.push({ field: key, handler });
    }

    onCleanup(() => {
      for (const { field, handler } of handlers) {
        currentApi.offFieldChanged(field, handler);
      }
    });
  });

  return values;
}

/**
 * Primitive to subscribe to the entire ViewModel state
 *
 * @param options - Configuration options
 * @returns An accessor for the current ViewModel state
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
 *   const state = createViewModel<GameState>({ fetchInitialState: true });
 *
 *   return (
 *     <div>
 *       <div>Player: {state()?.PlayerName}</div>
 *       <div>Score: {state()?.Score}</div>
 *     </div>
 *   );
 * }
 * ```
 */
export function createViewModel<T extends ViewModelState = ViewModelState>(
  options: CreateViewModelOptions<T> = {}
): Accessor<T | null> {
  const { fields, fetchInitialState = true } = options;
  const { api, isReady, getViewModelState } = useChromiumViewContext();
  const [state, setState] = createSignal<T | null>(null);

  // Fetch initial state
  createEffect(() => {
    if (!fetchInitialState || !isReady()) return;

    getViewModelState<T>()
      .then((viewModelState) => setState(() => viewModelState))
      .catch((error) => {
        console.error("Failed to fetch ViewModel state:", error);
      });
  });

  // Subscribe to field changes
  createEffect(() => {
    const currentApi = api();
    if (!currentApi) return;

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

      currentApi.onFieldChanged(key, handler);
      handlers.push({ field: key, handler });
    }

    onCleanup(() => {
      for (const { field, handler } of handlers) {
        currentApi.offFieldChanged(field, handler);
      }
    });
  });

  return state;
}

/**
 * Primitive to send events to the ViewModel
 *
 * Provides loading and error states for the event.
 *
 * @param eventName - The name of the event to send
 * @param options - Configuration options
 * @returns Object with send function and state accessors
 *
 * @example
 * ```tsx
 * function ActionButton() {
 *   const { send, isLoading, error } = createSendEvent<{ action: string }, void>(
 *     "PlayerAction",
 *     { onSuccess: () => console.log("Action sent!") }
 *   );
 *
 *   return (
 *     <button onClick={() => send({ action: "attack" })} disabled={isLoading()}>
 *       {isLoading() ? "..." : "Attack"}
 *     </button>
 *   );
 * }
 * ```
 */
export function createSendEvent<TPayload = ViewEventPayload, TResponse = unknown>(
  eventName: string,
  options: CreateSendEventOptions = {}
): CreateSendEventResult<TPayload, TResponse> {
  const { throwOnError = false, onSuccess, onError } = options;
  const { sendEvent } = useChromiumViewContext();

  const [isLoading, setIsLoading] = createSignal(false);
  const [error, setError] = createSignal<Error | null>(null);
  const [data, setData] = createSignal<TResponse | undefined>(undefined);

  const send = async (payload?: TPayload): Promise<TResponse | undefined> => {
    setIsLoading(true);
    setError(null);

    try {
      const result = await sendEvent<TPayload, TResponse>(eventName, payload);
      setData(() => result);
      onSuccess?.(result);
      return result;
    } catch (err) {
      const errorObj = err instanceof Error ? err : new Error(String(err));
      setError(errorObj);
      onError?.(errorObj);
      if (throwOnError) {
        throw errorObj;
      }
      return undefined;
    } finally {
      setIsLoading(false);
    }
  };

  const reset = () => {
    setIsLoading(false);
    setError(null);
    setData(undefined);
  };

  return { send, isLoading, error, data, reset };
}

/**
 * Primitive to get a stable sendEvent function
 *
 * Simpler alternative to createSendEvent when you don't need loading/error states.
 *
 * @returns Function to send events
 *
 * @example
 * ```tsx
 * function ActionButton() {
 *   const sendEvent = createSendEventCallback();
 *
 *   return (
 *     <button onClick={() => sendEvent("PlayerAction", { action: "jump" })}>
 *       Jump
 *     </button>
 *   );
 * }
 * ```
 */
export function createSendEventCallback(): <TPayload = ViewEventPayload, TResponse = unknown>(
  eventName: string,
  payload?: TPayload
) => Promise<TResponse> {
  const { sendEvent } = useChromiumViewContext();
  return sendEvent;
}

/**
 * Primitive to check if the ChromiumView is ready
 *
 * @returns Accessor for whether the View is ready
 *
 * @example
 * ```tsx
 * function LoadingScreen() {
 *   const isReady = createIsReady();
 *
 *   return (
 *     <Show when={isReady()} fallback={<div>Loading...</div>}>
 *       <GameHUD />
 *     </Show>
 *   );
 * }
 * ```
 */
export function createIsReady(): Accessor<boolean> {
  const { isReady } = useChromiumViewContext();
  return isReady;
}

/**
 * Primitive to get the ChromiumView API instance
 *
 * @returns Accessor for the ChromiumView API or null if not available
 *
 * @example
 * ```tsx
 * function AdvancedComponent() {
 *   const api = createChromiumViewApi();
 *
 *   createEffect(() => {
 *     const currentApi = api();
 *     if (!currentApi) return;
 *     // Direct API access for advanced use cases
 *   });
 * }
 * ```
 */
export function createChromiumViewApi() {
  const { api } = useChromiumViewContext();
  return api;
}

/**
 * Primitive to set the desired size of the View
 *
 * @returns Function to set the desired size
 *
 * @example
 * ```tsx
 * function ResizableView() {
 *   const setSize = createSetDesiredSize();
 *
 *   onMount(() => {
 *     setSize(1920, 1080);
 *   });
 * }
 * ```
 */
export function createSetDesiredSize() {
  const { setDesiredSize } = useChromiumViewContext();
  return setDesiredSize;
}

/**
 * Primitive that runs a callback when a field changes
 *
 * @param fieldName - The name of the field to watch
 * @param callback - Callback to run when the field changes
 *
 * @example
 * ```tsx
 * function HealthMonitor() {
 *   createFieldEffect("Health", (health) => {
 *     if (health < 20) {
 *       playLowHealthWarning();
 *     }
 *   });
 *
 *   return null;
 * }
 * ```
 */
export function createFieldEffect<T = unknown>(
  fieldName: string,
  callback: (value: T) => void
): void {
  const { api } = useChromiumViewContext();

  createEffect(() => {
    const currentApi = api();
    if (!currentApi) return;

    const handler = (value: unknown) => {
      callback(value as T);
    };

    currentApi.onFieldChanged(fieldName, handler);

    onCleanup(() => {
      currentApi.offFieldChanged(fieldName, handler);
    });
  });
}

/**
 * Primitive that runs a callback once when the View becomes ready
 *
 * @param callback - Callback to run when ready
 *
 * @example
 * ```tsx
 * function InitComponent() {
 *   createOnReady(() => {
 *     console.log("View is ready!");
 *     initializeGame();
 *   });
 *
 *   return <div>Game</div>;
 * }
 * ```
 */
export function createOnReady(callback: () => void): void {
  const { api, isReady } = useChromiumViewContext();
  let hasCalled = false;

  createEffect(() => {
    if (isReady() && !hasCalled) {
      hasCalled = true;
      callback();
    }
  });

  createEffect(() => {
    const currentApi = api();
    if (!currentApi || hasCalled) return;

    currentApi.onReady(() => {
      if (!hasCalled) {
        hasCalled = true;
        callback();
      }
    });
  });
}
