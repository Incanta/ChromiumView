import type { Meta, StoryObj } from "@storybook/react";
import { NameplateView } from "./nameplate";

const meta: Meta<typeof NameplateView> = {
  title: "Views/Nameplate",
  component: NameplateView,
  parameters: {
    layout: "centered",
  },
  argTypes: {
    fontSize: {
      control: "select",
      options: ["text-xs", "text-sm", "text-base", "text-lg", "text-xl"],
    },
  },
};

export default meta;
type Story = StoryObj<typeof NameplateView>;

export const Default: Story = {
  args: {
    name: "PlayerOne",
  },
};

export const NPC: Story = {
  args: {
    name: "Friendly Merchant",
    backgroundColor: "bg-amber-900/80",
    borderColor: "border-amber-400/60",
    textColor: "text-amber-100",
  },
};

export const Enemy: Story = {
  args: {
    name: "Dark Knight",
    backgroundColor: "bg-red-900/80",
    borderColor: "border-red-500/60",
    textColor: "text-red-100",
    fontSize: "text-base",
  },
};

export const TeamMember: Story = {
  args: {
    name: "Ally_2024",
    backgroundColor: "bg-blue-900/80",
    borderColor: "border-blue-400/60",
    textColor: "text-blue-100",
  },
};
