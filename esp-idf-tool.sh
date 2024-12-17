#!/bin/bash

# Set verbose mode if passed as an argument
verbose=false
if [[ "$1" == "--verbose" ]]; then
	verbose=true
elif [[ "$1" == "--help" ]]; then
	echo "Usage: ./esp-idf_cli.sh [--verbose] [--help]"
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

info() { log "$GREEN[INFO] " "$1"; }
warn() { log "$YELLOW[WARN] " "$1"; }
error() { log "$RED[ERROR] " "$1"; }

# Check for necessary dependencies and prompt for installation
check_dependencies() {
	dependencies=("idf.py")

	for dep in "${dependencies[@]}"; do
		if ! command -v "$dep" &>/dev/null; then
			error "$dep is not found. Please ensure ESP-IDF is installed and sourced."
			exit 1
		fi
	done
}

# Validate that we are in or have selected an ESP-IDF project directory
validate_idf_project() {
	if [ ! -f "CMakeLists.txt" ] || [ ! -f "main/CMakeLists.txt" ]; then
		warn "No ESP-IDF project detected in the current directory."
		return 1
	fi
	return 0
}

# Prompt user for input with validation
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

# Initialize or open an ESP-IDF project
init_project() {
	project_dir=$(get_user_input "Enter the directory where you want to initialize/open the ESP-IDF project: ")
	mkdir -p "$project_dir"
	cd "$project_dir" || exit

	if ! validate_idf_project; then
		info "Initializing new ESP-IDF project..."
		project_name=$(get_user_input "Enter the project name: ")
		# Create a new project using the template from ESP-IDF
		if ! idf.py create-project "$project_name"; then
			error "Failed to create new project."
			exit 1
		fi
		cd "$project_name" || exit
		info "New ESP-IDF project '$project_name' initialized."
	else
		info "ESP-IDF project detected."
	fi
}

# Set or change the target (e.g., esp32, esp32s3)
set_target() {
	if ! validate_idf_project; then
		error "Not a valid ESP-IDF project. Please initialize or open a project first."
		return
	fi
	current_target=$(idf.py get-property target | grep 'target:' | awk '{print $2}')
	info "Current target: ${current_target:-None}"
	new_target=$(get_user_input "Enter the new target (e.g. esp32, esp32s3): ")
	if ! idf.py set-target "$new_target"; then
		error "Failed to set target."
		return 1
	fi
	info "Target set to $new_target"
}

# Build the ESP-IDF project
build_project() {
	if ! validate_idf_project; then
		warn "Project not valid. Attempting to build anyway..."
	fi
	info "Building the ESP-IDF project..."
	if ! idf.py build; then
		error "Build failed."
		exit 1
	fi
	info "Build successful."
}

# Flash the firmware to the device
flash_firmware() {
	if ! validate_idf_project; then
		error "Not a valid ESP-IDF project. Cannot flash."
		return
	fi

	info "Flashing the firmware to the device..."
	if ! idf.py flash; then
		error "Flash failed."
		exit 1
	fi
}

# Open the serial monitor
open_serial_monitor() {
	if ! validate_idf_project; then
		error "Not a valid ESP-IDF project. Cannot open monitor."
		return
	fi

	baud_rate=$(get_user_input "Enter the baud rate for the serial monitor (default 115200): ")
	baud_rate="${baud_rate:-115200}"
	info "Opening the serial monitor at baud rate: $baud_rate..."
	if ! idf.py -B build monitor --monitor-baud "$baud_rate"; then
		error "Failed to open serial monitor."
		exit 1
	fi
}

# Clean the project (idf.py clean removes build artifacts)
clean_project() {
	if ! validate_idf_project; then
		error "Not a valid ESP-IDF project. Cannot clean."
		return
	fi
	info "Cleaning the project..."
	if ! idf.py clean; then
		error "Clean failed."
		exit 1
	fi
	info "Project cleaned."
}

# Fullclean the project (more thorough cleanup)
full_clean_project() {
	if ! validate_idf_project; then
		error "Not a valid ESP-IDF project. Cannot full clean."
		return
	fi
	info "Performing full clean..."
	if ! idf.py fullclean; then
		error "Full clean failed."
		exit 1
	fi
	info "Project fully cleaned."
}

# Run tests (ESP-IDF has a testing framework)
run_tests() {
	if ! validate_idf_project; then
		error "Not a valid ESP-IDF project. Cannot run tests."
		return
	fi

	info "Running tests for the ESP-IDF project..."
	# This assumes you have test code and CMake configured for testing.
	if ! idf.py test; then
		error "Tests failed."
		exit 1
	fi
}

# Install project dependencies (components)
install_dependencies() {
	if ! validate_idf_project; then
		error "Not a valid ESP-IDF project. Cannot install dependencies."
		return
	fi
	info "Installing project dependencies..."
	if ! idf.py install; then
		error "Failed to install dependencies."
		exit 1
	fi
	info "Dependencies installed."
}

# Manage configuration using menuconfig
menuconfig() {
	if ! validate_idf_project; then
		error "Not a valid ESP-IDF project. Cannot run menuconfig."
		return
	fi
	info "Opening menuconfig..."
	if ! idf.py menuconfig; then
		error "menuconfig failed."
		exit 1
	fi
}

# Doctor command can help troubleshoot issues with the environment
run_doctor() {
	info "Running idf.py doctor to troubleshoot environment..."
	if ! idf.py doctor; then
		warn "idf.py doctor encountered issues."
	fi
}

# Update ESP-IDF tools (not a direct `idf.py` command for updates, but we can suggest user actions)
# We'll just inform the user how to update.
update_all() {
	info "To update ESP-IDF and its tools:"
	echo "1. Update ESP-IDF by pulling latest changes (if using git):"
	echo "   cd $IDF_PATH && git pull && git submodule update --init --recursive"
	echo "2. Re-install tools:"
	echo "   idf.py install"
}

# Interactive menu
menu() {
	echo "ESP-IDF CLI Tool"
	echo "1. Initialize/Open ESP-IDF project"
	echo "2. Set/Change Target"
	echo "3. Build the project"
	echo "4. Flash firmware"
	echo "5. Open serial monitor"
	echo "6. Clean project"
	echo "7. Full clean project"
	echo "8. Run tests"
	echo "9. Install project dependencies"
	echo "10. Run menuconfig"
	echo "11. Run idf.py doctor"
	echo "12. Update instructions"
	echo "13. Exit"
	choice=$(get_user_input "Choose an option: ")

	case $choice in
	1) init_project ;;
	2) set_target ;;
	3) build_project ;;
	4) flash_firmware ;;
	5) open_serial_monitor ;;
	6) clean_project ;;
	7) full_clean_project ;;
	8) run_tests ;;
	9) install_dependencies ;;
	10) menuconfig ;;
	11) run_doctor ;;
	12) update_all ;;
	13) exit 0 ;;
	*) error "Invalid option" ;;
	esac
}

# Check dependencies before running
check_dependencies

# Main loop
while true; do
	menu
done
