#!/usr/bin/env bash
# render_trial3.sh — Kings Dream & Clifford, 5 parameter regions each
# Navy/ink color variations, tight loops (delta=0.018), fixed 500x500, no resize

set -euo pipefail

FRAMES=90
RENDER_SIZE=1000
ITERATIONS=2000000
FPS=15
GIF_SCALE=1000

cd "$(dirname "$0")"

if [ ! -x ./attractor ]; then
    echo "Binary not found — running make first"
    make
fi

BG="--bg-r 1.0 --bg-g 1.0 --bg-b 1.0"

make_gif() {
    local dir=$1 out=$2
    ffmpeg -y -r "$FPS" -i "$dir/%05d.ppm" \
        -vf "fps=${FPS},scale=${GIF_SCALE}:-1:flags=lanczos,\
split[s0][s1];\
[s0]palettegen=max_colors=256:stats_mode=diff[p];\
[s1][p]paletteuse=dither=bayer:bayer_scale=3:diff_mode=rectangle" \
        "$out" 2>/dev/null
    printf "  %-40s %s\n" "$(basename "$out")" \
        "$(numfmt --to=iec --suffix=B "$(stat -c%s "$out")")"
}

run_output() {
    local name=$1 label=$2; shift 2
    local outdir="trial3/$name"
    mkdir -p "$outdir"
    echo "--- $label"
    ./attractor \
        --save --output "$outdir" \
        --frames     "$FRAMES" \
        --width      "$RENDER_SIZE" \
        --height     "$RENDER_SIZE" \
        --iterations "$ITERATIONS" \
        $BG \
        --delta 0.018 \
        "$@"
    make_gif "$outdir" "$outdir/animation.gif"
}

mkdir -p trial3 trial3/animations

# ================================================================
# KING'S DREAM  (5 parameter regions)
# Formula: x = sin(y*b) + c*sin(x*b)
#          y = sin(x*a) + d*sin(y*a)
# ================================================================
echo "=== King's Dream ==="

# 1 — canonical region, deep navy
run_output kd1 "KD-1  a=-0.967 b=2.880 c=0.580 d=0.813" \
    --type kings_dream \
    --a -0.966918 --b 2.879879 --c 0.580 --d 0.813 \
    --mono-r 0.02 --mono-g 0.04 --mono-b 0.18 \
    --alpha 0.022

# 2 — shifted a/d, keep b near canonical, prussian blue
run_output kd2 "KD-2  a=-0.800 b=2.880 c=0.620 d=0.780" \
    --type kings_dream \
    --a -0.800 --b 2.880 --c 0.620 --d 0.780 \
    --mono-r 0.00 --mono-g 0.05 --mono-b 0.16 \
    --alpha 0.022

# 3 — higher b, tighter spirals, indigo ink
run_output kd3 "KD-3  a=-1.100 b=3.000 c=0.550 d=0.750" \
    --type kings_dream \
    --a -1.100 --b 3.000 --c 0.550 --d 0.750 \
    --mono-r 0.05 --mono-g 0.02 --mono-b 0.20 \
    --alpha 0.022

# 4 — denser filaments, midnight blue
run_output kd4 "KD-4  a=-0.850 b=2.700 c=0.620 d=0.880" \
    --type kings_dream \
    --a -0.850 --b 2.700 --c 0.620 --d 0.880 \
    --mono-r 0.01 --mono-g 0.02 --mono-b 0.14 \
    --alpha 0.022

# 5 — higher b for tighter spiral structure, marine ink
run_output kd5 "KD-5  a=-1.100 b=3.100 c=0.520 d=0.850" \
    --type kings_dream \
    --a -1.100 --b 3.100 --c 0.520 --d 0.850 \
    --mono-r 0.01 --mono-g 0.07 --mono-b 0.18 \
    --alpha 0.022

# ================================================================
# CLIFFORD  (5 parameter regions)
# Formula: x = sin(a*y) + c*cos(a*x)
#          y = sin(b*x) + d*cos(b*y)
# ================================================================
echo "=== Clifford ==="

# 6 — canonical, blue-black
run_output cf1 "CF-1  a=-1.40 b=1.60 c=1.00 d=0.70" \
    --type clifford \
    --a -1.40 --b 1.60 --c 1.00 --d 0.70 \
    --mono-r 0.02 --mono-g 0.03 --mono-b 0.10 \
    --alpha 0.020

# 7 — broader loops, cobalt
run_output cf2 "CF-2  a=-1.70 b=1.80 c=0.90 d=0.50" \
    --type clifford \
    --a -1.70 --b 1.80 --c 0.90 --d 0.50 \
    --mono-r 0.03 --mono-g 0.06 --mono-b 0.22 \
    --alpha 0.020

# 8 — higher b, more fine detail, slate navy
run_output cf3 "CF-3  a=-1.50 b=2.00 c=0.80 d=0.60" \
    --type clifford \
    --a -1.50 --b 2.00 --c 0.80 --d 0.60 \
    --mono-r 0.04 --mono-g 0.07 --mono-b 0.15 \
    --alpha 0.020

# 9 — stronger a, denser core, deep indigo
run_output cf4 "CF-4  a=-2.00 b=1.50 c=0.60 d=0.80" \
    --type clifford \
    --a -2.00 --b 1.50 --c 0.60 --d 0.80 \
    --mono-r 0.04 --mono-g 0.02 --mono-b 0.16 \
    --alpha 0.020

# 10 — between canonical and CF-3, ink teal-blue
run_output cf5 "CF-5  a=-1.300 b=2.150 c=0.950 d=0.600" \
    --type clifford \
    --a -1.300 --b 2.150 --c 0.950 --d 0.600 \
    --mono-r 0.02 --mono-g 0.08 --mono-b 0.16 \
    --alpha 0.020

# ================================================================
# Collect named GIFs
# ================================================================
cp trial3/kd1/animation.gif trial3/animations/01_kings_dream_canonical.gif
cp trial3/kd2/animation.gif trial3/animations/02_kings_dream_soft.gif
cp trial3/kd3/animation.gif trial3/animations/03_kings_dream_spirals.gif
cp trial3/kd4/animation.gif trial3/animations/04_kings_dream_filaments.gif
cp trial3/kd5/animation.gif trial3/animations/05_kings_dream_open.gif
cp trial3/cf1/animation.gif trial3/animations/06_clifford_canonical.gif
cp trial3/cf2/animation.gif trial3/animations/07_clifford_broad.gif
cp trial3/cf3/animation.gif trial3/animations/08_clifford_detail.gif
cp trial3/cf4/animation.gif trial3/animations/09_clifford_dense.gif
cp trial3/cf5/animation.gif trial3/animations/10_clifford_open.gif

echo ""
echo "Animations in trial3/animations/:"
ls -lh trial3/animations/
