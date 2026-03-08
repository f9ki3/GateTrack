from flask import Flask, render_template
from models import db, User, validate_password_required

app = Flask(__name__)

# SQLite Database Configuration
app.config['SQLALCHEMY_DATABASE_URI'] = 'sqlite:///gatetrack.db'
app.config['SQLALCHEMY_TRACK_MODIFICATIONS'] = False

# Initialize the database
db.init_app(app)

# Create database tables
with app.app_context():
    db.create_all()

@app.route('/')
def home():
    return render_template('login.html')

if __name__ == "__main__":
    app.run(host="0.0.0.0", debug=True)
