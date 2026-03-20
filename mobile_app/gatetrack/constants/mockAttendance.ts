export interface AttendanceRecord {
  id: string;
  studentName: string;
  studentId: string;
  date: string;
  timeIn: string;
  timeOut: string;
}

export const mockAttendance: AttendanceRecord[] = [
  {
    id: "1",
    studentName: "John Doe",
    studentId: "STU001",
    date: "2024-10-01",
    timeIn: "08:30 AM",
    timeOut: "04:30 PM",
  },
  {
    id: "2",
    studentName: "Jane Smith",
    studentId: "STU002",
    date: "2024-10-01",
    timeIn: "08:32 AM",
    timeOut: "04:35 PM",
  },
  {
    id: "3",
    studentName: "Bob Johnson",
    studentId: "STU003",
    date: "2024-10-01",
    timeIn: "08:35 AM",
    timeOut: "05:00 PM",
  },
  {
    id: "4",
    studentName: "Alice Brown",
    studentId: "STU004",
    date: "2024-10-01",
    timeIn: "08:28 AM",
    timeOut: "04:25 PM",
  },
  {
    id: "5",
    studentName: "Charlie Wilson",
    studentId: "STU005",
    date: "2024-10-01",
    timeIn: "09:00 AM",
    timeOut: "01:00 PM",
  },
  {
    id: "6",
    studentName: "Diana Davis",
    studentId: "STU006",
    date: "2024-10-02",
    timeIn: "08:31 AM",
    timeOut: "04:45 PM",
  },
  {
    id: "7",
    studentName: "Eve Miller",
    studentId: "STU007",
    date: "2024-10-02",
    timeIn: "08:29 AM",
    timeOut: "04:30 PM",
  },
  {
    id: "8",
    studentName: "Frank Garcia",
    studentId: "STU008",
    date: "2024-10-02",
    timeIn: "08:40 AM",
    timeOut: "04:55 PM",
  },
  {
    id: "9",
    studentName: "Grace Lee",
    studentId: "STU009",
    date: "2024-10-02",
    timeIn: "08:25 AM",
    timeOut: "04:20 PM",
  },
  {
    id: "10",
    studentName: "Henry Taylor",
    studentId: "STU010",
    date: "2024-10-03",
    timeIn: "08:33 AM",
    timeOut: "04:40 PM",
  },
  {
    id: "11",
    studentName: "Ivy Anderson",
    studentId: "STU011",
    date: "2024-10-03",
    timeIn: "09:10 AM",
    timeOut: "05:15 PM",
  },
  {
    id: "12",
    studentName: "Jack Thomas",
    studentId: "STU012",
    date: "2024-10-03",
    timeIn: "08:30 AM",
    timeOut: "04:30 PM",
  },
  {
    id: "13",
    studentName: "Karen White",
    studentId: "STU013",
    date: "2024-10-04",
    timeIn: "08:15 AM",
    timeOut: "04:00 PM",
  },
  {
    id: "14",
    studentName: "Leo Martinez",
    studentId: "STU014",
    date: "2024-10-04",
    timeIn: "08:45 AM",
    timeOut: "05:10 PM",
  },
];
