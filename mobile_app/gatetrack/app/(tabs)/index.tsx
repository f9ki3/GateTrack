import React, { useState, useCallback } from "react";
import {
  FlatList,
  RefreshControl,
  StyleSheet,
  TextInput,
  View,
  Alert,
  TouchableOpacity,
  Platform,
} from "react-native";
import { useSafeAreaInsets } from "react-native-safe-area-context";

import { IconSymbol } from "@/components/ui/icon-symbol";
import { ThemedText } from "@/components/themed-text";
import { ThemedView } from "@/components/themed-view";
import { Colors } from "@/constants/theme";
import { useColorScheme } from "@/hooks/use-color-scheme";
import { AttendanceRecord, mockAttendance } from "@/constants/mockAttendance";

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

  const handleExport = () => {
    Alert.alert("Export", "Attendance logs have been saved to your device.");
  };

  const onRefresh = useCallback(() => {
    setRefreshing(true);
    setTimeout(() => setRefreshing(false), 1000);
  }, []);

  const filteredData = mockAttendance.filter((record) =>
    record.date.includes(searchQuery),
  );

  const startIndex = (page - 1) * itemsPerPage;
  const paginatedData = filteredData.slice(
    startIndex,
    startIndex + itemsPerPage,
  );
  const totalPages = Math.ceil(filteredData.length / itemsPerPage);

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
            onChangeText={(text) => {
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
        />
      </View>
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
});
