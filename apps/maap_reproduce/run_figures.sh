#!/bin/bash
# Regenerate the two CPU2026 figures into figures_out/.
# Deps: python3 with pandas, numpy, matplotlib.
set -eu
HERE="$(cd "$(dirname "$(readlink -f "$0")")" && pwd)"
mkdir -p "$HERE/figures_out"
python3 "$HERE/plots/plot_wss.py"
python3 "$HERE/plots/plot_boxplot.py"
echo "Done -> $HERE/figures_out"
