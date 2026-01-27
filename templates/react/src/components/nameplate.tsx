import { motion } from "motion/react";

/**
 * Props for the Nameplate component
 */
export interface NameplateProps {
  /** The name/text to display */
  name: string;
  /** Optional background color (default: semi-transparent dark) */
  backgroundColor?: string;
  /** Optional text color (default: white) */
  textColor?: string;
  /** Optional border color (default: semi-transparent neutral) */
  borderColor?: string;
  /** Optional font size class (default: text-sm) */
  fontSize?: "text-xs" | "text-sm" | "text-base" | "text-lg" | "text-xl";
}

/**
 * Nameplate component
 * Displays a name/string in a compact rounded-corner box
 * Auto-sizes to fit the content
 */
export function Nameplate({
  name,
  backgroundColor = "bg-neutral-900/80",
  textColor = "text-white",
  borderColor = "border-neutral-500/60",
  fontSize = "text-sm",
}: NameplateProps) {
  return (
    <motion.div
      className={`
        inline-flex
        items-center
        justify-center
        px-3
        py-1.5
        ${backgroundColor}
        border
        ${borderColor}
        rounded-lg
        shadow-lg
      `}
      initial={{ opacity: 0, y: -10 }}
      animate={{ opacity: 1, y: 0 }}
      transition={{ duration: 0.2 }}
    >
      <span
        className={`
          ${fontSize}
          ${textColor}
          font-medium
          whitespace-nowrap
          drop-shadow-sm
        `}
      >
        {name}
      </span>
    </motion.div>
  );
}
