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
    [10, 20, 30, 40, 50],
    [10, 20, 30, 40, 50]
]

ys_list=[
    # UserKeyUpdateLaunchV2
    [
        [1.835, 1.734, 1.639, 1.588, 1.617],
        [3.44, 3.609, 3.268, 3.198, 3.223],
        [5.424, 5.331, 4.874, 4.787, 4.884],
        [7.051, 7.125, 6.494, 6.384, 6.558],
        [8.824, 8.949, 8.121, 8.023, 8.189]
    ],

    # UserKeyUpdateLaunchV2 (128-bit)
    [
        [16.994, 15.464, 15.491, 18.325, 16.095],
        [33.325, 31.084, 31.024, 33.854, 31.494],
        [49.727, 46.605, 46.45, 52.165, 48.79],
        [70.623, 61.84, 61.977, 68.781, 62.851],
        [86.363, 77.86, 77.43, 87.7, 80.255]
    ],

    # UserKeyUpdateV2
    [
        [42.253, 157.444, 281.865, 485.503, 756.863],
        [378.886, 1136.73, 2500.12, 5175.83, 7013.42]
    ],

    # GroupKeyUpdate
    [
        [0.973, 1.03, 1.257, 1.17, 1.292],
        [8.171, 9.113, 9.469, 9.505, 10.036]
    ]
]

titles = [
    "(a) Time Cost of UserKeyUpdateLaunch (80-bit)",
    "(b) Time Cost of UserKeyUpdateLaunch (128-bit)",
    "(c) Time Cost of UserKeyUpdate",
    "(d) Time Cost of GroupKeyUpdate"
]

line_labels_list = [
    ["L=1", "L=2", "L=3", "L=4", "L=5"],
    ["L=1", "L=2", "L=3", "L=4", "L=5"],
    ["80-bit", "128-bit"],
    ["80-bit", "128-bit"]
]

x_labels = ["N", "N", "N", "n"]
y_labels = [
    "Execution Time (ms)",
    "Execution Time (ms)",
    "Execution Time (ms)",
    "Execution Time (ms)"
]

draw_n(group_size, ys_list,
       titles,
       x_labels, 
       y_labels,
       line_labels_list,
       max_cols=2,
       save_path="/Users/zxr/workspace/FNIAGKA/figures/test_upk_update_v2.pdf")
