import React, { useState } from "react";
import {
  Alert,
  StyleSheet,
  ScrollView,
  TextInput,
  View,
  TouchableOpacity,
  KeyboardAvoidingView,
  Platform,
} from "react-native";
import { useSafeAreaInsets } from "react-native-safe-area-context";
import { router } from "expo-router";
import AsyncStorage from "@react-native-async-storage/async-storage";
import { IconSymbol } from "@/components/ui/icon-symbol";
import { ThemedText } from "@/components/themed-text";
import { ThemedView } from "@/components/themed-view";
import { Colors } from "@/constants/theme";
import { useColorScheme } from "@/hooks/use-color-scheme";

// --- 1. STYLES DEFINED AT TOP ---
const styles = StyleSheet.create({
  container: { flex: 1 },
  scrollContent: {
    paddingHorizontal: 24,
    paddingBottom: 40,
    alignItems: "center",
  },
  header: {
    alignItems: "center",
    marginBottom: 32,
  },
  iconCircle: {
    width: 80,
    height: 80,
    borderRadius: 40,
    justifyContent: "center",
    alignItems: "center",
    marginBottom: 20,
  },
  title: {
    fontSize: 28,
    fontWeight: "800",
    marginBottom: 8,
  },
  subtitle: {
    fontSize: 15,
    opacity: 0.6,
    textAlign: "center",
    lineHeight: 22,
    paddingHorizontal: 20,
  },
  card: {
    width: "100%",
    borderRadius: 24,
    padding: 24,
    gap: 20,
    shadowColor: "#000",
    shadowOffset: { width: 0, height: 10 },
    shadowOpacity: 0.05,
    shadowRadius: 20,
    elevation: 2,
  },
  inputGroup: { gap: 8 },
  label: {
    fontSize: 14,
    fontWeight: "600",
    marginLeft: 4,
    opacity: 0.7,
  },
  inputWrapper: {
    flexDirection: "row",
    alignItems: "center",
    height: 56,
    borderRadius: 16,
    borderWidth: 1, // Decreased to 1px as requested
    paddingHorizontal: 16,
  },
  inputIcon: { marginRight: 12, opacity: 0.4 },
  input: {
    flex: 1,
    fontSize: 16,
    fontWeight: "500",
  },
  primaryButton: {
    flexDirection: "row",
    height: 56,
    borderRadius: 16,
    alignItems: "center",
    justifyContent: "center",
    marginTop: 10,
    gap: 8,
  },
  buttonText: {
    color: "#fff",
    fontSize: 16,
    fontWeight: "700",
  },
  footerNote: {
    marginTop: 24,
    fontSize: 12,
    opacity: 0.4,
    textAlign: "center",
  },
});

// --- 2. SUB-COMPONENTS ---
const InputField = ({
  label,
  value,
  onChangeText,
  placeholder,
  icon,
  colors,
  isDark,
}: any) => {
  // Soft gray border colors for a subtle modern look
  const softBorder = isDark ? "#3A3A3C" : "#D1D1D6";

  return (
    <View style={styles.inputGroup}>
      <ThemedText style={styles.label}>{label}</ThemedText>
      <View
        style={[
          styles.inputWrapper,
          {
            backgroundColor: colors.background,
            borderColor: softBorder,
          },
        ]}
      >
        <IconSymbol
          name={icon}
          size={20}
          color={colors.tabIconDefault}
          style={styles.inputIcon}
        />
        <TextInput
          style={[styles.input, { color: colors.text }]}
          value={value}
          onChangeText={onChangeText}
          placeholder={placeholder}
          placeholderTextColor="#A1A1AA"
          autoCapitalize="none"
          autoCorrect={false}
          keyboardType={label.includes("Port") ? "numeric" : "default"}
        />
      </View>
    </View>
  );
};

// --- 3. MAIN SCREEN ---
export default function SetupScreen() {
  const insets = useSafeAreaInsets();
  const colorScheme = useColorScheme() ?? "light";
  const colors = Colors[colorScheme];
  const isDark = colorScheme === "dark";

  const [host, setHost] = useState("localhost");
  const [port, setPort] = useState("5000");

  const saveServer = async () => {
    if (!host.trim() || !port.trim()) {
      Alert.alert("Required", "Please enter both host and port.");
      return;
    }
    const serverUrl = `http://${host.trim()}:${port.trim()}`;
    try {
      await AsyncStorage.setItem("serverConfig", JSON.stringify({ serverUrl }));
      router.replace("/login");
    } catch (error) {
      Alert.alert("Error", "Could not save settings.");
    }
  };

  return (
    <ThemedView style={styles.container}>
      <KeyboardAvoidingView
        behavior={Platform.OS === "ios" ? "padding" : "height"}
        style={{ flex: 1 }}
      >
        <ScrollView
          contentContainerStyle={[
            styles.scrollContent,
            { paddingTop: insets.top + 40 },
          ]}
        >
          {/* Header */}
          <View style={styles.header}>
            <View
              style={[
                styles.iconCircle,
                { backgroundColor: colors.tint + "15" },
              ]}
            >
              <IconSymbol name="server.rack" size={32} color={colors.tint} />
            </View>
            <ThemedText type="title" style={styles.title}>
              Server Setup
            </ThemedText>
            <ThemedText style={styles.subtitle}>
              Connect to your backend instance to get started.
            </ThemedText>
          </View>

          {/* Card */}
          <View
            style={[
              styles.card,
              { backgroundColor: isDark ? "#1c1c1e" : "#f9f9f9" },
            ]}
          >
            <InputField
              label="Host Address"
              icon="globe"
              value={host}
              onChangeText={setHost}
              placeholder="192.168.1.1"
              colors={colors}
              isDark={isDark}
            />
            <InputField
              label="Port Number"
              icon="number"
              value={port}
              onChangeText={setPort}
              placeholder="5000"
              colors={colors}
              isDark={isDark}
            />

            <TouchableOpacity
              style={[styles.primaryButton, { backgroundColor: colors.tint }]}
              onPress={saveServer}
              activeOpacity={0.85}
            >
              <ThemedText style={styles.buttonText}>Connect Server</ThemedText>
              <IconSymbol name="chevron.right" size={18} color="#fff" />
            </TouchableOpacity>
          </View>

          <ThemedText style={styles.footerNote}>
            Defaulting to HTTP. Ensure your server is reachable.
          </ThemedText>
        </ScrollView>
      </KeyboardAvoidingView>
    </ThemedView>
  );
}
