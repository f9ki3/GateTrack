# RFID Scanner - 4 Modes Implementation

## Serial Commands:

| Command | Action                                          |
| ------- | ----------------------------------------------- |
| `1`     | Set mode: Door Only (scan RFID to open door)    |
| `2`     | Set mode: Time In + Door + Light ON             |
| `3`     | Set mode: Time Out + Door + Light OFF           |
| `4`     | **EMERGENCY: Toggle door NOW (no RFID needed)** |

## How Mode 4 Works:

- Press `4` in Serial Monitor → Door immediately toggles
- First press: Opens door (stays open)
- Second press: Closes door
- No RFID scan needed

## Example:

```
> 4
>>> EMERGENCY: Toggling door NOW
>>> EMERGENCY: DOOR OPEN

> 4
>>> EMERGENCY: Toggling door NOW
>>> EMERGENCY: DOOR CLOSE
```

## Connection Error Fix:

- SERVER_IP updated to: 192.168.1.100
- Make sure Flask is running on this IP
