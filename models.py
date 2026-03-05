from flask_sqlalchemy import SQLAlchemy
from flask_login import UserMixin
from flask_bcrypt import Bcrypt
from datetime import datetime

db = SQLAlchemy()
bcrypt = Bcrypt()

# Role choices
ROLES = ['super_admin', 'teacher', 'student', 'guest']

# Gender choices
GENDERS = ['male', 'female', 'other']

# Attendance type choices
ATTENDANCE_TYPES = ['present', 'absent', 'late']


class User(db.Model, UserMixin):
    __tablename__ = 'users'
    
    id = db.Column(db.Integer, primary_key=True)
    username = db.Column(db.String(80), nullable=False)
    email = db.Column(db.String(120), nullable=False, unique=True)
    password = db.Column(db.String(200), nullable=False)
    contact = db.Column(db.String(20))
    address = db.Column(db.String(200))
    gender = db.Column(db.String(10))
    role = db.Column(db.String(20), nullable=False, default='guest')
    created_at = db.Column(db.DateTime, default=datetime.utcnow)
    
    # Relationship with Attendance
    attendances = db.relationship('Attendance', backref='user', lazy=True)
    
    def set_password(self, password):
        self.password = bcrypt.generate_password_hash(password).decode('utf-8')
    
    def check_password(self, password):
        return bcrypt.check_password_hash(self.password, password)
    
    def to_dict(self):
        return {
            'id': self.id,
            'username': self.username,
            'email': self.email,
            'contact': self.contact,
            'address': self.address,
            'gender': self.gender,
            'role': self.role,
            'created_at': self.created_at.isoformat() if self.created_at else None
        }


class Attendance(db.Model):
    __tablename__ = 'attendance'
    
    id = db.Column(db.Integer, primary_key=True)
    user_id = db.Column(db.Integer, db.ForeignKey('users.id'), nullable=False)
    time_in = db.Column(db.DateTime)
    time_out = db.Column(db.DateTime)
    type = db.Column(db.String(20), default='present')
    time = db.Column(db.DateTime, default=datetime.utcnow)
    created_at = db.Column(db.DateTime, default=datetime.utcnow)
    
    def to_dict(self):
        return {
            'id': self.id,
            'user_id': self.user_id,
            'time_in': self.time_in.isoformat() if self.time_in else None,
            'time_out': self.time_out.isoformat() if self.time_out else None,
            'type': self.type,
            'time': self.time.isoformat() if self.time else None,
            'created_at': self.created_at.isoformat() if self.created_at else None
        }

