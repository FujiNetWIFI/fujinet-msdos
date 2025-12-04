#!/bin/bash

CONFIG_DIR="/home/thomc/Workspace/fujinet-pc-rs232/build/dist"
VM_DIR="/home/thomc/Vintage/IBM 4860 PCjr"
CONFIG_FILE="$CONFIG_DIR/fnconfig.ini"
VM_CONFIG_FILE="$VM_DIR/86box.cfg"
FUJINET_BIN="$CONFIG_DIR/fujinet"
FUJINET_URL="0.0.0.0:8005"

LOGFILE=$(mktemp)

cleanup() {
    # Kill FujiNet if still running
    if [[ -n "$PID_FN" ]] && kill -0 "$PID_FN" 2>/dev/null; then
        echo "Shutting down FujiNet RS-232 (PID $PID_FN)..."
        kill "$PID_FN"
    fi
    # Optionally: kill 86Box (if not already)
    if [[ -n "$PID_86" ]] && kill -0 "$PID_86" 2>/dev/null; then
        echo "Shutting down 86Box (PID $PID_86)..."
        kill "$PID_86"
    fi
}

# Ensure cleanup on script exit / interrupt
trap cleanup EXIT

echo "Starting 86Box..."

# Launch 86Box in background, logging output
cd "$VM_DIR"
86Box "$VM_CONFIG_FILE" 2>&1 | tee "$LOGFILE" &
PID_86=$!

# Wait for serial_passthrough message
PTS_DEVICE=""
echo -n "Waiting for 86Box to report PTY device..."

for i in {1..200}; do   # ~20 seconds max
    if grep -q "serial_passthrough: Slave side is /dev/pts/" "$LOGFILE"; then
        PTS_DEVICE=$(grep "serial_passthrough: Slave side is" "$LOGFILE" \
            | tail -n1 | awk '{print $NF}')
        echo " found: $PTS_DEVICE"
        break
    fi
    echo -n "."
    sleep 0.1
done

if [[ -z "$PTS_DEVICE" ]]; then
    echo
    echo "ERROR: Could not detect /dev/pts assignment from 86Box."
    exit 1
fi

# Update fnconfig.ini
sed -i "s|^port=.*|port=$PTS_DEVICE|" "$CONFIG_FILE"
echo "Updated $CONFIG_FILE → port=$PTS_DEVICE"

# Wait until PTY appears
echo -n "Waiting for $PTS_DEVICE to appear..."
for i in {1..50}; do
    if [[ -e "$PTS_DEVICE" ]]; then
        echo " OK"
        break
    fi
    echo -n "."
    sleep 0.1
done

if [[ ! -e "$PTS_DEVICE" ]]; then
    echo
    echo "ERROR: $PTS_DEVICE never appeared."
    exit 1
fi

# Start FujiNet RS-232
echo "Starting FujiNet RS-232..."
cd $CONFIG_DIR
"$FUJINET_BIN" -u $FUJINET_URL &
PID_FN=$!

echo "FujiNet RS-232 running with PID $PID_FN"
echo "86Box running with PID $PID_86"
echo "Logfile: $LOGFILE"

# Wait for 86Box to exit; once it does, cleanup() will run via trap.
wait "$PID_86"

# Script will now exit → trap cleanup will kill FujiNet (if running)

