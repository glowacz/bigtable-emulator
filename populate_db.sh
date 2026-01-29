#!/bin/bash

# --- Configuration ---
PROJECT="p"
INSTANCE="i"

# Helper function to keep commands clean and consistent
run_cbt() {
    cbt -project="$PROJECT" -instance="$INSTANCE" "$@"
}

echo ">>> Starting Bigtable Emulator Population..."
echo "If it takes too long for you, comment out some parts of this script"
echo "But for the original emulator it is also slow"
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

# ==========================================
# TABLE 2: iot-metrics
# Use Case: Time-series data
# ==========================================

echo "--- Creating Table 2: iot-metrics ---"
run_cbt createtable iot-metrics
run_cbt createfamily iot-metrics device_meta
run_cbt createfamily iot-metrics sensor_data
run_cbt createfamily iot-metrics device_params

echo "Populating sensor-A..."
run_cbt set iot-metrics sensor-A device_meta:model="v1.0"
run_cbt set iot-metrics sensor-A sensor_data:temp="20.5"
run_cbt set iot-metrics sensor-A sensor_data:temp="21.0"
run_cbt set iot-metrics sensor-A device_params:weight="92 g"

# --- Row 2: sensor-B ---
echo "Populating sensor-B..."
run_cbt set iot-metrics sensor-B device_meta:model="v2.0"
run_cbt set iot-metrics sensor-B sensor_data:temp="18.2"
run_cbt set iot-metrics sensor-B sensor_data:humidity="60"
run_cbt set iot-metrics sensor-B device_params:weight="104 g"
run_cbt set iot-metrics sensor-A device_params:height="8 mm"

# ==========================================
# TABLE 3: cars
# ==========================================

echo "--- Creating Table 3: cars ---"
run_cbt createtable cars
run_cbt createfamily cars id
run_cbt createfamily cars params
run_cbt createfamily cars state

echo "Populating car-1..."
run_cbt set cars car-1 params:weight="1400kg"
run_cbt set cars car-1 params:power="97HP"
run_cbt set cars car-1 params:displacement="1.4 l"
run_cbt set cars car-1 id:brand="Toyota"
run_cbt set cars car-1 id:model="Corolla"
run_cbt set cars car-1 id:generation="VIII"
run_cbt set cars car-1 state:mileage="175000km"

echo "Populating car-2..."
run_cbt set cars car-2 params:weight="1800kg"
run_cbt set cars car-2 params:power="140HP"
run_cbt set cars car-2 params:displacement="1.5 l"
run_cbt set cars car-2 id:brand="Hyundai"
run_cbt set cars car-2 id:model="Tuscon"
run_cbt set cars car-2 id:generation="3"
run_cbt set cars car-2 state:mileage="110000km"

echo ">>> Population Complete!"