#!/usr/bin/env bash
# render_trial2.sh — white background, dark attractor
# Fixed 500x500 GIFs, small parameter oscillation for subtle loops

set -euo pipefail

FRAMES=90
RENDER_SIZE=500    # render and GIF at same size — no scaling, no compression
ITERATIONS=900000
FPS=15
GIF_SCALE=500      # all GIFs identical canvas size

cd "$(dirname "$0")"

if [ ! -x ./attractor ]; then
    echo "Binary not found — running make first"
    make
fi

BG="--bg-r 1 --bg-g 1 --bg-b 1"
FG="--mono-r 0.05 --mono-g 0.05 --mono-b 0.05"

make_gif() {
    local dir=$1 out=$2
    ffmpeg -y -r "$FPS" -i "$dir/%05d.ppm" \
        -vf "fps=${FPS},scale=${GIF_SCALE}:-1:flags=lanczos,\
split[s0][s1];\
[s0]palettegen=max_colors=256:stats_mode=diff[p];\
[s1][p]paletteuse=dither=bayer:bayer_scale=3:diff_mode=rectangle" \
        "$out" 2>/dev/null
    printf "  GIF %-34s %s\n" "$(basename "$out")" \
        "$(numfmt --to=iec --suffix=B "$(stat -c%s "$out")")"
}

run_output() {
    local name=$1; shift
    local outdir="trial2/$name"
    mkdir -p "$outdir"
    echo "=== $name ==="
    ./attractor \
        --save --output "$outdir" \
        --frames     "$FRAMES" \
        --width      "$RENDER_SIZE" \
        --height     "$RENDER_SIZE" \
        --iterations "$ITERATIONS" \
        $BG $FG \
        "$@"
    make_gif "$outdir" "$outdir/animation.gif"
}

mkdir -p trial2

# 1. King's Dream
run_output output1 \
    --type kings_dream --color mono \
    --alpha 0.025 --delta 0.025

# 2. De Jong — default params
run_output output2 \
    --type de_jong --color mono \
    --alpha 0.020 --delta 0.025

# 3. Clifford — sepia tint
run_output output3 \
    --type clifford --color mono \
    --mono-r 0.10 --mono-g 0.06 --mono-b 0.02 \
    --alpha 0.022 --delta 0.025

# 4. De Jong — dense filament params
run_output output4 \
    --type de_jong --color mono \
    --a -2.7 --b -0.09 --c -0.86 --d -2.2 \
    --alpha 0.030 --delta 0.025

# 5. Svensson — blue-gray ink
run_output output5 \
    --type svensson --color mono \
    --mono-r 0.03 --mono-g 0.05 --mono-b 0.12 \
    --alpha 0.025 --delta 0.025

# Collect into animations/
mkdir -p trial2/animations
cp trial2/output1/animation.gif trial2/animations/01_kings_dream.gif
cp trial2/output2/animation.gif trial2/animations/02_de_jong.gif
cp trial2/output3/animation.gif trial2/animations/03_clifford_sepia.gif
cp trial2/output4/animation.gif trial2/animations/04_de_jong_filaments.gif
cp trial2/output5/animation.gif trial2/animations/05_svensson_navy.gif

echo ""
echo "Animations in trial2/animations/:"
ls -lh trial2/animations/
