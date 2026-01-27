import { useField } from "@unreal/chromium-view-react";
import { createApp } from "../utils/create-app";

/**
 * Sample HUD component that demonstrates useField hook
 * Displays player name and health from the ViewModel
 */
function SampleHUD() {
  const playerName = useField<string>("PlayerName", { initialValue: "Player" });
  const health = useField<number>("Health", { initialValue: 100 });
  const score = useField<number>("Score", { initialValue: 0 });

  return (
    <div className="fixed inset-0 w-screen h-screen bg-transparent text-white font-sans p-5 box-border flex flex-col justify-start items-start">
      <div className="mb-5">
        <span className="text-3xl font-bold drop-shadow-lg">{playerName}</span>
      </div>
      <div className="flex flex-col gap-2.5">
        <div className="flex gap-2.5 text-2xl drop-shadow-md">
          <span className="font-bold text-neutral-400">Score:</span>
          <span className="text-white">{score}</span>
        </div>
        <div className="flex gap-2.5 text-2xl drop-shadow-md">
          <span className="font-bold text-neutral-400">Health:</span>
          <span className="text-white">{health}</span>
        </div>
      </div>
    </div>
  );
}

createApp(<SampleHUD />, {
  loadingFallback: (
    <div className="fixed inset-0 w-screen h-screen bg-transparent text-white font-sans p-5 box-border flex flex-col justify-start items-start">
      Loading...
    </div>
  ),
});
