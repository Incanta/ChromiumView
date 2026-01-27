import type { Meta, StoryObj } from "@storybook/react";
import { InventoryView } from "./inventory";
import { InventoryItemData } from "../types/view-model/index";

const meta: Meta<typeof InventoryView> = {
  title: "Views/Inventory",
  component: InventoryView,
  parameters: {
    layout: "fullscreen",
  },
  argTypes: {
    onDismiss: { action: "dismiss" },
    onItemClick: { action: "itemClicked" },
  },
};

export default meta;
type Story = StoryObj<typeof InventoryView>;

const sampleItems: InventoryItemData[] = [
  { id: "1", name: "Wood Plank", stackCount: 24, icon: "https://via.placeholder.com/48/8B4513/FFFFFF?text=W" },
  { id: "2", name: "Nails", stackCount: 64, icon: "https://via.placeholder.com/48/808080/FFFFFF?text=N" },
  { id: "3", name: "Hammer", stackCount: 1, icon: "https://via.placeholder.com/48/4A4A4A/FFFFFF?text=H" },
  { id: "4", name: "Rope", stackCount: 12, icon: "https://via.placeholder.com/48/DAA520/FFFFFF?text=R" },
  { id: "5", name: "Screws", stackCount: 48, icon: "https://via.placeholder.com/48/C0C0C0/FFFFFF?text=S" },
  { id: "6", name: "Paint Bucket", stackCount: 3, icon: "https://via.placeholder.com/48/FF6347/FFFFFF?text=P" },
];

export const Default: Story = {
  args: {
    inventoryItems: sampleItems,
  },
};

export const Empty: Story = {
  args: {
    inventoryItems: [],
  },
};

export const SingleItem: Story = {
  args: {
    inventoryItems: [sampleItems[0]],
  },
};

export const ManyItems: Story = {
  args: {
    inventoryItems: [
      ...sampleItems,
      { id: "7", name: "Saw", stackCount: 1, icon: "https://via.placeholder.com/48/2F4F4F/FFFFFF?text=Sw" },
      { id: "8", name: "Drill", stackCount: 1, icon: "https://via.placeholder.com/48/006400/FFFFFF?text=D" },
      { id: "9", name: "Ladder", stackCount: 2, icon: "https://via.placeholder.com/48/CD853F/FFFFFF?text=L" },
      { id: "10", name: "Tarp", stackCount: 5, icon: "https://via.placeholder.com/48/4169E1/FFFFFF?text=T" },
      { id: "11", name: "Brackets", stackCount: 16, icon: "https://via.placeholder.com/48/708090/FFFFFF?text=B" },
      { id: "12", name: "Glue", stackCount: 8, icon: "https://via.placeholder.com/48/FFD700/FFFFFF?text=G" },
    ],
  },
};
