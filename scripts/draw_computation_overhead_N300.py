# Usage:
# cd ${ROOT}
# python3 scripts/draw_computation_overhead_N300.py --log log/log_20260503_011503/exp_data_summary_20260504-164731.log --subplots Agree Add Remove Split "Merge(standard)" "Merge(extended)" Encap Decap --limit 30 --step 3 --marker --save figures/proto_computation_cost_N300.pdf

import math
import os
import argparse

import matplotlib.pyplot as plt

def setup_plot_style():
    # 字体：Times New Roman（若环境缺失会自动回退到 Times/DejaVu Serif）
    plt.rcParams.update({
        "font.family": "serif",
        "font.serif": ["Times New Roman", "Times", "DejaVu Serif"],
        "font.size": 14,
        "axes.titlesize": 16,
        "axes.labelsize": 15,
        "legend.fontsize": 12,
        "xtick.labelsize": 12,
        "ytick.labelsize": 12,
    })


def draw_n(xs,
           ys_list,
           titles,
           x_labels,
           y_labels,
           line_labels_list,
           max_cols=4,
           figsize=(5, 4),
           save_path=None,
           show_marker=False):
    """
    xs:                [x1, x2, ..., xn]  (每个子图一个 x)
    ys_list:           [[ys1_lines], [ys2_lines], ..., [ysn_lines]]
                       其中 ys_i_lines = [y_line1, y_line2, ...]
    titles:            [title1, ..., titlen]
    x_labels:          [xlabel1, ..., xlabeln]
    y_labels:          [ylabel1, ..., ylabeln]
    line_labels_list:  [[labels1], [labels2], ..., [labelsn]]
    max_cols:          每行最多子图数量（默认 4）
    figsize:           单个子图的宽高 (w, h)
    show_marker:       是否显示每个点的小圆点 marker
    """

    n = len(xs)
    if not (len(ys_list) == len(titles) == len(x_labels) == len(y_labels) == len(line_labels_list) == n):
        raise ValueError(
            f"Length mismatch: len(xs)={n}, len(ys_list)={len(ys_list)}, len(titles)={len(titles)}, "
            f"len(x_labels)={len(x_labels)}, len(y_labels)={len(y_labels)}, len(line_labels_list)={len(line_labels_list)}"
        )

    cols = min(max_cols, n)
    rows = math.ceil(n / cols)

    fig, axes = plt.subplots(rows, cols, figsize=(figsize[0] * cols, figsize[1] * rows))

    if rows == 1 and cols == 1:
        axes = [axes]
    else:
        axes = axes.flatten()

    marker_fmt = 'o' if show_marker else None

    for i in range(n):
        ax = axes[i]
        for y, label in zip(ys_list[i], line_labels_list[i]):
            ax.plot(xs[i], y, marker=marker_fmt, linewidth=2, label=label)

        ax.set_title(titles[i])
        ax.set_xlabel(x_labels[i])
        ax.set_ylabel(y_labels[i])
        ax.legend(frameon=False)
        ax.grid(True, linestyle='--', alpha=0.5)

    for j in range(n, rows * cols):
        axes[j].axis('off')

    plt.tight_layout()

    if save_path:
        plt.savefig(save_path, bbox_inches='tight')

    plt.show()


SUBPLOTS_CONFIG = {
    "Setup": {
        "title": "Time Cost of Setup",
        "keys": ["kComputeSetup", "kComputeSetup (128 bit)"],
        "labels": ["80-bit", "128-bit"],
        "xlabel": "N",
        "ylabel": "Execution Time (ms)",
    },
    "PNGen": {
        "title": "Time Cost of PNGen",
        "keys": ["kComputePNGen", "kComputePNGen (128 bit)"],
        "labels": ["80-bit", "128-bit"],
        "xlabel": "N",
        "ylabel": "Execution Time (ms)",
    },
    "Negotiate": {
        "title": "Time Cost of Negotiate",
        "keys": ["kComputeNegotiate", "kComputeNegotiate (128 bit)"],
        "labels": ["80-bit", "128-bit"],
        "xlabel": "N",
        "ylabel": "Execution Time (ms)",
    },
    "UserGen": {
        "title": "Time Cost of UserGen",
        "keys": ["kComputeUserGen", "kComputeUserGen (128 bit)"],
        "labels": ["80-bit", "128-bit"],
        "xlabel": "N",
        "ylabel": "Execution Time (ms)",
    },
    "Agree": {
        "title": "Time Cost of Agreement",
        "keys": ["kComputeAgree", "kComputeAgree (128 bit)"],
        "labels": ["80-bit", "128-bit"],
        "xlabel": "n",
        "ylabel": "Execution Time (ms)",
    },
    "Add": {
        "title": "Time Cost of Add",
        # 复用 Agree(新用户) 与 AddUpd(老用户) 作为 add 的 4 条线
        "keys": ["kComputeAgree", "kComputeAgree (128 bit)", "kComputeAddUpd", "kComputeAddUpd (128 bit)"],
        "labels": [
            "80-bit (for new user)",
            "128-bit (for new user)",
            "80-bit (for old user)",
            "128-bit (for old user)",
        ],
        "xlabel": "n",
        "ylabel": "Execution Time (ms)",
    },
    "Remove": {
        "title": "Time Cost of Remove",
        "keys": ["kComputeRemove", "kComputeRemove (128 bit)"],
        "labels": ["80-bit", "128-bit"],
        "xlabel": "n",
        "ylabel": "Execution Time (ms)",
    },
    "Split": {
        "title": "Time Cost of Split",
        "keys": [
            "kComputeSplit_2",
            "kComputeSplit_2 (128 bit)",
            "kComputeSplit_3",
            "kComputeSplit_3 (128 bit)",
            "kComputeSplit_4",
            "kComputeSplit_4 (128 bit)",
            "kComputeSplit_5",
            "kComputeSplit_5 (128 bit)",
        ],
        "labels": [
            "80-bit (L=2)",
            "128-bit (L=2)",
            "80-bit (L=3)",
            "128-bit (L=3)",
            "80-bit (L=4)",
            "128-bit (L=4)",
            "80-bit (L=5)",
            "128-bit (L=5)",
        ],
        "xlabel": "n",
        "ylabel": "Execution Time (ms)",
    },
    "Merge(standard)": {
        "title": "Time Cost of Merge (standard mode)",
        "keys": [
            "kComputeMergeStandard_2",
            "kComputeMergeStandard_2 (128 bit)",
            "kComputeMergeStandard_3",
            "kComputeMergeStandard_3 (128 bit)",
            "kComputeMergeStandard_4",
            "kComputeMergeStandard_4 (128 bit)",
            "kComputeMergeStandard_5",
            "kComputeMergeStandard_5 (128 bit)",
        ],
        "labels": [
            "80-bit (L=2)",
            "128-bit (L=2)",
            "80-bit (L=3)",
            "128-bit (L=3)",
            "80-bit (L=4)",
            "128-bit (L=4)",
            "80-bit (L=5)",
            "128-bit (L=5)",
        ],
        "xlabel": "n",
        "ylabel": "Execution Time (ms)",
    },
    "Merge(extended)": {
        "title": "Time Cost of Merge (extended mode)",
        "keys": [
            "kComputeMergeExtended_2",
            "kComputeMergeExtended_2 (128 bit)",
            "kComputeMergeExtended_3",
            "kComputeMergeExtended_3 (128 bit)",
            "kComputeMergeExtended_4",
            "kComputeMergeExtended_4 (128 bit)",
            "kComputeMergeExtended_5",
            "kComputeMergeExtended_5 (128 bit)",
        ],
        "labels": [
            "80-bit (L=2)",
            "128-bit (L=2)",
            "80-bit (L=3)",
            "128-bit (L=3)",
            "80-bit (L=4)",
            "128-bit (L=4)",
            "80-bit (L=5)",
            "128-bit (L=5)",
        ],
        "xlabel": "n",
        "ylabel": "Execution Time (ms)",
    },
    "Encap": {
        "title": "Time Cost of Encap",
        "keys": ["kComputeEncap", "kComputeEncap (128 bit)"],
        "labels": ["80-bit", "128-bit"],
        "xlabel": "n",
        "ylabel": "Execution Time (ms)",
    },
    "Decap": {
        "title": "Time Cost of Decap",
        "keys": ["kComputeDecap", "kComputeDecap (128 bit)"],
        "labels": ["80-bit", "128-bit"],
        "xlabel": "n",
        "ylabel": "Execution Time (ms)",
    },
}


def parse_log(filepath: str):
    data = {}
    with open(filepath, 'r') as f:
        for line in f:
            line = line.strip()
            if not line:
                continue
            key, sep, rest = line.partition(":[")
            if not sep:
                continue
            values_str = rest.replace("]", "").strip()
            values = [float(v.strip()) for v in values_str.split(",") if v.strip()]
            data[key.strip()] = values
    return data


def build_series(data: dict, keys: list, labels: list, limit: int, step: int):
    """返回 (xs, ys_lines, labels_kept)。不同 key 长度不一致时，自动取所有线的最小长度对齐。"""
    ys_lines = []
    labels_kept = []
    for k, lb in zip(keys, labels):
        if k not in data:
            print(f"Warning: key '{k}' not found in log")
            continue
        ys_lines.append(data[k])
        labels_kept.append(lb)

    if not ys_lines:
        return [], [], []

    min_len = min(len(y) for y in ys_lines)
    if limit is not None:
        min_len = min(min_len, limit)
    if min_len <= 0:
        return [], [], []

    if step is None or step <= 0:
        step = 1

    base_x = [10 * (i + 1) for i in range(min_len)]

    if step > 1:
        # 始终保留第一个点和最后一个点，避免 step 采样漏掉末尾
        indices = list(range(0, min_len, step))
        if indices[-1] != (min_len - 1):
            indices.append(min_len - 1)
        xs = [base_x[i] for i in indices]
        ys_lines = [[y[i] for i in indices] for y in ys_lines]
    else:
        xs = base_x
        ys_lines = [y[:min_len] for y in ys_lines]

    return xs, ys_lines, labels_kept


def main():
    setup_plot_style()

    parser = argparse.ArgumentParser(description="Draw computation overhead figures from exp_data_summary_xxx.log")
    parser.add_argument("--log", type=str, required=True, help="Path to exp_data_summary_xxx.log")
    parser.add_argument(
        "--subplots",
        type=str,
        nargs="+",
        choices=list(SUBPLOTS_CONFIG.keys()),
        default=list(SUBPLOTS_CONFIG.keys()),
        help="Specify which subplots to draw (e.g. Agree Add Remove Split 'Merge(standard)')",
    )
    parser.add_argument("--save", type=str, default=None, help="Path to save the generated figure (optional)")
    parser.add_argument("--limit", type=int, default=None, help="截断数据点个数（每条线取前 limit 个点）")
    parser.add_argument("--step", type=int, default=1, help="稀疏采样步长：每 step 个点取 1 个（默认 1 不稀疏）")
    parser.add_argument("--marker", action="store_true", help="显示每个数据点的小圆点 marker（默认不显示）")
    args = parser.parse_args()

    if not os.path.exists(args.log):
        raise FileNotFoundError(f"Log file not found: {args.log}")

    data = parse_log(args.log)

    xs_all = []
    ys_all = []
    titles_all = []
    xlabels_all = []
    ylabels_all = []
    line_labels_all = []

    # 先构建，再给标题编号，保证跳过空子图后字母仍连续
    tmp = []
    for subplot_name in args.subplots:
        cfg = SUBPLOTS_CONFIG[subplot_name]
        xs, ys_lines, labels_kept = build_series(
            data=data,
            keys=cfg["keys"],
            labels=cfg["labels"],
            limit=args.limit,
            step=args.step,
        )
        if not xs or not ys_lines:
            print(f"Warning: subplot '{subplot_name}' has no valid data, skipped")
            continue
        tmp.append((subplot_name, cfg, xs, ys_lines, labels_kept))

    for idx, (subplot_name, cfg, xs, ys_lines, labels_kept) in enumerate(tmp):
        letter = chr(ord('a') + idx)
        xs_all.append(xs)
        ys_all.append(ys_lines)
        titles_all.append(f"({letter}) {cfg['title']}")
        xlabels_all.append(cfg["xlabel"])
        ylabels_all.append(cfg["ylabel"])
        line_labels_all.append(labels_kept)

    if not xs_all:
        raise RuntimeError("No valid subplots to draw. Please check --subplots and log content.")

    draw_n(
        xs_all,
        ys_all,
        titles_all,
        xlabels_all,
        ylabels_all,
        line_labels_all,
        save_path=args.save,
        show_marker=args.marker,
    )


if __name__ == "__main__":
    main()
