/**
 * Theme colors mapped from provided CSS variables for light/dark modes.
 * Compatible with Expo useColorScheme, react-navigation ThemeProvider, and ThemedView/ThemedText.
 */

import { Platform } from "react-native";

const tintColorLight = "#3b5bff";
const tintColorDark = "#4c6fff";

export const Colors = {
  light: {
    text: "#1c1e21", // --text-primary
    textSecondary: "#6c757d", // --text-secondary
    background: "#f5f6fa", // --bg-primary (page bg)
    bgSecondary: "#ffffff", // --bg-secondary (cards)
    tint: "#3b5bff", // --primary (main accent)
    primaryHover: "#2f49d1", // --primary-hover
    icon: "#6c757d", // --text-secondary for icons
    tabIconDefault: "#6c757d",
    tabIconSelected: "#3b5bff",
    borderColor: "#e5e7eb", // --border-color
    cardShadow: [
      // --card-shadow RN shadow array [offsetX, offsetY, blurRadius, opacity, ...]
      0,
      1, 2, 0.04, 0, 6, 16, 0.06,
    ],
  },
  dark: {
    text: "#f1f3f5", // --text-primary
    textSecondary: "#9aa0a6", // --text-secondary
    background: "#121317", // --bg-primary
    bgSecondary: "#1f2126", // --bg-secondary
    tint: "#4c6fff", // --primary
    primaryHover: "#5c7cff", // --primary-hover
    icon: "#9aa0a6",
    tabIconDefault: "#9aa0a6",
    tabIconSelected: "#4c6fff",
    borderColor: "#2b2f36", // --border-color
    cardShadow: [
      // --card-shadow
      0, 4, 14, 0.45,
    ],
  },
};

export const Fonts = Platform.select({
  ios: {
    /** iOS `UIFontDescriptorSystemDesignDefault` */
    sans: "system-ui",
    /** iOS `UIFontDescriptorSystemDesignSerif` */
    serif: "ui-serif",
    /** iOS `UIFontDescriptorSystemDesignRounded` */
    rounded: "ui-rounded",
    /** iOS `UIFontDescriptorSystemDesignMonospaced` */
    mono: "ui-monospace",
  },
  default: {
    sans: "normal",
    serif: "serif",
    rounded: "normal",
    mono: "monospace",
  },
  web: {
    sans: "system-ui, -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, Helvetica, Arial, sans-serif",
    serif: "Georgia, 'Times New Roman', serif",
    rounded:
      "'SF Pro Rounded', 'Hiragino Maru Gothic ProN', Meiryo, 'MS PGothic', sans-serif",
    mono: "SFMono-Regular, Menlo, Monaco, Consolas, 'Liberation Mono', 'Courier New', monospace",
  },
});
