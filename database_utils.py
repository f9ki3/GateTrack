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
    rows = cursor.fetchall()
    
    # Convert Row objects to dicts to allow modification
    from datetime import datetime
    users = []
    for row in rows:
        user_dict = dict(row)
        if user_dict['created_at']:
            try:
                user_dict['created_at'] = datetime.strptime(user_dict['created_at'], '%Y-%m-%d %H:%M:%S')
            except ValueError:
                pass
        users.append(user_dict)
    
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
        ORDER BY id
    ''', (pattern, pattern))
    
    users = cursor.fetchall()
    conn.close()
    return users


def get_paginated_users(page: int = 1, per_page: int = 10, search_term: str = '', role_filter: str = '') -> tuple[list[sqlite3.Row], int]:
    """Get paginated users with optional search and role filter. Returns (users, total_count)."""
    conn = get_connection()
    cursor = conn.cursor()
    
    offset = (page - 1) * per_page
    
    # Base query parts
    where_conditions = []
    params = []
    
    if search_term:
        where_conditions.append('(username LIKE ? OR email LIKE ? OR first_name LIKE ? OR last_name LIKE ?)')
        pattern = f'%{search_term}%'
        params.extend([pattern, pattern, pattern, pattern])
    
    if role_filter:
        where_conditions.append('role = ?')
        params.append(role_filter)
    
    where_clause = 'WHERE ' + ' AND '.join(where_conditions) if where_conditions else ''
    
    # Count total
    count_query = f'SELECT COUNT(*) FROM users {where_clause}'
    cursor.execute(count_query, params)
    total_count = cursor.fetchone()[0]
    
    # Paginated query
    paginated_query = f'''
        SELECT * FROM users 
        {where_clause}
        ORDER BY id
        LIMIT ? OFFSET ?
    '''
    params.extend([per_page, offset])
    cursor.execute(paginated_query, params)
    rows = cursor.fetchall()
    
    # Convert Row objects to dicts to allow modification
    from datetime import datetime
    users = []
    for row in rows:
        user_dict = dict(row)
        if user_dict['created_at']:
            try:
                user_dict['created_at'] = datetime.strptime(user_dict['created_at'], '%Y-%m-%d %H:%M:%S')
            except ValueError:
                pass
        users.append(user_dict)
    
    conn.close()
    return users, total_count


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


# ==================== EXPORT OPERATIONS ====================

def export_users_csv_data(search_term: str = '', role_filter: str = '', limit: int = 10000) -> List[dict]:
    """Get filtered users data for CSV export. Returns list of dicts."""
    conn = get_connection()
    cursor = conn.cursor()
    
    # Base query parts
    where_conditions = []
    params = []
    
    if search_term:
        where_conditions.append('(username LIKE ? OR email LIKE ? OR first_name LIKE ? OR last_name LIKE ?)')
        pattern = f'%{search_term}%'
        params.extend([pattern, pattern, pattern, pattern])
    
    if role_filter:
        where_conditions.append('role = ?')
        params.append(role_filter)
    
    where_clause = 'WHERE ' + ' AND '.join(where_conditions) if where_conditions else ''
    
    # Query with large limit for full export
    query = f'''
        SELECT id, username, email, first_name, last_name, contact, gender, role, rfid, created_at
        FROM users 
        {where_clause}
        ORDER BY id
        LIMIT ?
    '''
    params.append(limit)
    cursor.execute(query, params)
    rows = cursor.fetchall()
    
    # Convert to dicts with string dates
    users = []
    for row in rows:
        user_dict = dict(row)
        # Keep created_at as string for CSV
        users.append(user_dict)
    
    conn.close()
    return users


def export_attendance_csv_data(role_filter: str = '', date_from: str = None, date_to: str = None) -> List[dict]:
    """Get attendance data joined with users for CSV export. Returns list of dicts."""
    conn = get_connection()
    cursor = conn.cursor()
    
    query = '''
        SELECT a.id, a.user_id, a.time_in, a.time_out, a.created_at,
               u.email, u.username, u.first_name, u.last_name, u.role, u.rfid
        FROM attendance a
        LEFT JOIN users u ON a.user_id = u.id
    '''
    params = []
    where_conditions = []
    
    if role_filter:
        where_conditions.append('u.role = ?')
        params.append(role_filter)
    
    if date_from:
        where_conditions.append('date(a.created_at) >= ?')
        params.append(date_from)
    
    if date_to:
        where_conditions.append('date(a.created_at) <= ?')
        params.append(date_to)
    
    if where_conditions:
        query += ' WHERE ' + ' AND '.join(where_conditions)
    
    query += ' ORDER BY a.created_at DESC'
    
    cursor.execute(query, params)
    rows = cursor.fetchall()
    
    # Convert to dicts
    records = [dict(row) for row in rows]
    conn.close()
    return records


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

