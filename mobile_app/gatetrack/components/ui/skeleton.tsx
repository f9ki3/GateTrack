import React from "react";
import { useColorScheme } from "@/hooks/use-color-scheme";
import { Colors } from "@/constants/theme";
import { View, StyleSheet, Dimensions } from "react-native";
import { LinearGradient } from "expo-linear-gradient";
import Animated, {
  useSharedValue,
  useAnimatedStyle,
  withRepeat,
  withTiming,
} from "react-native-reanimated";

const { width: screenWidth } = Dimensions.get("window");

interface SkeletonProps {
  width?: number;
  height: number;
  radius?: number;
}

export function Skeleton({
  width = screenWidth,
  height = 16,
  radius = 8,
}: SkeletonProps) {
  const shimmerPosition = useSharedValue(-screenWidth);

  const animatedStyle = useAnimatedStyle(() => {
    return {
      transform: [{ translateX: shimmerPosition.value }],
    };
  });

  const colorScheme = useColorScheme() ?? "light";
  const bgColor = Colors[colorScheme].bgSecondary;
  const shimmerColor =
    colorScheme === "light" ? "rgba(255,255,255,0.4)" : `rgba(145,160,166,0.4)`; // approx Colors.dark.textSecondary alpha

  React.useEffect(() => {
    shimmerPosition.value = withRepeat(
      withTiming(screenWidth * 2, { duration: 1500 }),
      -1,
      false,
    );
  }, []);

  return (
    <View
      style={[
        [styles.base, { backgroundColor: bgColor }],
        {
          width,
          height,
          borderRadius: radius,
        },
      ]}
    >
      <LinearGradient
        colors={["transparent", shimmerColor, "transparent"]}
        style={[StyleSheet.absoluteFill, animatedStyle]}
        start={{ x: 0, y: 0 }}
        end={{ x: 1, y: 0 }}
      />
    </View>
  );
}

const styles = StyleSheet.create({
  base: {
    overflow: "hidden",
  },
});

export function SkeletonTableRow() {
  return (
    <View style={stylesRow.row}>
      <View style={stylesRow.col1}>
        <Skeleton width={80} height={14} />
      </View>
      <View style={stylesRow.col2}>
        <Skeleton width={60} height={14} />
      </View>
      <View style={stylesRow.col3}>
        <Skeleton width={60} height={14} />
      </View>
    </View>
  );
}

const stylesRow = StyleSheet.create({
  row: {
    flexDirection: "row",
    paddingVertical: 18,
    paddingHorizontal: 20,
    alignItems: "center",
    gap: 8,
  },
  col1: { flex: 1.3 },
  col2: { flex: 1, alignItems: "center" },
  col3: { flex: 1, alignItems: "flex-end" },
});

export function SkeletonFormInput() {
  return (
    <View style={stylesInput.inputSkeleton}>
      <Skeleton height={12} width={60} radius={4} />
      <Skeleton height={50} radius={12} />
    </View>
  );
}

const stylesInput = StyleSheet.create({
  inputSkeleton: {
    gap: 6,
  },
});

export function SkeletonHeader() {
  return (
    <View style={stylesHeader.headerSkeleton}>
      <Skeleton width={40} height={12} radius={4} />
      <Skeleton width={40} height={12} radius={4} />
      <Skeleton width={40} height={12} radius={4} />
    </View>
  );
}

const stylesHeader = StyleSheet.create({
  headerSkeleton: {
    flexDirection: "row",
    paddingVertical: 16,
    paddingHorizontal: 20,
    gap: 20,
  },
});
