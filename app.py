from flask import Flask, render_template, request, redirect, url_for, session, flash, jsonify, Response

from database_utils import insert_user, get_all_users, get_user_by_email, delete_user, update_user, get_user_by_id

import sqlite3
from functools import wraps
from datetime import datetime, date

app = Flask(__name__)
app.secret_key = 'gatetrack_secret_key_2024'  # Change this in production

def format_time_12hr(time_str):
    """Convert 24-hour 'HH:MM:SS' to 12-hour 'hh:mm AM/PM'."""
    if not time_str:
        return ''
    try:
        dt = datetime.strptime(time_str, '%H:%M:%S')
        return dt.strftime('%I:%M %p')
    except (ValueError, TypeError):
        return time_str or ''

app.jinja_env.filters['format_time_12hr'] = format_time_12hr

# Database path
DB_PATH = 'instance/gatetrack.db'

# Flask server IP (change to your server's IP address)
FLASK_SERVER_IP = '192.168.1.100'  # Update this to your Flask server IP
FLASK_SERVER_PORT = 5000

def get_connection():
    conn = sqlite3.connect(DB_PATH)
    conn.row_factory = sqlite3.Row
    return conn

def get_all_users():
    """Get all users from the database."""
    conn = get_connection()
    cursor = conn.cursor()
    cursor.execute('SELECT * FROM users ORDER BY id')
    users = cursor.fetchall()
    conn.close()
    return users

def get_user_count_by_role(role=None):
    """Get user count by role."""
    conn = get_connection()
    cursor = conn.cursor()
    if role:
        cursor.execute('SELECT COUNT(*) FROM users WHERE role = ?', (role,))
    else:
        cursor.execute('SELECT COUNT(*) FROM users')
    count = cursor.fetchone()[0]
    conn.close()
    return count

def authenticate_user(email, password):
    """Authenticate user by email and password."""
    conn = get_connection()
    cursor = conn.cursor()
    cursor.execute('SELECT * FROM users WHERE email = ?', (email,))
    user = cursor.fetchone()
    conn.close()
    
    if user:
        # Check if password matches
        if user['password'] == password:
            return dict(user)
    return None

def login_required(f):
    """Decorator to require login for routes."""
    @wraps(f)
    def decorated_function(*args, **kwargs):
        if 'user_id' not in session:
            flash('Please log in to access this page.', 'error')
            return redirect(url_for('login'))
        return f(*args, **kwargs)
    return decorated_function

@app.route('/')
def home():
    return redirect(url_for('login'))

@app.route('/login', methods=['GET', 'POST'])
def login():
    if request.method == 'POST':
        email = request.form.get('email', '').strip()
        password = request.form.get('password', '').strip()
        
        if not email or not password:
            flash('Please enter both email and password.', 'error')
            return render_template('login.html')
        
        user = authenticate_user(email, password)
        
        if user:
            # Set session variables
            session['user_id'] = user['id']
            session['email'] = user['email']
            session['last_name'] = user['last_name']
            session['first_name'] = user['first_name']
            session['username'] = user['username']
            session['role'] = user['role']
            
            return redirect(url_for('dashboard'))
        else:
            flash('Invalid email or password. Please try again.', 'error')
            return render_template('login.html')
    
    # GET request - show login form
    # If already logged in, redirect to dashboard
    if 'user_id' in session:
        return redirect(url_for('dashboard'))
    
    return render_template('login.html')

@app.route('/logout')
def logout():
    # Clear session
    session.clear()
    flash('You have been logged out successfully.', 'success')
    return redirect(url_for('login'))

@app.route('/dashboard')
@login_required
def dashboard():
    # Get user statistics
    user_count = get_user_count_by_role()

    super_admin_count = get_user_count_by_role('super_admin')
    teacher_count = get_user_count_by_role('teacher')
    staff_count = get_user_count_by_role('staff')
    technician_count = get_user_count_by_role('technician')
    guest_count = get_user_count_by_role('guest')
    

    # Laboratory status
    from database_utils import get_today_attendance_count, get_teachers_currently_present
    teachers_present = get_teachers_currently_present()
    lab_status = 'Available' if teachers_present > 0 else 'Not Available'

    
    # Get all users (for admin/teacher view)
    users = get_all_users()
    
    return render_template(
        'dashboard.html',
        user_count=user_count,
        super_admin_count=super_admin_count,
        teacher_count=teacher_count,
        staff_count=staff_count,
        technician_count=technician_count,
        guest_count=guest_count,

        users=users,
        teachers_present=teachers_present,
        lab_status=lab_status

    )


@app.route('/users')
@login_required
def users():
    # Check if user has permission (super_admin or teacher)
    if session.get('role') not in ['super_admin', 'teacher']:
        flash('You do not have permission to view this page.', 'error')
        return redirect(url_for('dashboard'))
    
    # Pagination and filter params
    page = int(request.args.get('page', 1))
    per_page = int(request.args.get('per_page', 10))
    search_term = request.args.get('search', '').strip()
    role_filter = request.args.get('role', '')
    
    if per_page not in [10, 20, 30, 100]:
        per_page = 10
    
    # Ensure page >= 1
    page = max(1, page)
    
    # Get paginated users and total count
    from database_utils import get_paginated_users
    users, total_count = get_paginated_users(page, per_page, search_term, role_filter)
    
    total_pages = (total_count + per_page - 1) // per_page
    
    # Adjust page if beyond total
    if page > total_pages and total_pages > 0:
        return redirect(url_for('users', page=total_pages, per_page=per_page, 
                               search=search_term, role=role_filter))
    
    return render_template('users.html', 
                         users=users,
                         page=page,
                         per_page=per_page,
                         total_count=total_count,
                         total_pages=total_pages,
                         search_term=search_term,
                         role_filter=role_filter)


@app.route('/add_user', methods=['GET', 'POST'])
@login_required
def add_user():
    # Check if user has permission (super_admin or teacher)
    if session.get('role') not in ['super_admin', 'teacher']:
        flash('You do not have permission to add users.', 'error')
        return redirect(url_for('dashboard'))
    
    if request.method == 'POST':
        role = request.form.get('role', '').strip()
        email = request.form.get('email', '').strip()
        username = request.form.get('username', '').strip()
        password = request.form.get('password', '').strip()
        contact = request.form.get('contact', '').strip()
        gender = request.form.get('gender', '').strip()
        rfid = request.form.get('rfid', '').strip().upper()
        first_name = request.form.get('first_name', '').strip()
        last_name = request.form.get('last_name', '').strip()
        
        # Validation
        if not role or not email:
            flash('Role and Email are required.', 'error')
            return render_template('users.html')
        
        # RFID is required for student and guest roles
        if role in ['student', 'guest'] and not rfid:
            flash('RFID is required for Student and Guest roles.', 'error')
            return render_template('users.html')
        
        # Check if email already exists
        existing_user = get_user_by_email(email)
        if existing_user:
            flash('A user with this email already exists.', 'error')
            return render_template('users.html')

        # Check RFID uniqueness if provided
        if rfid:
            existing_rfid_user = get_user_by_rfid(rfid)
            if existing_rfid_user:
                flash('This RFID tag is already assigned to another user.', 'error')
                return render_template('users.html')
        
        # For super_admin and teacher roles, username and password are required
        if role in ['super_admin', 'teacher']:
            if not username or not password:
                flash('Username and Password are required for Super Admin and Teacher roles.', 'error')
                return render_template('users.html')

        # Auto-generate username for staff, technician, guest if not provided
        if role in ['staff', 'technician', 'guest'] and not username:
            count = get_user_count_by_role(role)
            username = f"{role}{count + 1:03d}"

        try:
            # Insert user into database
            user_id = insert_user(
                username=username,
                email=email,
                password=password if password else None,
                contact=contact if contact else None,
                gender=gender if gender else None,
                role=role,
                rfid=rfid if rfid else None,
                first_name=first_name if first_name else None,
                last_name=last_name if last_name else None
            )
            
            flash(f'User added successfully!', 'success')
            return redirect(url_for('users'))
            
        except Exception as e:
            flash(f'Error adding user: {str(e)}', 'error')
            return render_template('add_users.html')
    
    # GET request - show add user form
    return render_template('add_users.html')

@app.route('/edit_user', methods=['POST'])
@login_required
def edit_user():
    # Only super_admin and teacher can edit
    if session.get('role') not in ['super_admin', 'teacher']:
        flash('You do not have permission to edit users.', 'error')
        return redirect(url_for('dashboard'))

    user_id = request.form.get('user_id', '').strip()
    if not user_id.isdigit():
        flash('Invalid user ID.', 'error')
        return redirect(url_for('users'))

    user_id = int(user_id)

    # Prevent editing yourself
    if session.get('user_id') == user_id:
        flash('You cannot edit your own account from this page.', 'error')
        return redirect(url_for('users'))

    # Get current user first
    current_user = get_user_by_id(user_id)
    if not current_user:
        flash('User not found.', 'error')
        return redirect(url_for('users'))

    # Read form values
    username = request.form.get('username', '').strip()
    role = request.form.get('role', '').strip().lower()
    email = request.form.get('email', '').strip().lower()
    contact = request.form.get('contact', '').strip()
    rfid = request.form.get('rfid', '').strip().upper()
    first_name = request.form.get('first_name', '').strip()
    last_name = request.form.get('last_name', '').strip()

    allowed_roles = ['super_admin', 'teacher', 'staff', 'technician', 'guest']
    if role not in allowed_roles:
        flash('Invalid role selected.', 'error')
        return redirect(url_for('users'))

    # Optional: teacher should not be able to assign super_admin
    if session.get('role') == 'teacher' and role == 'super_admin':
        flash('Teachers cannot assign Super Admin role.', 'error')
        return redirect(url_for('users'))

    # Required validation
    if not first_name or not last_name or not email:
        flash('First name, last name, and email are required.', 'error')
        return redirect(url_for('users'))

    # RFID required for all users if your system needs RFID always
    if not rfid:
        flash('RFID is required.', 'error')
        return redirect(url_for('users'))

    # Check email uniqueness except current user
    existing_user = get_user_by_email(email)
    if existing_user and existing_user['id'] != user_id:
        flash('A user with this email already exists.', 'error')
        return redirect(url_for('users'))

    # Optional RFID uniqueness check
    existing_rfid_user = get_user_by_rfid(rfid)
    if existing_rfid_user and existing_rfid_user['id'] != user_id:
        flash('This RFID tag is already assigned to another user.', 'error')
        return redirect(url_for('users'))

    try:
        update_data = {
            'username': username if username else current_user['username'],
            'role': role,
            'email': email,
            'first_name': first_name,
            'last_name': last_name,
            'contact': contact,
            'rfid': rfid
        }

        success = update_user(user_id, **update_data)

        if success:
            flash('User updated successfully!', 'success')
        else:
            flash('No changes were made or update failed.', 'error')

    except Exception as e:
        app.logger.exception('Error updating user')
        flash(f'Error updating user: {str(e)}', 'error')

    return redirect(url_for('users'))

@app.route('/delete_user/<int:user_id>', methods=['POST'])
@login_required
def delete_user_route(user_id):
    # Check if user has permission (super_admin or teacher)
    if session.get('role') not in ['super_admin', 'teacher']:
        flash('You do not have permission to delete users.', 'error')
        return redirect(url_for('dashboard'))
    
    # Prevent deleting yourself
    if session.get('user_id') == user_id:
        flash('You cannot delete your own account.', 'error')
        return redirect(url_for('users'))
    
    # Get user info before deletion for logging
    user = get_user_by_id(user_id) if 'get_user_by_id' in dir() else None
    if not user:
        user = get_all_users()
        user = next((u for u in user if u['id'] == user_id), None)
    
    try:
        delete_user(user_id)
        flash(f'User deleted successfully!', 'success')
    except Exception as e:
        flash(f'Error deleting user: {str(e)}', 'error')
    
    return redirect(url_for('users'))

# ==================== Attendance Functions ====================

def get_all_attendance():
    """Get all attendance records with user info - only known users."""
    conn = get_connection()
    cursor = conn.cursor()
    cursor.execute('''
        SELECT a.id, a.user_id, a.time_in, a.time_out, a.created_at,
               u.email, u.username, u.role, u.first_name, u.last_name
        FROM attendance a
        INNER JOIN users u ON a.user_id = u.id AND (u.username IS NOT NULL OR u.email IS NOT NULL OR u.first_name IS NOT NULL OR u.last_name IS NOT NULL)
        ORDER BY a.created_at DESC
    ''')
    records = cursor.fetchall()
    conn.close()
    # Convert Row objects to dictionaries for JSON serialization
    return [dict(record) for record in records]

@app.route('/attendance')
@login_required
def attendance():
    # Pagination and filter params (follow users exactly)
    page = int(request.args.get('page', 1))
    per_page = int(request.args.get('per_page', 10))
    search_term = request.args.get('search', '').strip()
    role_filter = request.args.get('role', '')
    
    if per_page not in [10, 20, 30, 100]:
        per_page = 10
    
    page = max(1, page)
    
    # Get paginated attendance
    from database_utils import get_paginated_attendance
    records, total_count = get_paginated_attendance(page, per_page, search_term, role_filter)
    
    total_pages = (total_count + per_page - 1) // per_page
    
    if page > total_pages and total_pages > 0:
        return redirect(url_for('attendance', page=total_pages, per_page=per_page, 
                               search=search_term, role=role_filter))
    
    return render_template('attendance.html', 
                         records=records,
                         page=page,
                         per_page=per_page,
                         total_count=total_count,
                         total_pages=total_pages,
                         search_term=search_term,
                         role_filter=role_filter)

@app.route('/api/attendance/data')
@login_required
def api_attendance_data():
    """API endpoint to get attendance data for polling."""
    try:
        records = get_all_attendance()
        
        # Process records and track max_id
        data = []
        max_id = 0
        for record in records:
            # Track max_id for change detection
            if record.get('id', 0) > max_id:
                max_id = record['id']
            
            # Calculate duration
            if record.get('time_in') and record.get('time_out'):
                try:
                    time_in = record['time_in'].split(' ')[1]
                    time_out = record['time_out'].split(' ')[1]
                    in_mins = int(time_in.split(':')[0]) * 60 + int(time_in.split(':')[1])
                    out_mins = int(time_out.split(':')[0]) * 60 + int(time_out.split(':')[1])
                    duration = out_mins - in_mins
                    if duration >= 60:
                        record['duration'] = f"{duration // 60}h {duration % 60}m"
                    else:
                        record['duration'] = f"{duration}m"
                except:
                    record['duration'] = '-'
            else:
                record['duration'] = '-'
            
            # Get initial for avatar
            username = record.get('username')
            email = record.get('email')
            record['initial'] = (username or email or 'U')[0].upper() if (username or email) else 'U'
            
            data.append(record)
        
        return jsonify({
            'status': 'success',
            'data': data,
            'count': len(data),
            'max_id': max_id
        }), 200
        
    except Exception as e:
        return jsonify({
            'status': 'error',
            'message': str(e)
        }), 500

# ==================== RFID & Attendance API ====================

def get_user_by_rfid(rfid):
    """Get a user by their RFID tag."""
    conn = get_connection()
    cursor = conn.cursor()
    # Normalize RFID by removing spaces for comparison
    rfid_normalized = rfid.replace(" ", "").upper()
    cursor.execute('SELECT * FROM users WHERE UPPER(REPLACE(rfid, " ", "")) = ?', (rfid_normalized,))
    user = cursor.fetchone()
    conn.close()
    return user

def get_today_attendance(user_id):
    """Get today's attendance record for a user."""
    conn = get_connection()
    cursor = conn.cursor()
    today = date.today().strftime('%Y-%m-%d')
    
    cursor.execute('''
        SELECT * FROM attendance 
        WHERE user_id = ? AND date(created_at) = ?
    ''', (user_id, today))
    
    record = cursor.fetchone()
    conn.close()
    return record

def log_attendance(user_id, time_in=None, time_out=None):
    """Create or update attendance record."""
    conn = get_connection()
    cursor = conn.cursor()
    
    now = datetime.now().strftime('%Y-%m-%d %H:%M:%S')
    
    # Check if record exists today
    today = date.today().strftime('%Y-%m-%d')
    cursor.execute('''
        SELECT * FROM attendance 
        WHERE user_id = ? AND date(created_at) = ?
    ''', (user_id, today))
    
    existing = cursor.fetchone()
    
    if existing:
        # Update existing record
        if time_out:
            cursor.execute('''
                UPDATE attendance SET time_out = ? WHERE id = ?
            ''', (time_out, existing['id']))
        conn.commit()
    else:
        # Insert new record
        cursor.execute('''
            INSERT INTO attendance (user_id, time_in, created_at)
            VALUES (?, ?, ?)
        ''', (user_id, time_in if time_in else now, now))
        conn.commit()
    
    conn.close()

@app.route('/api/attendance', methods=['POST'])
def api_attendance():
    try:
        data = request.get_json(silent=True) or {}

        mode = int(data.get('mode', 2))

        if mode not in [1, 2, 3]:
            return jsonify({
                'status': 'error',
                'message': 'Invalid mode. Use 1, 2, or 3'
            }), 400

        rfid = str(data.get('rfid', '')).strip().upper()
        if not rfid:
            return jsonify({
                'status': 'error',
                'message': 'Missing RFID parameter'
            }), 400

        user = get_user_by_rfid(rfid)
        if not user:
            return jsonify({
                'status': 'denied',
                'message': 'Invalid RFID - User not found',
                'mode': mode
            }), 404

        attendance = get_today_attendance(user['id'])
        now = datetime.now().strftime('%Y-%m-%d %H:%M:%S')

        if mode == 1:
            return jsonify({
                'status': 'success',
                'message': 'Door Access Granted',
                'user': {
                    'id': user['id'],
                    'email': user['email'],
                    'role': user['role']
                },
                'type': 'door_access',
                'mode': 1,
                'door': 'toggle',
                'light': 'no_change',
                'time': now
            }), 200

        elif mode == 2:
            if attendance and attendance['time_in']:
                return jsonify({
                    'status': 'success',
                    'message': 'Already timed in today',
                    'user': {
                        'id': user['id'],
                        'email': user['email'],
                        'role': user['role']
                    },
                    'type': 'already_logged',
                    'mode': 2,
                    'door': 'open',
                    'light': 'on',
                    'time_in': attendance['time_in'],
                    'time_out': attendance['time_out']
                }), 200

            log_attendance(user['id'], time_in=now)

            return jsonify({
                'status': 'success',
                'message': 'Time In Recorded',
                'user': {
                    'id': user['id'],
                    'email': user['email'],
                    'role': user['role']
                },
                'type': 'time_in',
                'mode': 2,
                'door': 'open',
                'light': 'on',
                'time': now
            }), 200

        elif mode == 3:
            if not attendance or not attendance['time_in']:
                return jsonify({
                    'status': 'denied',
                    'message': 'No Time In record found. Please time in first.',
                    'mode': 3
                }), 400

            if attendance['time_out']:
                return jsonify({
                    'status': 'success',
                    'message': 'Already logged out today',
                    'user': {
                        'id': user['id'],
                        'email': user['email'],
                        'role': user['role']
                    },
                    'type': 'already_logged',
                    'mode': 3,
                    'door': 'open',
                    'light': 'on',
                    'time_in': attendance['time_in'],
                    'time_out': attendance['time_out']
                }), 200

            log_attendance(user['id'], time_out=now)

            return jsonify({
                'status': 'success',
                'message': 'Time Out Recorded',
                'user': {
                    'id': user['id'],
                    'email': user['email'],
                    'role': user['role']
                },
                'type': 'time_out',
                'mode': 3,
                'door': 'open',
                'light': 'on',
                'time_in': attendance['time_in'],
                'time_out': now
            }), 200

    except Exception as e:
        return jsonify({
            'status': 'error',
            'message': str(e)
        }), 500

# ==================== EXPORT ROUTES ====================

from database_utils import export_users_csv_data, export_attendance_csv_data
import csv
from io import StringIO

@app.route('/export/users_csv')
@login_required
def export_users_csv():
    """Export filtered users to CSV."""
    # Get filter params from query string
    search_term = request.args.get('search', '').strip()
    role_filter = request.args.get('role', '')
    
    # Export data
    users_data = export_users_csv_data(search_term=search_term, role_filter=role_filter)
    
    # Create CSV in memory
    output = StringIO()
    writer = csv.DictWriter(output, fieldnames=[
        'id', 'username', 'email', 'first_name', 'last_name', 
        'contact', 'gender', 'role', 'rfid', 'created_at'
    ])
    writer.writeheader()
    writer.writerows(users_data)
    
    # Prepare response
    csv_content = output.getvalue()
    output.close()
    
    filename = f"users_export_{datetime.now().strftime('%Y%m%d_%H%M%S')}.csv"
    
    return Response(
        csv_content,
        mimetype='text/csv',
        headers={'Content-Disposition': f'attachment; filename={filename}'}
    )

@app.route('/export/attendance_csv')
@login_required
def export_attendance_csv():
    """Export attendance records to CSV."""
    # Get filter params
    role_filter = request.args.get('role_filter', '')
    
    # Export data
    attendance_data = export_attendance_csv_data(role_filter=role_filter)
    
    # Create CSV
    output = StringIO()
    writer = csv.DictWriter(output, fieldnames=[
        'id', 'username', 'first_name', 'last_name', 'email', 'date', 'time_in', 'time_out', 'duration', 'role'
    ])
    writer.writeheader()
    writer.writerows(attendance_data)
    
    csv_content = output.getvalue()
    output.close()
    
    filename = f"attendance_export_{datetime.now().strftime('%Y%m%d_%H%M%S')}.csv"
    
    return Response(
        csv_content,
        mimetype='text/csv',
        headers={'Content-Disposition': f'attachment; filename={filename}'}
    )

@app.route('/export/user_attendance/<int:user_id>')
@login_required
def export_user_attendance(user_id):
    """Export attendance records for specific user to CSV."""
    from database_utils import get_user_attendance_csv_data
    
    # Verify user exists
    user = get_user_by_id(user_id)
    if not user:
        flash('User not found.', 'error')
        return redirect(url_for('users'))
    
    # Get user's attendance data
    attendance_data = get_user_attendance_csv_data(user_id)
    
    # Create CSV
    output = StringIO()
    writer = csv.DictWriter(output, fieldnames=[
        'id', 'username', 'first_name', 'last_name', 'email', 'date', 'time_in', 'time_out', 'duration', 'role'
    ])
    writer.writeheader()
    writer.writerows(attendance_data)
    
    csv_content = output.getvalue()
    output.close()
    
    # Filename with user info
    username = user['username'] or user['email'].split('@')[0]
    filename = f"{username}_attendance_{datetime.now().strftime('%Y%m%d_%H%M%S')}.csv"
    
    return Response(
        csv_content,
        mimetype='text/csv',
        headers={'Content-Disposition': f'attachment; filename={filename}'}
    )
    
if __name__ == "__main__":
    app.run(host="0.0.0.0", debug=True)

