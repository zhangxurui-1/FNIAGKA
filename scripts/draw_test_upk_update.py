import math
import matplotlib.pyplot as plt

def draw_n(xs, ys_list, titles,
           x_labels, y_labels, line_labels_list,
           max_cols=3, figsize=(5, 4), save_path=None):
    """
    xs:                [x1, x2, ..., xn]  (每个子图一个 x)
    ys_list:           [[ys1_lines], [ys2_lines], ..., [ysn_lines]]
                       其中 ys_i_lines = [y_line1, y_line2, ...]
    titles:            [title1, ..., titlen]
    x_labels:          [xlabel1, ..., xlabeln]
    y_labels:          [ylabel1, ..., ylabeln]
    line_labels_list:  [[labels1], [labels2], ..., [labelsn]]
    max_cols:          每行最多子图数量（默认 3）
    figsize:           单个子图的宽高 (w, h)
    """

    n = len(xs)
    # -------- 防呆校验（强烈建议保留）--------
    if not (len(ys_list) == len(titles) == len(x_labels) == len(y_labels) == len(line_labels_list) == n):
        raise ValueError(
            f"Length mismatch: len(xs)={n}, len(ys_list)={len(ys_list)}, len(titles)={len(titles)}, "
            f"len(x_labels)={len(x_labels)}, len(y_labels)={len(y_labels)}, len(line_labels_list)={len(line_labels_list)}"
        )

    cols = min(max_cols, n)
    rows = math.ceil(n / cols)

    fig, axes = plt.subplots(rows, cols, figsize=(figsize[0] * cols, figsize[1] * rows))

    # axes 统一拉平成一维，便于索引
    if rows == 1 and cols == 1:
        axes = [axes]
    else:
        axes = axes.flatten()

    # -------- 画每个子图 --------
    for i in range(n):
        ax = axes[i]
        for y, label in zip(ys_list[i], line_labels_list[i]):
            ax.plot(xs[i], y, marker='o', linewidth=2, label=label)

        ax.set_title(titles[i])
        ax.set_xlabel(x_labels[i])
        ax.set_ylabel(y_labels[i])
        ax.legend(frameon=False)
        ax.grid(True, linestyle='--', alpha=0.5)

    # -------- 多出来的空子图隐藏 --------
    for j in range(n, rows * cols):
        axes[j].axis('off')

    plt.tight_layout()

    if save_path:
        plt.savefig(save_path, bbox_inches='tight')

    plt.show()

group_size = [
    [10, 20, 30, 40, 50],
    [10, 20, 30, 40, 50],
    [10, 20, 30, 40, 50]
]

ys_list=[
    # UserKeyUpdate
    [
        [11.522, 23.335, 36.496, 49.282, 63.692],
        [103.217, 210.382, 323.515, 430.983, 547.927]
    ],

    # GroupKeyUpdate
    [
        [0.016, 0.016, 0.017, 0.017, 0.018],
        [0.094, 0.095, 0.095, 0.095, 0.098]
    ],

    # UserGen
    [
        [71.604, 281.22, 621.76, 1052.62, 1581.95],
        [655.8, 2369.02, 5329.77, 9223.1, 14038.8]
    ],
]

titles = [
    "(a) Time Cost of UserKeyUpdate",
    "(b) Time Cost of GroupKeyUpdate",
    "(c) Time Cost of UserGen"
]

line_labels_list = [
    ["80-bit", "128-bit"],
    ["80-bit", "128-bit"],
    ["80-bit", "128-bit"]
]

x_labels = ["N", "N", "N"]
y_labels = [
    "Execution Time (ms)",
    "Execution Time (ms)",
    "Execution Time (ms)"
]

draw_n(group_size, ys_list,
       titles,
       x_labels, 
       y_labels,
       line_labels_list,
       save_path="/Users/zxr/workspace/FNIAGKA/figures/test_upk_update.pdf")
