#!/bin/bash
# ==========================================
# ryu_ldn_nx Autonomous Debugger
# ==========================================
# Usage: ./debug.sh <IP> [PID]
#
# Fonctionnement autonome:
# 1. Connexion automatique à la Switch
# 2. Détection du process et ASLR
# 3. Chargement des symboles
# 4. Menu interactif de sélection des composants
# 5. Crash handlers toujours actifs
# 6. Auto-continue sur breakpoints, STOP sur crash
#
# ==========================================

# set -e removed: script handles errors explicitly (grep non-match, GDB exit codes)
# pipefail removed: GDB batch often exits non-zero even on success

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Fallback function: drop into interactive GDB when automated setup fails
fallback_interactive() {
    local reason="$1"
    echo ""
    echo -e "${YELLOW}${BOLD}═══════════════════════════════════════════════════════════════${NC}"
    echo -e "${YELLOW}${BOLD}  FALLBACK: Mode interactif${NC}"
    echo -e "${YELLOW}${BOLD}═══════════════════════════════════════════════════════════════${NC}"
    echo -e "${YELLOW}  Raison: ${reason}${NC}"
    echo ""
    echo -e "${CYAN}Options:${NC}"
    echo -e "  ${BOLD}1)${NC} Lancer GDB interactif (connexion manuelle)"
    echo -e "  ${BOLD}2)${NC} Lancer GDB interactif avec connexion auto"
    echo -e "  ${BOLD}3)${NC} Quitter"
    echo ""
    read -p "Choix [2]: " FALLBACK_CHOICE
    FALLBACK_CHOICE="${FALLBACK_CHOICE:-2}"

    case "$FALLBACK_CHOICE" in
        1)
            echo ""
            echo -e "${GREEN}Lancement GDB interactif (connexion manuelle)...${NC}"
            echo -e "${YELLOW}Commandes utiles:${NC}"
            echo "  target extended-remote $SWITCH_IP:$GDB_PORT"
            echo "  attach <PID>"
            echo "  add-symbol-file $ELF_FILE <base_addr>"
            echo ""
            "$GDB" -q -x "$TOOLS_DIR/common.gdb"
            ;;
        2)
            echo ""
            echo -e "${GREEN}Lancement GDB interactif avec connexion auto...${NC}"
            FALLBACK_INIT=$(mktemp)
            cat > "$FALLBACK_INIT" << EOFGDB
set architecture aarch64
set pagination off
set confirm off
set auto-load safe-path /
set breakpoint pending on
set logging file $LOG_FILE
set logging overwrite off
set logging enabled on
target extended-remote $SWITCH_IP:$GDB_PORT
echo \\n[INTERACTIVE] Connecté à $SWITCH_IP:$GDB_PORT\\n
echo [INTERACTIVE] Utilisez 'attach <PID>' puis 'add-symbol-file $ELF_FILE <base>'\\n
echo [INTERACTIVE] Tapez 'ryu-help' pour la liste des commandes\\n
EOFGDB
            if [ -f "$TOOLS_DIR/common.gdb" ]; then
                echo "source $GDB_TOOLS_DIR/common.gdb" >> "$FALLBACK_INIT"
            fi
            "$GDB" -q -x "$FALLBACK_INIT"
            rm -f "$FALLBACK_INIT"
            ;;
        3)
            echo -e "${YELLOW}Au revoir.${NC}"
            exit 0
            ;;
        *)
            echo -e "${YELLOW}Choix invalide. Lancement interactif avec connexion auto...${NC}"
            "$GDB" -q -ex "set auto-load safe-path /" -ex "target extended-remote $SWITCH_IP:$GDB_PORT"
            ;;
    esac

    echo ""
    echo -e "${CYAN}${BOLD}Session GDB terminée.${NC}"
    echo -e "Log: ${YELLOW}$LOG_FILE${NC}"
    exit 0
}

# Local paths (for file listing)
COMPONENTS_DIR="$SCRIPT_DIR/components"
TOOLS_DIR="$SCRIPT_DIR/tools"
PRESETS_DIR="$SCRIPT_DIR/presets"

# Docker/GDB paths (for generated init file)
GDB_COMPONENTS_DIR="/workspace/scripts/debugger/components"
GDB_TOOLS_DIR="/workspace/scripts/debugger/tools"

# Configuration
GDB="/opt/devkitpro/devkitA64/bin/aarch64-none-elf-gdb"
GDB_PORT="22225"
ELF_FILE="/workspace/sysmodule/ryu_ldn_nx.elf"
LOG_DIR="/workspace/debug_logs"
TITLE_ID="4200000000000010"

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
BLUE='\033[0;34m'
MAGENTA='\033[0;35m'
BOLD='\033[1m'
DIM='\033[2m'
NC='\033[0m'

# ==========================================
# Arguments
# ==========================================
SWITCH_IP="${1:-}"
EXPLICIT_PID="${2:-}"

if [ -z "$SWITCH_IP" ]; then
    echo -e "${CYAN}${BOLD}╔══════════════════════════════════════════════════════════════╗${NC}"
    echo -e "${CYAN}${BOLD}║         ryu_ldn_nx Autonomous Debugger                       ║${NC}"
    echo -e "${CYAN}${BOLD}╚══════════════════════════════════════════════════════════════╝${NC}"
    echo ""
    echo "Usage: $0 <SWITCH_IP> [PID]"
    echo ""
    echo "Examples:"
    echo "  $0 192.168.1.25           # Auto-detect process"
    echo "  $0 192.168.1.25 134       # Attach to specific PID"
    echo ""
    exit 1
fi

# ==========================================
# Validation
# ==========================================
if ! command -v "$GDB" &> /dev/null; then
    echo -e "${RED}ERROR: GDB not found at $GDB${NC}"
    echo "Run inside Docker: docker compose run --rm gdb"
    exit 1
fi

if [ ! -f "$ELF_FILE" ]; then
    echo -e "${RED}ERROR: ELF not found at $ELF_FILE${NC}"
    echo "Build first: make -C sysmodule"
    exit 1
fi

mkdir -p "$LOG_DIR"
TIMESTAMP=$(date +"%Y%m%d_%H%M%S")

# Create session subdirectory
SESSION_DIR="$LOG_DIR/session_$TIMESTAMP"
mkdir -p "$SESSION_DIR"

# Define log/init files early so fallback_interactive can reference them
LOG_FILE="$SESSION_DIR/session_$TIMESTAMP.log"
INIT_FILE="$SESSION_DIR/init_$TIMESTAMP.gdb"

# ==========================================
# Banner
# ==========================================
clear
echo ""
echo -e "${CYAN}${BOLD}╔══════════════════════════════════════════════════════════════╗${NC}"
echo -e "${CYAN}${BOLD}║         ryu_ldn_nx Autonomous Debugger                       ║${NC}"
echo -e "${CYAN}${BOLD}╚══════════════════════════════════════════════════════════════╝${NC}"
echo ""

# ==========================================
# Step 1: Connection
# ==========================================
echo -e "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
echo -e "${BLUE}[1/3] Connexion, détection process & ASLR${NC}"
echo -e "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
echo ""
echo -e "Target: ${YELLOW}$SWITCH_IP:$GDB_PORT${NC}"

# ==========================================
# Single GDB session for everything:
# 1. Connect to Switch
# 2. List processes (info os processes)
# 3. Attach to process, get ASLR info (monitor get info, monitor get mappings)
# 4. Detach
# One TCP connection, one batch script.
# ==========================================
DETECT_FILE="$SESSION_DIR/detect_$TIMESTAMP.txt"
GDB_BATCH=$(mktemp)

if [ -n "$EXPLICIT_PID" ]; then
    # PID known — list processes + attach directly
    cat > "$GDB_BATCH" << EOF
set pagination off
set confirm off
set auto-load safe-path /
target extended-remote $SWITCH_IP:$GDB_PORT
info os processes
attach $EXPLICIT_PID
monitor get info
monitor get mappings $EXPLICIT_PID
info registers pc
detach
quit
EOF
else
    # PID unknown — list processes only, we'll parse PID then do a second session for attach
    # (can't attach without knowing PID first)
    cat > "$GDB_BATCH" << EOF
set pagination off
set confirm off
set auto-load safe-path /
target extended-remote $SWITCH_IP:$GDB_PORT
info os processes
quit
EOF
fi

echo -e "${DIM}Connexion en cours...${NC}"
GDB_OUTPUT=$("$GDB" -batch -q -x "$GDB_BATCH" 2>&1 || true)

rm -f "$GDB_BATCH"

# Save raw output for diagnostics
echo "$GDB_OUTPUT" > "$SESSION_DIR/raw_full_output.txt"

# Filter warnings
echo "$GDB_OUTPUT" | grep -Ev "(warning:.*auto-loading|add-auto-load-safe-path|Reading)" > "$DETECT_FILE"

if [ ! -s "$DETECT_FILE" ] && ! echo "$GDB_OUTPUT" | grep -qi "remote\|extended-remote\|target\|process\|pid\|Attached"; then
    echo -e "${RED}ERROR: Cannot connect to $SWITCH_IP:$GDB_PORT${NC}"
    echo -e "${YELLOW}GDB output:${NC}"
    echo "$GDB_OUTPUT" | head -10
    fallback_interactive "Impossible de se connecter à $SWITCH_IP:$GDB_PORT"
fi

echo -e "${GREEN}✓ Connecté${NC}"
echo ""

# ==========================================
# Step 2: Process Detection
# ==========================================
echo -e "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
echo -e "${BLUE}[2/3] Détection du process${NC}"
echo -e "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
echo ""

if [ -n "$EXPLICIT_PID" ]; then
    PROCESS_ID="$EXPLICIT_PID"
    echo -e "PID fourni: ${YELLOW}$PROCESS_ID${NC}"
else
    # Strategy 1: process name
    PROCESS_ID=$(echo "$GDB_OUTPUT" | grep -i "ryu_ldn_nx" | grep -oE '[0-9]+' | head -1)

    # Strategy 2: title ID
    if [ -z "$PROCESS_ID" ]; then
        PROCESS_ID=$(echo "$GDB_OUTPUT" | grep -i "4200000000000010" | grep -oE '[0-9]+' | head -1)
    fi

    # Strategy 3: XML column
    if [ -z "$PROCESS_ID" ]; then
        PROCESS_ID=$(echo "$GDB_OUTPUT" | grep -A1 "ryu_ldn_nx\|4200000000000010" | grep -oE '<column name="pid">[0-9]+' | grep -oE '[0-9]+' | head -1)
    fi

    if [ -z "$PROCESS_ID" ]; then
        echo -e "${RED}ERROR: ryu_ldn_nx non trouvé (TID: 0x$TITLE_ID)${NC}"
        echo ""
        echo "Sortie de 'info os processes':"
        cat "$DETECT_FILE" 2>/dev/null | head -40 || true
        echo ""
        echo "Processus disponibles (PIDs extraits):"
        echo "$GDB_OUTPUT" | grep -oE '<column name="pid">[0-9]+' | grep -oE '[0-9]+' | sort -un | head -20 || echo "  (aucun process trouvé)"
        echo ""
        echo "Astuce: spécifiez le PID manuellement:"
        echo -e "  ${CYAN}docker compose run --rm debugger $SWITCH_IP <PID>${NC}"
        echo ""
        fallback_interactive "Process ryu_ldn_nx non trouvé"
    fi
    echo -e "Process trouvé: ${GREEN}PID $PROCESS_ID${NC} (TID: 0x$TITLE_ID)"
fi
echo ""

# ==========================================
# Step 3: ASLR Detection
# ==========================================
echo -e "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
echo -e "${BLUE}[3/3] Détection ASLR${NC}"
echo -e "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
echo ""

MAPPING_FILE="$SESSION_DIR/mapping_$TIMESTAMP.txt"

# If PID was provided, ASLR info was already collected in the single session.
# Otherwise we need one more session to attach and get mappings.
if [ -n "$EXPLICIT_PID" ]; then
    # Already have everything from the first session
    echo "$GDB_OUTPUT" | grep -Ev "(warning:.*auto-loading|add-auto-load-safe-path|Reading)" > "$MAPPING_FILE"
else
    # Second (and final) GDB session: attach + get ASLR info
    GDB_ATTACH=$(mktemp)
    cat > "$GDB_ATTACH" << EOF
set pagination off
set confirm off
set auto-load safe-path /
target extended-remote $SWITCH_IP:$GDB_PORT
attach $PROCESS_ID
monitor get info
monitor get mappings $PROCESS_ID
info registers pc
detach
quit
EOF

    echo -e "${DIM}Attach au process $PROCESS_ID...${NC}"
    GDB_OUTPUT=$("$GDB" -batch -q -x "$GDB_ATTACH" 2>&1 || true)

    rm -f "$GDB_ATTACH"

    echo "$GDB_OUTPUT" > "$SESSION_DIR/raw_aslr_output.txt"
    echo "$GDB_OUTPUT" | grep -Ev "(warning:.*auto-loading|add-auto-load-safe-path|Reading)" > "$MAPPING_FILE"
fi

# Extract base address from Atmosphère gdbstub output.
# "monitor get info" outputs lines like:
#   Modules:
#     0x5ef2c00000 - 0x5ef2c74fff ryu_ldn_nx.elf
#   Aslr: 0x0008000000 - 0x7fffffffff   <-- range, NOT module base!
#   Heap: 0x580d600000 - 0x5a0d5fffff
# "monitor get mappings" outputs lines like:
#   0x5ef2c00000 - 0x5ef2c74fff r-x Code            ----
#   Mappings (starting from 0x0000000134):
#
# IMPORTANT: The "Aslr:" line gives the overall ASLR virtual address range,
# NOT the module's loaded base address. The correct base comes from the
# "Modules:" section or the first r-x Code mapping.

# Helper: convert hex string (0x...) to decimal for arithmetic
hex_to_dec() {
    printf "%d" "$1" 2>/dev/null || echo "0"
}

BASE_ADDR=""

# Strategy 1: "Modules:" section — first .elf line
# This is the most reliable: Atmosphère outputs the exact module base address.
MODULE_LINE=$(grep -E '\.elf|\.nss' "$MAPPING_FILE" 2>/dev/null | head -1 || true)
if [ -n "$MODULE_LINE" ]; then
    BASE_ADDR=$(echo "$MODULE_LINE" | grep -oE '0x[0-9a-fA-F]+' | head -1 || true)
fi

# Strategy 2: first r-x mapping line (executable code = sysmodule base)
# Falls back if Modules line is missing but Code mapping is present.
if [ -z "$BASE_ADDR" ]; then
    BASE_ADDR=$(grep 'r-x' "$MAPPING_FILE" 2>/dev/null | head -1 | grep -oE '0x[0-9a-fA-F]+' | head -1 || true)
fi

# Strategy 3: "Mappings (starting from 0x...):" header
if [ -z "$BASE_ADDR" ]; then
    BASE_ADDR=$(grep -iE 'Mappings.*starting from' "$MAPPING_FILE" 2>/dev/null | grep -oE '0x[0-9a-fA-F]+' | head -1 || true)
fi

# Strategy 4: first indented hex address in mapping lines
if [ -z "$BASE_ADDR" ]; then
    BASE_ADDR=$(grep -E '^\s+0x[0-9a-fA-F]+\s+-\s+0x' "$MAPPING_FILE" 2>/dev/null | head -1 | grep -oE '0x[0-9a-fA-F]+' | head -1 || true)
fi

# Strategy 5: parse "Aslr:" line from monitor get info
# WARNING: This gives the ASLR range start (e.g. 0x0008000000), NOT the
# module base address. Only use as a last resort.
if [ -z "$BASE_ADDR" ]; then
    ASLR_LINE=$(grep -iE '^\s*Aslr:' "$MAPPING_FILE" 2>/dev/null | head -1 || true)
    if [ -n "$ASLR_LINE" ]; then
        BASE_ADDR=$(echo "$ASLR_LINE" | grep -oE '0x[0-9a-fA-F]+' | head -1 || true)
    fi
fi

# Strategy 6: align PC to 2MB boundary
if [ -z "$BASE_ADDR" ]; then
    PC_RAW=$(grep -E "^pc\s+=" "$MAPPING_FILE" 2>/dev/null | head -1 | awk '{print $NF}' || true)
    if [ -z "$PC_RAW" ]; then
        PC_RAW=$(echo "$GDB_MAP_OUTPUT" | grep -E "^pc\s+" 2>/dev/null | head -1 | awk '{print $NF}' || true)
    fi
    if [ -n "$PC_RAW" ]; then
        PC_DEC=$(hex_to_dec "$PC_RAW")
        if [ "$PC_DEC" != "0" ]; then
            BASE_ADDR=$(printf "0x%x" $(( PC_DEC & ~0x1FFFFF )) 2>/dev/null || true)
        fi
    fi
fi

if [ -z "$BASE_ADDR" ]; then
    echo -e "${RED}ERROR: Cannot detect base address${NC}"
    echo -e "${YELLOW}Mapping file content:${NC}"
    cat "$MAPPING_FILE" 2>/dev/null | head -20 || true
    fallback_interactive "Impossible de détecter l'adresse de base ASLR"
fi

# Adjust base address for add-symbol-file: GDB expects the runtime address
# of the .text section, not the module load base. If the ELF's .text VMA
# is non-zero, we must subtract it from the runtime base so GDB adds the
# correct per-section offsets.
READELF="/opt/devkitpro/devkitA64/bin/aarch64-none-elf-readelf"
TEXT_VMA=$("$READELF" -S "$ELF_FILE" 2>/dev/null | grep '\.text' | head -1 | awk '{print $4}' || true)
if [ -n "$TEXT_VMA" ]; then
    TEXT_VMA_DEC=$(hex_to_dec "0x$TEXT_VMA" 2>/dev/null || echo "0")
    BASE_DEC=$(hex_to_dec "$BASE_ADDR" 2>/dev/null || echo "0")
    if [ "$TEXT_VMA_DEC" != "0" ] && [ "$BASE_DEC" != "0" ]; then
        ADJUSTED=$(printf "0x%x" $(( BASE_DEC - TEXT_VMA_DEC )) 2>/dev/null || true)
        echo -e "ELF .text VMA: ${DIM}0x$TEXT_VMA${NC}"
        echo -e "Adjusted base: ${YELLOW}$ADJUSTED${NC} (runtime base $BASE_ADDR - .text VMA)"
        BASE_ADDR="$ADJUSTED"
    fi
fi

echo -e "Base address: ${GREEN}$BASE_ADDR${NC}"
echo ""

# ==========================================
# Step 4: Component Selection (Interactive)
# ==========================================
echo -e "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
echo -e "${BLUE}[4/5] Sélection des composants à debugger${NC}"
echo -e "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
echo ""

# Count files per component
count_files() {
    local dir="$COMPONENTS_DIR/$1"
    [ -d "$dir" ] && ls -1 "$dir"/*.gdb 2>/dev/null | wc -l || echo "0"
}

echo -e "${YELLOW}Composants disponibles:${NC}"
echo ""
echo -e "  ${BOLD}1.${NC} BSD          - Socket MITM service       ${DIM}($(count_files bsd) fichiers)${NC}"
echo -e "  ${BOLD}2.${NC} LDN          - LDN service handlers      ${DIM}($(count_files ldn) fichiers)${NC}"
echo -e "  ${BOLD}3.${NC} Network      - Network client             ${DIM}($(count_files network) fichiers)${NC}"
echo -e "  ${BOLD}4.${NC} P2P          - P2P proxy server/client   ${DIM}($(count_files p2p) fichiers)${NC}"
echo -e "  ${BOLD}5.${NC} Config       - Configuration management  ${DIM}($(count_files config) fichiers)${NC}"
echo -e "  ${BOLD}6.${NC} Debug        - Logging utilities         ${DIM}($(count_files debug) fichiers)${NC}"
echo ""
echo -e "${CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
echo -e "  ${BOLD}7.${NC} ${GREEN}TOUS${NC}         - Charger tous les composants"
echo -e "  ${BOLD}8.${NC} ${YELLOW}Minimal${NC}      - Lifecycle uniquement (01-*.gdb)"
echo -e "  ${BOLD}9.${NC} ${MAGENTA}Aucun${NC}        - Crash handlers seulement"
echo -e "${CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
echo ""
echo -e "${MAGENTA}${BOLD}Note:${NC} ${MAGENTA}Les crash handlers (SIGSEGV, SIGBUS, SIGILL, SIGFPE, SIGABRT)${NC}"
echo -e "${MAGENTA}      sont TOUJOURS actifs automatiquement.${NC}"
echo ""
read -p "Sélection (ex: 1 3 4 ou 7 pour tout) [7]: " COMPONENT_CHOICE

# Parse selection
SELECTED_COMPONENTS=()
LOAD_MODE="full"  # full or minimal

if [ -z "$COMPONENT_CHOICE" ]; then
    COMPONENT_CHOICE="7"
fi

for choice in $COMPONENT_CHOICE; do
    case $choice in
        1) SELECTED_COMPONENTS+=("bsd") ;;
        2) SELECTED_COMPONENTS+=("ldn") ;;
        3) SELECTED_COMPONENTS+=("network") ;;
        4) SELECTED_COMPONENTS+=("p2p") ;;
        5) SELECTED_COMPONENTS+=("config") ;;
        6) SELECTED_COMPONENTS+=("debug") ;;
        7) SELECTED_COMPONENTS=("bsd" "ldn" "network" "p2p" "config" "debug") ;;
        8)
            SELECTED_COMPONENTS=("bsd" "ldn" "network" "p2p")
            LOAD_MODE="minimal"
            ;;
        9) SELECTED_COMPONENTS=() ;;
        *) echo -e "${YELLOW}Choix inconnu: $choice${NC}" ;;
    esac
done

echo ""
if [ ${#SELECTED_COMPONENTS[@]} -eq 0 ]; then
    echo -e "${YELLOW}Mode: Crash handlers uniquement${NC}"
else
    echo -e "${GREEN}Composants: ${SELECTED_COMPONENTS[*]}${NC}"
    [ "$LOAD_MODE" = "minimal" ] && echo -e "${YELLOW}Mode: Minimal (lifecycle seulement)${NC}"
fi
echo ""

# ==========================================
# Step 5: Generate GDB Init & Launch
# ==========================================
echo -e "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
echo -e "${BLUE}[5/5] Génération et lancement${NC}"
echo -e "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
echo ""

# Generate init file
cat > "$INIT_FILE" << EOF
# ==========================================
# ryu_ldn_nx Autonomous Debugger
# Generated: $(date)
# ==========================================
# Target: $SWITCH_IP:$GDB_PORT
# PID: $PROCESS_ID
# Base: $BASE_ADDR
# Components: ${SELECTED_COMPONENTS[*]:-none}
# Mode: $LOAD_MODE
# ==========================================

set architecture aarch64
set pagination off
set confirm off
set height 0
set width 0
set print pretty on
set breakpoint pending on
set auto-load safe-path /
set print object on

# Signal handling
# SIGINT: stop GDB on Ctrl+C (needed for interactive interruption).
# Remote targets don't propagate SIGINT from keyboard — GDB must catch it.
handle SIGINT stop pass
handle SIGTERM nostop pass
handle SIGPIPE nostop pass

# CRASH SIGNALS - STOP execution for analysis
handle SIGSEGV stop nopass
handle SIGBUS stop nopass
handle SIGILL stop nopass
handle SIGFPE stop nopass
handle SIGABRT stop nopass

# Logging
set logging file $LOG_FILE
set logging overwrite off
set logging enabled on

echo 
echo ══════════════════════════════════════════════════════════════
echo  ryu_ldn_nx Autonomous Debugger
echo ══════════════════════════════════════════════════════════════
echo  Target:  $SWITCH_IP:$GDB_PORT
echo  PID:     $PROCESS_ID
echo  Base:    $BASE_ADDR
echo ══════════════════════════════════════════════════════════════
echo

# Connect and attach
target extended-remote $SWITCH_IP:$GDB_PORT
attach $PROCESS_ID

# Load symbols
echo [SYMBOLS] Loading from $ELF_FILE...
add-symbol-file $ELF_FILE $BASE_ADDR
echo [SYMBOLS] Done\n
echo \n

EOF

# Load common tools
if [ -f "$TOOLS_DIR/common.gdb" ]; then
    echo "source $GDB_TOOLS_DIR/common.gdb" >> "$INIT_FILE"
fi

# ==========================================
# CRASH HANDLERS (Always active)
# ==========================================
cat >> "$INIT_FILE" << 'CRASH_EOF'

# ==========================================
# CRASH HANDLERS (Always Active)
# ==========================================
echo [CRASH] Setting up crash handlers...

catch signal SIGSEGV
commands
    silent
    echo 
    echo ╔══════════════════════════════════════════════════════════════════════════════╗
    echo ║                        SEGMENTATION FAULT (SIGSEGV)                          ║
    echo ╚══════════════════════════════════════════════════════════════════════════════╝
    shell date "+Timestamp: %Y-%m-%d %H:%M:%S"
    echo 
    echo ┌──────────────────────────────────────────────────────────────────────────────┐
    echo │ CRASH LOCATION                                                               │
    echo └──────────────────────────────────────────────────────────────────────────────┘
    printf "  PC  = 0x%lx", $pc
    printf "  LR  = 0x%lx", $x30
    printf "  SP  = 0x%lx", $sp
    printf "  FP  = 0x%lx", $x29
    echo 
    echo ┌──────────────────────────────────────────────────────────────────────────────┐
    echo │ CALL STACK                                                                   │
    echo └──────────────────────────────────────────────────────────────────────────────┘
    backtrace 30
    echo 
    echo ┌──────────────────────────────────────────────────────────────────────────────┐
    echo │ FRAME POINTER CHAIN (manual unwind)                                          │
    echo └──────────────────────────────────────────────────────────────────────────────┘
    echo   [FP] -> [next_FP, return_addr]
    x/2xg $x29
    x/2xg *(void**)$x29
    x/2xg *(void**)(*(void**)$x29)
    x/2xg *(void**)(*(void**)(*(void**)$x29))
    x/2xg *(void**)(*(void**)(*(void**)(*(void**)$x29)))
    echo 
    echo ┌──────────────────────────────────────────────────────────────────────────────┐
    echo │ REGISTERS                                                                    │
    echo └──────────────────────────────────────────────────────────────────────────────┘
    info registers
    echo 
    echo ┌──────────────────────────────────────────────────────────────────────────────┐
    echo │ ARM64 ALIGNMENT CHECK                                                        │
    echo └──────────────────────────────────────────────────────────────────────────────┘
    printf "  x0 = 0x%016lx  (mod4=%d, mod8=%d, mod16=%d)\n", $x0, ($x0 & 3), ($x0 & 7), ($x0 & 0xf)
    printf "  x1 = 0x%016lx  (mod4=%d, mod8=%d, mod16=%d)\n", $x1, ($x1 & 3), ($x1 & 7), ($x1 & 0xf)
    printf "  x2 = 0x%016lx  (mod4=%d, mod8=%d, mod16=%d)\n", $x2, ($x2 & 3), ($x2 & 7), ($x2 & 0xf)
    printf "  SP = 0x%016lx  (mod16=%d)\n", $sp, ($sp & 0xf)
    echo 
    echo ┌──────────────────────────────────────────────────────────────────────────────┐
    echo │ THREADS                                                                      │
    echo └──────────────────────────────────────────────────────────────────────────────┘
    info threads
    echo 
    echo ┌──────────────────────────────────────────────────────────────────────────────┐
    echo │ DISASSEMBLY @ PC                                                             │
    echo └──────────────────────────────────────────────────────────────────────────────┘
    x/16i $pc-32
    echo 
    echo ┌──────────────────────────────────────────────────────────────────────────────┐
    echo │ DISASSEMBLY @ LR                                                             │
    echo └──────────────────────────────────────────────────────────────────────────────────────┘
    x/8i $x30-16
    echo 
    echo ┌──────────────────────────────────────────────────────────────────────────────┐
    echo │ STACK MEMORY                                                                 │
    echo └──────────────────────────────────────────────────────────────────────────────────────┘
    x/32xg $sp
    echo 
    echo ╔══════════════════════════════════════════════════════════════════════════════╗
    echo ║                           SESSION TERMINATED                                 ║
    echo ╚══════════════════════════════════════════════════════════════════════════════╝
    quit
end

catch signal SIGBUS
commands
    silent
    echo 
    echo ╔══════════════════════════════════════════════════════════════════════════════╗
    echo ║             BUS ERROR / DATA ABORT (SIGBUS) — ARM64 ALIGNMENT FAULT           ║
    echo ╚══════════════════════════════════════════════════════════════════════════════╝
    echo NOTE: On Switch ARM64, DABRT 0x101 (data abort from lower EL) arrives as
    echo SIGBUS. Common cause: bswap32/ldr/str on misaligned IPC buffer pointer.
    echo Check SockAddrIn::IsLdnAddress() calls in SendTo/Bind/Connect.
    shell date "+Timestamp: %Y-%m-%d %H:%M:%S"
    echo 
    echo ┌──────────────────────────────────────────────────────────────────────────────┐
    echo │ CRASH LOCATION                                                               │
    echo └──────────────────────────────────────────────────────────────────────────────┘
    printf "  PC  = 0x%016lx\n", $pc
    printf "  LR  = 0x%016lx\n", $x30
    printf "  SP  = 0x%016lx\n", $sp
    printf "  FP  = 0x%016lx\n", $x29
    echo 
    echo ┌──────────────────────────────────────────────────────────────────────────────┐
    echo │ FAULTING INSTRUCTION                                                         │
    echo └──────────────────────────────────────────────────────────────────────────────┘
    x/4i $pc
    echo 
    echo Instructions leading to fault (24 bytes before PC):
    x/8i $pc-24
    echo 
    echo ┌──────────────────────────────────────────────────────────────────────────────┐
    echo │ ARM64 ALIGNMENT ANALYSIS                                                     │
    echo └──────────────────────────────────────────────────────────────────────────────┘
    echo Checking argument registers for misalignment:
    printf "  x0 = 0x%016lx  (mod4=%d, mod8=%d, mod16=%d)\n", $x0, ($x0 & 3), ($x0 & 7), ($x0 & 0xf)
    printf "  x1 = 0x%016lx  (mod4=%d, mod8=%d, mod16=%d)\n", $x1, ($x1 & 3), ($x1 & 7), ($x1 & 0xf)
    printf "  x2 = 0x%016lx  (mod4=%d, mod8=%d, mod16=%d)\n", $x2, ($x2 & 3), ($x2 & 7), ($x2 & 0xf)
    printf "  x3 = 0x%016lx  (mod4=%d, mod8=%d, mod16=%d)\n", $x3, ($x3 & 3), ($x3 & 7), ($x3 & 0xf)
    printf "  SP = 0x%016lx  (mod16=%d, AAPCS64 requires 0)\n", $sp, ($sp & 0xf)
    echo 
    echo If any register holds a misaligned pointer (mod4!=0), this is the
    echo faulting address. Common pattern: reinterpret_cast<SockAddrIn*>
    echo on IPC buffer → IsLdnAddress → __builtin_bswap32
    echo 
    echo ┌──────────────────────────────────────────────────────────────────────────────┐
    echo │ CALL STACK                                                                   │
    echo └──────────────────────────────────────────────────────────────────────────────┘
    backtrace 30
    echo 
    echo ┌──────────────────────────────────────────────────────────────────────────────┐
    echo │ REGISTERS                                                                    │
    echo └──────────────────────────────────────────────────────────────────────────────┘
    info registers
    echo 
    echo ┌──────────────────────────────────────────────────────────────────────────────┐
    echo │ THREADS                                                                      │
    echo └──────────────────────────────────────────────────────────────────────────────┘
    info threads
    echo 
    echo ┌──────────────────────────────────────────────────────────────────────────────┐
    echo │ STACK MEMORY                                                                 │
    echo └──────────────────────────────────────────────────────────────────────────────┘
    x/32xg $sp
    echo 
    echo ╔══════════════════════════════════════════════════════════════════════════════╗
    echo ║                           SESSION TERMINATED                                 ║
    echo ╚══════════════════════════════════════════════════════════════════════════════╝
    quit
end

catch signal SIGILL
commands
    silent
    echo 
    echo ╔══════════════════════════════════════════════════════════════════════════════╗
    echo ║                       ILLEGAL INSTRUCTION (SIGILL)                           ║
    echo ╚══════════════════════════════════════════════════════════════════════════════╝
    shell date "+Timestamp: %Y-%m-%d %H:%M:%S"
    echo 
    echo ┌──────────────────────────────────────────────────────────────────────────────┐
    echo │ CRASH LOCATION                                                               │
    echo └──────────────────────────────────────────────────────────────────────────────┘
    printf "  PC  = 0x%lx", $pc
    printf "  LR  = 0x%lx", $x30
    printf "  SP  = 0x%lx", $sp
    echo 
    echo ┌──────────────────────────────────────────────────────────────────────────────┐
    echo │ CALL STACK                                                                   │
    echo └──────────────────────────────────────────────────────────────────────────────┘
    backtrace 30
    echo 
    echo ┌──────────────────────────────────────────────────────────────────────────────┐
    echo │ DISASSEMBLY @ PC                                                             │
    echo └──────────────────────────────────────────────────────────────────────────────┘
    x/10i $pc-20
    echo 
    echo ┌──────────────────────────────────────────────────────────────────────────────┐
    echo │ REGISTERS                                                                    │
    echo └──────────────────────────────────────────────────────────────────────────────┘
    info registers
    echo 
    echo ╔══════════════════════════════════════════════════════════════════════════════╗
    echo ║                           SESSION TERMINATED                                 ║
    echo ╚══════════════════════════════════════════════════════════════════════════════╝
    quit
end

catch signal SIGFPE
commands
    silent
    echo 
    echo ╔══════════════════════════════════════════════════════════════════════════════╗
    echo ║                     FLOATING POINT EXCEPTION (SIGFPE)                        ║
    echo ╚══════════════════════════════════════════════════════════════════════════════╝
    shell date "+Timestamp: %Y-%m-%d %H:%M:%S"
    echo 
    echo ┌──────────────────────────────────────────────────────────────────────────────┐
    echo │ CRASH LOCATION                                                               │
    echo └──────────────────────────────────────────────────────────────────────────────┘
    printf "  PC  = 0x%lx", $pc
    printf "  LR  = 0x%lx", $x30
    printf "  SP  = 0x%lx", $sp
    echo 
    echo ┌──────────────────────────────────────────────────────────────────────────────┐
    echo │ CALL STACK                                                                   │
    echo └──────────────────────────────────────────────────────────────────────────────┘
    backtrace 30
    echo 
    echo ┌──────────────────────────────────────────────────────────────────────────────┐
    echo │ REGISTERS                                                                    │
    echo └──────────────────────────────────────────────────────────────────────────────┘
    info registers
    echo 
    echo ╔══════════════════════════════════════════════════════════════════════════════╗
    echo ║                           SESSION TERMINATED                                 ║
    echo ╚══════════════════════════════════════════════════════════════════════════════╝
    quit
end

catch signal SIGABRT
commands
    silent
    echo 
    echo ╔══════════════════════════════════════════════════════════════════════════════╗
    echo ║                              ABORT (SIGABRT)                                 ║
    echo ╚══════════════════════════════════════════════════════════════════════════════╝
    shell date "+Timestamp: %Y-%m-%d %H:%M:%S"
    echo 
    echo   Possible causes: assert(), std::terminate(), unhandled exception
    echo 
    echo ┌──────────────────────────────────────────────────────────────────────────────┐
    echo │ CRASH LOCATION                                                               │
    echo └──────────────────────────────────────────────────────────────────────────────┘
    printf "  PC  = 0x%lx", $pc
    printf "  LR  = 0x%lx", $x30
    printf "  SP  = 0x%lx", $sp
    echo 
    echo ┌──────────────────────────────────────────────────────────────────────────────┐
    echo │ CALL STACK                                                                   │
    echo └──────────────────────────────────────────────────────────────────────────────┘
    backtrace 30
    echo 
    echo ┌──────────────────────────────────────────────────────────────────────────────┐
    echo │ REGISTERS                                                                    │
    echo └──────────────────────────────────────────────────────────────────────────────┘
    info registers
    echo 
    echo ╔══════════════════════════════════════════════════════════════════════════════╗
    echo ║                           SESSION TERMINATED                                 ║
    echo ╚══════════════════════════════════════════════════════════════════════════════╝
    quit
end

echo [CRASH] 5 signal handlers ready (SIGSEGV, SIGBUS, SIGILL, SIGFPE, SIGABRT)
echo 

CRASH_EOF

# ==========================================
# Load selected components (from existing files)
# ==========================================
echo "# ==========================================" >> "$INIT_FILE"
echo "# COMPONENT BREAKPOINTS" >> "$INIT_FILE"
echo "# ==========================================" >> "$INIT_FILE"

TOTAL_FILES=0

for component in "${SELECTED_COMPONENTS[@]}"; do
    COMP_DIR="$COMPONENTS_DIR/$component"
    GDB_COMP_DIR="$GDB_COMPONENTS_DIR/$component"

    if [ -d "$COMP_DIR" ]; then
        echo "" >> "$INIT_FILE"
        echo "# --- Component: $component ---" >> "$INIT_FILE"

        if [ "$LOAD_MODE" = "minimal" ]; then
            # Only load 01-*.gdb files (lifecycle)
            for gdb_file in "$COMP_DIR"/01-*.gdb; do
                if [ -f "$gdb_file" ]; then
                    # Get just the filename and use Docker path
                    filename=$(basename "$gdb_file")
                    echo "source $GDB_COMP_DIR/$filename" >> "$INIT_FILE"
                    TOTAL_FILES=$((TOTAL_FILES + 1))
                fi
            done
            echo -e "  ${GREEN}✓${NC} $component ${DIM}(minimal)${NC}"
        else
            # Load all .gdb files in order
            for gdb_file in "$COMP_DIR"/*.gdb; do
                if [ -f "$gdb_file" ]; then
                    # Get just the filename and use Docker path
                    filename=$(basename "$gdb_file")
                    echo "source $GDB_COMP_DIR/$filename" >> "$INIT_FILE"
                    TOTAL_FILES=$((TOTAL_FILES + 1))
                fi
            done
            FILE_COUNT=$(ls -1 "$COMP_DIR"/*.gdb 2>/dev/null | wc -l)
            echo -e "  ${GREEN}✓${NC} $component ${DIM}($FILE_COUNT fichiers)${NC}"
        fi
    fi
done

echo ""
echo -e "Total: ${GREEN}$TOTAL_FILES${NC} fichiers de breakpoints"
echo ""

# ==========================================
# AUTO-CONTINUE: dprintf handles this automatically
# ==========================================
cat >> "$INIT_FILE" << 'MODE_EOF'

echo 
echo [AUTONOMOUS] Mode configured
echo [AUTONOMOUS] dprintf points: log and auto-continue
echo [AUTONOMOUS] Crashes: full dump + EXIT
echo 
echo ════════════════════════════════════════════════════════════════════════════════
echo  DEBUGGER READY - Waiting for events...
echo ════════════════════════════════════════════════════════════════════════════════
echo 

MODE_EOF

# ==========================================
# Session start
# ==========================================
cat >> "$INIT_FILE" << 'START_EOF'

# ==========================================
# SESSION START
# ==========================================
define session-info
    echo 
    echo ══════════════════════════════════════════════════════════════
    echo  Session Ready
    echo ══════════════════════════════════════════════════════════════
    echo 
    echo Commands:
    echo   c / continue    - Resume execution
    echo   bt / backtrace  - Show call stack
    echo   info threads    - List threads
    echo   ryu-help        - Full command reference
    echo   bpl             - List breakpoints
    echo 
    info breakpoints
    echo 
end

session-info

echo 
echo [START] Launching autonomous execution...
echo [START] Press Ctrl+C to interrupt
echo 

continue

START_EOF

echo -e "${GREEN}Init: $INIT_FILE${NC}"
echo -e "${GREEN}Log:  $LOG_FILE${NC}"
echo ""

# ==========================================
# Launch GDB
# ==========================================
echo -e "${MAGENTA}${BOLD}╔══════════════════════════════════════════════════════════════╗${NC}"
echo -e "${MAGENTA}${BOLD}║  AUTONOMOUS MODE                                             ║${NC}"
echo -e "${MAGENTA}${BOLD}║  • Breakpoints: auto-continue + logging                      ║${NC}"
echo -e "${MAGENTA}${BOLD}║  • Crashes: full dump + EXIT                                 ║${NC}"
echo -e "${MAGENTA}${BOLD}║  • Press Ctrl+C to interrupt                                 ║${NC}"
echo -e "${MAGENTA}${BOLD}╚══════════════════════════════════════════════════════════════╝${NC}"
echo ""

# Run GDB directly — no pipe.
# Piping through grep breaks signal delivery: grep absorbs SIGINT
# and GDB never receives the interrupt, making Ctrl+C non-functional.
# GDB output goes to both the terminal and the log file (via set logging).
"$GDB" -q -x "$INIT_FILE"
GDB_EXIT=$?

if [ $GDB_EXIT -ne 0 ]; then
    echo ""
    echo -e "${YELLOW}GDB exited with code $GDB_EXIT${NC}"
    echo -e "${YELLOW}L'init file est disponible pour un retry manuel:${NC}"
    echo -e "  ${CYAN}$GDB -q -x $INIT_FILE${NC}"
    echo ""
    read -p "Ouvrir une session GDB interactive? [Y/n]: " OPEN_INTERACTIVE
    OPEN_INTERACTIVE="${OPEN_INTERACTIVE:-Y}"
    if [ "$OPEN_INTERACTIVE" = "Y" ] || [ "$OPEN_INTERACTIVE" = "y" ]; then
        fallback_interactive "GDB crashed (exit $GDB_EXIT)"
    fi
fi

# ==========================================
# Session Complete
# ==========================================
echo ""
echo -e "${CYAN}${BOLD}══════════════════════════════════════════════════════════════${NC}"
echo -e "${CYAN}${BOLD}  Session terminée${NC}"
echo -e "${CYAN}${BOLD}══════════════════════════════════════════════════════════════${NC}"
echo ""
echo -e "Log:  ${YELLOW}$LOG_FILE${NC}"
echo -e "Init: ${YELLOW}$INIT_FILE${NC}"
echo ""
echo -e "Replay: ${CYAN}$GDB -q -x $INIT_FILE${NC}"
echo ""
