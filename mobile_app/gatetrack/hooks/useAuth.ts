import { useState, useEffect } from "react";
import AsyncStorage from "@react-native-async-storage/async-storage";
import { router } from "expo-router";
import { Alert } from "react-native";

interface User {
  id: number;
  first_name: string;
  last_name: string;
  email: string;
  contact?: string;
  // Add other user fields as needed
}

export function useAuth() {
  const [isAuthenticated, setIsAuthenticated] = useState(false);
  const [isLoading, setIsLoading] = useState(true);
  const [user, setUser] = useState<User | null>(null);
  const [serverUrl, setServerUrl] = useState("");

  const validateToken = async (
    token: string,
    url: string,
  ): Promise<boolean> => {
    const controller = new AbortController();
    const timeoutId = setTimeout(() => controller.abort(), 5000); // 5s timeout

    try {
      console.log("Validating token against:", url);
      const response = await fetch(`${url}/api/v1/mobile/profile`, {
        method: "GET",
        headers: {
          Authorization: `Bearer ${token}`,
        },
        signal: controller.signal,
      });
      clearTimeout(timeoutId);

      if (response.ok) {
        const data = await response.json();
        setUser(data.user);
        await AsyncStorage.setItem(
          "@gatetrack:user",
          JSON.stringify(data.user),
        );
        console.log("Token valid, user:", data.user);
        return true;
      }
      console.log("Token invalid, status:", response.status);
      return false;
    } catch (error: any) {
      clearTimeout(timeoutId);
      console.error("Token validation error:", error.message);
      return false;
    }
  };

  const checkAuth = async () => {
    try {
      setIsLoading(true);
      console.log("Checking auth...");

      const configJson = await AsyncStorage.getItem("serverConfig");
      if (!configJson) {
        console.log("No serverConfig, redirect to setup");
        setIsAuthenticated(false);
        setIsLoading(false);
        return; // Don't navigate here, let layout handle
      }

      const config = JSON.parse(configJson);
      setServerUrl(config.serverUrl || "");
      if (!config.serverUrl) {
        console.log("No serverUrl in config");
        setIsAuthenticated(false);
        setIsLoading(false);
        return;
      }

      const token = await AsyncStorage.getItem("@gatetrack:token");
      if (!token) {
        console.log("No token");
        setIsAuthenticated(false);
        setIsLoading(false);
        return;
      }

      const valid = await validateToken(token, config.serverUrl);
      setIsAuthenticated(valid);
      if (!valid) {
        console.log("Invalid token");
        await AsyncStorage.multiRemove(["@gatetrack:token", "@gatetrack:user"]);
      }
    } catch (error) {
      console.error("Auth check error:", error);
      setIsAuthenticated(false);
    } finally {
      setIsLoading(false);
      console.log("Auth check complete, isAuthenticated:", isAuthenticated);
    }
  };

  const logout = async () => {
    try {
      await AsyncStorage.multiRemove(["@gatetrack:token", "@gatetrack:user"]);
      setIsAuthenticated(false);
      setUser(null);
      router.replace("/login");
    } catch (error) {
      Alert.alert("Error", "Logout failed");
    }
  };

  useEffect(() => {
    checkAuth();
  }, []);

  return {
    isAuthenticated,
    isLoading,
    user,
    serverUrl,
    validateAuth: checkAuth,
    logout,
  };
}
