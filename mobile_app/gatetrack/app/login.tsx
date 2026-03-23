import React, { useState } from "react";
import {
  Alert,
  ActivityIndicator,
  StyleSheet,
  ScrollView,
  TouchableOpacity,
  KeyboardAvoidingView,
  Platform,
  View,
  TextInput,
} from "react-native";
import { useSafeAreaInsets } from "react-native-safe-area-context";
import { router } from "expo-router";
import AsyncStorage from "@react-native-async-storage/async-storage";
import Svg, { Path } from "react-native-svg";
import EnvelopeFill from "react-native-bootstrap-icons/icons/envelope-fill";
import KeyFill from "react-native-bootstrap-icons/icons/key-fill";
import EyeFill from "react-native-bootstrap-icons/icons/eye-fill";
import EyeSlashFill from "react-native-bootstrap-icons/icons/eye-slash-fill";
import ArrowRight from "react-native-bootstrap-icons/icons/arrow-right";
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
    justifyContent: "center",
    gap: 16,
    paddingVertical: 32,
    marginBottom: 16,
  },
  // iconCircle: removed
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
    borderWidth: 1, // Consistent 1px border
    paddingHorizontal: 16,
  },
  inputIcon: { marginRight: 12, opacity: 0.4 },
  input: {
    flex: 1,
    fontSize: 16,
    fontWeight: "500",
  },
  loginBtn: {
    flexDirection: "row",
    height: 56,
    borderRadius: 16,
    alignItems: "center",
    justifyContent: "center",
    marginTop: 10,
    gap: 8,
  },
  loginText: {
    color: "#fff",
    fontSize: 16,
    fontWeight: "700",
  },
  footerNote: {
    marginTop: 24,
    fontSize: 13,
    opacity: 0.5,
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
  isPassword,
  hidden,
  toggleHidden,
}: any) => {
  const softBorder = isDark ? "#3A3A3C" : "#D1D1D6";

  return (
    <View style={styles.inputGroup}>
      <ThemedText style={styles.label}>{label}</ThemedText>
      <View
        style={[
          styles.inputWrapper,
          { backgroundColor: colors.background, borderColor: softBorder },
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
          secureTextEntry={isPassword && hidden}
          autoCapitalize="none"
          autoCorrect={false}
        />
        {isPassword && (
          <TouchableOpacity onPress={toggleHidden} hitSlop={10}>
            <IconSymbol
              name={hidden ? "eye.slash.fill" : "eye.fill"}
              size={18}
              color={colors.tabIconDefault}
              style={{ opacity: 0.6 }}
            />
          </TouchableOpacity>
        )}
      </View>
    </View>
  );
};

// --- 3. MAIN SCREEN ---
export default function LoginScreen() {
  const insets = useSafeAreaInsets();
  const colorScheme = useColorScheme() ?? "light";
  const colors = Colors[colorScheme];
  const isDark = colorScheme === "dark";

  const [email, setEmail] = useState("");
  const [password, setPassword] = useState("");
  const [passwordHidden, setPasswordHidden] = useState(true);
  const [isLoading, setIsLoading] = useState(false);

  const handleLogin = async () => {
    if (!email.trim() || !password.trim()) {
      Alert.alert("Invalid Input", "Please enter email and password.");
      return;
    }

    const serverConfig = await AsyncStorage.getItem("serverConfig");
    if (!serverConfig) {
      Alert.alert("Setup Required", "Please configure the server URL first.");
      router.replace("/setup");
      return;
    }

    setIsLoading(true);
    try {
      const config = JSON.parse(serverConfig);
      const serverUrl = config.serverUrl;
      if (!serverUrl) {
        Alert.alert("Config Error", "Server URL not found in config.");
        return;
      }

      const response = await fetch(`${serverUrl}/api/v1/mobile/login`, {
        method: "POST",
        headers: {
          "Content-Type": "application/json",
        },
        body: JSON.stringify({
          email: email.trim().toLowerCase(),
          password: password,
        }),
      });

      const data = await response.json();

      if (response.ok && data.success && data.token) {
        await AsyncStorage.setItem("@gatetrack:token", data.token);
        if (data.user) {
          await AsyncStorage.setItem(
            "@gatetrack:user",
            JSON.stringify(data.user),
          );
        }
        router.replace("/(tabs)");
      } else {
        Alert.alert(
          "Login Failed",
          data.message || "Invalid email or password.",
        );
      }
    } catch (error) {
      console.error("Login error:", error);
      Alert.alert(
        "Network Error",
        "Unable to connect to server. Check your connection.",
      );
    } finally {
      setIsLoading(false);
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
          keyboardShouldPersistTaps="handled"
        >
          {/* Brand Header */}
          <View style={styles.header}>
            <IconSymbol name="key.fill" size={70} color={colors.tint} />
            <ThemedText type="title" style={styles.title}>
              GateTrack
            </ThemedText>
            <ThemedText style={styles.subtitle}>
              Sign in to your account to manage attendance
            </ThemedText>
          </View>

          {/* Login Form Card */}
          <View
            style={[
              styles.card,
              { backgroundColor: isDark ? "#1c1c1e" : "#f9f9f9" },
            ]}
          >
            <InputField
              label="Email Address"
              icon="envelope.fill"
              value={email}
              onChangeText={setEmail}
              placeholder="admin@gatetrack.com"
              colors={colors}
              isDark={isDark}
            />

            <InputField
              label="Password"
              icon="key.fill"
              value={password}
              onChangeText={setPassword}
              placeholder="••••••••"
              isPassword
              hidden={passwordHidden}
              toggleHidden={() => setPasswordHidden(!passwordHidden)}
              colors={colors}
              isDark={isDark}
            />

            <TouchableOpacity
              style={[styles.loginBtn, { backgroundColor: colors.tint }]}
              onPress={handleLogin}
              disabled={isLoading}
              activeOpacity={0.85}
            >
              {isLoading ? (
                <>
                  <ActivityIndicator size="small" color="#fff" />
                  <ThemedText style={styles.loginText}>
                    Signing In...
                  </ThemedText>
                </>
              ) : (
                <>
                  <ThemedText style={styles.loginText}>Sign In</ThemedText>
                  <IconSymbol name="arrow.right" size={18} color="#fff" />
                </>
              )}
            </TouchableOpacity>
          </View>

          <TouchableOpacity onPress={() => router.push("/setup")}>
            <ThemedText style={[styles.footerNote, { color: colors.tint }]}>
              Change Server Settings
            </ThemedText>
          </TouchableOpacity>
        </ScrollView>
      </KeyboardAvoidingView>
    </ThemedView>
  );
}
