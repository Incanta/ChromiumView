import React from "react";
import { motion, AnimatePresence } from "motion/react";

/**
 * Props for an individual inventory item
 */
export interface InventoryItemProps {
  /** URL or path to the item icon */
  icon?: string;
  /** Number of items in the stack */
  stackCount: number;
  /** Display name of the item */
  itemName: string;
  /** Optional click handler */
  onClick?: () => void;
}

/**
 * Individual inventory item component
 * Displays an icon with stack count and item name on hover
 */
export function InventoryItem({ icon, stackCount, itemName, onClick }: InventoryItemProps) {
  const [isHovered, setIsHovered] = React.useState(false);

  return (
    <motion.div
      className="
        relative
        w-16
        h-16
        bg-neutral-800/80
        border
        border-neutral-500/60
        rounded-lg
        cursor-pointer
        transition-all
        duration-200
        flex
        items-center
        justify-center
      "
      onClick={onClick}
      onMouseEnter={() => setIsHovered(true)}
      onMouseLeave={() => setIsHovered(false)}
      whileHover={{
        rotate: [0, -30, 30, -30, 30, 0],
        transition: {
          duration: 0.4,
          repeatDelay: 0.1,
        },
      }}
      whileTap={{ scale: 0.95 }}
    >
      <div className="relative w-12 h-12">
        {icon && (
          <img
            src={icon}
            alt={itemName}
            className="w-full h-full object-contain"
          />
        )}
        {stackCount > 1 && (
          <span className="absolute -bottom-1 -right-1 bg-black/80 text-white text-xs font-bold px-1.5 py-0.5 rounded min-w-4 text-center">
            {stackCount}
          </span>
        )}
      </div>
      <AnimatePresence>
        {isHovered && (
          <motion.div
            className="absolute top-1 left-1/2 -translate-x-1/2 bg-neutral-900/90 text-white text-[10px] px-1 py-0.5 rounded whitespace-nowrap z-[100] max-w-14 overflow-hidden text-ellipsis text-center"
            initial={{ opacity: 0, y: 4 }}
            animate={{ opacity: 1, y: 0 }}
            exit={{ opacity: 0, y: 4 }}
            transition={{ duration: 0.15 }}
          >
            {itemName}
          </motion.div>
        )}
      </AnimatePresence>
    </motion.div>
  );
}
