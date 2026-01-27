/**
 * Type definitions for ChromiumView React bindings
 */

import type {
  ChromiumViewAPI,
  ViewModelState,
  ViewEventPayload,
  ChromiumViewSizeMode,
} from "@unreal/chromium-view";

/**
 * Props for the ChromiumViewProvider component
 */
export interface ChromiumViewProviderProps {
  /** Child components */
  children: React.ReactNode;
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
  api: ChromiumViewAPI | null;
  /** Whether the View is ready */
  isReady: boolean;
  /** Whether the API is available */
  isAvailable: boolean;
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
 * Options for the useField hook
 */
export interface UseFieldOptions<T> {
  /** Initial value before the first update from ViewModel */
  initialValue?: T;
  /** Transform function to apply to the value */
  transform?: (value: unknown) => T;
  /** Whether to skip the initial state fetch */
  skipInitialFetch?: boolean;
}

/**
 * Options for the useViewModel hook
 */
export interface UseViewModelOptions<T extends ViewModelState> {
  /** Fields to subscribe to (subscribes to all if not specified) */
  fields?: (keyof T)[];
  /** Whether to fetch initial state */
  fetchInitialState?: boolean;
}

/**
 * Options for the useSendEvent hook
 */
export interface UseSendEventOptions {
  /** Whether to throw errors (default: false, returns them instead) */
  throwOnError?: boolean;
  /** Callback when the event is sent successfully */
  onSuccess?: <T>(result: T) => void;
  /** Callback when an error occurs */
  onError?: (error: Error) => void;
}

/**
 * Result from the useSendEvent hook
 */
export interface UseSendEventResult<TPayload, TResponse> {
  /** Function to send the event */
  send: (payload?: TPayload) => Promise<TResponse | undefined>;
  /** Whether the event is currently being sent */
  isLoading: boolean;
  /** The last error that occurred */
  error: Error | null;
  /** The last response received */
  data: TResponse | undefined;
  /** Reset the state */
  reset: () => void;
}

/**
 * Props for the ChromiumView component
 */
export interface ChromiumViewProps {
  /** Child components to render */
  children: React.ReactNode;
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
  className?: string;
  /** Inline styles */
  style?: React.CSSProperties;
}

/**
 * Props for field binding HOC
 */
export interface WithFieldProps<T> {
  /** The bound field value */
  value: T;
  /** Whether the field is loading */
  isLoading: boolean;
}
