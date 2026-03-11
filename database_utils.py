"""
Database utility examples for pure sqlite3 operations in GateTrack.
This file demonstrates how to INSERT and ACCESS data using sqlite3 directly.
"""

import sqlite3
from datetime import datetime
from typing import Optional, List, Tuple, Any

# Database path
DB_PATH = 'instance/gatetrack.db'


def get_connection() -> sqlite3.Connection:
    """Get a database connection with row factory set to sqlite3.Row."""
    conn = sqlite3.connect(DB_PATH)
    conn.row_factory = sqlite3.Row
    return conn


# ==================== INSERT OPERATIONS ====================

def insert_user(
    username: str,
    email: str,
    password: Optional[str] = None,
    contact: Optional[str] = None,
    gender: Optional[str] = None,
    role: str = 'student',
    rfid: Optional[str] = None,
    first_name: Optional[str] = None,
    last_name: Optional[str] = None
) -> int:
    """Insert a single user into the database. Returns the user ID."""
    conn = get_connection()
    cursor = conn.cursor()
    
    created_at = datetime.now().strftime('%Y-%m-%d %H:%M:%S')
    
    cursor.execute('''
        INSERT INTO users (username, email, password, contact, gender, role, rfid, first_name, last_name, created_at)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
    ''', (username, email, password, contact, gender, role, rfid, first_name, last_name, created_at))
    
    conn.commit()
    user_id = cursor.lastrowid
    conn.close()
    
    print(f"Inserted user: {email} with ID: {user_id}")
    return user_id


def insert_multiple_users(users_data: list[dict]) -> list[int]:
    """Insert multiple users into the database. Returns list of user IDs."""
    conn = get_connection()
    cursor = conn.cursor()
    
    created_at = datetime.now().strftime('%Y-%m-%d %H:%M:%S')
    user_ids = []
    
    for user_data in users_data:
        cursor.execute('''
            INSERT INTO users (username, email, password, contact, gender, role, rfid, created_at)
            VALUES (?, ?, ?, ?, ?, ?, ?, ?)
        ''', (
            user_data.get('username'),
            user_data.get('email'),
            user_data.get('password'),
            user_data.get('contact'),
            user_data.get('gender'),
            user_data.get('role', 'student'),
            user_data.get('rfid'),
            created_at
        ))
        user_ids.append(cursor.lastrowid)
    
    conn.commit()
    conn.close()
    
    print(f"Inserted {len(user_ids)} users")
    return user_ids


# ==================== ACCESS/QUERY OPERATIONS ====================

def get_all_users() -> list[sqlite3.Row]:
    """Get all users from the database."""
    conn = get_connection()
    cursor = conn.cursor()
    
    cursor.execute('SELECT * FROM users')
    users = cursor.fetchall()
    
    conn.close()
    return users


def get_user_by_id(user_id: int) -> Optional[sqlite3.Row]:
    """Get a user by their ID."""
    conn = get_connection()
    cursor = conn.cursor()
    
    cursor.execute('SELECT * FROM users WHERE id = ?', (user_id,))
    user = cursor.fetchone()
    
    conn.close()
    return user


def get_user_by_email(email: str) -> Optional[sqlite3.Row]:
    """Get a user by their email address."""
    conn = get_connection()
    cursor = conn.cursor()
    
    cursor.execute('SELECT * FROM users WHERE email = ?', (email,))
    user = cursor.fetchone()
    
    conn.close()
    return user


def get_users_by_role(role: str) -> list[sqlite3.Row]:
    """Get all users with a specific role."""
    conn = get_connection()
    cursor = conn.cursor()
    
    cursor.execute('SELECT * FROM users WHERE role = ?', (role,))
    users = cursor.fetchall()
    
    conn.close()
    return users


def get_user_count() -> int:
    """Get total count of users."""
    conn = get_connection()
    cursor = conn.cursor()
    
    cursor.execute('SELECT COUNT(*) FROM users')
    count = cursor.fetchone()[0]
    
    conn.close()
    return count


def search_users(search_term: str) -> list[sqlite3.Row]:
    """Search users by username or email."""
    conn = get_connection()
    cursor = conn.cursor()
    
    pattern = f'%{search_term}%'
    cursor.execute('''
        SELECT * FROM users 
        WHERE username LIKE ? 
        OR email LIKE ?
    ''', (pattern, pattern))
    
    users = cursor.fetchall()
    conn.close()
    return users


# ==================== UPDATE OPERATIONS ====================

def update_user(user_id: int, **kwargs) -> bool:
    """Update user fields. Returns True if successful."""
    conn = get_connection()
    cursor = conn.cursor()
    
    # Build dynamic update query
    fields = []
    values = []
    
    allowed_fields = ['username', 'email', 'password', 'contact', 'gender', 'role', 'rfid']
    for key, value in kwargs.items():
        if key in allowed_fields:
            fields.append(f'{key} = ?')
            values.append(value)
    
    if not fields:
        return False
    
    values.append(user_id)
    
    query = f"UPDATE users SET {', '.join(fields)} WHERE id = ?"
    cursor.execute(query, values)
    
    conn.commit()
    success = cursor.rowcount > 0
    conn.close()
    
    if success:
        print(f"Updated user ID: {user_id}")
    return success


# ==================== DELETE OPERATIONS ====================

def delete_user(user_id: int) -> bool:
    """Delete a user by ID. Returns True if successful."""
    conn = get_connection()
    cursor = conn.cursor()
    
    cursor.execute('DELETE FROM users WHERE id = ?', (user_id,))
    
    conn.commit()
    success = cursor.rowcount > 0
    conn.close()
    
    if success:
        print(f"Deleted user ID: {user_id}")
    return success


# ==================== EXAMPLE USAGE ====================

if __name__ == "__main__":
    # Example: Insert a new user
    new_user_data = {
        'username': 'johndoe',
        'email': 'john.doe@example.com',
        'password': 'password123',
        'contact': '1234567890',
        'address': '123 Main St',
        'gender': 'male',
        'role': 'student'
    }
    
    # Insert user
    insert_user(**new_user_data)
    
    # Access/Query examples
    print("\n--- All Users ---")
    for user in get_all_users():
        print(f"{user['id']}: {user['username']} - {user['email']} ({user['role']})")
    
    print("\n--- Teachers ---")
    for teacher in get_users_by_role('teacher'):
        print(f"{teacher['username']} {teacher['email']}")
    
    print("\n--- User Count ---")
    print(f"Total users: {get_user_count()}")
    
    print("\n--- Search 'john' ---")
    for user in search_users('john'):
        print(f"{user['username']} {user['email']}")

