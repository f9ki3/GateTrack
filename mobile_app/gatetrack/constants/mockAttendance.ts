export interface AttendanceRecord {
  id: string;
  date: string;
  timeIn: string;
  timeOut: string;
  // Real API optional fields
  time_in?: string;
  time_out?: string;
  duration?: string;
  email?: string;
  first_name?: string;
  last_name?: string;
  username?: string;
  role?: string;
}

export const mockAttendance: AttendanceRecord[] = [
  { id: "1", date: "2024-10-01", timeIn: "08:30 AM", timeOut: "04:30 PM" },
  { id: "2", date: "2024-10-01", timeIn: "08:32 AM", timeOut: "04:35 PM" },
  { id: "3", date: "2024-10-01", timeIn: "08:35 AM", timeOut: "05:00 PM" },
  { id: "4", date: "2024-10-01", timeIn: "08:28 AM", timeOut: "04:25 PM" },
  { id: "5", date: "2024-10-01", timeIn: "09:00 AM", timeOut: "01:00 PM" },
  { id: "6", date: "2024-10-02", timeIn: "08:31 AM", timeOut: "04:45 PM" },
  { id: "7", date: "2024-10-02", timeIn: "08:29 AM", timeOut: "04:30 PM" },
  { id: "8", date: "2024-10-02", timeIn: "08:40 AM", timeOut: "04:55 PM" },
  { id: "9", date: "2024-10-02", timeIn: "08:25 AM", timeOut: "04:20 PM" },
  { id: "10", date: "2024-10-03", timeIn: "08:33 AM", timeOut: "04:40 PM" },
  { id: "11", date: "2024-10-03", timeIn: "09:10 AM", timeOut: "05:15 PM" },
  { id: "12", date: "2024-10-03", timeIn: "08:30 AM", timeOut: "04:30 PM" },
];
