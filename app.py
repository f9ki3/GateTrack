from flask import Flask, request, jsonify, render_template, redirect, url_for, flash
from flask_login import LoginManager, login_user, login_required, logout_user, current_user
from datetime import datetime

import os

# Get the base directory of the application
basedir = os.path.abspath(os.path.dirname(__file__))

app = Flask(__name__)

# Secret key for sessions
app.config['SECRET_KEY'] = 'gatetrack-secret-key-2024'

# Database configuration - use absolute path
app.config['SQLALCHEMY_DATABASE_URI'] = f'sqlite:///{os.path.join(basedir, "instance", "gatetrack.db")}'
app.config['SQLALCHEMY_TRACK_MODIFICATIONS'] = False

# Import db and models AFTER app config is set
from models import db, User, Attendance, ROLES, GENDERS, ATTENDANCE_TYPES
db.init_app(app)

# Flask-Login configuration
login_manager = LoginManager()
login_manager.init_app(app)
login_manager.login_view = 'login'

@login_manager.user_loader
def load_user(user_id):
    return User.query.get(int(user_id))


# ==================== USER ROUTES ====================

@app.route('/users', methods=['POST'])
def create_user():
    """Create a new user"""
    data = request.get_json()
    
    # Validate required fields
    if not data.get('username'):
        return jsonify({'error': 'Username is required'}), 400
    if not data.get('email'):
        return jsonify({'error': 'Email is required'}), 400
    if not data.get('role'):
        data['role'] = 'guest'
    
    # Validate role
    if data.get('role') not in ROLES:
        return jsonify({'error': f'Invalid role. Must be one of: {ROLES}'}), 400
    
    # Validate gender if provided
    if data.get('gender') and data.get('gender') not in GENDERS:
        return jsonify({'error': f'Invalid gender. Must be one of: {GENDERS}'}), 400
    
    # Check if email already exists
    existing_user = User.query.filter_by(email=data['email']).first()
    if existing_user:
        return jsonify({'error': 'Email already exists'}), 400
    
    user = User(
        username=data['username'],
        email=data['email'],
        contact=data.get('contact'),
        address=data.get('address'),
        gender=data.get('gender'),
        role=data.get('role', 'guest')
    )
    
    db.session.add(user)
    db.session.commit()
    
    return jsonify(user.to_dict()), 201


@app.route('/users', methods=['GET'])
def get_all_users():
    """Get all users"""
    users = User.query.all()
    return jsonify([user.to_dict() for user in users]), 200


@app.route('/users/<int:user_id>', methods=['GET'])
def get_user(user_id):
    """Get a single user by ID"""
    user = User.query.get(user_id)
    if not user:
        return jsonify({'error': 'User not found'}), 404
    return jsonify(user.to_dict()), 200


@app.route('/users/<int:user_id>', methods=['PUT'])
def update_user(user_id):
    """Update a user"""
    user = User.query.get(user_id)
    if not user:
        return jsonify({'error': 'User not found'}), 404
    
    data = request.get_json()
    
    # Update fields if provided
    if 'username' in data:
        user.username = data['username']
    if 'email' in data:
        # Check if new email already exists
        existing = User.query.filter_by(email=data['email']).first()
        if existing and existing.id != user_id:
            return jsonify({'error': 'Email already exists'}), 400
        user.email = data['email']
    if 'contact' in data:
        user.contact = data['contact']
    if 'address' in data:
        user.address = data['address']
    if 'gender' in data:
        if data['gender'] not in GENDERS:
            return jsonify({'error': f'Invalid gender. Must be one of: {GENDERS}'}), 400
        user.gender = data['gender']
    if 'role' in data:
        if data['role'] not in ROLES:
            return jsonify({'error': f'Invalid role. Must be one of: {ROLES}'}), 400
        user.role = data['role']
    
    db.session.commit()
    return jsonify(user.to_dict()), 200


@app.route('/users/<int:user_id>', methods=['DELETE'])
def delete_user(user_id):
    """Delete a user"""
    user = User.query.get(user_id)
    if not user:
        return jsonify({'error': 'User not found'}), 404
    
    db.session.delete(user)
    db.session.commit()
    return jsonify({'message': 'User deleted successfully'}), 200


# ==================== ATTENDANCE ROUTES ====================

@app.route('/attendance', methods=['POST'])
def create_attendance():
    """Create a new attendance record"""
    data = request.get_json()
    
    # Validate required fields
    if not data.get('user_id'):
        return jsonify({'error': 'User ID is required'}), 400
    
    # Check if user exists
    user = User.query.get(data['user_id'])
    if not user:
        return jsonify({'error': 'User not found'}), 404
    
    # Validate attendance type
    attendance_type = data.get('type', 'present')
    if attendance_type not in ATTENDANCE_TYPES:
        return jsonify({'error': f'Invalid type. Must be one of: {ATTENDANCE_TYPES}'}), 400
    
    # Parse time_in if provided
    time_in = None
    if data.get('time_in'):
        try:
            time_in = datetime.fromisoformat(data['time_in'].replace('Z', '+00:00'))
        except ValueError:
            return jsonify({'error': 'Invalid time_in format. Use ISO format'}), 400
    
    # Parse time_out if provided
    time_out = None
    if data.get('time_out'):
        try:
            time_out = datetime.fromisoformat(data['time_out'].replace('Z', '+00:00'))
        except ValueError:
            return jsonify({'error': 'Invalid time_out format. Use ISO format'}), 400
    
    attendance = Attendance(
        user_id=data['user_id'],
        time_in=time_in,
        time_out=time_out,
        type=attendance_type
    )
    
    db.session.add(attendance)
    db.session.commit()
    
    return jsonify(attendance.to_dict()), 201


@app.route('/attendance', methods=['GET'])
def get_all_attendance():
    """Get all attendance records"""
    attendance = Attendance.query.all()
    return jsonify([att.to_dict() for att in attendance]), 200


@app.route('/attendance/<int:attendance_id>', methods=['GET'])
def get_attendance(attendance_id):
    """Get a single attendance record by ID"""
    attendance = Attendance.query.get(attendance_id)
    if not attendance:
        return jsonify({'error': 'Attendance not found'}), 404
    return jsonify(attendance.to_dict()), 200


@app.route('/attendance/<int:attendance_id>', methods=['PUT'])
def update_attendance(attendance_id):
    """Update an attendance record"""
    attendance = Attendance.query.get(attendance_id)
    if not attendance:
        return jsonify({'error': 'Attendance not found'}), 404
    
    data = request.get_json()
    
    # Update user_id if provided
    if 'user_id' in data:
        user = User.query.get(data['user_id'])
        if not user:
            return jsonify({'error': 'User not found'}), 404
        attendance.user_id = data['user_id']
    
    # Update time_in if provided
    if 'time_in' in data:
        if data['time_in']:
            try:
                attendance.time_in = datetime.fromisoformat(data['time_in'].replace('Z', '+00:00'))
            except ValueError:
                return jsonify({'error': 'Invalid time_in format. Use ISO format'}), 400
        else:
            attendance.time_in = None
    
    # Update time_out if provided
    if 'time_out' in data:
        if data['time_out']:
            try:
                attendance.time_out = datetime.fromisoformat(data['time_out'].replace('Z', '+00:00'))
            except ValueError:
                return jsonify({'error': 'Invalid time_out format. Use ISO format'}), 400
        else:
            attendance.time_out = None
    
    # Update type if provided
    if 'type' in data:
        if data['type'] not in ATTENDANCE_TYPES:
            return jsonify({'error': f'Invalid type. Must be one of: {ATTENDANCE_TYPES}'}), 400
        attendance.type = data['type']
    
    db.session.commit()
    return jsonify(attendance.to_dict()), 200


@app.route('/attendance/<int:attendance_id>', methods=['DELETE'])
def delete_attendance(attendance_id):
    """Delete an attendance record"""
    attendance = Attendance.query.get(attendance_id)
    if not attendance:
        return jsonify({'error': 'Attendance not found'}), 404
    
    db.session.delete(attendance)
    db.session.commit()
    return jsonify({'message': 'Attendance deleted successfully'}), 200


# ==================== ADDITIONAL UTILITY ROUTES ====================

@app.route('/users/<int:user_id>/attendance', methods=['GET'])
def get_user_attendance(user_id):
    """Get all attendance records for a specific user"""
    user = User.query.get(user_id)
    if not user:
        return jsonify({'error': 'User not found'}), 404
    
    attendance = Attendance.query.filter_by(user_id=user_id).all()
    return jsonify([att.to_dict() for att in attendance]), 200


@app.route('/roles', methods=['GET'])
def get_roles():
    """Get all available roles"""
    return jsonify(ROLES), 200


@app.route('/genders', methods=['GET'])
def get_genders():
    """Get all available genders"""
    return jsonify(GENDERS), 200


@app.route('/attendance-types', methods=['GET'])
def get_attendance_types():
    """Get all available attendance types"""
    return jsonify(ATTENDANCE_TYPES), 200


# ==================== AUTHENTICATION ROUTES ====================

@app.route('/login', methods=['GET', 'POST'])
def login():
    """Login page"""
    if current_user.is_authenticated:
        return redirect(url_for('dashboard'))
    
    if request.method == 'POST':
        email = request.form.get('email')
        password = request.form.get('password')
        
        user = User.query.filter_by(email=email).first()
        
        if user and user.check_password(password):
            login_user(user)
            next_page = request.args.get('next')
            return redirect(next_page) if next_page else redirect(url_for('dashboard'))
        else:
            flash('Invalid email or password', 'danger')
    
    return render_template('login.html')


@app.route('/logout')
@login_required
def logout():
    """Logout"""
    logout_user()
    return redirect(url_for('login'))


@app.route('/dashboard')
@login_required
def dashboard():
    """Admin dashboard"""
    users = User.query.all()
    attendance = Attendance.query.all()
    return render_template('dashboard.html', users=users, attendance=attendance)


# ==================== INITIALIZATION ROUTES ====================

@app.route('/init-db')
def init_db():
    """Initialize the database tables and create default admin"""
    db.create_all()
    
    # Create default admin if not exists
    admin = User.query.filter_by(email='admin@gatetrack.com').first()
    if not admin:
        admin = User(
            username='Admin',
            email='admin@gatetrack.com',
            role='super_admin'
        )
        admin.set_password('admin123')
        db.session.add(admin)
        db.session.commit()
        return jsonify({'message': 'Database initialized with default admin (admin@gatetrack.com / admin123)'}), 200
    
    return jsonify({'message': 'Database initialized successfully'}), 200


@app.route('/')
def root():
    return jsonify({
        'message': 'Welcome to GateTrack API',
        'endpoints': {
            'users': {
                'GET /users': 'Get all users',
                'GET /users/<id>': 'Get single user',
                'POST /users': 'Create user',
                'PUT /users/<id>': 'Update user',
                'DELETE /users/<id>': 'Delete user'
            },
            'attendance': {
                'GET /attendance': 'Get all attendance',
                'GET /attendance/<id>': 'Get single attendance',
                'POST /attendance': 'Create attendance',
                'PUT /attendance/<id>': 'Update attendance',
                'DELETE /attendance/<id>': 'Delete attendance'
            },
            'utility': {
                'GET /users/<id>/attendance': 'Get user attendance',
                'GET /roles': 'Get available roles',
                'GET /genders': 'Get available genders',
                'GET /attendance-types': 'Get attendance types',
                'GET /init-db': 'Initialize database'
            }
        }
    })


if __name__ == "__main__":
    app.run(host='0.0.0.0', debug=True)

