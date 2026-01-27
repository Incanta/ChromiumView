import React from "react";
import { createRoot } from "react-dom/client";
import {
  ChromiumView,
  WhenReady,
} from "@unreal/chromium-view-react";
import { type ChromiumViewSizeMode } from "@unreal/chromium-view";
import "../main.css";

export interface CreateAppOptions {
  /** Width of the ChromiumView */
  width?: number;
  /** Height of the ChromiumView */
  height?: number;
  /** Size mode for the ChromiumView */
  sizeMode?: ChromiumViewSizeMode;
  /** Whether to auto-notify ready (default: true) */
  autoNotifyReady?: boolean;
  /** Custom loading fallback component */
  loadingFallback?: React.ReactNode;
}

const defaultLoadingFallback = (
  <div className="fixed inset-0 w-screen h-screen flex items-center justify-center bg-transparent text-white font-sans">
    Loading...
  </div>
);

/**
 * Creates and mounts a ChromiumView app with the given component.
 * Handles all the boilerplate of setting up ChromiumView, WhenReady, and mounting.
 *
 * @example
 * ```tsx
 * // inventory.tsx
 * import { createApp } from "./lib/create-app";
 *
 * function Inventory() {
 *   // Your component logic
 *   return <div>Inventory</div>;
 * }
 *
 * createApp(<Inventory />);
 * ```
 */
export function createApp(
  component: React.ReactNode,
  options: CreateAppOptions = {}
): void {
  const {
    width,
    height,
    sizeMode,
    autoNotifyReady = true,
    loadingFallback = defaultLoadingFallback,
  } = options;

  function App() {
    return (
      <ChromiumView
        {...(width !== undefined && { width })}
        {...(height !== undefined && { height })}
        {...(sizeMode !== undefined && { sizeMode })}
        autoNotifyReady={autoNotifyReady}
      >
        <WhenReady fallback={loadingFallback}>
          {component}
        </WhenReady>
      </ChromiumView>
    );
  }

  const root = document.getElementById("root");
  if (root) {
    createRoot(root).render(<App />);
  } else {
    console.error("ERROR: Root element not found");
  }
}
