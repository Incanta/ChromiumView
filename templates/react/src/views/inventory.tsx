import { motion } from "motion/react";
import { useField, useSendEvent } from "@unreal/chromium-view-react";
import { createApp } from "../utils/create-app";
import { InventoryItem } from "../components";
import { InventoryItemData } from "../types/view-model";

/**
 * Props for the InventoryView presentational component
 */
export interface InventoryViewProps {
  inventoryItems: InventoryItemData[];
  onDismiss?: () => void;
  onItemClick?: (item: InventoryItemData) => void;
}

/**
 * Presentational Inventory View component
 * Displays player inventory as a grid of items with a dismiss button
 */
export function InventoryView({ inventoryItems, onDismiss, onItemClick }: InventoryViewProps) {
  return (
    <motion.div
      className="fixed inset-0 w-screen h-screen flex items-center justify-center bg-black/50"
      initial={{ opacity: 0 }}
      animate={{ opacity: 1 }}
      transition={{ duration: 0.3 }}
    >
      <motion.div
        className="w-[600px] max-w-[90vw] max-h-[80vh] bg-neutral-900/95 border-2 border-neutral-500/60 rounded-2xl flex flex-col overflow-hidden shadow-2xl"
        initial={{ opacity: 0, scale: 0.8 }}
        animate={{ opacity: 1, scale: 1 }}
        transition={{
          duration: 0.4,
          scale: {
            type: "spring",
            damping: 12,
            stiffness: 200,
          },
        }}
      >
        {/* Header with title and dismiss button */}
        <div className="flex justify-between items-center px-5 py-4 border-b border-neutral-500/40 bg-neutral-800/80">
          <span className="text-2xl font-bold text-white drop-shadow-md">Inventory 1</span>
          <button
            className="w-8 h-8 rounded-lg border-none bg-neutral-700/80 text-white text-base cursor-pointer flex items-center justify-center transition-colors duration-200 hover:bg-red-500/80"
            onClick={onDismiss}
          >
            ✕
          </button>
        </div>

        {/* Scrollable items container */}
        <div className="flex-1 p-4 overflow-y-auto overflow-x-hidden">
          <div className="flex flex-row flex-wrap gap-3 content-start">
            {inventoryItems.map((item) => (
              <InventoryItem
                key={item.id}
                icon={item.icon}
                stackCount={item.stackCount}
                itemName={item.name}
                onClick={() => onItemClick?.(item)}
              />
            ))}
          </div>
        </div>
      </motion.div>
    </motion.div>
  );
}

/**
 * Connected Inventory component with Unreal hooks
 */
function Inventory() {
  const inventoryItems = useField<InventoryItemData[]>("InventoryItems", {
    initialValue: [],
  });
  const isVisible = useField<boolean>("InventoryVisible", {
    initialValue: true,
  });
  const { send: sendEventCloseInventory } = useSendEvent("CloseInventory");
  const { send: sendEventSelectInventoryItem } = useSendEvent("SelectInventoryItem");

  if (!isVisible) {
    return null;
  }

  return (
    <InventoryView
      inventoryItems={inventoryItems ?? []}
      onDismiss={() => sendEventCloseInventory()}
      onItemClick={(item) => sendEventSelectInventoryItem({ itemId: item.id })}
    />
  );
}

createApp(<Inventory />, {
  loadingFallback: (
    <div className="fixed inset-0 w-screen h-screen flex items-center justify-center bg-black/50">
      Loading...
    </div>
  ),
});
