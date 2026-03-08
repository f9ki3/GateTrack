from flask import Flask, render_template, request, redirect, url_for, session, flash
import sqlite3
from functools import wraps

app = Flask(__name__)
app.secret_key = 'gatetrack_secret_key_2024'  # Change this in production

# Database path
DB_PATH = 'instance/gatetrack.db'

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

if __name__ == "__main__":
    app.run(host="0.0.0.0", debug=True)

