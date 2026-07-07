/**
 * Type definitions for ChromiumView SolidJS bindings
 */

import type { JSX, Accessor } from "solid-js";
import type {
  ChromiumViewAPI,
  ViewModelState,
  ViewEventPayload,
  ChromiumViewSizeMode,
} from "@incanta/ue-view-base";

/**
 * Props for the ChromiumViewProvider component
 */
export interface ChromiumViewProviderProps {
  /** Child components */
  children: JSX.Element;
  /** Optional custom ChromiumView API instance */
  api?: ChromiumViewAPI;
  /** Whether to automatically notify ready when mounted */
  autoNotifyReady?: boolean;
  /** Callback when the View is ready */
  onReady?: () => void;
  /** Callback when an error occurs */
  onError?: (error: Error) => void;
}

/**
 * Context value for ChromiumView
 */
export interface ChromiumViewContextValue {
  /** The ChromiumView API instance */
  api: Accessor<ChromiumViewAPI | null>;
  /** Whether the View is ready */
  isReady: Accessor<boolean>;
  /** Whether the API is available */
  isAvailable: Accessor<boolean>;
  /** Send an event to the ViewModel */
  sendEvent: <TPayload = ViewEventPayload, TResponse = unknown>(
    eventName: string,
    payload?: TPayload
  ) => Promise<TResponse>;
  /** Get the current ViewModel state */
  getViewModelState: <T = ViewModelState>() => Promise<T>;
  /** Set the desired size of the View */
  setDesiredSize: (width: number, height: number, sizeMode?: ChromiumViewSizeMode) => void;
  /** Notify that the View is ready */
  notifyReady: () => void;
}

/**
 * Options for the createField primitive
 */
export interface CreateFieldOptions<T> {
  /** Initial value before the first update from ViewModel */
  initialValue?: T;
  /** Transform function to apply to the value */
  transform?: (value: unknown) => T;
  /** Whether to skip the initial state fetch */
  skipInitialFetch?: boolean;
}

/**
 * Options for the createViewModel primitive
 */
export interface CreateViewModelOptions<T extends ViewModelState> {
  /** Fields to subscribe to (subscribes to all if not specified) */
  fields?: (keyof T)[];
  /** Whether to fetch initial state */
  fetchInitialState?: boolean;
}

/**
 * Options for the createSendEvent primitive
 */
export interface CreateSendEventOptions {
  /** Whether to throw errors (default: false, returns them instead) */
  throwOnError?: boolean;
  /** Callback when the event is sent successfully */
  onSuccess?: <T>(result: T) => void;
  /** Callback when an error occurs */
  onError?: (error: Error) => void;
}

/**
 * Result from the createSendEvent primitive
 */
export interface CreateSendEventResult<TPayload, TResponse> {
  /** Function to send the event */
  send: (payload?: TPayload) => Promise<TResponse | undefined>;
  /** Whether the event is currently being sent */
  isLoading: Accessor<boolean>;
  /** The last error that occurred */
  error: Accessor<Error | null>;
  /** The last response received */
  data: Accessor<TResponse | undefined>;
  /** Reset the state */
  reset: () => void;
}

/**
 * Props for the ChromiumView component
 */
export interface ChromiumViewProps {
  /** Child components to render */
  children: JSX.Element;
  /** Whether to automatically notify ready */
  autoNotifyReady?: boolean;
  /** Desired width in pixels */
  width?: number;
  /** Desired height in pixels */
  height?: number;
  /** Size mode */
  sizeMode?: ChromiumViewSizeMode;
  /** Callback when the View is ready */
  onReady?: () => void;
  /** CSS class name */
  class?: string;
  /** Inline styles */
  style?: JSX.CSSProperties;
}

/**
 * Props for field binding components
 */
export interface WithFieldProps<T> {
  /** The bound field value */
  value: T;
  /** Whether the field is loading */
  isLoading: boolean;
}
