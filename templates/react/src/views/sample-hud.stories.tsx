import type { Meta, StoryObj } from "@storybook/react";

interface SampleHUDViewProps {
  playerName: string;
  health: number;
  score: number;
}

/**
 * Presentational Sample HUD View for Storybook
 */
function SampleHUDView({ playerName, health, score }: SampleHUDViewProps) {
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

const meta: Meta<typeof SampleHUDView> = {
  title: "Views/SampleHUD",
  component: SampleHUDView,
  parameters: {
    layout: "fullscreen",
  },
};

export default meta;
type Story = StoryObj<typeof SampleHUDView>;

export const Default: Story = {
  args: {
    playerName: "Player",
    health: 100,
    score: 0,
  },
};

export const InGame: Story = {
  args: {
    playerName: "TreeBuilder42",
    health: 75,
    score: 1250,
  },
};

export const LowHealth: Story = {
  args: {
    playerName: "DangerZone",
    health: 15,
    score: 8500,
  },
};

export const HighScore: Story = {
  args: {
    playerName: "Champion",
    health: 100,
    score: 99999,
  },
};
