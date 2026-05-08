# Usage:
# cd ${ROOT}
# python3 scripts/draw_store_overhead_bls_security256.py --log log/log_20260509_xxxxxx/exp_storage_bls_security256.log --save figures/store_overhead_bls_security256.pdf

import argparse
import os

import matplotlib.pyplot as plt


def setup_plot_style():
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


def parse_size_log(path: str):
    sizes = {}
    with open(path, "r") as f:
        for line in f:
            line = line.strip()
            if not line or ":" not in line:
                continue
            key, value = line.split(":", 1)
            sizes[key.strip()] = int(value.strip())
    required = ["G1_bits", "G2_bits", "GT_bits"]
    missing = [key for key in required if key not in sizes]
    if missing:
        raise RuntimeError(f"Missing keys in size log: {missing}")
    return sizes


def bits_to_bytes(bits: int):
    return bits / 8.0


def build_series(g1_bits: int, g2_bits: int, gt_bits: int, max_group_size: int, step: int):
    group_sizes = list(range(step, max_group_size + 1, step))

    g1_bytes = bits_to_bytes(g1_bits)
    g2_bytes = bits_to_bytes(g2_bits)

    parameter_upk_sizes_mb = []
    ek_sizes_kb = []
    dk_sizes_kb = []

    for n in group_sizes:
        parameter_upk_bytes = (2 * n) * g1_bytes + (n * n - n) * g2_bytes

        parameter_upk_sizes_mb.append(parameter_upk_bytes / 1024 / 1024)
        ek_sizes_kb.append((2 * g1_bytes) / 1024)
        dk_sizes_kb.append(g2_bytes / 1024)

    return group_sizes, parameter_upk_sizes_mb, ek_sizes_kb, dk_sizes_kb


def draw_two(group_sizes,
             parameter_upk_sizes_mb,
             ek_sizes_kb,
             dk_sizes_kb,
             save_path=None):
    fig, axes = plt.subplots(1, 2, figsize=(10, 4))

    axes[0].plot(group_sizes, parameter_upk_sizes_mb, marker='o', linewidth=2)
    axes[0].set_title('Storage Cost of system parameter/upk')
    axes[0].set_xlabel('N')
    axes[0].set_ylabel('Size (MB)')
    axes[0].grid(True, linestyle='--', alpha=0.5)

    axes[1].plot(group_sizes, ek_sizes_kb, marker='o', linewidth=2, label='ek')
    axes[1].plot(group_sizes, dk_sizes_kb, marker='s', linewidth=2, label='dk')
    axes[1].set_title('Storage Cost of ek/dk')
    axes[1].set_xlabel('N')
    axes[1].set_ylabel('Size (KB)')
    axes[1].legend(frameon=False)
    axes[1].grid(True, linestyle='--', alpha=0.5)

    plt.tight_layout()

    if save_path:
        plt.savefig(save_path, bbox_inches='tight')

    plt.show()


def main():
    setup_plot_style()

    parser = argparse.ArgumentParser(description='Draw BLS 256-bit storage overhead figures from measured element sizes')
    parser.add_argument('--log', type=str, required=True, help='Path to storage size log with G1_bits/G2_bits/GT_bits')
    parser.add_argument('--save', type=str, default=None, help='Path to save the generated figure')
    parser.add_argument('--max-group-size', type=int, default=100, help='Maximum group size N')
    parser.add_argument('--step', type=int, default=10, help='Group size step')
    args = parser.parse_args()

    if not os.path.exists(args.log):
        raise FileNotFoundError(f'Log file not found: {args.log}')
    if args.max_group_size <= 0:
        raise ValueError('--max-group-size must be positive')
    if args.step <= 0:
        raise ValueError('--step must be positive')

    sizes = parse_size_log(args.log)
    group_sizes, parameter_upk_sizes_mb, ek_sizes_kb, dk_sizes_kb = build_series(
        sizes['G1_bits'],
        sizes['G2_bits'],
        sizes['GT_bits'],
        args.max_group_size,
        args.step,
    )

    draw_two(
        group_sizes,
        parameter_upk_sizes_mb,
        ek_sizes_kb,
        dk_sizes_kb,
        save_path=args.save,
    )


if __name__ == '__main__':
    main()
