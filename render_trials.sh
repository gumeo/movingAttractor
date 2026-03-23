#!/usr/bin/env bash
# render_trials.sh — render 5 interesting attractor animations and produce GIFs < 1MB each

set -euo pipefail

FRAMES=60          # frames per animation
RENDER_SIZE=600    # PPM render resolution (square)
ITERATIONS=700000  # iterations per frame (balanced quality/speed at 600x600)
FPS=12             # GIF playback framerate
MAX_BYTES=$((1024 * 1024))  # 1 MB

cd "$(dirname "$0")"

if [ ! -x ./attractor ]; then
    echo "Binary not found — running make first"
    make
fi

# -------------------------------------------------------
# make_gif: create optimised GIF, retry smaller if > 1MB
# Args: ppm_dir gif_path
# -------------------------------------------------------
make_gif() {
    local dir=$1 out=$2
    for scale in 360 300 240 180 120; do
        ffmpeg -y -r "$FPS" -i "$dir/%05d.ppm" \
            -vf "fps=${FPS},scale=${scale}:-1:flags=lanczos,\
split[s0][s1];\
[s0]palettegen=max_colors=128:stats_mode=diff[p];\
[s1][p]paletteuse=dither=bayer:bayer_scale=3:diff_mode=rectangle" \
            "$out" 2>/dev/null
        local bytes
        bytes=$(stat -c%s "$out")
        if (( bytes <= MAX_BYTES )); then
            printf "  GIF %-30s %s @ %dpx\n" "$(basename "$out")" \
                "$(numfmt --to=iec --suffix=B "$bytes")" "$scale"
            return
        fi
        echo "  $(numfmt --to=iec --suffix=B "$bytes") > 1MB — retrying at ${scale}px..."
    done
    echo "  WARNING: could not get $(basename "$out") under 1MB at minimum scale"
}

# -------------------------------------------------------
# run_output: render frames then make GIF
# Args: dir_name [extra attractor flags...]
# -------------------------------------------------------
run_output() {
    local name=$1; shift
    local outdir="trial1/$name"
    mkdir -p "$outdir"
    echo "=== $name ==="
    ./attractor \
        --save --output "$outdir" \
        --frames   "$FRAMES" \
        --width    "$RENDER_SIZE" \
        --height   "$RENDER_SIZE" \
        --iterations "$ITERATIONS" \
        "$@"
    make_gif "$outdir" "$outdir/animation.gif"
}

mkdir -p trial1

# ----------------------------------------------------------
# 1. King's Dream — full hue rainbow, gentle oscillation
# ----------------------------------------------------------
run_output output1 \
    --type kings_dream \
    --color hue_cycle \
    --alpha 0.015 \
    --delta 0.12

# ----------------------------------------------------------
# 2. Clifford — velocity coloring, blue-to-red flow
# ----------------------------------------------------------
run_output output2 \
    --type clifford \
    --color velocity \
    --alpha 0.018 \
    --delta 0.10 \
    --hue-start 0.55 \
    --hue-range 0.50

# ----------------------------------------------------------
# 3. De Jong — angle coloring (direction-based rainbow)
# ----------------------------------------------------------
run_output output3 \
    --type de_jong \
    --color angle \
    --alpha 0.016 \
    --delta 0.14 \
    --hue-start 0.0 \
    --hue-range 1.0

# ----------------------------------------------------------
# 4. Svensson — hue cycle, warm palette, wide oscillation
# ----------------------------------------------------------
run_output output4 \
    --type svensson \
    --color hue_cycle \
    --alpha 0.02 \
    --delta 0.09 \
    --hue-start 0.02 \
    --hue-range 0.45

# ----------------------------------------------------------
# 5. De Jong with striking alternate params — velocity
#    (a=-2.7, b=-0.09, c=-0.86, d=-2.2 — well-known dense pattern)
# ----------------------------------------------------------
run_output output5 \
    --type de_jong \
    --color velocity \
    --a -2.7 --b -0.09 --c -0.86 --d -2.2 \
    --alpha 0.025 \
    --delta 0.06 \
    --hue-start 0.3 \
    --hue-range 0.7

echo ""
echo "All done. GIFs:"
for d in trial1/output*/; do
    gif="$d/animation.gif"
    [ -f "$gif" ] && printf "  %s  %s\n" "$(numfmt --to=iec --suffix=B "$(stat -c%s "$gif")")" "$gif"
done
