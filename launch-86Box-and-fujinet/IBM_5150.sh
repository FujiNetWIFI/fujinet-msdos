#!/bin/bash

CONFIG_DIR="/home/thomc/Workspace/fujinet-pc-rs232/build/dist"
VM_DIR="/home/thomc/Vintage/IBM 5150 PC"
CONFIG_FILE="$CONFIG_DIR/fnconfig.ini"
FUJINET_BIN="$CONFIG_DIR/fujinet"
FUJINET_URL="0.0.0.0:8005"
FUJINET_BOIP_PORT=1987

cleanup() {
    if [[ -n "$PID_86" ]] && kill -0 "$PID_86" 2>/dev/null; then
        echo "Shutting down 86Box (PID $PID_86)..."
        kill "$PID_86"
    fi
    if [[ -n "$PID_FN" ]] && kill -0 "$PID_FN" 2>/dev/null; then
        echo "Shutting down FujiNet RS-232 (PID $PID_FN)..."
        kill "$PID_FN"
    fi
}

trap cleanup EXIT

# Make sure fnconfig.ini has BoIP enabled and pointed at our port. FujiNet
# listens; 86Box's `fujinet` char device (86Box 6.0+, FujiNetWIFI/86Box
# feat/fujinet) connects out to it - no PTY plumbing required.
if grep -q '^\[BOIP\]' "$CONFIG_FILE"; then
    sed -i '/^\[BOIP\]/,/^\[/ s/^enabled=.*/enabled=1/; /^\[BOIP\]/,/^\[/ s/^host=.*/host=127.0.0.1/; /^\[BOIP\]/,/^\[/ s/^port=.*/port='"$FUJINET_BOIP_PORT"'/' "$CONFIG_FILE"
else
    printf '\n[BOIP]\nenabled=1\nhost=127.0.0.1\nport=%s\n' "$FUJINET_BOIP_PORT" >> "$CONFIG_FILE"
fi

echo "Starting FujiNet RS-232 (BoIP listening on 127.0.0.1:${FUJINET_BOIP_PORT})..."
cd "$CONFIG_DIR"
"$FUJINET_BIN" -c "$CONFIG_FILE" -u "$FUJINET_URL" &
PID_FN=$!
echo "FujiNet RS-232 running with PID $PID_FN"

echo "Starting 86Box..."
# Using -P so 86Box resolves relative disk paths correctly from the VM directory.
86Box -P "$VM_DIR" &
PID_86=$!
echo "86Box running with PID $PID_86"

# Wait for 86Box to exit; trap cleanup() will then shut down FujiNet
wait "$PID_86"
