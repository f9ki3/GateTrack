# Auto-Generate Username for Staff/Technician/Guest

## Steps (Approved Plan):

- [x] 1. Edit app.py: Add auto-generation logic in /add_user route using get_user_count_by_role.
- [x] 2. Skipped optional frontend (backend handles empty username).
- [x] 3. Verified logic (added in app.py).
- [x] 4. Edge cases handled (count+1 incremental, manual if provided).
- [x] 5. Task complete: Backend updated for auto-gen usernames.

## Feedback Iteration Complete

**Original Task**: Backend auto-gen usernames ✅
**Feedback Fix**: Edit username support added ✅

- Backend (`app.py /edit_user`): Handles `username` from form.
- Frontend (`users.html`): Added username input + JS populate/toggle/validation.

**RFID Uniqueness Added** (feedback #3): Prevent duplicate RFID on add_user + flash error. Matches edit_user logic.

**Final Status**: ✅ Auto-gen usernames, ✅ Edit username, ✅ Unique RFID (add/edit).
