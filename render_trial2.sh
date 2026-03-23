#!/usr/bin/env bash
# render_trial2.sh — white background, dark attractor, GIFs up to 5MB each

set -euo pipefail

FRAMES=90          # more frames for smoother loops
RENDER_SIZE=800    # larger render for better detail
ITERATIONS=900000
FPS=15
MAX_BYTES=$((5 * 1024 * 1024))  # 5MB

cd "$(dirname "$0")"

if [ ! -x ./attractor ]; then
    echo "Binary not found — running make first"
    make
fi

# White background, near-black point color shared by all outputs
BG="--bg-r 1 --bg-g 1 --bg-b 1"
FG="--mono-r 0.05 --mono-g 0.05 --mono-b 0.05"

make_gif() {
    local dir=$1 out=$2
    for scale in 600 500 420 340 260; do
        ffmpeg -y -r "$FPS" -i "$dir/%05d.ppm" \
            -vf "fps=${FPS},scale=${scale}:-1:flags=lanczos,\
split[s0][s1];\
[s0]palettegen=max_colors=256:stats_mode=diff[p];\
[s1][p]paletteuse=dither=bayer:bayer_scale=3:diff_mode=rectangle" \
            "$out" 2>/dev/null
        local bytes
        bytes=$(stat -c%s "$out")
        if (( bytes <= MAX_BYTES )); then
            printf "  GIF %-30s %s @ %dpx\n" "$(basename "$out")" \
                "$(numfmt --to=iec --suffix=B "$bytes")" "$scale"
            return
        fi
        echo "  $(numfmt --to=iec --suffix=B "$bytes") > 5MB — retrying at ${scale}px..."
    done
    echo "  WARNING: could not get $(basename "$out") under 5MB"
}

run_output() {
    local name=$1; shift
    local outdir="trial2/$name"
    mkdir -p "$outdir"
    echo "=== $name ==="
    ./attractor \
        --save --output "$outdir" \
        --frames   "$FRAMES" \
        --width    "$RENDER_SIZE" \
        --height   "$RENDER_SIZE" \
        --iterations "$ITERATIONS" \
        $BG $FG \
        "$@"
    make_gif "$outdir" "$outdir/animation.gif"
}

mkdir -p trial2

# ----------------------------------------------------------
# 1. King's Dream — classic form in ink-on-paper style
#    Higher alpha for denser, bolder strokes
# ----------------------------------------------------------
run_output output1 \
    --type kings_dream \
    --color mono \
    --alpha 0.025 \
    --delta 0.10

# ----------------------------------------------------------
# 2. De Jong — default params, very clean geometric structure
# ----------------------------------------------------------
run_output output2 \
    --type de_jong \
    --color mono \
    --alpha 0.020 \
    --delta 0.13

# ----------------------------------------------------------
# 3. Clifford — slightly warm gray (sepia feel)
# ----------------------------------------------------------
run_output output3 \
    --type clifford \
    --color mono \
    --mono-r 0.10 --mono-g 0.06 --mono-b 0.02 \
    --alpha 0.022 \
    --delta 0.09

# ----------------------------------------------------------
# 4. De Jong alternate params — dense filament structure
#    (a=-2.7, b=-0.09, c=-0.86, d=-2.2)
# ----------------------------------------------------------
run_output output4 \
    --type de_jong \
    --color mono \
    --a -2.7 --b -0.09 --c -0.86 --d -2.2 \
    --alpha 0.030 \
    --delta 0.07

# ----------------------------------------------------------
# 5. Svensson — cool blue-gray tint on white
# ----------------------------------------------------------
run_output output5 \
    --type svensson \
    --color mono \
    --mono-r 0.03 --mono-g 0.05 --mono-b 0.12 \
    --alpha 0.025 \
    --delta 0.08

echo ""
echo "All done. GIFs:"
for d in trial2/output*/; do
    gif="$d/animation.gif"
    [ -f "$gif" ] && printf "  %s  %s\n" "$(numfmt --to=iec --suffix=B "$(stat -c%s "$gif")")" "$gif"
done
