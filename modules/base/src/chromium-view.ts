/**
 * ChromiumView API Implementation
 *
 * This module provides the main API for communicating between
 * web Views and Unreal Engine ViewModels.
 */

import {
  ChromiumViewAPI,
  ChromiumViewSizeMode,
  FieldChangeCallback,
  ViewEventPayload,
  ViewModelState,
} from "./types";

interface PendingCallback {
  resolve: (value: unknown) => void;
  reject: (reason: unknown) => void;
}

/**
 * ChromiumView client class
 *
 * Provides a typed API for interacting with Unreal Engine ViewModels
 * from within a web View.
 *
 * @example
 * ```typescript
 * const chromiumView = new ChromiumViewClient();
 *
 * // Listen for ViewModel changes
 * chromiumView.onFieldChanged("playerHealth", (newValue) => {
 *   updateHealthBar(newValue);
 * });
 *
 * // Send events to the ViewModel
 * await chromiumView.sendEvent("buttonClicked", { buttonId: "start" });
 *
 * // Notify that the View is ready
 * chromiumView.notifyReady();
 * ```
 */
export class ChromiumViewClient implements ChromiumViewAPI {
  private callbacks: Map<number, PendingCallback> = new Map();
  private nextCallbackId = 1;
  private fieldChangeListeners: Map<string, Set<FieldChangeCallback>> = new Map();
  private readyListeners: Array<() => void> = [];
  private isReady = false;
  private initialState: ViewModelState | null = null;

  constructor() {
    this.setupGlobalAPI();
  }

  /**
   * Set up the global ChromiumView API for C++ to call
   */
  private setupGlobalAPI(): void {
    const api: ChromiumViewAPI = {
      onFieldChanged: this.onFieldChanged.bind(this),
      offFieldChanged: this.offFieldChanged.bind(this),
      sendEvent: this.sendEvent.bind(this),
      getViewModelState: this.getViewModelState.bind(this),
      setDesiredSize: this.setDesiredSize.bind(this),
      onReady: this.onReady.bind(this),
      notifyReady: this.notifyReady.bind(this),
      _onFieldChanged: this.handleFieldChanged.bind(this),
      _resolveCallback: this.resolveCallback.bind(this),
      _initialState: this.handleInitialState.bind(this),
    };

    window.ChromiumView = api;
  }

  /**
   * Register a callback for ViewModel field changes
   */
  onFieldChanged<T = unknown>(fieldName: string, callback: FieldChangeCallback<T>): void {
    console.log(`[ChromiumView] Registering listener for field "${fieldName}"`);
    if (!this.fieldChangeListeners.has(fieldName)) {
      this.fieldChangeListeners.set(fieldName, new Set());
    }
    this.fieldChangeListeners.get(fieldName)!.add(callback as FieldChangeCallback);
    console.log(`[ChromiumView] Now have ${this.fieldChangeListeners.get(fieldName)!.size} listeners for "${fieldName}"`);

    // Request the current value from C++ so new listeners (including after HMR) get the current value
    if (window.ue?.chromiumviewbridge) {
      window.ue.chromiumviewbridge.requestfieldvalue(fieldName);
    }
  }

  /**
   * Remove a callback for ViewModel field changes
   */
  offFieldChanged<T = unknown>(fieldName: string, callback: FieldChangeCallback<T>): void {
    const listeners = this.fieldChangeListeners.get(fieldName);
    if (listeners) {
      listeners.delete(callback as FieldChangeCallback);
    }
  }

  /**
   * Send an event to the ViewModel
   */
  sendEvent<TPayload = ViewEventPayload, TResponse = unknown>(
    eventName: string,
    payload?: TPayload
  ): Promise<TResponse> {
    return new Promise((resolve, reject) => {
      const callbackId = this.nextCallbackId++;
      this.callbacks.set(callbackId, {
        resolve: resolve as (value: unknown) => void,
        reject,
      });

      const jsonPayload = JSON.stringify(payload || {});

      if (window.ue?.chromiumviewbridge) {
        window.ue.chromiumviewbridge.handleviewevent(eventName, jsonPayload, callbackId);
      } else {
        this.callbacks.delete(callbackId);
        reject(new Error("ChromiumView bridge not available"));
      }
    });
  }

  /**
   * Get the current ViewModel state
   */
  async getViewModelState<T = ViewModelState>(): Promise<T> {
    // Return cached initial state if we have it
    if (this.initialState) {
      return this.initialState as T;
    }

    if (window.ue?.chromiumviewbridge) {
      const jsonString = await window.ue.chromiumviewbridge.getviewmodelstatejson();
      return JSON.parse(jsonString) as T;
    }

    throw new Error("ChromiumView bridge not available");
  }

  /**
   * Set the desired size of the View
   */
  setDesiredSize(
    width: number,
    height: number,
    sizeMode: ChromiumViewSizeMode = ChromiumViewSizeMode.DesiredSize
  ): void {
    if (window.ue?.chromiumviewbridge) {
      window.ue.chromiumviewbridge.setdesiredsize(width, height, sizeMode);
    }
  }

  /**
   * Register a callback for when the View is ready
   */
  onReady(callback: () => void): void {
    if (this.isReady) {
      callback();
    } else {
      this.readyListeners.push(callback);
    }
  }

  /**
   * Notify that the View is ready
   * Only notifies the bridge once - subsequent calls are ignored
   */
  notifyReady(): void {
    // Only notify once
    if (this.isReady) {
      return;
    }

    this.isReady = true;
    this.readyListeners.forEach((cb) => cb());
    this.readyListeners = [];

    if (window.ue?.chromiumviewbridge) {
      window.ue.chromiumviewbridge.notifyviewready();
    }
  }

  /**
   * Get the initial ViewModel state (if available)
   */
  getInitialState<T = ViewModelState>(): T | null {
    return this.initialState as T | null;
  }

  // Internal handlers called by C++

  private handleFieldChanged(fieldName: string, newValue: unknown): void {
    console.log(`[ChromiumView] Field changed: ${fieldName} =`, newValue);
    console.log(`[ChromiumView] Registered listeners:`, Array.from(this.fieldChangeListeners.keys()));

    const listeners = this.fieldChangeListeners.get(fieldName);
    console.log(`[ChromiumView] Listeners for "${fieldName}":`, listeners?.size ?? 0);

    if (listeners) {
      listeners.forEach((callback) => {
        try {
          callback(newValue);
        } catch (e) {
          console.error("Error in field change listener:", e);
        }
      });
    }
  }

  private resolveCallback(callbackId: number, result: unknown, isError: boolean): void {
    const callback = this.callbacks.get(callbackId);
    if (callback) {
      this.callbacks.delete(callbackId);
      if (isError) {
        callback.reject(result);
      } else {
        callback.resolve(result);
      }
    }
  }

  private handleInitialState(state: ViewModelState): void {
    this.initialState = state;
  }
}

/**
 * Get the global ChromiumView API instance
 *
 * If running in an Unreal Engine ChromiumView context, this will return
 * the API instance. Otherwise, it may return undefined.
 *
 * @returns The ChromiumView API or undefined
 */
export function getChromiumView(): ChromiumViewAPI | undefined {
  return window.ChromiumView;
}

/**
 * Check if running in a ChromiumView context
 *
 * @returns True if the Unreal Engine bridge is available
 */
export function isChromiumViewAvailable(): boolean {
  return !!window.ue?.chromiumviewbridge;
}

/**
 * Create and initialize a new ChromiumView client
 *
 * This is the recommended way to initialize ChromiumView in your application.
 *
 * @returns A new ChromiumViewClient instance
 */
export function createChromiumView(): ChromiumViewClient {
  return new ChromiumViewClient();
}
