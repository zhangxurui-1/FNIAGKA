# Usage:
# cd ${ROOT}
# python3 scripts/draw_test_upk_update_v2.py \
#   --log-dir log/log_test_upk_update_v2_20260517_161738 \
#   --save figures/test_upk_update_v2.pdf \
#   --font-path "/usr/share/fonts/truetype/msttcorefonts/Times_New_Roman.ttf"

import argparse
import math
import os
import re

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
        "font.size": 36,
        "axes.titlesize": 36,
        "axes.labelsize": 36,
        "legend.fontsize": 21,
        "xtick.labelsize": 32,
        "ytick.labelsize": 32,
        "axes.linewidth": 1.0,
        "lines.linewidth": 1.8,
        "figure.dpi": 300,
        "savefig.dpi": 300,
    })
    return font_name


def parse_experiment_log(path):
    metrics = {}
    current_metric = None
    with open(path, "r") as f:
        for line in f:
            line = line.strip()
            if not line:
                continue
            metric_match = re.match(r"^(k[A-Za-z0-9_]+):(\d+) events$", line)
            if metric_match:
                current_metric = metric_match.group(1)
                continue
            avg_match = re.match(r"^Avg:([0-9.]+) us$", line)
            if current_metric and avg_match:
                metrics[current_metric] = float(avg_match.group(1)) / 1000.0
                current_metric = None
    return metrics


def collect_results(log_dir):
    file_pattern = re.compile(r"exp_security(\d+)_size(\d+)\.log$")
    results = {}
    for name in sorted(os.listdir(log_dir)):
        match = file_pattern.match(name)
        if not match:
            continue
        security_level = int(match.group(1))
        group_size = int(match.group(2))
        metrics = parse_experiment_log(os.path.join(log_dir, name))
        if not metrics:
            continue
        if security_level not in results:
            results[security_level] = {}
        results[security_level][group_size] = metrics

    if not results:
        raise RuntimeError(f"No valid experiment logs found in {log_dir}")
    return results


def build_launch_subplot(results, security_level):
    launch_keys = [
        "kComputeUpdateUpkLaunchV2",
        "kComputeUpdateUpkLaunchV2_2",
        "kComputeUpdateUpkLaunchV2_3",
        "kComputeUpdateUpkLaunchV2_4",
        "kComputeUpdateUpkLaunchV2_5",
    ]
    launch_labels = ["L=1", "L=2", "L=3", "L=4", "L=5"]
    series = [{"x": [], "y": [], "label": label} for label in launch_labels]

    for group_size in sorted(results[security_level]):
        metrics = results[security_level][group_size]
        if any(key not in metrics for key in launch_keys):
            continue
        for idx, key in enumerate(launch_keys):
            series[idx]["x"].append(group_size)
            series[idx]["y"].append(metrics[key])

    series = [item for item in series if item["x"]]
    if not series:
        return None

    return {
        "title": f"Time Cost of UserKeyUpdateLaunch ({security_level}-bit)",
        "xlabel": "N",
        "ylabel": "Execution Time (ms)",
        "series": series,
    }


def build_metric_subplot(results, metric_key, title, xlabel):
    series = []
    for security_level in sorted(results):
        xs = []
        ys = []
        for group_size in sorted(results[security_level]):
            metrics = results[security_level][group_size]
            if metric_key not in metrics:
                continue
            xs.append(group_size)
            ys.append(metrics[metric_key])
        if xs:
            series.append({"x": xs, "y": ys, "label": f"{security_level}-bit"})

    if not series:
        return None

    return {
        "title": title,
        "xlabel": xlabel,
        "ylabel": "Execution Time (ms)",
        "series": series,
    }


def choose_legend_columns(label_count):
    if label_count == 2:
        return 2
    if label_count == 4:
        return 1
    if label_count >= 8:
        return 2
    return 1


def draw_n(subplots, max_cols=2, figsize=(8.6, 8.0), save_path=None):
    n = len(subplots)
    if n == 0:
        raise RuntimeError("No valid subplots to draw")

    cols = min(max_cols, n)
    rows = math.ceil(n / cols)
    fig, axes = plt.subplots(rows, cols, figsize=(figsize[0] * cols, figsize[1] * rows))

    if rows == 1 and cols == 1:
        axes = [axes]
    else:
        axes = axes.flatten()

    for idx, subplot in enumerate(subplots):
        ax = axes[idx]
        letter = chr(ord("a") + idx)
        for item in subplot["series"]:
            ax.plot(
                item["x"],
                item["y"],
                marker="o",
                markersize=4,
                linewidth=2,
                label=item["label"],
            )

        ax.set_title(f"({letter}) {subplot['title']}", pad=10)
        ax.set_xlabel(subplot["xlabel"])
        ax.set_ylabel(subplot["ylabel"])
        ax.grid(True, linestyle="--", alpha=0.5)

        labels = [item["label"] for item in subplot["series"] if item["label"]]
        if labels:
            ax.legend(
                loc="upper left",
                bbox_to_anchor=(0.02, 0.98),
                borderaxespad=0.0,
                ncol=choose_legend_columns(len(labels)),
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

    for idx in range(n, rows * cols):
        axes[idx].axis("off")

    fig.subplots_adjust(left=0.07, right=0.99, bottom=0.09, top=0.94, wspace=0.25, hspace=0.4)

    if save_path:
        plt.savefig(save_path, bbox_inches="tight")

    plt.show()


def main():
    parser = argparse.ArgumentParser(description="Draw UPK update v2 figures from experiment logs")
    parser.add_argument("--log-dir", type=str, required=True, help="Directory containing exp_security*_size*.log files")
    parser.add_argument("--save", type=str, default="figures/test_upk_update_v2.pdf", help="Path to save the generated figure")
    parser.add_argument("--font-path", type=str, default=None, help="Path to a Times New Roman .ttf/.otf font file")
    args = parser.parse_args()

    if not os.path.isdir(args.log_dir):
        raise FileNotFoundError(f"Log directory not found: {args.log_dir}")

    font_name = setup_plot_style(args.font_path)
    print(f"Using font: {font_name}")

    results = collect_results(args.log_dir)
    subplots = []

    for security_level in sorted(results):
        subplot = build_launch_subplot(results, security_level)
        if subplot:
            subplots.append(subplot)

    user_key_subplot = build_metric_subplot(results, "kComputeUpdateUpkV2", "Time Cost of UserKeyUpdate", "N")
    if user_key_subplot:
        subplots.append(user_key_subplot)

    group_key_subplot = build_metric_subplot(results, "kComputeUpdateGroupKeyV2", "Time Cost of GroupKeyUpdate", "n")
    if group_key_subplot:
        subplots.append(group_key_subplot)

    draw_n(subplots, save_path=args.save)


if __name__ == "__main__":
    main()
