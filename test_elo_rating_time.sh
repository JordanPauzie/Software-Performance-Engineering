#!/bin/bash
# Script to simulate games between original and optimized bots using time-based search
# Calculates Elo rating and prints descriptive statistics
#
# Usage: ./test_elo_rating_time.sh [num_games] [time_per_move_ms]
#   num_games: Number of games to play (default: 10)
#   time_per_move_ms: Time per move in milliseconds for both bots (default: 1000ms = 1 second)

set -e
cd "$(dirname "$0")"

BASELINE="./player/leiserchess_original"
OPTIMIZED="./player/leiserchess"
NUM_GAMES=${1:-10}
TIME_MS=${2:-1000}
REFEREE="./tester/referee"

# Check if binaries exist
if [ ! -f "$BASELINE" ]; then
    echo "Error: Baseline bot not found at $BASELINE"
    exit 1
fi

if [ ! -f "$OPTIMIZED" ]; then
    echo "Error: Optimized bot not found at $OPTIMIZED"
    exit 1
fi

if [ ! -f "$REFEREE" ]; then
    echo "Warning: Referee not found at $REFEREE, will use engine status command"
    USE_REFEREE=false
else
    USE_REFEREE=true
fi

echo "=========================================="
echo "Elo Rating Test - Bot Comparison (Time-based)"
echo "=========================================="
echo "Baseline:  $BASELINE"
echo "Optimized: $OPTIMIZED"
echo "Games:     $NUM_GAMES"
echo "Time/move: ${TIME_MS}ms"
echo ""

# Initialize game tracking
declare -a GAME_RESULTS
declare -a GAME_MOVES
declare -a GAME_DURATIONS
BASELINE_WINS=0
OPTIMIZED_WINS=0
DRAWS=0

# Function to get best move from engine using time control
get_bestmove() {
    local engine=$1
    local fen=$2
    local time_ms=$3
    
    # Add timeout to prevent hanging (time_ms * 2 + 10 seconds buffer)
    local timeout_sec=$((time_ms * 2 / 1000 + 10))
    
    {
        echo "uci"
        echo "isready"
        if [ -n "$fen" ] && [ "$fen" != "startpos" ]; then
            echo "position fen $fen"
        else
            echo "position startpos"
        fi
        echo "go time $((time_ms * 50))"  # Convert to centiseconds
        echo "quit"
    } | timeout "$timeout_sec" "$engine" 2>&1 | grep '^bestmove ' | sed 's/^bestmove //' | tail -1
}

# Function to get game status from engine
get_status() {
    local engine=$1
    local fen=$2
    
    {
        echo "uci"
        echo "isready"
        if [ -n "$fen" ] && [ "$fen" != "startpos" ]; then
            echo "position fen $fen"
        else
            echo "position startpos"
        fi
        echo "status"
        echo "quit"
    } | "$engine" 2>&1 | grep '^status ' | sed 's/^status //' | tail -1
}

# Function to apply move using referee and get new position/status
apply_move_referee() {
    local fen=$1
    local move=$2
    
    {
        if [ "$fen" = "startpos" ]; then
            echo "position startpos"
        else
            echo "position fen $fen"
        fi
        echo "move $move"
        echo "printfen"
        echo "status"
        echo "quit"
    } | "$REFEREE" 2>&1
}

# Function to get new FEN after move (using referee)
get_new_fen_referee() {
    local fen=$1
    local move=$2
    
    local output=$(apply_move_referee "$fen" "$move")
    # FEN is printed after printfen command, usually on the last line
    # Look for lines that match FEN pattern (contains slashes and spaces, ends with W or B)
    echo "$output" | grep -E "^[a-zA-Z0-9/+ -]+ [WB]$" | tail -1 | tr -d '\r\n'
}

# Function to get status after move (using referee)
get_status_referee() {
    local fen=$1
    local move=$2
    
    local output=$(apply_move_referee "$fen" "$move")
    local status=$(echo "$output" | grep '^status ' | sed 's/^status //' | tail -1)
    if [ -z "$status" ]; then
        echo "ok"
    else
        echo "$status"
    fi
}

# Function to get current status without making a move (using referee)
get_current_status_referee() {
    local fen=$1
    
    local status=$( {
        if [ "$fen" = "startpos" ]; then
            echo "position startpos"
        else
            echo "position fen $fen"
        fi
        echo "status"
        echo "quit"
    } | "$REFEREE" 2>&1 | grep '^status ' | sed 's/^status //' | tail -1)
    
    if [ -z "$status" ]; then
        echo "ok"
    else
        echo "$status"
    fi
}

# Function to check if move is legal
is_legal_move() {
    local fen=$1
    local move=$2
    
    if [ "$USE_REFEREE" = true ]; then
        local output=$(apply_move_referee "$fen" "$move")
        if echo "$output" | grep -q "info Illegal move"; then
            return 1
        fi
        # Check for "move victims -1" which also indicates illegal move
        if echo "$output" | grep -q "move victims -1"; then
            return 1
        fi
        return 0
    else
        # Use engine's move command
        local output=$(echo -e "uci\nisready\nposition fen $fen\nmove $move\nquit" | "$BASELINE" 2>&1)
        if echo "$output" | grep -q "info Illegal move"; then
            return 1
        fi
        if echo "$output" | grep -q "move victims -1"; then
            return 1
        fi
        return 0
    fi
}

# Play a single game
play_game() {
    local game_num=$1
    local white_engine=$2
    local black_engine=$3
    local white_name=$4
    local black_name=$5
    
    echo "Game $game_num: $white_name (White) vs $black_name (Black)"
    
    local fen="startpos"
    local moves=0
    local max_moves=400  # Adjudicate draw after 400 moves
    local start_time=$(date +%s)
    local status="ok"
    local move_history=""
    
    while [ "$moves" -lt "$max_moves" ]; do
        # Determine whose turn it is
        local current_engine
        local current_name
        if [ $((moves % 2)) -eq 0 ]; then
            current_engine="$white_engine"
            current_name="$white_name"
        else
            current_engine="$black_engine"
            current_name="$black_name"
        fi
        
        # Get best move
        local bestmove=$(get_bestmove "$current_engine" "$fen" "$TIME_MS")
        
        if [ -z "$bestmove" ] || [ "$bestmove" = "null" ] || [ "$bestmove" = "(none)" ]; then
            echo "  Error: No move returned by $current_name (game may be over)"
            # Check status to see if game is actually over
            if [ "$USE_REFEREE" = true ]; then
                status=$(get_current_status_referee "$fen")
            else
                status=$(get_status "$current_engine" "$fen")
            fi
            if echo "$status" | grep -q "mate\|draw"; then
                # Game is over, break and check result
                break
            else
                echo "  Fatal: Cannot continue game"
                GAME_RESULTS[$game_num]="error"
                break
            fi
        fi
        
        # Validate move
        if ! is_legal_move "$fen" "$bestmove"; then
            echo "  Error: Illegal move $bestmove by $current_name"
            break
        fi
        
        # Apply move and get new position
        if [ "$USE_REFEREE" = true ]; then
            local new_status=$(get_status_referee "$fen" "$bestmove")
            local new_fen=$(get_new_fen_referee "$fen" "$bestmove")
            if [ -n "$new_fen" ]; then
                fen="$new_fen"
            fi
            status="$new_status"
        else
            # Fallback: use engine status (less reliable, can't track FEN properly)
            echo "  Warning: Referee not available, game tracking may be inaccurate"
            status=$(get_status "$current_engine" "$fen")
            # Try to update FEN by making the move in the engine
            local temp_output=$(echo -e "uci\nisready\nposition fen $fen\nmove $bestmove\nprintfen\nquit" | "$current_engine" 2>&1)
            local temp_fen=$(echo "$temp_output" | grep -E "^[a-zA-Z0-9/+ -]+$" | head -1 | tr -d '\r\n')
            if [ -n "$temp_fen" ] && [ "$temp_fen" != "$fen" ]; then
                fen="$temp_fen"
            fi
        fi
        
        move_history="$move_history $bestmove"
        moves=$((moves + 1))
        
        if echo "$status" | grep -q "mate - white wins"; then
            echo "  Result: White wins (mate) after $moves moves"
            GAME_RESULTS[$game_num]="white"
            break
        elif echo "$status" | grep -q "mate - black wins"; then
            echo "  Result: Black wins (mate) after $moves moves"
            GAME_RESULTS[$game_num]="black"
            break
        elif echo "$status" | grep -q "draw"; then
            echo "  Result: Draw after $moves moves"
            GAME_RESULTS[$game_num]="draw"
            break
        fi
        
        # Progress indicator
        if [ $((moves % 20)) -eq 0 ]; then
            echo "  Move $moves: $bestmove (status: $status)"
        fi
    done
    
    # Adjudicate draw if max moves reached
    if [ "$moves" -ge "$max_moves" ]; then
        echo "  Result: Draw (adjudicated after $max_moves moves)"
        GAME_RESULTS[$game_num]="draw"
    fi
    
    local end_time=$(date +%s)
    local duration=$((end_time - start_time))
    GAME_MOVES[$game_num]=$moves
    GAME_DURATIONS[$game_num]=$duration
    
    echo "  Game completed in ${duration}s with $moves moves"
    echo ""
}

# Calculate Elo rating using standard Elo formula
# After N games, rating = initial + K * (actual_score - expected_score)
# For equal initial ratings, expected_score = N/2
calculate_elo() {
    local wins=$1
    local losses=$2
    local draws=$3
    local total_games=$4
    local initial_rating=1500  # Start both at 1500
    local K=32  # K-factor
    
    # Calculate actual score (1 point for win, 0.5 for draw, 0 for loss)
    local actual_score=$(echo "scale=4; $wins + $draws * 0.5" | bc)
    
    # Expected score for equal ratings is total_games / 2
    local expected_score=$(echo "scale=4; $total_games / 2" | bc)
    
    # Calculate rating change
    local score_diff=$(echo "scale=4; $actual_score - $expected_score" | bc)
    local rating_change=$(echo "scale=2; $K * $score_diff" | bc)
    local new_rating=$(echo "scale=2; $initial_rating + $rating_change" | bc)
    
    echo "$new_rating"
}

# Play all games
echo "Starting game simulation..."
echo ""

for i in $(seq 1 $NUM_GAMES); do
    # Alternate who plays white
    if [ $((i % 2)) -eq 1 ]; then
        play_game $i "$BASELINE" "$OPTIMIZED" "Baseline" "Optimized"
        
        case "${GAME_RESULTS[$i]}" in
            "white")
                BASELINE_WINS=$((BASELINE_WINS + 1))
                ;;
            "black")
                OPTIMIZED_WINS=$((OPTIMIZED_WINS + 1))
                ;;
            "draw")
                DRAWS=$((DRAWS + 1))
                ;;
            "error")
                echo "  Warning: Game $i ended with error, skipping from statistics"
                ;;
        esac
    else
        play_game $i "$OPTIMIZED" "$BASELINE" "Optimized" "Baseline"
        
        case "${GAME_RESULTS[$i]}" in
            "white")
                OPTIMIZED_WINS=$((OPTIMIZED_WINS + 1))
                ;;
            "black")
                BASELINE_WINS=$((BASELINE_WINS + 1))
                ;;
            "draw")
                DRAWS=$((DRAWS + 1))
                ;;
            "error")
                echo "  Warning: Game $i ended with error, skipping from statistics"
                ;;
        esac
    fi
done

# Count valid games (excluding errors)
VALID_GAMES=$((BASELINE_WINS + OPTIMIZED_WINS + DRAWS))
if [ "$VALID_GAMES" -eq 0 ]; then
    echo "Error: No valid games completed!"
    exit 1
fi

# Calculate statistics
TOTAL_GAMES=$VALID_GAMES
BASELINE_SCORE=$((BASELINE_WINS * 2 + DRAWS))  # 2 points for win, 1 for draw
OPTIMIZED_SCORE=$((OPTIMIZED_WINS * 2 + DRAWS))

# Calculate Elo ratings
# For baseline: wins against optimized, losses when optimized wins
BASELINE_ELO=$(calculate_elo $BASELINE_WINS $OPTIMIZED_WINS $DRAWS $TOTAL_GAMES)
# For optimized: wins when it wins, losses when baseline wins
OPTIMIZED_ELO=$(calculate_elo $OPTIMIZED_WINS $BASELINE_WINS $DRAWS $TOTAL_GAMES)

# Calculate average moves and duration (only for valid games)
TOTAL_MOVES=0
TOTAL_DURATION=0
VALID_COUNT=0
for i in $(seq 1 $NUM_GAMES); do
    if [ "${GAME_RESULTS[$i]}" != "error" ]; then
        TOTAL_MOVES=$((TOTAL_MOVES + ${GAME_MOVES[$i]}))
        TOTAL_DURATION=$((TOTAL_DURATION + ${GAME_DURATIONS[$i]}))
        VALID_COUNT=$((VALID_COUNT + 1))
    fi
done
if [ "$VALID_COUNT" -gt 0 ]; then
    AVG_MOVES=$(echo "scale=2; $TOTAL_MOVES / $VALID_COUNT" | bc)
    AVG_DURATION=$(echo "scale=2; $TOTAL_DURATION / $VALID_COUNT" | bc)
else
    AVG_MOVES=0
    AVG_DURATION=0
fi

# Find min/max moves (only for valid games)
MIN_MOVES=""
MAX_MOVES=""
for i in $(seq 1 $NUM_GAMES); do
    if [ "${GAME_RESULTS[$i]}" != "error" ]; then
        if [ -z "$MIN_MOVES" ]; then
            MIN_MOVES=${GAME_MOVES[$i]}
            MAX_MOVES=${GAME_MOVES[$i]}
        else
            if [ ${GAME_MOVES[$i]} -lt $MIN_MOVES ]; then
                MIN_MOVES=${GAME_MOVES[$i]}
            fi
            if [ ${GAME_MOVES[$i]} -gt $MAX_MOVES ]; then
                MAX_MOVES=${GAME_MOVES[$i]}
            fi
        fi
    fi
done
if [ -z "$MIN_MOVES" ]; then
    MIN_MOVES=0
    MAX_MOVES=0
fi

# Print results
echo "=========================================="
echo "FINAL RESULTS"
echo "=========================================="
echo ""
echo "Game Results:"
echo "  Baseline wins:  $BASELINE_WINS"
echo "  Optimized wins: $OPTIMIZED_WINS"
echo "  Draws:          $DRAWS"
echo "  Total games:    $TOTAL_GAMES"
echo ""
echo "Score (2=win, 1=draw, 0=loss):"
echo "  Baseline:  $BASELINE_SCORE points"
echo "  Optimized: $OPTIMIZED_SCORE points"
echo ""
echo "Elo Ratings (initial: 1500, K-factor: 32):"
echo "  Baseline:  $BASELINE_ELO"
echo "  Optimized: $OPTIMIZED_ELO"
echo "  Difference: $(echo "scale=2; $OPTIMIZED_ELO - $BASELINE_ELO" | bc) Elo points"
echo ""
echo "Game Statistics:"
echo "  Average moves per game: $AVG_MOVES"
echo "  Minimum moves:         $MIN_MOVES"
echo "  Maximum moves:         $MAX_MOVES"
echo "  Average duration:      ${AVG_DURATION}s"
echo "  Total duration:        ${TOTAL_DURATION}s"
echo ""
echo "Win Rates:"
BASELINE_WIN_RATE=$(echo "scale=2; $BASELINE_WINS * 100 / $TOTAL_GAMES" | bc)
OPTIMIZED_WIN_RATE=$(echo "scale=2; $OPTIMIZED_WINS * 100 / $TOTAL_GAMES" | bc)
DRAW_RATE=$(echo "scale=2; $DRAWS * 100 / $TOTAL_GAMES" | bc)
echo "  Baseline win rate:  ${BASELINE_WIN_RATE}%"
echo "  Optimized win rate: ${OPTIMIZED_WIN_RATE}%"
echo "  Draw rate:          ${DRAW_RATE}%"
echo ""
echo "Individual Game Results:"
for i in $(seq 1 $NUM_GAMES); do
    result_str=""
    case "${GAME_RESULTS[$i]}" in
        "white")
            if [ $((i % 2)) -eq 1 ]; then
                result_str="Baseline wins"
            else
                result_str="Optimized wins"
            fi
            ;;
        "black")
            if [ $((i % 2)) -eq 1 ]; then
                result_str="Optimized wins"
            else
                result_str="Baseline wins"
            fi
            ;;
        "draw")
            result_str="Draw"
            ;;
    esac
    echo "  Game $i: $result_str (${GAME_MOVES[$i]} moves, ${GAME_DURATIONS[$i]}s)"
done
echo ""
echo "=========================================="

# Determine winner
if (( $(echo "$OPTIMIZED_ELO > $BASELINE_ELO" | bc -l) )); then
    echo "✓ Optimized bot is stronger (higher Elo)"
    exit 0
elif (( $(echo "$BASELINE_ELO > $OPTIMIZED_ELO" | bc -l) )); then
    echo "✓ Baseline bot is stronger (higher Elo)"
    exit 0
else
    echo "≈ Bots are approximately equal in strength"
    exit 0
fi

