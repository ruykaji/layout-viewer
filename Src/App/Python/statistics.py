import os
from tqdm import tqdm
import pandas as pd
import matplotlib.pyplot as plt
import numpy as np
import torch
import torch.nn.functional as F

def process_npy(npy_path):
    arr = np.load(npy_path).astype(np.float32)
    tensor = torch.from_numpy(arr)

    if tensor.ndim == 3:
        tensor = tensor.permute(2, 0, 1)

    return tensor

if __name__ == "__main__":
    save_path = "./result_statistics"

    if not os.path.exists(save_path):
        os.makedirs(save_path)

    data_file = "/home/alaie/projects/layout-viewer/Src/App/Python/data.csv"  # 9
    df = pd.read_csv(data_file)

    all_pins = []
    all_number_of_pins = []
    all_number_of_nets = []

    pin_hot_spot_matrix = np.zeros((64, 64), dtype=np.float32)
    path_hot_spot_matrix = np.zeros((64, 64), dtype=np.float32)
    distance_h_hot_spot_matrix = np.zeros((64, 64), dtype=np.float32)
    distance_v_hot_spot_matrix = np.zeros((64, 64), dtype=np.float32)

    for id, row in tqdm(df.iterrows(), total=len(df), desc=f"Getting data"):
        source_h = process_npy(row["source_h"])
        source_v = process_npy(row["source_v"])
        target_path = process_npy(row["target_path"])

        _, h, w = source_h.shape

        source_h = F.pad(source_h, (0, 64-w,  0, 64-h, 0, 0,)).permute(1, 2, 0).numpy()
        source_v = F.pad(source_v, (0, 64-w, 0, 64-h, 0, 0)).permute(1, 2, 0).numpy()
        target_path = F.pad(target_path, (0, 64-w, 0, 64-h, 0, 0)).permute(1, 2, 0).numpy()

        path_hot_spot_matrix += np.sum(target_path, axis=-1)

        distance_h_hot_spot_matrix += np.sum(source_h, axis=-1)
        distance_v_hot_spot_matrix += np.sum(source_v, axis=-1)

        net_path = row['net']
        pins_count = row['pins_count']
        nets_count = row['nets_count']

        all_number_of_pins.append(pins_count)
        all_number_of_nets.append(nets_count)

        with open(net_path, 'r') as file:
            lines = file.readlines()
            current_net = None

            for line in lines:
                line = line.strip()

                if len(line) == 0:
                    break

                if line == "BEGIN":
                    current_net = None
                elif current_net is None:
                    current_net = line
                elif line == "END":
                    current_net = None
                else:
                    coordinates = tuple(map(int, line.split(',')))
                    pin_hot_spot_matrix[coordinates[1]][coordinates[0]] += 1

                    all_pins.append(coordinates)

    # Plot histogram
    if 1:
        plt.hist(all_number_of_pins)

        plt.xlabel('Pins count')
        plt.ylabel('Frequency')
        plt.title('Histogram of pins count')

        plt.tight_layout()
        plt.savefig(os.path.join(save_path, "pins_histogram.png"), dpi=400)
        plt.close()

    # Plot matrices
    if 1:
        fig, axes = plt.subplots(figsize=(8, 6), ncols=2, nrows=2)

        ax = axes[0, 0]
        im = ax.imshow(pin_hot_spot_matrix, cmap='plasma', interpolation='none')
        fig.colorbar(im, ax=ax)
        ax.set_title("Pin hot spots")
        ax.axis('off')

        ax = axes[0, 1]
        im = ax.imshow(path_hot_spot_matrix, cmap='plasma', interpolation='none')
        fig.colorbar(im, ax=ax)
        ax.set_title("Path hot spots")
        ax.axis('off')

        ax = axes[1, 0]
        im = ax.imshow(distance_h_hot_spot_matrix, cmap='plasma', interpolation='none')
        fig.colorbar(im, ax=ax)
        ax.set_title("Distance H hot spots")
        ax.axis('off')

        ax = axes[1, 1]
        im = ax.imshow(distance_v_hot_spot_matrix, cmap='plasma', interpolation='none')
        fig.colorbar(im, ax=ax)
        ax.set_title("Distance V hot spots")
        ax.axis('off')

        plt.tight_layout()
        plt.savefig(os.path.join(save_path, "hot_spots.png"), dpi=400)
        plt.close()
