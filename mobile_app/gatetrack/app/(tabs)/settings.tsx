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
import { router } from "expo-router";
import AsyncStorage from "@react-native-async-storage/async-storage";
import { useSafeAreaInsets } from "react-native-safe-area-context";
import { IconSymbol } from "@/components/ui/icon-symbol";
import { ThemedText } from "@/components/themed-text";
import { ThemedView } from "@/components/themed-view";
import { Colors } from "@/constants/theme";
import { useColorScheme } from "@/hooks/use-color-scheme";

// --- Sub-components (outside to prevent keyboard focus bugs) ---

const Input = ({
  label,
  value,
  onChangeText,
  isPassword = false,
  placeholder,
  colors,
  inputBgColor,
}: any) => {
  const [hidden, setHidden] = useState(true);

  return (
    <View style={styles.inputContainer}>
      <ThemedText style={styles.label}>{label}</ThemedText>
      <ThemedView
        lightColor={inputBgColor}
        darkColor={inputBgColor}
        style={styles.inputWrapper}
      >
        <TextInput
          style={[styles.input, { color: colors.text }]}
          value={value}
          onChangeText={onChangeText}
          secureTextEntry={isPassword && hidden}
          placeholder={placeholder}
          placeholderTextColor={colors.textSecondary}
          autoCapitalize="none"
          autoCorrect={false}
          textContentType={isPassword ? "oneTimeCode" : "none"}
          autoComplete={isPassword ? "off" : undefined}
          importantForAutofill={isPassword ? "no" : "auto"}
        />
        {isPassword && (
          <TouchableOpacity
            onPress={() => setHidden(!hidden)}
            hitSlop={{ top: 10, bottom: 10, left: 10, right: 10 }}
          >
            <IconSymbol
              name={hidden ? "eye.slash.fill" : "eye.fill"}
              size={18}
              color={colors.textSecondary}
            />
          </TouchableOpacity>
        )}
      </ThemedView>
    </View>
  );
};

const Section = ({
  title,
  children,
  onSave,
  colors,
  primaryColor,
  colorScheme,
}: any) => (
  <View style={styles.sectionContainer}>
    <ThemedText
      style={styles.sectionTitle}
      lightColor={colors.textSecondary}
      darkColor={colors.textSecondary}
    >
      {title}
    </ThemedText>
    <View style={styles.inputsContainer}>{children}</View>
    <TouchableOpacity
      style={[
        styles.sectionSaveBtn,
        {
          backgroundColor:
            colors.card || (colorScheme === "dark" ? "#2A2A2E" : "#FFFFFF"),
          borderWidth: 1,
          borderColor:
            colors.border || (colorScheme === "dark" ? "#3A3A3E" : "#E5E5E7"),
        },
      ]}
      onPress={onSave}
      activeOpacity={0.9}
    >
      <ThemedText
        lightColor={
          colors.text || (colorScheme === "dark" ? "#A0A0A5" : "#6B7280")
        }
        darkColor={
          colors.text || (colorScheme === "dark" ? "#A0A0A5" : "#6B7280")
        }
        style={[styles.sectionSaveText, { fontWeight: "700" }]}
      >
        Save {title}
      </ThemedText>
    </TouchableOpacity>
  </View>
);

// --- Main Screen ---

export default function SettingsScreen() {
  const insets = useSafeAreaInsets();
  const colorScheme = useColorScheme();
  const colors = Colors[colorScheme ?? "light"];
  const primaryColor = colors.tint;
  const inputBgColor = colors.bgSecondary || "rgba(150, 150, 150, 0.1)";

  const [name, setName] = useState("John Doe");
  const [email, setEmail] = useState("john@example.com");
  const [oldPassword, setOldPassword] = useState("");
  const [newPassword, setNewPassword] = useState("");
  const [confirmPassword, setConfirmPassword] = useState("");
  const [serverUrl, setServerUrl] = useState("http://localhost:5000");

  const saveProfile = () => {
    if (!name.trim() || !email.trim()) {
      Alert.alert("Error", "Name and email are required");
      return;
    }
    Alert.alert("Profile Saved", `Updated: ${name}`);
  };

  const savePassword = () => {
    if (newPassword !== confirmPassword) {
      Alert.alert("Error", "Passwords do not match");
      return;
    }
    if (newPassword.length < 6) {
      Alert.alert("Error", "Password must be at least 6 characters");
      return;
    }
    Alert.alert("Password Changed", "Success");
  };

  const saveServer = () => {
    if (!serverUrl.trim()) return;
    Alert.alert("Server Saved", `Updated to: ${serverUrl}`);
  };

  return (
    <ThemedView style={styles.container}>
      <KeyboardAvoidingView
        behavior={Platform.OS === "ios" ? "padding" : "height"}
        style={{ flex: 1 }}
      >
        {/* Fixed Header Height Issue */}
        <View
          style={{
            paddingTop: insets.top + 10,
            paddingHorizontal: 20,
            paddingBottom: 10,
          }}
        >
          <ThemedText style={styles.title}>Settings</ThemedText>
        </View>

        <ScrollView
          contentContainerStyle={styles.content}
          showsVerticalScrollIndicator={false}
          keyboardShouldPersistTaps="handled"
        >
          <Section
            title="Profile"
            onSave={saveProfile}
            colors={colors}
            primaryColor={primaryColor}
            colorScheme={colorScheme}
          >
            <Input
              label="Full Name"
              value={name}
              onChangeText={setName}
              colors={colors}
              inputBgColor={inputBgColor}
            />
            <Input
              label="Email Address"
              value={email}
              onChangeText={setEmail}
              colors={colors}
              inputBgColor={inputBgColor}
            />
          </Section>

          <Section
            title="Security"
            onSave={savePassword}
            colors={colors}
            primaryColor={primaryColor}
            colorScheme={colorScheme}
          >
            <Input
              label="Current Password"
              value={oldPassword}
              onChangeText={setOldPassword}
              isPassword
              colors={colors}
              inputBgColor={inputBgColor}
            />
            <Input
              label="New Password"
              value={newPassword}
              onChangeText={setNewPassword}
              isPassword
              colors={colors}
              inputBgColor={inputBgColor}
            />
            <Input
              label="Confirm Password"
              value={confirmPassword}
              onChangeText={setConfirmPassword}
              isPassword
              colors={colors}
              inputBgColor={inputBgColor}
            />
          </Section>

          <Section
            title="Network"
            onSave={saveServer}
            colors={colors}
            primaryColor={primaryColor}
            colorScheme={colorScheme}
          >
            <Input
              label="Server URL"
              value={serverUrl}
              onChangeText={setServerUrl}
              colors={colors}
              inputBgColor={inputBgColor}
            />
          </Section>
        </ScrollView>

        <TouchableOpacity
          style={[
            styles.sectionSaveBtn,
            {
              backgroundColor: "#EF4444",
              marginTop: 24,
              marginHorizontal: 20,
              marginBottom: 20,
            },
          ]}
          onPress={async () => {
            Alert.alert(
              "Logout",
              "This will clear server config and restart setup flow. Continue?",
              [
                { text: "Cancel", style: "cancel" },
                {
                  text: "Logout",
                  style: "destructive",
                  onPress: async () => {
                    try {
                      await AsyncStorage.removeItem("serverConfig");
                      Alert.alert("Logged out", "Restarting setup...");
                      router.replace("/setup");
                    } catch (error) {
                      Alert.alert("Error", "Logout failed");
                    }
                  },
                },
              ],
            );
          }}
          activeOpacity={0.8}
        >
          <ThemedText
            lightColor="#ffffff"
            darkColor="#ffffff"
            style={styles.sectionSaveText}
          >
            Logout (Restart Flow)
          </ThemedText>
        </TouchableOpacity>
      </KeyboardAvoidingView>
    </ThemedView>
  );
}

const styles = StyleSheet.create({
  container: {
    flex: 1,
  },
  title: {
    fontSize: 25,
    fontWeight: "800",
    letterSpacing: -0.5,
    textAlign: "center",
  },
  content: {
    paddingHorizontal: 20,
    paddingTop: 10,
    paddingBottom: 60,
  },
  sectionContainer: {
    marginBottom: 32,
  },
  sectionTitle: {
    fontSize: 12,
    fontWeight: "700",
    textTransform: "uppercase",
    letterSpacing: 1,
    marginBottom: 12,
    opacity: 0.6,
  },
  inputsContainer: {
    gap: 12,
  },
  inputContainer: {
    gap: 6,
  },
  label: {
    fontSize: 13,
    fontWeight: "600",
    marginLeft: 2,
  },
  inputWrapper: {
    flexDirection: "row",
    alignItems: "center",
    borderRadius: 12, // Little round, more square
    paddingHorizontal: 14,
    height: 50,
  },
  input: {
    flex: 1,
    fontSize: 15,
    height: "100%",
  },
  sectionSaveBtn: {
    height: 48,
    borderRadius: 12,
    alignItems: "center",
    justifyContent: "center",
    marginTop: 16,
  },
  sectionSaveText: {
    fontSize: 15,
    fontWeight: "700",
  },
});
