from flask import Flask, render_template, request, redirect, url_for, session, flash, jsonify
from database_utils import insert_user, get_all_users, get_user_by_email, delete_user
import sqlite3
from functools import wraps
from datetime import datetime, date

app = Flask(__name__)
app.secret_key = 'gatetrack_secret_key_2024'  # Change this in production

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
            session['username'] = user['username']
            session['role'] = user['role']
            
            flash(f'Welcome back, {user["username"] or user["email"]}!', 'success')
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
    teacher_count = get_user_count_by_role('teacher')
    student_count = get_user_count_by_role('student')
    guest_count = get_user_count_by_role('guest')
    
    # Get all users (for admin/teacher view)
    users = get_all_users()
    
    return render_template(
        'dashboard.html',
        user_count=user_count,
        teacher_count=teacher_count,
        student_count=student_count,
        guest_count=guest_count,
        users=users
    )

@app.route('/users')
@login_required
def users():
    # Check if user has permission (super_admin or teacher)
    if session.get('role') not in ['super_admin', 'teacher']:
        flash('You do not have permission to view this page.', 'error')
        return redirect(url_for('dashboard'))
    
    # Get all users
    users = get_all_users()
    
    return render_template('users.html', users=users)

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
        
        # Validation
        if not role or not email:
            flash('Role and Email are required.', 'error')
            return render_template('add_users.html')
        
        # RFID is required for student and guest roles
        if role in ['student', 'guest'] and not rfid:
            flash('RFID is required for Student and Guest roles.', 'error')
            return render_template('add_users.html')
        
        # Check if email already exists
        existing_user = get_user_by_email(email)
        if existing_user:
            flash('A user with this email already exists.', 'error')
            return render_template('add_users.html')
        
        # For teacher role, username and password are required
        if role == 'teacher':
            if not username or not password:
                flash('Username and Password are required for Teacher role.', 'error')
                return render_template('add_users.html')
        
        try:
            # Insert user into database
            user_id = insert_user(
                username=username if username else None,
                email=email,
                password=password if password else None,
                contact=contact if contact else None,
                gender=gender if gender else None,
                role=role,
                rfid=rfid if rfid else None
            )
            
            flash(f'User added successfully!', 'success')
            return redirect(url_for('users'))
            
        except Exception as e:
            flash(f'Error adding user: {str(e)}', 'error')
            return render_template('add_users.html')
    
    # GET request - show add user form
    return render_template('add_users.html')

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
    """Get all attendance records with user info."""
    conn = get_connection()
    cursor = conn.cursor()
    cursor.execute('''
        SELECT a.id, a.user_id, a.time_in, a.time_out, a.created_at,
               u.email, u.username, u.role
        FROM attendance a
        LEFT JOIN users u ON a.user_id = u.id
        ORDER BY a.created_at DESC
    ''')
    records = cursor.fetchall()
    conn.close()
    # Convert Row objects to dictionaries for JSON serialization
    return [dict(record) for record in records]

@app.route('/attendance')
@login_required
def attendance():
    # Get all attendance records
    attendance_records = get_all_attendance()
    
    return render_template('attendance.html', attendance_records=attendance_records)

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
    """
    API endpoint for RFID scanner to log attendance.
    Expects JSON: {"rfid": "XXXXXXXXXX", "mode": 1/2/3/4}
    
    Modes:
    - Mode 1: Door Access only (just open/close door)
    - Mode 2: Time In + Door Access + Light ON
    - Mode 3: Time Out + Door Access + Light OFF
    - Mode 4: Emergency Open/Close (toggle door state)
    
    Returns JSON with status, message, type, and mode.
    """
    try:
        data = request.get_json()
        
        if not data or 'rfid' not in data:
            return jsonify({
                'status': 'error',
                'message': 'Missing RFID parameter'
            }), 400
        
        rfid = data['rfid'].strip().upper()
        mode = int(data.get('mode', 2))  # Default to mode 2 (Time In)
        
        # Validate mode (1, 2, 3, or 4)
        if mode not in [1, 2, 3, 4]:
            return jsonify({
                'status': 'error',
                'message': 'Invalid mode. Use 1 (door), 2 (time in), 3 (time out), or 4 (emergency)'
            }), 400
        
        # Mode 4: Emergency - no RFID needed, just toggle door
        if mode == 4:
            return jsonify({
                'status': 'success',
                'message': 'Emergency Mode - Door Toggle',
                'type': 'emergency',
                'mode': 4,
                'door': 'toggle',
                'light': 'no_change',
                'time': datetime.now().strftime('%Y-%m-%d %H:%M:%S')
            }), 200
        
        # Find user by RFID (for modes 1, 2, 3)
        user = get_user_by_rfid(rfid)
        
        if not user:
            return jsonify({
                'status': 'denied',
                'message': 'Invalid RFID - User not found',
                'mode': mode
            }), 404
        
        # Check today's attendance
        attendance = get_today_attendance(user['id'])
        
        now = datetime.now().strftime('%Y-%m-%d %H:%M:%S')
        
        # Handle based on mode
        if mode == 1:
            # Mode 1: Door Access only (no attendance logging)
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
                'door': 'open',
                'light': 'no_change',
                'time': now
            }), 200
            
        elif mode == 2:
            # Mode 2: Time In + Door Access + Light ON
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
            # Mode 3: Time Out + Door Access + Light OFF
            if not attendance:
                # No time in record - cannot time out
                return jsonify({
                    'status': 'denied',
                    'message': 'No Time In record found. Please time in first.',
                    'mode': 3
                }), 400
            
            if attendance['time_out'] is not None and attendance['time_out'] != '':
                # Already has time out
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
                    'light': 'off',
                    'time_in': attendance['time_in'],
                    'time_out': attendance['time_out']
                }), 200
            
            # Log time out
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
                'light': 'off',
                'time_in': attendance['time_in'],
                'time_out': now
            }), 200
            
    except Exception as e:
        return jsonify({
            'status': 'error',
            'message': str(e)
        }), 500

if __name__ == "__main__":
    app.run(host="0.0.0.0", debug=True)

