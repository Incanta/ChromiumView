import type { Preview } from "@storybook/react";
import "../src/main.css";

const preview: Preview = {
  parameters: {
    backgrounds: {
      default: "dark",
      values: [
        { name: "dark", value: "#1a1a1a" },
        { name: "light", value: "#ffffff" },
        { name: "transparent", value: "transparent" },
      ],
    },
  },
};

export default preview;
