import matplotlib.pyplot as plt

# plt.rcParams.update({
#     "font.size": 12,
#     "font.family": "serif"
# })

def draw_two(x1, x2, ys1, ys2, title1, title2,
             x_label1, y_label1, line_labels1,
             x_label2, y_label2, line_labels2,
             save_path=None):

    fig, axes = plt.subplots(1, 2, figsize=(10, 4))

    # -------- 左图 --------
    for y, label in zip(ys1, line_labels1):
        axes[0].plot(x1, y, marker='o', linewidth=2, label=label)

    axes[0].set_title(title1)
    axes[0].set_xlabel(x_label1)
    axes[0].set_ylabel(y_label1)
    axes[0].legend(frameon=False)
    axes[0].grid(True, linestyle='--', alpha=0.5)

    # -------- 右图 --------
    for y, label in zip(ys2, line_labels2):
        axes[1].plot(x2, y, marker='s', linewidth=2, label=label)

    axes[1].set_title(title2)
    axes[1].set_xlabel(x_label2)
    axes[1].set_ylabel(y_label2)
    axes[1].legend(frameon=False)
    axes[1].grid(True, linestyle='--', alpha=0.5)

    plt.tight_layout()

    if save_path:
        plt.savefig(save_path, bbox_inches='tight')

    plt.show()



group_size = [10, 20, 30, 40, 50, 60, 70, 80, 90, 100]
G1_size_80 = 192
G1_size_128 = 576
GT_size_80 = 128
GT_size_128 = 384

ys1=[[],[],[]]
line_labels1 = ["upk/parameter (80-bit)", "upk/parameter (128-bit)"]
for size in group_size:
    ys1[0].append((size*size) * G1_size_80 / 1024 / 1024)
    ys1[1].append((size*size) * G1_size_128 / 1024 / 1024)


ys2=[[],[],[],[]]
line_labels2 = ["ek (80-bit)", "ek (128-bit)", "dk (80-bit)", "dk (128-bit)"]
for size in group_size:
    ys2[0].append(G1_size_80*2)
    ys2[1].append(G1_size_128*2)
    ys2[2].append(G1_size_80)
    ys2[3].append(G1_size_128)

draw_two(group_size, group_size, ys1, ys2,
         "Storage Cost of system parameter/upk",
         "Storage Cost of ek/dk",
         "N", "Size (MB)", line_labels1,
         "N", "Size (B)", line_labels2,
         save_path="/Users/zxr/workspace/FNIAGKA/figures/store_overhead.pdf")
