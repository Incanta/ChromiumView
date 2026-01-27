import { useField } from "@unreal/chromium-view-react";
import { createApp } from "../utils/create-app";
import { Nameplate, NameplateProps } from "../components";
import { ChromiumViewSizeMode } from "@unreal/chromium-view";

/**
 * Props for the NameplateView presentational component
 */
export interface NameplateViewProps extends NameplateProps {}

/**
 * Presentational Nameplate View component
 * Displays a player/entity name in a compact rounded box
 * Auto-sizes to fit content - not full screen
 */
export function NameplateView(props: NameplateViewProps) {
  return (
    <div className="inline-block">
      <Nameplate {...props} />
    </div>
  );
}

/**
 * Connected Nameplate component with Unreal hooks
 */
function NameplateConnected() {
  const name = useField<string>("Name", {
    initialValue: "Player",
  });
  const isVisible = useField<boolean>("NameplateVisible", {
    initialValue: true,
  });

  if (!isVisible) {
    return null;
  }

  return <NameplateView name={name ?? "Player"} />;
}

createApp(<NameplateConnected />, {
  sizeMode: ChromiumViewSizeMode.Auto,
  loadingFallback: (
    <div className="inline-block px-3 py-1.5 bg-neutral-900/80 border border-neutral-500/60 rounded-lg">
      ...
    </div>
  ),
});
