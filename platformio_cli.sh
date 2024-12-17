#!/bin/bash

# Set verbose mode if passed as an argument
verbose=false
if [[ "$1" == "--verbose" ]]; then
	verbose=true
elif [[ "$1" == "--help" ]]; then
	echo "Usage: ./platformio_cli.sh [--verbose] [--help]"
	echo "--verbose   Enable detailed logging"
	echo "--help      Display this help message"
	exit 0
fi

# ANSI color codes for improved output formatting
RESET='\033[0m'
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
BLUE='\033[0;34m'

log() {
	local level="$1"
	local message="$2"
	[[ "$verbose" = true ]] && echo -e "${level}${message}${RESET}"
}

# Improved logging levels
info() { log "$GREEN[INFO] " "$1"; }
warn() { log "$YELLOW[WARN] " "$1"; }
error() { log "$RED[ERROR] " "$1"; }

# Check for necessary dependencies and prompt for installation
check_dependencies() {
	dependencies=("platformio" "jq")

	for dep in "${dependencies[@]}"; do
		if ! command -v "$dep" &>/dev/null; then
			warn "$dep is not installed. Installing..."
			if ! pip install "$dep"; then
				error "Failed to install $dep. Ensure pip is installed."
				exit 1
			fi
		fi
	done
}

# Validate platformio.ini configuration file
validate_platformio_ini() {
	if ! [ -f platformio.ini ]; then
		error "platformio.ini not found!"
		return 1
	fi
	if ! grep -q '\[env:' platformio.ini; then
		error "No valid environments found in platformio.ini."
		return 1
	fi
	return 0
}

# Better Board Detection: Detect connected boards from various paths
detect_board() {
	info "Detecting connected boards..."
	devices=($(pio device list --serial | grep -oP '(?<= - ).*' | awk '{print $1}'))
	if [ ${#devices[@]} -eq 0 ]; then
		error "No connected boards found."
		return 1
	elif [ ${#devices[@]} -eq 1 ]; then
		info "Detected board on: ${devices[0]}"
		device="${devices[0]}"
	else
		echo "Multiple boards detected:"
		for i in "${!devices[@]}"; do
			echo "$i) ${devices[$i]}"
		done
		while true; do
			read -p "Select a board by index: " index
			if [[ "$index" =~ ^[0-9]+$ ]] && [ "$index" -lt "${#devices[@]}" ]; then
				device="${devices[$index]}"
				info "Selected board: $device"
				break
			else
				error "Invalid index. Please select a valid option."
			fi
		done
	fi
	return 0
}

# Function to list environments from platformio.ini
list_environments() {
	validate_platformio_ini || exit 1
	info "Listing environments from platformio.ini..."
	environments=$(grep -oP '\[env:[^\]]+' platformio.ini | cut -d: -f2)
	if [ -z "$environments" ]; then
		error "No environments found in platformio.ini."
		exit 1
	fi
	echo "$environments"
}

# Function to prompt user for input with validation
get_user_input() {
	local prompt="$1"
	local input
	while true; do
		read -p "$prompt" input
		if [ -n "$input" ]; then
			echo "$input"
			break
		else
			error "Input cannot be empty. Please try again."
		fi
	done
}

# Enhanced Environment Selection Logic
select_environment() {
	list_environments
	if detect_board; then
		matching_env=$(echo "$environments" | grep -m1 "$device" || true)
		if [ -n "$matching_env" ]; then
			info "Auto-selecting environment based on detected board: $matching_env"
			selected_env="$matching_env"
		else
			warn "No matching environment found for the detected board. Please select an environment."
			echo "$environments"
			selected_env=$(get_user_input "Enter the environment name: ")
		fi
	else
		warn "No boards detected."
		info "Available environments:"
		echo "$environments"
		selected_env=$(get_user_input "Enter the environment name: ")
	fi
}

# Function to initialize or open a PlatformIO project
init_project() {
	project_dir=$(get_user_input "Enter the directory where you want to initialize the project (or open existing): ")
	mkdir -p "$project_dir"
	cd "$project_dir" || exit
	if [ ! -f platformio.ini ]; then
		info "Initializing new PlatformIO project..."
		pio project init
	else
		info "PlatformIO project detected."
	fi
	list_environments
}

get_platform_for_env() {
	local env_name="$1"
	grep -A 5 "\[env:$env_name\]" platformio.ini | grep "platform" | head -n1 | cut -d= -f2 | tr -d ' '
}

# Function to build the PlatformIO project
build_project() {
	select_environment
	info "Building the PlatformIO project for environment: $selected_env..."
	if ! pio run -e "$selected_env"; then
		error "Build failed."
		exit 1
	fi
	generate_compile_commands

	# If the environment is native, offer to run the binary
	if [ "$(get_platform_for_env "$selected_env")" == "native" ]; then
		ask_to_run_native_binary
	fi
}

ask_to_run_native_binary() {
	local run_binary
	run_binary=$(get_user_input "Do you want to run the native binary now? (y/n): ")
	if [[ "$run_binary" =~ ^[Yy]$ ]]; then
		run_native_binary
	fi
}

run_native_binary() {
	# Prompt user to select an environment
	select_environment

	# Check if the selected environment is native
	if [ "$(get_platform_for_env "$selected_env")" == "native" ]; then
		info "Running the native binary for environment: $selected_env..."
		local binary_path=".pio/build/$selected_env/program"

		# Check if the compiled binary exists
		if [ -f "$binary_path" ]; then
			info "Executing $binary_path"
			"$binary_path"
		else
			error "Compiled binary not found at $binary_path"
			# Ask the user if they want to build the project
			local build_choice
			build_choice=$(get_user_input "Would you like to build the project first? (y/n): ")
			if [[ "$build_choice" =~ ^[Yy]$ ]]; then
				# Build the project
				build_project
				# After building, check again if the binary exists
				if [ -f "$binary_path" ]; then
					info "Executing $binary_path"
					"$binary_path"
				else
					error "Build failed or binary still not found."
					exit 1
				fi
			else
				error "Cannot run the binary without building it first."
			fi
		fi
	else
		error "Selected environment '$selected_env' is not a native environment."
	fi
}

# Function to generate compile_commands.json
generate_compile_commands() {
	info "Generating compile_commands.json for LSP integration..."
	if [ ! -d .pio/build/"$selected_env" ]; then
		error "Build directory for environment $selected_env not found!"
		exit 1
	fi

	compile_db=".pio/build/$selected_env/compile_commands.json"
	if [ -f "$compile_db" ]; then
		warn "compile_commands.json already exists."
	else
		info "Generating compile_commands.json..."
		if ! pio run -t compiledb -e "$selected_env"; then
			error "Failed to generate compile_commands.json."
			exit 1
		fi
	fi
}

# Function to upload firmware
upload_firmware() {
	select_environment
	if [ "$(get_platform_for_env "$selected_env")" == "native" ]; then
		warn "Upload is not applicable for the native environment."
	else
		info "Uploading firmware to the device for environment: $selected_env..."
		if ! pio run -t upload -e "$selected_env"; then
			error "Upload failed."
			exit 1
		fi
	fi
}

# Function to open the serial monitor
open_serial_monitor() {
	local baud_rate
	baud_rate=$(get_user_input "Enter the baud rate for the serial monitor (default 9600): ")
	baud_rate="${baud_rate:-9600}"
	info "Opening the serial monitor at baud rate: $baud_rate..."
	if ! pio device monitor --baud "$baud_rate"; then
		error "Failed to open serial monitor."
		exit 1
	fi
}

# Function to clean the project
clean_project() {
	select_environment
	info "Cleaning the PlatformIO project..."
	if ! pio run --target clean -e "$selected_env"; then
		error "Clean failed."
		exit 1
	fi
}

# Function to run tests
run_tests() {
	select_environment
	info "Running tests for the PlatformIO project..."
	if ! pio test -e "$selected_env"; then
		error "Tests failed."
		exit 1
	fi
}

# Function to install project dependencies
install_dependencies() {
	info "Installing project dependencies..."
	if ! pio lib install; then
		error "Failed to install dependencies."
		exit 1
	fi
}

# Function to manage platforms and libraries
manage_platforms_and_libraries() {
	info "Managing platforms and libraries..."
	echo "1. Install Platform"
	echo "2. Uninstall Platform"
	echo "3. Install Library"
	echo "4. Uninstall Library"
	choice=$(get_user_input "Choose an option: ")

	case $choice in
	1)
		platform=$(get_user_input "Enter the platform you want to install (e.g., espressif32): ")
		if ! pio pkg install -p "$platform"; then
			error "Failed to install platform."
		fi
		;;
	2)
		platform=$(get_user_input "Enter the platform you want to uninstall: ")
		if ! pio pkg uninstall -p "$platform"; then
			error "Failed to uninstall platform."
		fi
		;;
	3)
		library=$(get_user_input "Enter the library you want to install: ")
		if ! pio pkg install -l "$library"; then
			error "Failed to install library."
		fi
		;;
	4)
		library=$(get_user_input "Enter the library you want to uninstall: ")
		if ! pio pkg uninstall -l "$library"; then
			error "Failed to uninstall library."
		fi
		;;
	*)
		error "Invalid option"
		;;
	esac
}

# Update PlatformIO, libraries, and platforms
update_all() {
	info "Updating PlatformIO, libraries, and platforms..."
	if ! pio pkg update; then
		error "Update failed."
		exit 1
	fi
}

# Interactive menu to choose operations
menu() {
	echo "PlatformIO CLI Tool"
	echo "1. Initialize/Open PlatformIO project"
	echo "2. Build the project and generate compile_commands.json"
	echo "3. Upload firmware"
	echo "4. Open serial monitor"
	echo "5. Clean the project"
	echo "6. Run tests"
	echo "7. Install project dependencies"
	echo "8. Manage platforms and libraries"
	echo "9. Update PlatformIO, libraries, and platforms"
	echo "10. Run native binary"
	echo "11. Exit"
	choice=$(get_user_input "Choose an option: ")

	case $choice in
	1) init_project ;;
	2) build_project ;;
	3) upload_firmware ;;
	4) open_serial_monitor ;;
	5) clean_project ;;
	6) run_tests ;;
	7) install_dependencies ;;
	8) manage_platforms_and_libraries ;;
	9) update_all ;;
	10) run_native_binary ;;
	11) exit 0 ;;
	*) error "Invalid option" ;;
	esac
}

# Check dependencies before running the script
check_dependencies

# Main loop to show the interactive menu
while true; do
	menu
done
