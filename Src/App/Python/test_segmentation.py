import os
import ssl
import torch
import torch.nn as nn
import torch.nn.functional as F
import numpy as np
import pandas as pd

from pathlib import Path
from tqdm import tqdm
from torch.utils.data import DataLoader

from model import RecurrentUNet
from dataset import SegmentationDataset, pack_batch

import net_connectivity

ssl._create_default_https_context = ssl._create_unverified_context


def compute_metrics(preds: torch.Tensor, nets, metric_objects: dict):
    preds_resized = F.interpolate(preds, size=(64, 64), mode='bicubic', align_corners=False)

    preds_np = torch.argmax(preds_resized, dim=1).unsqueeze(1).cpu().numpy()
    overall, instance = net_connectivity.check_connectivity(nets, preds_np)

    metric_objects["overall_connectivity"].append(overall)
    metric_objects["instance_connectivity"].append(instance)

def get_final_metrics(metric_objects: dict) -> dict:
    return {
        "overall_connectivity": np.mean(metric_objects["overall_connectivity"]),
        "instance_connectivity": np.mean(metric_objects["instance_connectivity"])
    }

def validate_one_epoch(model: nn.Module, val_loader, device) -> dict:
    metric_objects = {
        "overall_connectivity": [],
        "instance_connectivity": []
    }

    with torch.no_grad():
        for idx, (inputs, targets, masks) in tqdm(enumerate(val_loader), total=len(val_loader), desc="Validation", leave=False):
            inputs, targets, masks = inputs.to(device), targets.to(device), masks.to(device)

            outputs = model(inputs, masks)
            nets = val_loader.dataset.get_nets(idx)

            compute_metrics(outputs, nets, metric_objects)

    return get_final_metrics(metric_objects)

def main() -> None:
    device = torch.device("cuda" if torch.cuda.is_available() else "cpu")
    
    base_name = "segmentation"
    results_dir = f"./results_8_{base_name}"
    os.makedirs(results_dir, exist_ok=True)
    
    base_dir = Path("/home/alaie/projects/layout-viewer/Output/Debug/bin/zipdiv_max/gcells/data.csv")
    batch_size = 64
    
    df = pd.read_csv(base_dir)
    print(df["nets_count"].value_counts())
    print(np.mean(df["nets_count"].values))

    val_dataset = SegmentationDataset(df, validation=True, remove_invalid=True, check_data=False, batch_size=batch_size)
    val_loader = DataLoader(val_dataset, batch_size=batch_size, collate_fn=pack_batch, shuffle=False, num_workers=4, pin_memory=True)
    
    model = RecurrentUNet(num_classes=3).to(device)
    model.load_state_dict(torch.load("/home/alaie/projects/layout-viewer/Src/App/Python/results_8_segmentation/model_fold_0_val.pth"))
    model.eval()

    val_metrics = validate_one_epoch(model, val_loader, device)
    print(f"Overall Connectivity: {val_metrics['overall_connectivity']:.4f} | "f"Instance Connectivity: {val_metrics['instance_connectivity']:.4f}")


if __name__ == "__main__":
    torch.backends.cudnn.benchmark = True
    main()
