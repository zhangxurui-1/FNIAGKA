import matplotlib.pyplot as plt
from pathlib import Path


def draw_parameter_upk(x, ys, title, x_label, y_label, line_labels, save_path=None):
    fig, ax = plt.subplots(figsize=(6.5, 4))

    for y, label in zip(ys, line_labels):
        ax.plot(x, y, marker="o", linewidth=2, label=label)

    ax.set_title(title)
    ax.set_xlabel(x_label)
    ax.set_ylabel(y_label)
    ax.legend(frameon=False)
    ax.grid(True, linestyle="--", alpha=0.5)

    plt.tight_layout()

    if save_path:
        plt.savefig(save_path, bbox_inches="tight")

    plt.show()


ROOT = Path(__file__).resolve().parents[1]

group_size = [10, 20, 30, 40, 50, 60, 70, 80, 90, 100]
G1_size_80 = 192
G1_size_128 = 576

ys1 = [[], []]
line_labels1 = ["upk/parameter (80-bit)", "upk/parameter (128-bit)"]
for size in group_size:
    ys1[0].append((size * size) * G1_size_80 / 1024 / 1024)
    ys1[1].append((size * size) * G1_size_128 / 1024 / 1024)

draw_parameter_upk(
    group_size,
    ys1,
    "Storage Cost of system parameter/upk",
    "N",
    "Size (MB)",
    line_labels1,
    save_path=ROOT / "figures" / "store_overhead.pdf",
)
