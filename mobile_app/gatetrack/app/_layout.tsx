import {
  DarkTheme,
  DefaultTheme,
  ThemeProvider,
} from "@react-navigation/native";
import { Stack } from "expo-router";
import { StatusBar } from "expo-status-bar";
import { View, ActivityIndicator } from "react-native";
import "react-native-reanimated";

import { useColorScheme } from "@/hooks/use-color-scheme";
import { useAuth } from "@/hooks/useAuth";
import AsyncStorage from "@react-native-async-storage/async-storage";
import { useEffect } from "react";
import { router } from "expo-router";

export const unstable_settings = {
  anchor: "(tabs)",
};

export default function RootLayout() {
  const colorScheme = useColorScheme();
  const { isLoading, isAuthenticated } = useAuth();

  useEffect(() => {
    if (!isLoading) {
      console.log("Layout ready, isAuthenticated:", isAuthenticated);
      if (!isAuthenticated) {
        // Only navigate after loading done to avoid conflicts
        const configJson = AsyncStorage.getItem("serverConfig");
        if (!configJson) {
          router.replace("/setup");
        } else {
          router.replace("/login");
        }
      }
    }
  }, [isLoading, isAuthenticated]);

  if (isLoading) {
    return (
      <View
        style={{
          flex: 1,
          justifyContent: "center",
          alignItems: "center",
          backgroundColor: colorScheme === "dark" ? "#000" : "#fff",
        }}
      >
        <ActivityIndicator
          size="large"
          color={colorScheme === "dark" ? "#fff" : "#000"}
        />
      </View>
    );
  }

  return (
    <ThemeProvider value={colorScheme === "dark" ? DarkTheme : DefaultTheme}>
      <Stack screenOptions={{ headerShown: false }}>
        <Stack.Screen name="setup" />
        <Stack.Screen name="login" options={{ headerShown: false }} />
        <Stack.Screen name="(tabs)" />
        <Stack.Screen
          name="modal"
          options={{ presentation: "modal", title: "Modal" }}
        />
      </Stack>
      <StatusBar style="auto" />
    </ThemeProvider>
  );
}
