import type { Meta, StoryObj } from "@storybook/react";
import { InventoryItem } from "./inventory-item";

const meta: Meta<typeof InventoryItem> = {
  title: "Components/InventoryItem",
  component: InventoryItem,
  tags: ["autodocs"],
  argTypes: {
    onClick: { action: "clicked" },
  },
};

export default meta;
type Story = StoryObj<typeof InventoryItem>;

export const Default: Story = {
  args: {
    itemName: "Wood Plank",
    stackCount: 1,
  },
};

export const WithStackCount: Story = {
  args: {
    itemName: "Nails",
    stackCount: 64,
  },
};

export const WithIcon: Story = {
  args: {
    itemName: "Hammer",
    stackCount: 1,
    icon: "https://via.placeholder.com/48",
  },
};

export const StackedWithIcon: Story = {
  args: {
    itemName: "Rope",
    stackCount: 12,
    icon: "https://via.placeholder.com/48",
  },
};
