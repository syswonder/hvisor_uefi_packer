RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
BOLD='\033[1m'
NC='\033[0m' # No Color

# Spinner characters
SPINNER=('⠋' '⠙' '⠹' '⠸' '⠼' '⠴' '⠦' '⠧' '⠇' '⠏')
SPINNER_INDEX=0

# Function to get next spinner character
next_spinner() {
  echo -n "${SPINNER[$SPINNER_INDEX]}"
  SPINNER_INDEX=$(((SPINNER_INDEX + 1) % ${#SPINNER[@]}))
}

# Function to format elapsed time
format_elapsed_time() {
  local minutes="$1"
  local seconds="$2"
  if [ "$minutes" -gt 0 ]; then
    echo -n "${minutes}m${seconds}s"
  else
    echo -n "${seconds}s"
  fi
}

# Function to update status line
update_status() {
  local status="$1"
  local line="$2"
  local minutes="$3"
  local seconds="$4"
  local elapsed_time=$(format_elapsed_time "$minutes" "$seconds")
  # Clear current line and print status with optional line
  if [ -n "$line" ]; then
    # Only style the spinner and status, keep the line output plain
    echo -ne "\r\033[K${BLUE}$(next_spinner)${NC} [${elapsed_time}] ${BOLD}${status}${NC} :: ${line}"
  else
    echo -ne "\r\033[K${BLUE}$(next_spinner)${NC} [${elapsed_time}] ${BOLD}${status}${NC}"
  fi
}

# Function to run command with logging
run_command() {
  local cmd="$1"
  local status="$2"
  local log_file="$3"

  # Start with empty status
  update_status "$status" "" "0" "0"

  # Create log file if it doesn't exist
  touch "$log_file" 2>/dev/null || {
    print_error "Failed to create log file at $log_file"
    return 1
  }

  # Create a named pipe for command output
  local pipe=$(mktemp -u)
  mkfifo "$pipe"

  # Record start time
  local start_time=$(date +%s)

  # Start a background process to read from the pipe and update status
  (
    while IFS= read -r line; do
      # Write to log file
      echo "$line" >>"$log_file"
      # Calculate elapsed time
      local current_time=$(date +%s)
      local elapsed=$((current_time - start_time))
      local minutes=$((elapsed / 60))
      local seconds=$((elapsed % 60))
      # Update status with latest line and elapsed time
      update_status "$status" "$line" "$minutes" "$seconds"
    done <"$pipe"
  ) &
  local reader_pid=$!

  # Run the command, redirecting output to the pipe
  if eval "$cmd" >"$pipe" 2>&1; then
    # Clean up
    rm -f "$pipe"
    wait $reader_pid 2>/dev/null

    # Calculate final elapsed time
    local end_time=$(date +%s)
    local total_elapsed=$((end_time - start_time))
    local total_minutes=$((total_elapsed / 60))
    local total_seconds=$((total_elapsed % 60))
    local total_time=$(format_elapsed_time "$total_minutes" "$total_seconds")

    print_success "$status [${total_time}]"
    return 0
  else
    # Clean up
    rm -f "$pipe"
    wait $reader_pid 2>/dev/null

    # Calculate final elapsed time
    local end_time=$(date +%s)
    local total_elapsed=$((end_time - start_time))
    local total_minutes=$((total_elapsed / 60))
    local total_seconds=$((total_elapsed % 60))
    local total_time=$(format_elapsed_time "$total_minutes" "$total_seconds")

    # On error, show last few lines of log
    echo -e "\n${RED}error:${NC} ${status} failed [${total_time}]"
    if [ -f "$log_file" ]; then
      tail -n 5 "$log_file" | while read -r line; do
        echo -e "  ${line}"
      done
    else
      echo -e "  log file not available"
    fi
    return 1
  fi
}

print_section() {
  local title="$1"
  local step="$2"
  local total="$3"
  echo -ne "${BLUE}==>${NC}${BOLD} ${title} [${step}/${total}]"
}

print_success() {
  local status="$1"
  echo -e "${GREEN}✓${NC}"
}

print_error() {
  echo -e "${RED}✗${NC} ${1}"
}

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Logging system
LOG_DIR="${SCRIPT_DIR}/logs"
mkdir -p "$LOG_DIR" 2>/dev/null || {
  print_error "Failed to create log directory at $LOG_DIR"
  exit 1
}
CURRENT_LOG="${LOG_DIR}/build_$(date +%Y%m%d_%H%M%S).log"

if [ ! -f "$SCRIPT_DIR/.config" ]; then
  print_error ".config file not found in $SCRIPT_DIR"
  exit 1
fi

# Check for zones configuration file
ZONES_CONFIG="$SCRIPT_DIR/zones.txt"
if [ ! -f "$ZONES_CONFIG" ]; then
  print_error "zones.txt not found in $SCRIPT_DIR"
  print_error "Please create zones.txt with format: zone_name entry_point"
  print_error "Example:"
  print_error "linux1 0x90000000c0200000"
  print_error "linux2 0x9000000040200000"
  print_error "linux3 0x9000000060200000"
  exit 1
fi

HVISOR_SRC_DIR=$(grep "^CONFIG_HVISOR_SRC_DIR=" "$SCRIPT_DIR/.config" | cut -d'"' -f2)
HVISOR_LA64_LINUX_DIR=$(grep "^CONFIG_HVISOR_LA64_LINUX_DIR=" "$SCRIPT_DIR/.config" | cut -d'"' -f2)
BUILDROOT_DIR=$(grep "^CONFIG_BUILDROOT_DIR=" "$SCRIPT_DIR/.config" | cut -d'"' -f2)
HVISOR_TOOL_DIR=$(grep "^CONFIG_HVISOR_TOOL_DIR=" "$SCRIPT_DIR/.config" | cut -d'"' -f2)

HVISOR_SRC_DIR=$(realpath "$HVISOR_SRC_DIR")
HVISOR_LA64_LINUX_DIR=$(realpath "$HVISOR_LA64_LINUX_DIR")
BUILDROOT_DIR=$(realpath "$BUILDROOT_DIR")
HVISOR_TOOL_DIR=$(realpath "$HVISOR_TOOL_DIR")

for dir in "$HVISOR_SRC_DIR" "$HVISOR_LA64_LINUX_DIR" "$BUILDROOT_DIR" "$HVISOR_TOOL_DIR"; do
  if [ ! -d "$dir" ]; then
    print_error "Directory not found: $dir"
    exit 1
  fi
done

HVISOR_LA64_LINUX_DIR_FULL_PATH="$HVISOR_LA64_LINUX_DIR"

if [ ! -f "$HVISOR_LA64_LINUX_DIR_FULL_PATH/chosen_root" ]; then
  print_error "CHOSEN_ROOT file not found at $HVISOR_LA64_LINUX_DIR_FULL_PATH/chosen_root"
  exit 1
fi

CHOSEN_ROOT=$(cat "$HVISOR_LA64_LINUX_DIR_FULL_PATH/chosen_root")
if [ -z "$CHOSEN_ROOT" ]; then
  print_error "CHOSEN_ROOT is not set in $HVISOR_LA64_LINUX_DIR_FULL_PATH/chosen_root"
  exit 1
fi

# Count number of zones for progress tracking
NUM_ZONES=$(wc -l <"$ZONES_CONFIG")
echo "Number of zones: $NUM_ZONES"
# dump info of zones.txt
echo "Zones config:"
cat "$ZONES_CONFIG"

TOTAL_STEPS=$((6 + NUM_ZONES))
CURRENT_STEP=0

unset LD_LIBRARY_PATH

CURRENT_STEP=$((CURRENT_STEP + 1))

run_command "cd \"$HVISOR_TOOL_DIR\" && make ARCH=loongarch KDIR=\"$HVISOR_LA64_LINUX_DIR_FULL_PATH/linux-$CHOSEN_ROOT\" all" \
  "Building hvisor kernel module and cmd tool" \
  "$CURRENT_LOG"

CURRENT_STEP=$((CURRENT_STEP + 1))
run_command "cp \"$HVISOR_TOOL_DIR/driver/hvisor.ko\" \"$BUILDROOT_DIR/board/loongson/ls3a5000/rootfs_ramdisk_overlay/tool/hvisor.ko\" && \
             cp \"$HVISOR_TOOL_DIR/tools/hvisor\" \"$BUILDROOT_DIR/board/loongson/ls3a5000/rootfs_ramdisk_overlay/tool/hvisor\"" \
  "Copying hvisor files" \
  "$CURRENT_LOG"

CURRENT_STEP=$((CURRENT_STEP + 1))
run_command "cd $HVISOR_LA64_LINUX_DIR && ./build nonroot_setup" \
  "Building nonroot rootfs" \
  "$CURRENT_LOG"

CURRENT_STEP=$((CURRENT_STEP + 1))
run_command "cd \"$HVISOR_LA64_LINUX_DIR\" && ./build def nonroot" \
  "Selecting nonroot kernels defconfig" \
  "$CURRENT_LOG"

# Create nonroot directory in buildroot
mkdir -p "$BUILDROOT_DIR/board/loongson/ls3a5000/rootfs_ramdisk_overlay/tool/nonroot"

# Read and process each zone from the configuration file
while IFS=' ' read -r zone_name entry_point; do
  # Skip empty lines and comments
  [[ -z "$zone_name" || "$zone_name" =~ ^# ]] && continue

  CURRENT_STEP=$((CURRENT_STEP + 1))

  run_command "cd \"$HVISOR_LA64_LINUX_DIR\" && ./build zone nonroot \"$zone_name\" \"$entry_point\"" \
    "Building $zone_name zone" \
    "$CURRENT_LOG"

  run_command "cp \"$HVISOR_LA64_LINUX_DIR/target/nonroot-$zone_name/vmlinux-$zone_name.bin\" \
                 \"$BUILDROOT_DIR/board/loongson/ls3a5000/rootfs_ramdisk_overlay/tool/nonroot/vmlinux-$zone_name.bin\" && \
                 cp \"$HVISOR_LA64_LINUX_DIR/target/nonroot-$zone_name/build_timestamp.txt\" \
                 \"$BUILDROOT_DIR/board/loongson/ls3a5000/rootfs_ramdisk_overlay/tool/nonroot/${zone_name}_build_timestamp.txt\"" \
    "Copying $zone_name files" \
    "$CURRENT_LOG"

done <"$ZONES_CONFIG"

CURRENT_STEP=$((CURRENT_STEP + 1))
run_command "cd \"$BUILDROOT_DIR\" && make -j12" \
  "Building buildroot" \
  "$CURRENT_LOG"

CURRENT_STEP=$((CURRENT_STEP + 1))
run_command "cd \"$HVISOR_LA64_LINUX_DIR\" && ./build def root && ./build kernel root" \
  "Building root kernel" \
  "$CURRENT_LOG"

echo -e "\nBuild log saved to: ${BLUE}$CURRENT_LOG${NC}"
