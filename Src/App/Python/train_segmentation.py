import os
import random
import ssl
import torch
import torch.nn as nn
import torch.optim as optim
import torch.nn.functional as F
import numpy as np
import pandas as pd

from pathlib import Path
from sklearn.model_selection import KFold
from tqdm import tqdm
from torch.utils.data import DataLoader
from segmentation_models_pytorch.losses import DiceLoss

from model import RecurrentUNet
from dataset import SegmentationDataset, pack_batch
from ademamix import AdEMAMix

import net_connectivity

ssl._create_default_https_context = ssl._create_unverified_context


def set_seed(seed: int = 42) -> None:
    torch.manual_seed(seed)
    np.random.seed(seed)
    random.seed(seed)
    
    if torch.cuda.is_available():
        torch.cuda.manual_seed_all(seed)

def train_criterion(pred: torch.Tensor, target: torch.Tensor) -> torch.Tensor:
    dice_loss_fn = DiceLoss(mode="multiclass")
    ce_loss_fn = torch.nn.CrossEntropyLoss()

    return ce_loss_fn(pred, target) + dice_loss_fn(pred, target)

def compute_metrics(targets: torch.Tensor, preds: torch.Tensor, nets, metric_objects: dict):
    # targets_resized = F.interpolate(targets, size=(64, 64), mode='bicubic', align_corners=False)
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

# -------------------------
# Training and Validation Loops
# -------------------------
def train_one_epoch(model: nn.Module, optimizer: optim.Optimizer, train_loader, device) -> float:
    model.train()

    total_loss = 0.0
    num_samples = 0

    for inputs, targets, masks in tqdm(train_loader, desc="Training", leave=False):
        inputs, targets, masks = inputs.to(device), targets.to(device), masks.to(device)
        
        optimizer.zero_grad()

        outputs = model(inputs, masks)

        loss = train_criterion(outputs, targets)
        loss.backward()
        
        optimizer.step()
        
        total_loss += loss.item() * targets.size(0)
        num_samples += targets.size(0)

    epoch_loss = total_loss / num_samples
    return epoch_loss

def validate_one_epoch(model: nn.Module, val_loader, device) -> dict:
    model.eval()

    metric_objects = {
        "overall_connectivity": [],
        "instance_connectivity": []
    }

    with torch.no_grad():
        for idx, (inputs, targets, masks) in tqdm(enumerate(val_loader), total=len(val_loader), desc="Validation", leave=False):
            inputs, targets, masks = inputs.to(device), targets.to(device), masks.to(device)

            outputs = model(inputs, masks)
            nets = val_loader.dataset.get_nets(idx)

            compute_metrics(targets, outputs, nets, metric_objects)

    return get_final_metrics(metric_objects)

def train_model(model: nn.Module, train_loader, val_loader, optimizer: optim.Optimizer, scheduler: optim.lr_scheduler.LRScheduler, device, num_epochs: int = 32, save_path: str = "./") -> tuple:
    train_losses = []
    val_metrics_history = []
    
    for epoch in range(num_epochs):
        print(f"Epoch [{epoch+1}/{num_epochs}]")
        
        train_loss = train_one_epoch(model, optimizer, train_loader, device)
        train_losses.append(train_loss)
        
        val_metrics = validate_one_epoch(model, val_loader, device)
        val_metrics_history.append(val_metrics)
        
        print(f"Train Loss: {train_loss:.4f} | "
              f"Overall Connectivity: {val_metrics['overall_connectivity']:.4f} | "
              f"Instance Connectivity: {val_metrics['instance_connectivity']:.4f}")

        scheduler.step()

        checkpoint_path = os.path.join(save_path)
        torch.save(model.state_dict(), checkpoint_path)

    return train_losses, val_metrics_history

def main() -> None:
    set_seed(42)
    device = torch.device("cuda" if torch.cuda.is_available() else "cpu")
    
    base_name = "segmentation"
    results_dir = f"./results_8_{base_name}"
    os.makedirs(results_dir, exist_ok=True)
    
    base_dir = Path("/home/alaie/projects/layout-viewer/Src/App/Python/data_16.csv")
    batch_size = 64
    num_epochs = 10
    
    df = pd.read_csv(base_dir)
    kf = KFold(n_splits=5, shuffle=True, random_state=42)
    
    fold_index = 0
    for train_indices, val_indices in kf.split(df, df["nets_count"]):
        csv_rows = []

        train_df = df.iloc[train_indices]
        print("Train data:")
        print(train_df["nets_count"].value_counts(), "\n")
        
        val_df = df.iloc[val_indices]
        print("Validation data:")
        print(val_df["nets_count"].value_counts(), "\n")
        
        train_dataset = SegmentationDataset(train_df, validation=False, remove_invalid=True, check_data=False, batch_size=batch_size)
        val_dataset = SegmentationDataset(val_df, validation=True, remove_invalid=True, check_data=False, batch_size=batch_size)
        
        train_loader = DataLoader(train_dataset, batch_size=batch_size, collate_fn=pack_batch, shuffle=True, num_workers=4, pin_memory=True)
        val_loader = DataLoader(val_dataset, batch_size=batch_size, collate_fn=pack_batch, shuffle=False, num_workers=4, pin_memory=True)
        
        model = RecurrentUNet(num_classes=3).to(device)
        model.load_state_dict(torch.load("/home/alaie/projects/layout-viewer/Src/App/Python/results_2_lstm_segmentation/model_fold_0_val.pth"))

        optimizer = AdEMAMix(model.parameters(), lr=1e-3, weight_decay=1e-2)
        scheduler = optim.lr_scheduler.CosineAnnealingLR(optimizer, T_max=10, eta_min=1e-5, verbose=True)

        train_losses, val_metrics_history = train_model(model, train_loader, val_loader, optimizer, scheduler, device, num_epochs, os.path.join(results_dir, f"model_fold_{fold_index}_val.pth"))
                
        for epoch in range(num_epochs):
            csv_rows.append({
                "epoch": epoch + 1,
                "train_loss": train_losses[epoch],
                "overall_connectivity": val_metrics_history[epoch]["overall_connectivity"],
                "instance_connectivity": val_metrics_history[epoch]["instance_connectivity"],
            })

        stats_df = pd.DataFrame(csv_rows)
        stats_csv_path = os.path.join(results_dir, f"training_{fold_index}_statistics.csv")
        stats_df.to_csv(stats_csv_path, index=False)
        print(f"Training statistics saved to {stats_csv_path}")
        
        fold_index += 1

if __name__ == "__main__":
    torch.backends.cudnn.benchmark = True

    main()
