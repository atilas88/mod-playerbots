#!/usr/bin/env bash
# =============================================================================
#  AzerothCore client-data extraction for mod-playerbots.
#  Run from anywhere — output lands in $SERVER_DATA_DIR.
# =============================================================================
set -euo pipefail

# ─── PATHS ──────────────────────────────────────────────────────────────────
WOW_CLIENT_DATA="/home/dev/wow_client_data"
TOOLS_DIR="/home/dev/azerothcore_installer/_server/azerothcore/env/dist/bin"
SERVER_DATA_DIR="$TOOLS_DIR"

# ─── TOGGLES ────────────────────────────────────────────────────────────────
EXTRACT_DBC_AND_MAPS=true
EXTRACT_VMAPS=true
EXTRACT_MMAPS=true

MMAP_THREADS=0           # 0 = auto-detect (each thread uses 1-2 GB RAM)
MMAP_SINGLE_MAP=""       # e.g. "489" for Warsong Gulch only

# ─── MMAP CONFIG (player-tuned, not NPC-tuned) ──────────────────────────────
#
# ac-stock NPC-tuned defaults for reference:
#   walkableSlopeAngle 60, walkableHeight 6, walkableClimb 6,
#   walkableRadius 2, verticesPerMapEdge 2000, verticesPerTileEdge 80,
#   maxSimplificationError 1.8
#
# Cell-unit values scale with verticesPerMapEdge:
#   cellSize = 533.3333 / (verticesPerMapEdge - 1)
# At our verticesPerMapEdge=3201 → cs ≈ 0.1667 wu, so:
#   walkableHeight 10 = 1.67 wu, walkableClimb 5 = 0.83 wu, walkableRadius 2 = 0.33 wu
#
MMAPS_CONFIG_YAML=$(cat <<'YAML_EOF'
mmapsConfig:
  skipLiquid: false
  skipContinents: false
  skipJunkMaps: false
  skipBattlegrounds: false
  dataDir: "."

  meshSettings:

    # Maximum slope angle (degrees) the navmesh marks walkable. Anything
    # steeper is excluded entirely — bots will route around. WoW client
    # slips players above ~50°, so we set this below the slip threshold:
    # bots only path through reliably-walkable terrain. Going lower makes
    # routing more conservative; higher lets bots try slippery slopes
    # they'll slide off in-game.
    # ac-stock value: 60
    walkableSlopeAngle: 50

    # Minimum ceiling clearance — how tall the "agent" must fit. In CELL
    # UNITS, so it scales with cellSize (set by verticesPerMapEdge below).
    # At our cs ≈ 0.1667 wu: 10 cells = 1.67 wu, which matches the player
    # collision capsule (~1.6 wu). Lowering kicks low-ceiling dungeon
    # corridors out of the navmesh; raising lets bots into spots players
    # can't fit.
    # ac-stock value: 6  (= 1.6 wu at ac-stock cs ≈ 0.2667 — same world footprint)
    walkableHeight: 10

    # Maximum step height (CELL UNITS) the navmesh treats as walkable
    # without a jump. Stairs are typically ~0.5 wu/step, fences ~1.0+ wu.
    # At our cs: 5 cells = 0.83 wu — handles stairs cleanly, blocks fences
    # so bots walk around them like players do. ac-stock 6 (= 1.6 wu)
    # lets NPCs hop fences. Drop to 4 (= 0.67 wu) for stricter
    # player-match — risks failing on chunky-step dungeon staircases.
    # ac-stock value: 6
    walkableClimb: 5

    # Minimum distance from walls (CELL UNITS) the navmesh maintains.
    # Effectively the agent's collision radius. Player capsule radius is
    # ~0.388 wu. At our cs: 2 cells = 0.33 wu — slightly tighter than the
    # player capsule, so paths can run very close to walls. 3 cells
    # (= 0.5 wu) adds a 0.1 wu buffer that reduces poly-edge ambiguity at
    # navmesh seams (root cause of cave→cliff wall-clipping). Trade-off:
    # higher loses access to the very narrowest doorways (rare in 3.3.5a).
    # ac-stock value: 2
    walkableRadius: 2

    # Number of vertices along one edge of the full map's navmesh grid.
    # Determines cellSize via:  cellSize = 533.3333 / (vertices - 1)
    # Higher = finer mesh, more detail, more RAM/time:
    #   2000 → cs ≈ 0.2667 wu  (ac-stock — too coarse for some caves)
    #   2667 → cs ≈ 0.2000 wu  (~1.78× ac-stock cost)
    #   3201 → cs ≈ 0.1667 wu  (ours; ~2.66× ac-stock cost)
    #   4001 → cs ≈ 0.1334 wu  (~4× cost; ~2GB+ extra RAM/thread)
    # ac-stock value: 2000
    verticesPerMapEdge: 3201

    # Number of vertices per tile (sub-grid). Must divide
    # (verticesPerMapEdge - 1) evenly for seamless tile borders. With
    # vertices=3201, (3201-1)/400 = 8 tiles per edge → 64 tiles per map.
    # Sweet spot between two extremes:
    #   - Smaller tiles (80, 200) → many tile boundaries, more path
    #     stitching artifacts, but per-tile loads are tiny.
    #   - Larger tiles (800+) → fewer seams, but each .mmtile is much
    #     bigger so the first-load when a bot enters a fresh region
    #     can be a perceptible hitch.
    # 400 keeps mid-range bot paths (50-150y) almost always in-tile
    # while keeping per-tile load size modest.
    # ac-stock value: 80
    verticesPerTileEdge: 400

    # Tolerance for how much simplified navmesh polygons may deviate from
    # the original geometry. Higher = fewer polygons, faster runtime, but
    # smooths over slope and ledge transitions that affect player-style
    # pathing. Lower = more polys, slower, more accurate. Don't go below
    # 1.0 — poly count explodes without measurable benefit.
    # ac-stock value: 1.8
    maxSimplificationError: 1.3

  mapsOverrides:
    # ─── Continent Z-accuracy overrides ──────────────────────────────
    # Default cellSizeVertical = baseUnitDim ≈ 0.167 wu at our
    # resolution. That's coarse enough that bots visibly hover
    # ~0.1-0.2 wu above textured ground on hilly outdoor terrain.
    # Tightening vertical cells to 0.05 wu (~5 cm) on the continent
    # maps gets bots much closer to the actual surface. Cost: ~30%
    # more RAM/time on these four maps. Indoor maps (dungeons, raids)
    # keep the default — vertical complexity there is quantized
    # (floors, stairs) and finer ch can confuse overlapping levels.
    "0":     # Eastern Kingdoms
      cellSizeVertical: 0.05
    "1":     # Kalimdor
      cellSizeVertical: 0.05
    "530":   # Outland
      cellSizeVertical: 0.05
    "571":   # Northrend
      cellSizeVertical: 0.05

    # ─── Other map-specific fixes ────────────────────────────────────
    "562":   # Blade's Edge Arena — walk on ropes to pillars
      walkableRadius: 0
    "48":    # Blackfathom Deeps — coarse ch separates overlapping levels
      cellSizeVertical: 0.5334
    "529":   # Arathi Basin Lumber Mill — don't drop feared players
      tilesOverrides:
        "30,29":
          walkableSlopeAngle: 45
YAML_EOF
)

# =============================================================================
# ─── DO NOT EDIT BELOW ──────────────────────────────────────────────────────
# =============================================================================

[ -n "$SERVER_DATA_DIR" ] || { echo "SERVER_DATA_DIR is not set"; exit 1; }
[ -n "$WOW_CLIENT_DATA" ] || { echo "WOW_CLIENT_DATA is not set"; exit 1; }
mkdir -p "$SERVER_DATA_DIR"
cd "$SERVER_DATA_DIR"

# ─── SAFETY: source MPQs are READ-ONLY to this script ──────────────────────
# Resolve both paths to canonical form and refuse to run if the output dir
# is inside the source. Combined with safe_rm() below, this script cannot
# touch any file inside WOW_CLIENT_DATA.
SERVER_DATA_DIR_REAL="$(cd "$SERVER_DATA_DIR" && pwd -P)"
WOW_CLIENT_DATA_REAL="$(cd "$WOW_CLIENT_DATA" && pwd -P 2>/dev/null || echo "$WOW_CLIENT_DATA")"
case "$SERVER_DATA_DIR_REAL/" in
    "$WOW_CLIENT_DATA_REAL"/|"$WOW_CLIENT_DATA_REAL"/*)
        echo "ERROR: SERVER_DATA_DIR ($SERVER_DATA_DIR_REAL) is inside WOW_CLIENT_DATA — refusing." >&2
        exit 1
        ;;
esac

# Refuses to remove anything outside SERVER_DATA_DIR. Resolves the parent
# to absolute path so a symlink inside cwd can't trick us into traversing
# into the source. Use this for every cleanup in this script.
safe_rm() {
    local target="$1"
    local parent_abs base
    parent_abs="$(cd "$(dirname -- "$target")" 2>/dev/null && pwd -P)" || return 0
    base="$(basename -- "$target")"
    local abs="$parent_abs/$base"
    case "$abs/" in
        "$SERVER_DATA_DIR_REAL"/|"$SERVER_DATA_DIR_REAL"/*) ;;
        *)
            echo "REFUSING to rm path outside SERVER_DATA_DIR: $target → $abs" >&2
            exit 1 ;;
    esac
    rm -rf -- "$target"
}

[ "$MMAP_THREADS" -eq 0 ] && MMAP_THREADS=$(nproc 2>/dev/null || echo 4)

echo "Working dir : $(pwd)"
echo "Tools dir   : $TOOLS_DIR"
echo "Threads     : $MMAP_THREADS"
echo "Steps       : maps=$EXTRACT_DBC_AND_MAPS  vmaps=$EXTRACT_VMAPS  mmaps=$EXTRACT_MMAPS"
echo

# ─── Symlink Data/ → MPQ source (only when extracting from client) ──────────
if [ "$EXTRACT_DBC_AND_MAPS" = true ] || [ "$EXTRACT_VMAPS" = true ]; then
    has_mpqs() { find "$1" -maxdepth 1 -iname "*.mpq" -print -quit 2>/dev/null | grep -q .; }

    if has_mpqs "$WOW_CLIENT_DATA"; then
        MPQ_DIR="$WOW_CLIENT_DATA"
    elif has_mpqs "$WOW_CLIENT_DATA/Data"; then
        MPQ_DIR="$WOW_CLIENT_DATA/Data"
    else
        echo "ERROR: no .mpq files in $WOW_CLIENT_DATA" >&2
        exit 1
    fi
    MPQ_DIR="$(cd "$MPQ_DIR" && pwd)"

    # Symlink only — refuse to clobber an existing real directory.
    if [ -e Data ] && [ ! -L Data ]; then
        echo "ERROR: Data/ exists in $(pwd) but is not a symlink" >&2
        exit 1
    fi
    ln -sfn "$MPQ_DIR" Data
    echo "Data/ → $MPQ_DIR"
fi

# ─── STEP 1: DBCs + Maps ────────────────────────────────────────────────────
if [ "$EXTRACT_DBC_AND_MAPS" = true ]; then
    echo
    echo "[1/3] Extracting DBCs + Maps..."
    # -e 7 = bitfield MAP(1)|DBC(2)|CAMERA(4) — extract everything.
    # The old "-e 2" was DBC-only and skipped maps + cameras entirely.
    "$TOOLS_DIR/map_extractor" -e 7 -f 0
    if [ ! -d maps ] || [ -z "$(ls -A maps 2>/dev/null)" ]; then
        echo "ERROR: map_extractor finished but maps/ is empty — check its output above" >&2
        exit 1
    fi
fi

# ─── STEP 2: VMaps ──────────────────────────────────────────────────────────
if [ "$EXTRACT_VMAPS" = true ]; then
    echo
    echo "[2/3] Extracting VMaps..."
    "$TOOLS_DIR/vmap4_extractor" -l -d ./Data
    mkdir -p vmaps
    "$TOOLS_DIR/vmap4_assembler" Buildings vmaps
    safe_rm Buildings
    if [ ! -d vmaps ] || [ -z "$(ls -A vmaps 2>/dev/null)" ]; then
        echo "ERROR: vmap4_assembler finished but vmaps/ is empty — check output above" >&2
        exit 1
    fi
fi

# ─── STEP 3: MMaps ──────────────────────────────────────────────────────────
if [ "$EXTRACT_MMAPS" = true ]; then
    if [ ! -d maps ]; then
        echo "ERROR: maps/ missing in $(pwd) — run with EXTRACT_DBC_AND_MAPS=true once" >&2
        exit 1
    fi
    if [ ! -d vmaps ]; then
        echo "ERROR: vmaps/ missing in $(pwd) — run with EXTRACT_VMAPS=true once" >&2
        exit 1
    fi

    echo
    echo "[3/3] Generating MMaps... (do not interrupt)"
    printf '%s\n' "$MMAPS_CONFIG_YAML" > mmaps-config.yaml

    # Wipe any existing tiles before regenerating. Mixed tiles from
    # previous runs (different cellSize / verticesPerTileEdge / etc.)
    # would otherwise be silently kept and mixed with new ones,
    # producing a corrupt navmesh. Clean slate every mmap run.
    safe_rm mmaps
    mkdir -p mmaps

    # Workaround: some mmaps_generator builds write a few tiles to /mmaps
    # via an absolute path. Pre-create it so the writes don't fail, then
    # fold the strays into our local mmaps/ at the end.
    sudo rm -rf /mmaps
    sudo mkdir -p /mmaps && sudo chmod 777 /mmaps

    CMD=("$TOOLS_DIR/mmaps_generator" --config mmaps-config.yaml --threads "$MMAP_THREADS")
    [ -n "$MMAP_SINGLE_MAP" ] && CMD+=("$MMAP_SINGLE_MAP")

    START=$(date +%s)
    "${CMD[@]}"
    ELAPSED=$(( $(date +%s) - START ))

    if compgen -G "/mmaps/*.mmtile" >/dev/null; then
        cp /mmaps/*.mmtile mmaps/ && rm -f /mmaps/*.mmtile
    fi

    echo
    echo "MMap done in $((ELAPSED / 60))m $((ELAPSED % 60))s"
    echo "Tiles: $(ls mmaps/*.mmtile 2>/dev/null | wc -l)"
fi

echo
echo "Done. Restart worldserver to pick up changes."
