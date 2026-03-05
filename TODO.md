# Task Plan: SQLite Database and Models for GateTrack

## Information Gathered

- Current project: Basic Flask app with "Hello World" route
- Working Directory: /Users/mac/GateTrack
- Files: app.py, templates/

## Requirements

1. **User Model** with fields:
   - username
   - email
   - contact
   - address
   - gender

2. **Roles**: super_admin, teacher, student, guest

3. **Attendance Model** with fields:
   - time_in
   - time_out
   - type (e.g., present, absent, late)
   - time (timestamp)

4. **CRUD Routes** for users and attendance

## Plan

### Step 1: Create database models file (models.py)

- Set up SQLAlchemy configuration
- Create User model with all required fields
- Create Role choices (super_admin, teacher, student, guest)
- Create Attendance model with time_in, time_out, type, time fields
- Create relationship between User and Attendance

### Step 2: Update app.py

- Import necessary Flask modules (request, jsonify)
- Import SQLAlchemy and models
- Configure database URI
- Create database tables
- Add CRUD routes for Users
- Add CRUD routes for Attendance

### Files to Create/Edit

1. **Create**: `/Users/mac/GateTrack/models.py` - Database models
2. **Edit**: `/Users/mac/GateTrack/app.py` - Add routes and configuration

## Implementation Details

### User Model

- id (Primary Key)
- username (String, required)
- email (String, required, unique)
- contact (String)
- address (String)
- gender (String - Male/Female/Other)
- role (String - super_admin/teacher/student/guest)

### Attendance Model

- id (Primary Key)
- user_id (Foreign Key to User)
- time_in (DateTime)
- time_out (DateTime)
- type (String - present/absent/late)
- time (DateTime - timestamp)

### Routes to Create

- POST /users - Create user
- GET /users - Get all users
- GET /users/<id> - Get single user
- PUT /users/<id> - Update user
- DELETE /users/<id> - Delete user

- POST /attendance - Create attendance
- GET /attendance - Get all attendance records
- GET /attendance/<id> - Get single attendance
- PUT /attendance/<id> - Update attendance
- DELETE /attendance/<id> - Delete attendance

- [x] Install required packages: flask, flask-sqlalchemy
- [x] Run the app to initialize the database
- [ ] Test the CRUD operations
