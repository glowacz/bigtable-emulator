#!/bin/bash

# --- Configuration ---
PROJECT="p"
INSTANCE="i"

# Helper function to keep commands clean and consistent
run_cbt() {
    cbt -project="$PROJECT" -instance="$INSTANCE" "$@"
}

echo ">>> Starting Bigtable Emulator Population..."
export BIGTABLE_EMULATOR_HOST=localhost:8888

# ==========================================
# TABLE 1: user-profiles
# Use Case: Storing user data with history
# ==========================================

echo "--- Creating Table 1: user-profiles ---"
run_cbt createtable user-profiles
run_cbt createfamily user-profiles basic_info
# run_cbt createfamily user-profiles preferences

# --- Row 1: user-001 (Demonstrating Multiple Columns) ---
echo "Populating user-001..."
run_cbt set user-profiles user-001 \
    basic_info:name="Alice Smith" \
    basic_info:email="alice@example.com" \
    # preferences:theme="dark" \
    # preferences:notifications="true"

# --- Row 2: user-002 (Demonstrating Multiple Timestamps/Versioning) ---
echo "Populating user-002 with version history..."

# 1. Initial write (Timestamp T1)
run_cbt set user-profiles user-002 basic_info:status="active"
run_cbt set user-profiles user-002 basic_info:location="New York"

# 2. Update write (Timestamp T2) - Simulates a move
# We allow a tiny pause to ensure the emulator clock ticks forward slightly, 
# though cbt usually handles ms precision well.
sleep 1
run_cbt set user-profiles user-002 basic_info:location="London"

# 3. Update write (Timestamp T3) - Simulates another move
sleep 1
run_cbt set user-profiles user-002 basic_info:location="Paris"

# # ==========================================
# # TABLE 2: iot-metrics
# # Use Case: Time-series data
# # ==========================================

# echo "--- Creating Table 2: iot-metrics ---"
# run_cbt createtable iot-metrics
# run_cbt createfamily iot-metrics device_meta
# run_cbt createfamily iot-metrics sensor_data

# # --- Row 1: sensor-A (Demonstrating Garbage Collection Policy) ---
# # We tell the family to only keep the last 1 version to save space
# run_cbt setgcpolicy iot-metrics sensor_data maxversions=1

# echo "Populating sensor-A..."
# run_cbt set iot-metrics sensor-A device_meta:model="v1.0"
# run_cbt set iot-metrics sensor-A sensor_data:temp="20.5"

# # This write will OVERWRITE the previous 'temp' because of the GC policy above
# run_cbt set iot-metrics sensor-A sensor_data:temp="21.0"

# # --- Row 2: sensor-B ---
# echo "Populating sensor-B..."
# run_cbt set iot-metrics sensor-B device_meta:model="v2.0"
# run_cbt set iot-metrics sensor-B sensor_data:temp="18.2"
# run_cbt set iot-metrics sensor-B sensor_data:humidity="60"

echo ">>> Population Complete!"
echo ""
echo "To verify the data and see the timestamps, run the verification commands below."