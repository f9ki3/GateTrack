# GateTrack Export Feature Implementation - CSV for filtered users + all attendance

## Plan Progress

✅ **Step 1: Create TODO.md** - DONE

## Remaining Steps

**Step 2: Update database_utils.py**

- Add `export_users_csv_data(search_term='', role_filter='', limit=10000)`: Return filtered users as list[dict] for CSV
- Add `export_attendance_csv_data(role_filter='', date_from=None, date_to=None)`: Return attendance records joined with users

**Step 3: Update app.py**

- Add `@app.route('/export/users_csv')`: login*required, get params, csv response 'users*{date}.csv'
- Add `@app.route('/export/attendance_csv')`: login*required, csv 'attendance*{date}.csv'

**Step 4: Update templates/users.html**

- Replace Export dropdown href='#' with real urls preserving filters/page/per_page/search/role

**Step 5: Add to templates/attendance.html**

- Add Export dropdown near search bar, similar to users.html

**Step 6: Test**

- `python app.py`
- Login -> /users -> filter -> Export CSV (verify filtered data)
- /attendance -> Export CSV (verify all records)
- Mark COMPLETE
