import type { Meta, StoryObj } from "@storybook/react";
import { Nameplate } from "./nameplate";

const meta: Meta<typeof Nameplate> = {
  title: "Components/Nameplate",
  component: Nameplate,
  tags: ["autodocs"],
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
type Story = StoryObj<typeof Nameplate>;

export const Default: Story = {
  args: {
    name: "PlayerOne",
  },
};

export const LongName: Story = {
  args: {
    name: "SuperLongPlayerUsername123",
  },
};

export const ShortName: Story = {
  args: {
    name: "Bob",
  },
};

export const LargeText: Story = {
  args: {
    name: "PlayerOne",
    fontSize: "text-xl",
  },
};

export const SmallText: Story = {
  args: {
    name: "PlayerOne",
    fontSize: "text-xs",
  },
};

export const CustomColors: Story = {
  args: {
    name: "GuildMaster",
    backgroundColor: "bg-indigo-900/80",
    borderColor: "border-indigo-400/60",
    textColor: "text-indigo-100",
  },
};

export const GreenTheme: Story = {
  args: {
    name: "FriendlyPlayer",
    backgroundColor: "bg-green-900/80",
    borderColor: "border-green-400/60",
    textColor: "text-green-100",
  },
};

export const RedTheme: Story = {
  args: {
    name: "EnemyPlayer",
    backgroundColor: "bg-red-900/80",
    borderColor: "border-red-400/60",
    textColor: "text-red-100",
  },
};
