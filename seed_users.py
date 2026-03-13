"""
Seed script to add initial users to the GateTrack database.
"""

from datetime import datetime
import sqlite3
import random

DB_PATH = 'instance/gatetrack.db'

def get_connection():
    conn = sqlite3.connect(DB_PATH)
    conn.row_factory = sqlite3.Row
    return conn

def clear_existing_users():
    """Clear all existing users from the database."""
    conn = get_connection()
    cursor = conn.cursor()
    cursor.execute('DELETE FROM users')
    conn.commit()
    conn.close()
    print("Cleared existing users from database")

def generate_rfid():
    """Generate a random RFID value."""
    # Generate a 10-character hex RFID
    return ''.join(random.choices('0123456789ABCDEF', k=10))

def insert_user(username, email, password, role, rfid=None, contact=None, gender=None):
    conn = get_connection()
    cursor = conn.cursor()
    
    created_at = datetime.now().strftime('%Y-%m-%d %H:%M:%S')
    
    # For non-teacher and non-super_admin roles, set username and password to NULL
    if role not in ['teacher', 'super_admin']:
        username = None
        password = None
    
    cursor.execute('''
        INSERT INTO users (username, email, password, contact, gender, role, rfid, created_at)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?)
    ''', (username, email, password, contact, gender, role, rfid, created_at))
    
    conn.commit()
    user_id = cursor.lastrowid
    conn.close()
    
    print(f"Inserted {role}: {email} with ID: {user_id}, RFID: {rfid}")
    return user_id

if __name__ == "__main__":
    # Clear existing users first to avoid duplicate errors
    clear_existing_users()
    
    # 1. Create 1 super_admin (must have username and password)
    insert_user(
        username='super_admin',
        email='superadmin@gatetrack.com',
        password='superadmin123',
        role='super_admin',
        rfid=generate_rfid(),
        contact='9999999999',
        gender='male'
    )
    
    # 2. Create 5 teachers (must have username and password)
    teachers = [
        {'username': 'teacher1', 'email': 'teacher1@gatetrack.com', 'password': 'teacher123', 'contact': '1111111111', 'gender': 'male'},
        {'username': 'teacher2', 'email': 'teacher2@gatetrack.com', 'password': 'teacher123', 'contact': '2222222222', 'gender': 'female'},
        {'username': 'teacher3', 'email': 'teacher3@gatetrack.com', 'password': 'teacher123', 'contact': '3333333333', 'gender': 'male'},
        {'username': 'teacher4', 'email': 'teacher4@gatetrack.com', 'password': 'teacher123', 'contact': '4444444444', 'gender': 'female'},
        {'username': 'teacher5', 'email': 'teacher5@gatetrack.com', 'password': 'teacher123', 'contact': '5555555555', 'gender': 'male'},
    ]
    
    for teacher in teachers:
        insert_user(
            username=teacher['username'],
            email=teacher['email'],
            password=teacher['password'],
            role='teacher',
            rfid=generate_rfid(),
            contact=teacher['contact'],
            gender=teacher['gender']
        )
    

    
    # 4. Create 2 guests (no username, no password - only rfid)
    guests = [
        {'email': 'guest1@gatetrack.com', 'contact': '8888888888', 'gender': 'male'},
        {'email': 'guest2@gatetrack.com', 'contact': '0000000000', 'gender': 'female'},
    ]
    
    for guest in guests:
        insert_user(
            username=None,  # NULL for non-teacher/non-super_admin
            email=guest['email'],
            password=None,  # NULL for non-teacher/non-super_admin
            role='guest',
            rfid=generate_rfid(),
            contact=guest['contact'],
            gender=guest['gender']
        )

    # 5. Create 2 staff (no username, no password - only rfid)
    staff = [
        {'email': 'staff1@gatetrack.com', 'contact': '1111111111', 'gender': 'male'},
        {'email': 'staff2@gatetrack.com', 'contact': '2222222222', 'gender': 'female'},
    ]
    
    for s in staff:
        insert_user(
            username=None,
            email=s['email'],
            password=None,
            role='staff',
            rfid=generate_rfid(),
            contact=s['contact'],
            gender=s['gender']
        )

    # 6. Create 2 technicians (no username, no password - only rfid)
    technicians = [
        {'email': 'tech1@gatetrack.com', 'contact': '3333333333', 'gender': 'male'},
        {'email': 'tech2@gatetrack.com', 'contact': '4444444444', 'gender': 'female'},
    ]
    
    for t in technicians:
        insert_user(
            username=None,
            email=t['email'],
            password=None,
            role='technician',
            rfid=generate_rfid(),
            contact=t['contact'],
            gender=t['gender']
        )
    
    # Verify
    print("\n=== All Users Created ===")
    conn = get_connection()
    cursor = conn.cursor()
    cursor.execute('SELECT id, username, email, password, role, rfid FROM users')
    for row in cursor.fetchall():
        print(f"ID: {row[0]}, Username: {row[1]}, Password: {row[2]}, Role: {row[3]}, RFID: {row[4]}")
    conn.close()
    
    print(f"\nTotal users created: 1 super_admin + 5 teachers + 2 students + 2 guests + 2 staff + 2 technicians = 14 users")

