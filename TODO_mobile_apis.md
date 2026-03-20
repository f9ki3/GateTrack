# Mobile APIs Implementation Plan (Approved ✅)

## Overview

Implement REST API for mobile app with JWT token auth:

1. Login (token)
2. Paginated/search attendance (+CSV download)
3. Profile CRUD
4. Password update

## Breakdown & Progress Tracking

### Step 1: Dependencies & Setup [✅ DONE]

- [x] Create/update `requirements.txt` + `pip install flask-jwt-extended`
- [x] Test JWT import (flask-jwt-extended v4.6.0 installed)

### Step 2: Database Utils [✅ DONE]

- [x] `database_utils.py`: get_user_attendance_paginated ✅
- [x] `database_utils.py`: get_user_attendance_csv_data_filtered ✅

### Step 3: app.py JWT Config + Auth [PENDING]

- [x] Imports/config/JWTManager ✅
- [x] JWT decorator `@mobile_required` ✅
- [x] POST `/api/v1/mobile/login` ✅

### Step 4: Profile Endpoints [✅ DONE]

- [x] GET `/api/v1/mobile/profile` ✅
- [x] PUT `/api/v1/mobile/profile` ✅
- [x] PUT `/api/v1/mobile/password` ✅

### Step 5: Attendance Endpoints [✅ DONE]

- [x] GET `/api/v1/mobile/attendance` (paginated) ✅
- [x] GET `/api/v1/mobile/attendance/export/csv` ✅

### Step 6: Testing & Completion [✅ COMPLETE]

**All mobile APIs implemented:**

1. ✅ POST `/api/v1/mobile/login` - returns JWT token
2. ✅ GET/PUT `/api/v1/mobile/profile`
3. ✅ PUT `/api/v1/mobile/password`
4. ✅ GET `/api/v1/mobile/attendance?page=&per_page=&search=&date_from=&date_to=`
5. ✅ GET `/api/v1/mobile/attendance/export/csv?search=&date_from=&date_to=`

**Auth**: Bearer token in Authorization header.

**Test**:

```
curl -X POST http://localhost:5000/api/v1/mobile/login \\
  -H "Content-Type: application/json" \\
  -d '{"email":"your@email.com","password":"yourpass"}'

curl -X GET http://localhost:5000/api/v1/mobile/profile \\
  -H "Authorization: Bearer YOUR_TOKEN"
```

Server ready: `python3 app.py`

**Next**: Dependencies → DB utils → app.py → test

**Notes**: Plain passwords kept (upgrade later), JSON responses, user-specific data, date filters optional (?date_from=YYYY-MM-DD&date_to=YYYY-MM-DD).
