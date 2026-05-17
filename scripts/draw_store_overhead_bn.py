# Usage:
# cd ${ROOT}
# python3 scripts/draw_store_overhead_bn.py \
#   --series 128 log/exp_storage_bn_security128.log \
#   --series 192 log/exp_storage_bn_security192.log \
#   --save figures/store_overhead_bn.pdf

import argparse
import os

import matplotlib.pyplot as plt
from matplotlib import font_manager


def resolve_times_font(font_path=None):
    if font_path:
        if not os.path.exists(font_path):
            raise FileNotFoundError(f"Font file not found: {font_path}")
        font_manager.fontManager.addfont(font_path)
        return font_manager.FontProperties(fname=font_path).get_name()

    candidates = [
        "Times New Roman",
        "Times",
        "Nimbus Roman No9 L",
        "Nimbus Roman",
        "TeX Gyre Termes",
        "STIXGeneral",
        "DejaVu Serif",
    ]
    available = {f.name for f in font_manager.fontManager.ttflist}
    for name in candidates:
        if name in available:
            return name
    return "DejaVu Serif"


def setup_plot_style(font_path=None):
    font_name = resolve_times_font(font_path)
    math_fontset = "stix" if font_name == "STIXGeneral" else "dejavuserif"
    plt.rcParams.update({
        "font.family": "serif",
        "font.serif": [font_name],
        "mathtext.fontset": math_fontset,
        "pdf.fonttype": 42,
        "ps.fonttype": 42,
        "font.size": 28,
        "axes.titlesize": 28,
        "axes.labelsize": 28,
        "legend.fontsize": 21,
        "xtick.labelsize": 24,
        "ytick.labelsize": 24,
        "axes.linewidth": 1.0,
        "lines.linewidth": 1.8,
        "figure.dpi": 300,
        "savefig.dpi": 300,
    })
    return font_name


def parse_size_log(path: str):
    sizes = {}
    required = ["G1_bits", "G2_bits", "GT_bits"]
    with open(path, "r") as f:
        for line in f:
            line = line.strip()
            if not line:
                continue
            for key in required:
                marker = f"{key}:"
                if marker not in line:
                    continue
                value = line.split(marker, 1)[1].strip().split()[0]
                sizes[key] = int(value)
    missing = [key for key in required if key not in sizes]
    if missing:
        raise RuntimeError(f"Missing keys in size log {path}: {missing}")
    return sizes


def bits_to_bytes(bits: int):
    return bits / 8.0


def build_series(g1_bits: int, g2_bits: int, max_group_size: int, step: int):
    group_sizes = list(range(step, max_group_size + 1, step))

    g1_bytes = bits_to_bytes(g1_bits)
    g2_bytes = bits_to_bytes(g2_bits)

    parameter_upk_sizes_mb = []

    for n in group_sizes:
        parameter_upk_bytes = (2 * n) * g1_bytes + (n * n - n) * g2_bytes

        parameter_upk_sizes_mb.append(parameter_upk_bytes / 1024 / 1024)

    return group_sizes, parameter_upk_sizes_mb


def draw_combined(series_data, save_path=None):
    fig, ax = plt.subplots(figsize=(8.6, 6.0))
    colors = ["C0", "C1", "C2", "C3"]

    for i, (label, group_sizes, parameter_upk_mb) in enumerate(series_data):
        c = colors[i % len(colors)]
        ax.plot(
            group_sizes,
            parameter_upk_mb,
            marker="o",
            markersize=4,
            linewidth=2,
            label=f"{label}-bit",
            color=c,
        )

    ax.set_title("Storage Cost of System Parameter/upk", pad=10)
    ax.set_xlabel("N")
    ax.set_ylabel("Size (MB)")
    ax.grid(True, linestyle="--", alpha=0.5)
    ax.legend(
        loc="upper left",
        bbox_to_anchor=(0.02, 0.98),
        borderaxespad=0.0,
        frameon=True,
        facecolor="white",
        edgecolor="0.8",
        framealpha=0.92,
        handlelength=1.4,
        handletextpad=0.5,
        labelspacing=0.2,
        columnspacing=0.8,
    )
    ax.margins(x=0.04, y=0.18)

    fig.subplots_adjust(left=0.07, right=0.99, bottom=0.09, top=0.94, wspace=0.25, hspace=0.3)

    if save_path:
        plt.savefig(save_path, bbox_inches="tight")
    else:
        plt.show()


def main():
    parser = argparse.ArgumentParser(
        description="Draw BN storage overhead (multiple security levels on one figure)"
    )
    parser.add_argument(
        "--series",
        nargs=2,
        action="append",
        metavar=("LABEL", "LOG"),
        required=True,
        help="Security label and path to log with G1_bits/G2_bits/GT_bits (repeat per curve)",
    )
    parser.add_argument("--save", type=str, default=None, help="Path to save the generated figure")
    parser.add_argument("--max-group-size", type=int, default=500, help="Maximum group size N")
    parser.add_argument("--step", type=int, default=10, help="Group size step")
    parser.add_argument("--font-path", type=str, default=None, help="Path to a Times New Roman .ttf/.otf font file")
    args = parser.parse_args()

    if args.max_group_size <= 0:
        raise ValueError("--max-group-size must be positive")
    if args.step <= 0:
        raise ValueError("--step must be positive")

    font_name = setup_plot_style(args.font_path)
    print(f"Using font: {font_name}")

    series_data = []
    for label, log_path in args.series:
        if not os.path.exists(log_path):
            raise FileNotFoundError(f"Log file not found: {log_path}")
        sizes = parse_size_log(log_path)
        gs, param_mb = build_series(
            sizes["G1_bits"],
            sizes["G2_bits"],
            args.max_group_size,
            args.step,
        )
        series_data.append((label, gs, param_mb))

    draw_combined(series_data, save_path=args.save)


if __name__ == "__main__":
    main()
