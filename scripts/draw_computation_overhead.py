import math
import matplotlib.pyplot as plt

# Times New Roman + 较大字号（论文图常用）
plt.rcParams.update({
    "font.family": "serif",
    "font.serif": ["Times New Roman", "Times", "DejaVu Serif"],
    "font.size": 14,
    "axes.titlesize": 15,
    "axes.labelsize": 14,
    "legend.fontsize": 12,
    "xtick.labelsize": 12,
    "ytick.labelsize": 12,
})


def draw_n(xs, ys_list, titles,
           x_labels, y_labels, line_labels_list,
           max_cols=4, figsize=(5, 4), save_path=None):
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
    [10, 20, 30, 40, 50],
    [10, 20, 30, 40, 50],
    [10, 20, 30, 40, 50],
    [10, 20, 30, 40, 50],
    [10, 20, 30, 40, 50],
]

ys_list = [
    # Agreement
    [
        [0.168, 0.316, 0.458, 0.622, 0.751],
        [0.751, 1.542, 2.369, 3.581, 3.979]
    ],

    # Add
    [
        [0.170, 0.326, 0.452, 0.680, 0.731],
        [0.712, 1.611, 2.339, 3.532, 3.970],
        [0.024, 0.031, 0.028, 0.029, 0.027],
        [0.106, 0.111, 0.108, 0.116, 0.115]
    ],

    # Remove
    [
        [0.024, 0.023, 0.028, 0.025, 0.026],
        [0.107, 0.114, 0.105, 0.102, 0.111]
    ],

    # Split
    [
        [0.073, 0.16, 0.238, 0.329, 0.57],
        [0.408, 0.823, 1.262, 1.808, 2.654],
        [0.09, 0.226, 0.322, 0.429, 0.679],
        [0.516, 1.022, 1.632, 2.421, 2.928],
        [0.098, 0.216, 0.334, 0.61, 0.714],
        [0.547, 1.165, 1.903, 2.493, 3.124],
        [0.121, 0.265, 0.367, 0.628, 0.764],
        [0.608, 1.26, 2.581, 2.693, 3.638]
    ],

    # Merge-standard
    [
        [0.032, 0.067, 0.164, 0.19, 0.261],
        [0.151, 0.314, 0.506, 0.761, 1.029],
        [0.021, 0.046, 0.077, 0.106, 0.168],
        [0.096, 0.216, 0.35, 0.506, 0.634],
        [0.018, 0.037, 0.06, 0.091, 0.125],
        [0.073, 0.161, 0.279, 0.331, 0.429],
        [0.016, 0.03, 0.051, 0.083, 0.095],
        [0.057, 0.124, 0.237, 0.269, 0.355]
    ],

    # Merge-extended
    [
        [0.124, 0.306, 0.64, 0.732, 1.108],
        [0.709, 1.516, 2.383, 3.883, 4.548],
        [0.167, 0.354, 0.556, 0.846, 1.294],
        [0.916, 1.837, 2.996, 4.51, 5.806],
        [0.17, 0.397, 0.604, 1.047, 1.231],
        [0.893, 2.071, 3.874, 4.586, 5.914],
        [0.199, 0.415, 0.659, 1.064, 1.349],
        [1.25, 2.39, 4.231, 4.892, 5.819]
    ],

    # Encap
    [
        [1.82, 1.923, 1.881, 1.77, 1.77],
        [17.597, 17.101, 17.932, 16.142, 17.199]
    ],

    # Decap
    [
        [1.733, 1.786, 1.753, 1.664, 1.658],
        [18.07, 17.025, 17.803, 16.328, 17.397]
    ]
]

# 仅保留原图中第 5–12 个子图对应内容，标题按 (a)–(h) 重排
titles = [
    "(a) Time Cost of Agreement",
    "(b) Time Cost of Add",
    "(c) Time Cost of Remove",
    "(d) Time Cost of Split",
    "(e) Time Cost of Merge (standard mode)",
    "(f) Time Cost of Merge (extended mode)",
    "(g) Time Cost of Encap",
    "(h) Time Cost of Decap",
]

line_labels_list = [
    ["80-bit", "128-bit"],
    ["80-bit (for new user)", "128-bit (for new user)", "80-bit (for old user)", "128-bit (for old user)"],
    ["80-bit", "128-bit"],
    ["80-bit (L=2)", "128-bit (L=2)", "80-bit (L=3)", "128-bit (L=3)", "80-bit (L=4)", "128-bit (L=4)", "80-bit (L=5)", "128-bit (L=5)"],
    ["80-bit (L=2)", "128-bit (L=2)", "80-bit (L=3)", "128-bit (L=3)", "80-bit (L=4)", "128-bit (L=4)", "80-bit (L=5)", "128-bit (L=5)"],
    ["80-bit (L=2)", "128-bit (L=2)", "80-bit (L=3)", "128-bit (L=3)", "80-bit (L=4)", "128-bit (L=4)", "80-bit (L=5)", "128-bit (L=5)"],
    ["80-bit", "128-bit"],
    ["80-bit", "128-bit"],
]

x_labels = ["n"] * 8
y_labels = ["Execution Time (ms)"] * 8

draw_n(
    group_size,
    ys_list,
    titles,
    x_labels,
    y_labels,
    line_labels_list,
    max_cols=4,
    save_path="/Users/zxr/workspace/FNIAGKA/figures/proto_computation_cost.pdf",
)


# kComputeAddGen:[0.025, 0.03, 0.053, 0.049, 0.055, ]
# kComputeAddGen (128 bit):[0.105, 0.112, 0.131, 0.127, 0.146, ]
# kComputeAddUpd:[0.024, 0.031, 0.048, 0.049, 0.057, ]
# kComputeAddUpd (128 bit):[0.106, 0.111, 0.128, 0.146, 0.135, ]
# kComputeAgree:[0.168, 0.316, 0.458, 0.622, 0.751, ]
# kComputeAgree (128 bit):[0.751, 1.542, 2.369, 3.581, 3.979, ]
# kComputeDecap:[1.733, 1.786, 1.753, 1.664, 1.658, ]
# kComputeDecap (128 bit):[18.07, 17.025, 17.803, 16.328, 17.397, ]
# kComputeEncap:[1.82, 1.923, 1.881, 1.77, 1.77, ]
# kComputeEncap (128 bit):[17.597, 17.101, 17.932, 16.142, 17.199, ]
# kComputeMergeExtended:[0.195, 0.21, 0.216, 0.194, 0.188, ]
# kComputeMergeExtended (128 bit):[1.109, 0.99, 1.08, 1.085, 0.983, ]
# kComputeMergeStandard:[0.025, 0.029, 0.028, 0.027, 0.025, ]
# kComputeMergeStandard (128 bit):[0.117, 0.104, 0.109, 0.123, 0.101, ]
# kComputeNegotiate:[0.795, 4.401, 9.746, 15.83, 21.469, ]
# kComputeNegotiate (128 bit):[5.987, 19.856, 42.891, 86.666, 121.574, ]
# kComputePNGen:[36.052, 137.06, 314.148, 548.22, 818.783, ]
# kComputePNGen (128 bit):[349.815, 1271.11, 2536.53, 4405.91, 7040.31, ]
# kComputeRemove:[0.024, 0.043, 0.038, 0.05, 0.056, ]
# kComputeRemove (128 bit):[0.107, 0.114, 0.125, 0.152, 0.131, ]
# kComputeSetup:[17.488, 18.628, 28.352, 36.006, 42.89, ]
# kComputeSetup (128 bit):[199.688, 347.706, 497.628, 680.787, 813.665, ]
# kComputeSplit:[0.112, 0.111, 0.109, 0.105, 0.106, ]
# kComputeSplit (128 bit):[0.589, 0.567, 0.562, 0.547, 0.576, ]
# kComputeUserGen:[71.604, 281.22, 621.76, 1052.62, 1581.95, ]
# kComputeUserGen (128 bit):[655.8, 2369.02, 5329.77, 9223.1, 14038.8, ]
# kUnknown:[0, 0, 0, 0, 0, ]
# kUnknown (128 bit):[0, 0, 0, 0, 0, ]
