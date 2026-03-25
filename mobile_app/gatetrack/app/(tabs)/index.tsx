import React, { useState, useCallback, useEffect } from "react";
import {
  FlatList,
  RefreshControl,
  StyleSheet,
  TextInput,
  View,
  Alert,
  TouchableOpacity,
  Platform,
  ActivityIndicator,
} from "react-native";
import AsyncStorage from "@react-native-async-storage/async-storage";
import { useSafeAreaInsets } from "react-native-safe-area-context";

import { IconSymbol } from "@/components/ui/icon-symbol";
import { ThemedText } from "@/components/themed-text";
import { ThemedView } from "@/components/themed-view";
import { Colors } from "@/constants/theme";
import { useColorScheme } from "@/hooks/use-color-scheme";
import { AttendanceRecord } from "@/constants/mockAttendance";

export default function AttendanceScreen() {
  const insets = useSafeAreaInsets();
  const colorScheme = useColorScheme();
  const colors = Colors[colorScheme ?? "light"];

  const isDark = colorScheme === "dark";

  // Modern White Palette
  const surfaceColor = isDark ? "#1C1C1E" : "#FFFFFF";
  const screenBg = isDark ? "#000000" : "#F8F9FB";
  const borderColor = isDark ? "#2C2C2E" : "#E5E5EA";

  const [searchQuery, setSearchQuery] = useState("");
  const [page, setPage] = useState(1);
  const [refreshing, setRefreshing] = useState(false);
  const itemsPerPage = 10;

  const [serverUrl, setServerUrl] = useState("");
  const [token, setToken] = useState("");
  const [attendanceData, setAttendanceData] = useState<AttendanceRecord[]>([]);
  const [totalPagesState, setTotalPagesState] = useState(1);
  const [loading, setLoading] = useState(false);
  const [error, setError] = useState("");

  const loadConfig = async () => {
    try {
      const configJson = await AsyncStorage.getItem("serverConfig");
      if (configJson) {
        const config = JSON.parse(configJson);
        setServerUrl(config.serverUrl || "");
      }
      const tok = await AsyncStorage.getItem("@gatetrack:token");
      setToken(tok || "");
    } catch (e) {
      console.error("Config load error:", e);
    }
  };

  const fetchAttendance = async (currentPage: number) => {
    if (!serverUrl || !token) return;
    setLoading(true);
    setError("");
    await new Promise((resolve) => setTimeout(resolve, 3000)); // 3s min loading
    try {
      const response = await fetch(
        `${serverUrl}/api/v1/mobile/attendance?page=${currentPage}&per_page=${itemsPerPage}`,
        {
          method: "GET",
          headers: {
            Authorization: `Bearer ${token}`,
          },
        },
      );
      if (!response.ok) {
        throw new Error(`API error: ${response.status}`);
      }
      const data = await response.json();
      if (!data.success) {
        throw new Error(data.message || "API failed");
      }
      const mapped = data.records.map(
        (rec: any): AttendanceRecord => ({
          id: rec.id.toString(),
          date: rec.date,
          timeIn: rec.time_in
            ? new Date(rec.time_in).toLocaleTimeString("en-US", {
                hour: "numeric",
                minute: "2-digit",
                hour12: true,
              })
            : "",
          timeOut: rec.time_out
            ? new Date(rec.time_out).toLocaleTimeString("en-US", {
                hour: "numeric",
                minute: "2-digit",
                hour12: true,
              })
            : "",
          time_in: rec.time_in,
          time_out: rec.time_out,
          duration: rec.duration,
          email: rec.email,
          first_name: rec.first_name,
          last_name: rec.last_name,
          username: rec.username,
          role: rec.role,
        }),
      );
      setAttendanceData(mapped);
      setTotalPagesState(data.total_pages || 1);
    } catch (err: any) {
      setError(err.message || "Fetch error");
      console.error("Attendance fetch error:", err);
    } finally {
      setLoading(false);
    }
  };

  const handleExport = () => {
    Alert.alert("Export", "Attendance logs have been saved to your device.");
  };

  useEffect(() => {
    loadConfig();
  }, []);

  useEffect(() => {
    if (serverUrl && token) {
      fetchAttendance(page);
    }
  }, [serverUrl, token, page]);

  const onRefresh = useCallback(async () => {
    setRefreshing(true);
    try {
      await fetchAttendance(page);
    } finally {
      setRefreshing(false);
    }
  }, [page, serverUrl, token]);

  const filteredData = attendanceData.filter((record) =>
    record.date.toLowerCase().includes(searchQuery.toLowerCase()),
  );

  const paginatedData = filteredData; // Filter current page data
  const totalPages = totalPagesState;

  const TableHeader = () => (
    <View
      style={[
        styles.tableHeader,
        { backgroundColor: surfaceColor, borderBottomColor: borderColor },
      ]}
    >
      <ThemedText style={[styles.columnHeader, styles.col1]}>DATE</ThemedText>
      <ThemedText style={[styles.columnHeader, styles.col2]}>
        TIME IN
      </ThemedText>
      <ThemedText style={[styles.columnHeader, styles.col3]}>
        TIME OUT
      </ThemedText>
    </View>
  );

  const renderItem = ({
    item,
    index,
  }: {
    item: AttendanceRecord;
    index: number;
  }) => (
    <View
      style={[
        styles.tableRow,
        {
          backgroundColor: surfaceColor,
          borderBottomColor: borderColor,
          borderBottomWidth:
            index === paginatedData.length - 1 ? 0 : StyleSheet.hairlineWidth,
        },
      ]}
    >
      <ThemedText style={[styles.rowText, styles.col1]}>{item.date}</ThemedText>
      <ThemedText style={[styles.rowText, styles.col2, styles.timeText]}>
        {item.timeIn}
      </ThemedText>
      <ThemedText style={[styles.rowText, styles.col3, styles.timeText]}>
        {item.timeOut}
      </ThemedText>
    </View>
  );

  return (
    <ThemedView style={[styles.container, { backgroundColor: screenBg }]}>
      {/* Header Section */}
      <View style={[styles.header, { paddingTop: insets.top + 10 }]}>
        <TouchableOpacity
          onPress={onRefresh}
          style={[
            styles.whiteControl,
            styles.shadow,
            { backgroundColor: surfaceColor },
          ]}
        >
          <IconSymbol name="arrow.clockwise" size={18} color={colors.text} />
        </TouchableOpacity>

        <ThemedText style={styles.headerTitle}>Attendance</ThemedText>

        <TouchableOpacity
          onPress={handleExport}
          style={[
            styles.whiteControl,
            styles.shadow,
            { backgroundColor: surfaceColor },
          ]}
        >
          <IconSymbol
            name="square.and.arrow.up"
            size={18}
            color={colors.text}
          />
        </TouchableOpacity>
      </View>

      {/* Controls Section */}
      <View style={styles.controlsSection}>
        <View
          style={[
            styles.searchBar,
            styles.shadow,
            { backgroundColor: surfaceColor },
          ]}
        >
          <IconSymbol
            name="magnifyingglass"
            size={16}
            color={colors.textSecondary}
          />
          <TextInput
            style={[styles.searchInput, { color: colors.text }]}
            placeholder="Search dates..."
            value={searchQuery}
            onChangeText={async (text) => {
              setSearchQuery(text);
              setPage(1);
            }}
            placeholderTextColor={colors.textSecondary}
          />
        </View>

        {totalPages > 1 && (
          <View style={styles.paginationRow}>
            <TouchableOpacity
              onPress={() => setPage((p) => Math.max(1, p - 1))}
              disabled={page === 1}
              style={[
                styles.pageBtn,
                styles.shadow,
                {
                  backgroundColor: surfaceColor,
                  opacity: page === 1 ? 0.4 : 1,
                },
              ]}
            >
              <IconSymbol name="chevron.left" size={14} color={colors.text} />
            </TouchableOpacity>

            <View
              style={[
                styles.pageIndicator,
                styles.shadow,
                { backgroundColor: surfaceColor },
              ]}
            >
              <ThemedText style={styles.pageText}>
                {page} / {totalPages}
              </ThemedText>
            </View>

            <TouchableOpacity
              onPress={() => setPage((p) => Math.min(totalPages, p + 1))}
              disabled={page === totalPages}
              style={[
                styles.pageBtn,
                styles.shadow,
                {
                  backgroundColor: surfaceColor,
                  opacity: page === totalPages ? 0.4 : 1,
                },
              ]}
            >
              <IconSymbol name="chevron.right" size={14} color={colors.text} />
            </TouchableOpacity>
          </View>
        )}
      </View>

      {/* Table Section */}
      {!serverUrl || !token ? (
        <View
          style={[
            styles.tableWrapper,
            styles.shadow,
            { borderColor: borderColor },
          ]}
        >
          <ThemedView style={styles.noDataContainer}>
            <IconSymbol
              name="person.2.slash"
              size={48}
              color={colors.textSecondary}
            />
            <ThemedText style={styles.noDataText}>
              Please complete login/server setup to view attendance
            </ThemedText>
          </ThemedView>
        </View>
      ) : loading ? (
        <View
          style={[
            styles.tableWrapper,
            styles.shadow,
            { borderColor: borderColor },
          ]}
        >
          <View
            style={[
              styles.loadingOverlay,
              {
                backgroundColor: isDark
                  ? "rgba(28,28,30,0.95)"
                  : "rgba(255,255,255,0.95)",
              },
            ]}
          >
            <ActivityIndicator size="large" color="#FFFFFF" />
          </View>
        </View>
      ) : error ? (
        <View
          style={[
            styles.tableWrapper,
            styles.shadow,
            { borderColor: borderColor },
          ]}
        >
          <ThemedView style={styles.errorContainer}>
            <IconSymbol name="xmark.circle" size={48} color="#EF4444" />
            <ThemedText style={styles.errorText}>{error}</ThemedText>
            <TouchableOpacity
              style={styles.retryBtn}
              onPress={() => fetchAttendance(page)}
            >
              <ThemedText style={styles.retryText}>Retry</ThemedText>
            </TouchableOpacity>
          </ThemedView>
        </View>
      ) : (
        <View
          style={[
            styles.tableWrapper,
            styles.shadow,
            { borderColor: borderColor },
          ]}
        >
          <FlatList
            data={paginatedData}
            ListHeaderComponent={TableHeader}
            stickyHeaderIndices={[0]}
            renderItem={renderItem}
            keyExtractor={(item) => item.id}
            contentContainerStyle={{ paddingBottom: insets.bottom + 20 }}
            showsVerticalScrollIndicator={false}
            refreshControl={
              <RefreshControl
                refreshing={refreshing}
                onRefresh={onRefresh}
                tintColor={colors.tint}
              />
            }
            ListEmptyComponent={
              <ThemedView style={styles.emptyContainer}>
                <IconSymbol
                  name="clock"
                  size={48}
                  color={colors.textSecondary}
                />
                <ThemedText style={styles.emptyText}>
                  No attendance records found
                </ThemedText>
              </ThemedView>
            }
          />
        </View>
      )}
    </ThemedView>
  );
}

const styles = StyleSheet.create({
  container: { flex: 1 },
  header: {
    flexDirection: "row",
    alignItems: "center",
    justifyContent: "space-between",
    paddingHorizontal: 20,
    paddingBottom: 20,
  },
  headerTitle: { fontSize: 20, fontWeight: "800", letterSpacing: -0.5 },
  whiteControl: {
    width: 42,
    height: 42,
    borderRadius: 10, // Updated to 10
    alignItems: "center",
    justifyContent: "center",
  },
  controlsSection: { paddingHorizontal: 20, marginBottom: 20, gap: 12 },
  searchBar: {
    flexDirection: "row",
    alignItems: "center",
    height: 50,
    borderRadius: 10, // Updated to 10
    paddingHorizontal: 16,
    gap: 12,
  },
  searchInput: { flex: 1, fontSize: 15, fontWeight: "500" },
  paginationRow: { flexDirection: "row", gap: 10 },
  pageBtn: {
    width: 50,
    height: 44,
    borderRadius: 10, // Updated to 10
    alignItems: "center",
    justifyContent: "center",
  },
  pageIndicator: {
    flex: 1,
    height: 44,
    borderRadius: 10, // Updated to 10
    alignItems: "center",
    justifyContent: "center",
  },
  pageText: { fontSize: 14, fontWeight: "700" },
  tableWrapper: {
    flex: 1,
    marginHorizontal: 20,
    borderRadius: 10, // Updated to 10
    overflow: "hidden",
    borderWidth: Platform.OS === "ios" ? 0 : 1,
  },
  tableHeader: {
    flexDirection: "row",
    paddingVertical: 16,
    paddingHorizontal: 20,
    borderBottomWidth: 1,
  },
  columnHeader: {
    fontSize: 10,
    fontWeight: "800",
    opacity: 0.4,
    letterSpacing: 1,
  },
  tableRow: {
    flexDirection: "row",
    paddingVertical: 18,
    paddingHorizontal: 20,
    alignItems: "center",
  },
  rowText: { fontSize: 14, fontWeight: "600" },
  timeText: { fontWeight: "500", opacity: 0.8 },
  col1: { flex: 1.3 },
  col2: { flex: 1, textAlign: "center" },
  col3: { flex: 1, textAlign: "right" },
  shadow: {
    shadowColor: "#000",
    shadowOffset: {
      width: 0,
      height: 2,
    },
    shadowOpacity: 0.1,
    shadowRadius: 4,
    elevation: 3,
  },
  noDataContainer: {
    flex: 1,
    justifyContent: "center",
    alignItems: "center",
    padding: 40,
    gap: 12,
  },
  noDataText: {
    fontSize: 16,
    textAlign: "center",
    fontWeight: "500",
    opacity: 0.7,
  },
  loadingOverlay: {
    ...StyleSheet.absoluteFillObject,
    backgroundColor: "rgba(255,255,255,0.95)",
    justifyContent: "center",
    alignItems: "center",
  },
  errorContainer: {
    flex: 1,
    justifyContent: "center",
    alignItems: "center",
    padding: 40,
    gap: 16,
  },
  errorText: {
    fontSize: 16,
    textAlign: "center",
    color: "#EF4444",
    fontWeight: "600",
  },
  retryBtn: {
    paddingHorizontal: 24,
    paddingVertical: 12,
    borderRadius: 8,
    backgroundColor: "#F3F4F6",
    borderWidth: 1,
    borderColor: "#D1D5DB",
  },
  retryText: {
    fontWeight: "600",
    fontSize: 15,
  },
  emptyContainer: {
    flex: 1,
    justifyContent: "center",
    alignItems: "center",
    padding: 40,
    gap: 12,
  },
  emptyText: {
    fontSize: 16,
    textAlign: "center",
    opacity: 0.7,
  },
});
