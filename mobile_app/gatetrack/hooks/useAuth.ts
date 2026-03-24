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
    try {
      const response = await fetch(`${url}/api/v1/mobile/profile`, {
        method: "GET",
        headers: {
          Authorization: `Bearer ${token}`,
        },
      });
      if (response.ok) {
        const data = await response.json();
        setUser(data.user);
        await AsyncStorage.setItem(
          "@gatetrack:user",
          JSON.stringify(data.user),
        );
        return true;
      }
      return false;
    } catch (error) {
      console.error("Token validation error:", error);
      return false;
    }
  };

  const checkAuth = async () => {
    try {
      setIsLoading(true);
      const configJson = await AsyncStorage.getItem("serverConfig");
      if (!configJson) {
        setIsAuthenticated(false);
        router.replace("/setup");
        return;
      }
      const config = JSON.parse(configJson);
      setServerUrl(config.serverUrl || "");
      if (!config.serverUrl) {
        setIsAuthenticated(false);
        router.replace("/setup");
        return;
      }

      const token = await AsyncStorage.getItem("@gatetrack:token");
      if (!token) {
        setIsAuthenticated(false);
        router.replace("/login");
        return;
      }

      const valid = await validateToken(token, config.serverUrl);
      setIsAuthenticated(valid);
      if (!valid) {
        await AsyncStorage.multiRemove(["@gatetrack:token", "@gatetrack:user"]);
        router.replace("/login");
      }
    } catch (error) {
      console.error("Auth check error:", error);
      setIsAuthenticated(false);
      router.replace("/setup");
    } finally {
      setIsLoading(false);
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
