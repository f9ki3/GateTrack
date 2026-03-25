import React, { useState, useEffect } from "react";
import {
  Alert,
  ActivityIndicator,
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

// --- Sub-components ---

const Input = ({
  label,
  value,
  onChangeText,
  isPassword = false,
  placeholder,
  colors,
  inputBgColor,
}) => {
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
  isLoading,
  colors,
  colorScheme,
}) => (
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
      disabled={isLoading}
      activeOpacity={0.9}
    >
      {isLoading ? (
        <ActivityIndicator size="small" color={colors.text} />
      ) : (
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
      )}
    </TouchableOpacity>
  </View>
);

// --- Main Screen ---

export default function SettingsScreen() {
  const insets = useSafeAreaInsets();
  const colorScheme = useColorScheme();
  const colors = Colors[colorScheme ?? "light"];
  const inputBgColor = colors.bgSecondary || "rgba(150, 150, 150, 0.1)";

  const [firstName, setFirstName] = useState("");
  const [lastName, setLastName] = useState("");
  const [email, setEmail] = useState("");
  const [contact, setContact] = useState("");
  const [oldPassword, setOldPassword] = useState("");
  const [newPassword, setNewPassword] = useState("");
  const [confirmPassword, setConfirmPassword] = useState("");
  const [serverUrl, setServerUrl] = useState("http://localhost:5000");
  const [token, setToken] = useState("");
  const [userData, setUserData] = useState(null);

  const [isReady, setIsReady] = useState(false);
  const [profileLoading, setProfileLoading] = useState(false);
  const [passwordLoading, setPasswordLoading] = useState(false);

  useEffect(() => {
    const loadAuthData = async () => {
      try {
        const tokenData = await AsyncStorage.getItem("@gatetrack:token");
        const userJson = await AsyncStorage.getItem("@gatetrack:user");
        const configJson = await AsyncStorage.getItem("serverConfig");

        if (tokenData) setToken(tokenData);
        if (userJson) {
          const user = JSON.parse(userJson);
          setUserData(user);
          setFirstName(user.first_name || "");
          setLastName(user.last_name || "");
          setEmail(user.email || "");
          setContact(user.contact || "");
        }
        if (configJson) {
          const config = JSON.parse(configJson);
          if (config.serverUrl) setServerUrl(config.serverUrl);
        }
      } catch (error) {
        console.error("Failed to load auth data:", error);
      } finally {
        setIsReady(true);
      }
    };

    loadAuthData();
  }, []);

  useEffect(() => {
    const fetchProfile = async () => {
      if (!token || !serverUrl) return;
      try {
        const response = await fetch(`${serverUrl}/api/v1/mobile/profile`, {
          method: "GET",
          headers: {
            Authorization: `Bearer ${token}`,
          },
        });
        if (response.ok) {
          const profile = await response.json();
          setFirstName(profile.user.first_name || "");
          setLastName(profile.user.last_name || "");
          setEmail(profile.user.email || "");
          setContact(profile.user.contact || "");
          setUserData(profile.user);
        }
      } catch (error) {
        console.error("Profile fetch error:", error);
      }
    };

    if (isReady && token && serverUrl) {
      fetchProfile();
    }
  }, [isReady, token, serverUrl]);

  const saveProfile = async () => {
    const trimmedFirst = (firstName || "").trim();
    const trimmedLast = (lastName || "").trim();
    const trimmedEmail = (email || "").trim();
    const trimmedContact = (contact || "").trim();

    if (!trimmedFirst || !trimmedLast || !trimmedEmail) {
      Alert.alert("Error", "First name, last name, and email are required");
      return;
    }

    if (!trimmedEmail.includes("@") || !trimmedEmail.includes(".")) {
      Alert.alert("Error", "Please enter a valid email address");
      return;
    }

    if (
      trimmedContact &&
      trimmedContact.length > 0 &&
      trimmedContact.length < 5
    ) {
      Alert.alert("Error", "Contact number is too short (min 5 characters)");
      return;
    }

    setProfileLoading(true);
    try {
      const response = await fetch(`${serverUrl}/api/v1/mobile/profile`, {
        method: "PUT",
        headers: {
          "Content-Type": "application/json",
          Authorization: `Bearer ${token}`,
        },
        body: JSON.stringify({
          first_name: trimmedFirst,
          last_name: trimmedLast,
          email: trimmedEmail,
          contact: trimmedContact || null,
        }),
      });

      if (response.ok) {
        const data = await response.json();
        setUserData(data.user);
        await AsyncStorage.setItem(
          "@gatetrack:user",
          JSON.stringify(data.user),
        );
        Alert.alert("Success", "Profile updated successfully!");
        return;
      }

      let errorMessage = "Failed to update profile";
      try {
        const errorData = await response.json();
        if (errorData.errors && typeof errorData.errors === "object") {
          const firstErrorKey = Object.keys(errorData.errors)[0];
          errorMessage = `${firstErrorKey}: ${errorData.errors[firstErrorKey][0]}`;
        } else if (Array.isArray(errorData.detail)) {
          errorMessage = errorData.detail
            .map((err) => `${err.loc[err.loc.length - 1]}: ${err.msg}`)
            .join("\n");
        } else {
          errorMessage =
            errorData.message ||
            errorData.detail ||
            errorData.error ||
            `Server validation error (${response.status})`;
        }
      } catch (jsonError) {
        errorMessage = `Server error ${response.status}: ${response.statusText}`;
      }

      Alert.alert("Update Failed", errorMessage);
    } catch (error) {
      console.error("Profile update error:", error);
      Alert.alert(
        "Network Error",
        "Unable to connect to server. Check your connection.",
      );
    } finally {
      setProfileLoading(false);
    }
  };

  const savePassword = async () => {
    if (newPassword !== confirmPassword) {
      Alert.alert("Error", "Passwords do not match");
      return;
    }
    if (newPassword.length < 6) {
      Alert.alert("Error", "Password must be at least 6 characters");
      return;
    }
    setPasswordLoading(true);
    try {
      const response = await fetch(`${serverUrl}/api/v1/mobile/password`, {
        method: "PUT",
        headers: {
          "Content-Type": "application/json",
          Authorization: `Bearer ${token}`,
        },
        body: JSON.stringify({
          current_password: oldPassword,
          new_password: newPassword,
          confirm_password: confirmPassword,
        }),
      });
      const data = await response.json();
      if (response.ok && data.success) {
        Alert.alert("Success", "Password changed successfully");
        setOldPassword("");
        setNewPassword("");
        setConfirmPassword("");
      } else {
        let errorMessage =
          data?.message || data?.detail || "Failed to change password";
        Alert.alert("Error", errorMessage);
      }
    } catch (error) {
      console.error(error);
      Alert.alert("Error", "Network error. Could not reach server.");
    } finally {
      setPasswordLoading(false);
    }
  };

  const saveServer = async () => {
    if (!(serverUrl || "").trim()) {
      Alert.alert("Error", "Server URL is required");
      return;
    }
    try {
      await AsyncStorage.setItem("serverConfig", JSON.stringify({ serverUrl }));
      Alert.alert("Server Saved", `Updated to: ${serverUrl}`);
    } catch (error) {
      Alert.alert("Error", "Could not save server config locally.");
    }
  };

  if (!isReady) {
    return (
      <ThemedView style={styles.container}>
        <View
          style={{
            flex: 1,
            justifyContent: "center",
            alignItems: "center",
            backgroundColor:
              colorScheme === "dark"
                ? "rgba(28,28,30,0.95)"
                : "rgba(255,255,255,0.95)",
          }}
        >
          <ActivityIndicator size="large" color={colors.text} />
        </View>
      </ThemedView>
    );
  }

  return (
    <ThemedView style={styles.container}>
      <KeyboardAvoidingView
        behavior={Platform.OS === "ios" ? "padding" : "height"}
        style={{ flex: 1 }}
      >
        <View
          style={{
            paddingTop: insets.top + 10,
            paddingHorizontal: 20,
            paddingBottom: 10,
          }}
        >
          <ThemedText style={styles.title}>Settings</ThemedText>
        </View>

        <View style={{ flex: 1 }}>
          <ScrollView
            contentContainerStyle={styles.content}
            showsVerticalScrollIndicator={false}
            keyboardShouldPersistTaps="handled"
          >
            <Section
              title="Profile"
              onSave={saveProfile}
              isLoading={profileLoading}
              colors={colors}
              colorScheme={colorScheme}
            >
              <Input
                label="First Name"
                value={firstName}
                onChangeText={setFirstName}
                colors={colors}
                inputBgColor={inputBgColor}
              />
              <Input
                label="Last Name"
                value={lastName}
                onChangeText={setLastName}
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
              <Input
                label="Contact"
                value={contact}
                onChangeText={setContact}
                colors={colors}
                inputBgColor={inputBgColor}
              />
            </Section>

            <Section
              title="Security"
              onSave={savePassword}
              isLoading={passwordLoading}
              colors={colors}
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

            <TouchableOpacity
              style={[
                styles.sectionSaveBtn,
                {
                  backgroundColor: "#EF4444",
                  marginTop: 24,
                  marginBottom: 20,
                },
              ]}
              onPress={() => {
                Alert.alert("Logout", "Clear token/user and go to login?", [
                  { text: "Cancel", style: "cancel" },
                  {
                    text: "Logout",
                    style: "destructive",
                    onPress: async () => {
                      try {
                        await AsyncStorage.multiRemove([
                          "@gatetrack:token",
                          "@gatetrack:user",
                        ]);
                        router.replace("/login");
                      } catch (error) {
                        Alert.alert("Error", "Logout failed");
                      }
                    },
                  },
                ]);
              }}
              activeOpacity={0.8}
            >
              <ThemedText
                lightColor="#ffffff"
                darkColor="#ffffff"
                style={styles.sectionSaveText}
              >
                Logout
              </ThemedText>
            </TouchableOpacity>
          </ScrollView>
        </View>
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
    borderRadius: 12,
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
