/**
 * Decorators for ChromiumView TypeScript Views
 *
 * These decorators provide a convenient way to bind ViewModel properties
 * and events in TypeScript/JavaScript Views.
 */

import { getChromiumView } from "./chromium-view";
import { FieldChangeCallback } from "./types";

/**
 * Decorator to bind a property to a ViewModel field
 *
 * When the ViewModel field changes, the decorated property will be updated.
 *
 * @param fieldName - The name of the ViewModel field to bind to
 *
 * @example
 * ```typescript
 * class MyView {
 *   @BindField("playerHealth")
 *   health: number = 100;
 * }
 * ```
 */
export function BindField(fieldName: string) {
  return function <T>(target: unknown, propertyKey: string) {
    const privateKey = `_${propertyKey}`;

    Object.defineProperty(target, propertyKey, {
      get: function (this: Record<string, T>) {
        return this[privateKey];
      },
      set: function (this: Record<string, T>, value: T) {
        this[privateKey] = value;
      },
      enumerable: true,
      configurable: true,
    });

    // Store binding info for initialization
    const bindings: Array<{ fieldName: string; propertyKey: string }> =
      (target as Record<string, unknown>).__chromiumViewBindings as Array<{
        fieldName: string;
        propertyKey: string;
      }> || [];
    bindings.push({ fieldName, propertyKey });
    (target as Record<string, unknown>).__chromiumViewBindings = bindings;
  };
}

/**
 * Decorator to mark a method as a ViewModel event handler
 *
 * The method will be called when a field on the ViewModel changes.
 *
 * @param fieldName - The name of the ViewModel field to watch
 *
 * @example
 * ```typescript
 * class MyView {
 *   @OnFieldChange("playerHealth")
 *   onHealthChanged(newValue: number) {
 *     this.updateHealthBar(newValue);
 *   }
 * }
 * ```
 */
export function OnFieldChange(fieldName: string) {
  return function (
    target: unknown,
    propertyKey: string,
    descriptor: PropertyDescriptor
  ) {
    const handlers: Array<{ fieldName: string; methodKey: string }> =
      (target as Record<string, unknown>).__chromiumViewHandlers as Array<{
        fieldName: string;
        methodKey: string;
      }> || [];
    handlers.push({ fieldName, methodKey: propertyKey });
    (target as Record<string, unknown>).__chromiumViewHandlers = handlers;

    return descriptor;
  };
}

/**
 * Initialize ChromiumView bindings for a view instance
 *
 * Call this method after constructing a view class that uses
 * @BindField or @OnFieldChange decorators.
 *
 * @param instance - The view instance to initialize
 *
 * @example
 * ```typescript
 * class MyView {
 *   @BindField("playerHealth")
 *   health: number = 100;
 *
 *   @OnFieldChange("playerName")
 *   onNameChanged(name: string) {
 *     console.log("Player name:", name);
 *   }
 * }
 *
 * const view = new MyView();
 * initializeBindings(view);
 * ```
 */
export function initializeBindings(instance: object): void {
  const chromiumView = getChromiumView();
  if (!chromiumView) {
    console.warn("ChromiumView not available, bindings will not work");
    return;
  }

  const prototype = Object.getPrototypeOf(instance);

  // Initialize field bindings
  const bindings: Array<{ fieldName: string; propertyKey: string }> =
    prototype.__chromiumViewBindings || [];
  for (const binding of bindings) {
    chromiumView.onFieldChanged(binding.fieldName, (newValue) => {
      (instance as Record<string, unknown>)[binding.propertyKey] = newValue;
    });
  }

  // Initialize change handlers
  const handlers: Array<{ fieldName: string; methodKey: string }> =
    prototype.__chromiumViewHandlers || [];
  for (const handler of handlers) {
    const method = (instance as Record<string, FieldChangeCallback>)[handler.methodKey];
    if (typeof method === "function") {
      chromiumView.onFieldChanged(handler.fieldName, method.bind(instance));
    }
  }
}

/**
 * Base class for ChromiumView-enabled views
 *
 * Extend this class to automatically initialize bindings
 * when using decorators.
 *
 * @example
 * ```typescript
 * class MyView extends ChromiumViewBase {
 *   @BindField("score")
 *   score: number = 0;
 *
 *   @OnFieldChange("gameState")
 *   onGameStateChanged(state: string) {
 *     console.log("Game state:", state);
 *   }
 * }
 *
 * const view = new MyView();
 * // Bindings are automatically initialized
 * ```
 */
export abstract class ChromiumViewBase {
  constructor() {
    // Defer initialization to allow subclass constructor to complete
    setTimeout(() => {
      initializeBindings(this);
    }, 0);
  }

  /**
   * Send an event to the ViewModel
   */
  protected async sendEvent<TPayload = unknown, TResponse = unknown>(
    eventName: string,
    payload?: TPayload
  ): Promise<TResponse | undefined> {
    const chromiumView = getChromiumView();
    if (chromiumView) {
      return chromiumView.sendEvent<TPayload, TResponse>(eventName, payload);
    }
    return undefined;
  }

  /**
   * Get the current ViewModel state
   */
  protected async getViewModelState<T = Record<string, unknown>>(): Promise<T | undefined> {
    const chromiumView = getChromiumView();
    if (chromiumView) {
      return chromiumView.getViewModelState<T>();
    }
    return undefined;
  }
}
