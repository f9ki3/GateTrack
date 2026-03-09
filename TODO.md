# TODO - Real-time Attendance Polling

## Task

Make attendance.html realtime using polling technique - if has changes update else don't update

## Steps

- [x] 1. Analyze current code in attendance.html and app.py
- [x] 2. Update app.py - modify `/api/attendance/data` endpoint to return max_id
- [x] 3. Update attendance.html - change JavaScript to track lastMaxId instead of lastCount
- [x] 4. Test the implementation (linter errors are false positives from Jinja2 template syntax)
