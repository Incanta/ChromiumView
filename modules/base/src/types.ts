/**
 * Type definitions for ChromiumView MVVM bindings
 */

/**
 * Size modes for ChromiumView
 */
export enum ChromiumViewSizeMode {
  /** The view fills the entire screen/parent widget */
  FullScreen = 0,
  /** The view uses a specific size defined by the HTML content */
  DesiredSize = 1,
  /** The view size is controlled by parent widget */
  Auto = 2
}

/**
 * Desired size information for a View
 */
export interface ChromiumViewDesiredSize {
  width: number;
  height: number;
  sizeMode: ChromiumViewSizeMode;
}

/**
 * Generic ViewModel state object
 * Can be extended with specific ViewModel properties
 */
export type ViewModelState<T = Record<string, unknown>> = T;

/**
 * Event payload sent from View to ViewModel
 */
export interface ViewEventPayload {
  [key: string]: unknown;
}

/**
 * Callback function for field changes
 */
export type FieldChangeCallback<T = unknown> = (newValue: T) => void;

/**
 * Internal UE bridge interface exposed by Unreal Engine
 * This is the low-level interface injected by Chromium
 */
export interface UEBridge {
  chromiumviewbridge?: {
    handleviewevent: (eventName: string, jsonPayload: string, callbackId: number) => void;
    getviewmodelstatejson: () => Promise<string>;
    setdesiredsize: (width: number, height: number, sizeMode: number) => void;
    notifyviewready: () => void;
    requestfieldvalue: (fieldName: string) => void;
  };
}

/**
 * Declaration for the UE global object
 */
declare global {
  interface Window {
    ue?: UEBridge;
    ChromiumView?: ChromiumViewAPI;
  }
}

/**
 * ChromiumView API interface
 */
export interface ChromiumViewAPI {
  /**
   * Register a callback for ViewModel field changes
   * @param fieldName - The name of the field to watch
   * @param callback - Callback function receiving the new value
   */
  onFieldChanged<T = unknown>(fieldName: string, callback: FieldChangeCallback<T>): void;

  /**
   * Remove a callback for ViewModel field changes
   * @param fieldName - The name of the field
   * @param callback - The callback to remove
   */
  offFieldChanged<T = unknown>(fieldName: string, callback: FieldChangeCallback<T>): void;

  /**
   * Send an event to the ViewModel
   * @param eventName - The name of the event
   * @param payload - The event payload
   * @returns Promise that resolves with the response from the ViewModel
   */
  sendEvent<TPayload = ViewEventPayload, TResponse = unknown>(
    eventName: string,
    payload?: TPayload
  ): Promise<TResponse>;

  /**
   * Get the current ViewModel state
   * @returns Promise that resolves with the ViewModel state
   */
  getViewModelState<T = ViewModelState>(): Promise<T>;

  /**
   * Set the desired size of the View
   * @param width - Desired width in pixels
   * @param height - Desired height in pixels
   * @param sizeMode - Size mode (default: DesiredSize)
   */
  setDesiredSize(width: number, height: number, sizeMode?: ChromiumViewSizeMode): void;

  /**
   * Register a callback for when the View is ready
   * @param callback - Callback function
   */
  onReady(callback: () => void): void;

  /**
   * Notify that the View is ready
   * Should be called by the View HTML when it's finished loading
   */
  notifyReady(): void;

  // Internal methods (used by C++ bridge)
  /** @internal */
  _onFieldChanged?(fieldName: string, newValue: unknown): void;
  /** @internal */
  _resolveCallback?(callbackId: number, result: unknown, isError: boolean): void;
  /** @internal */
  _initialState?(state: ViewModelState): void;
}
