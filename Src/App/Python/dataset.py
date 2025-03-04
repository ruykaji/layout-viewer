import torch
import random
import net_connectivity

import matplotlib.pyplot as plt
import torch.nn.functional as F
import numpy as np
import pandas as pd

import torchvision.transforms.v2 as transforms
from torch.nn.utils.rnn import pad_sequence, pack_padded_sequence
from torch.utils.data import Dataset
from tqdm import tqdm

def load_numpy(file_path, dtype=torch.float64):
    raw_data = np.transpose(np.load(file_path), axes=(2, 0, 1))
    return torch.tensor(raw_data, dtype=dtype)

def pad_and_resize(tensor, pad_size=(64, 64), resize_size=(128, 128)):
    pad_h = pad_size[0] - tensor.shape[-2]
    pad_w = pad_size[1] - tensor.shape[-1]

    padded = F.pad(tensor, (0, pad_w, 0, pad_h, 0, 0), mode='constant', value=0)
    
    if pad_size != resize_size:
        resized = F.interpolate(padded.unsqueeze(0), size=resize_size, mode='nearest').squeeze(0)
        return resized
    
    return padded

def pack_batch(batch):
    source, target = zip(*batch)
    lengths = torch.tensor([t.size(0) for t in source])
    
    padded_source = pad_sequence(source, batch_first=True, padding_value=0)
    
    batch_size, max_length = padded_source.shape[0], padded_source.shape[1]
    mask = torch.arange(max_length).unsqueeze(0).expand(batch_size, max_length).to(lengths.device) < lengths.unsqueeze(1)
    mask = mask.float()
    
    packed_source = pack_padded_sequence(padded_source, lengths, batch_first=True, enforce_sorted=False)
    
    return packed_source, torch.stack(target), mask

class ShuffleSequencePair:
    def __call__(self, source):
        T = source.shape[0]
        shuffled_indices = torch.randperm(T)

        return source[shuffled_indices]

    def __repr__(self):
        return self.__class__.__name__ + "()"

class SegmentationDataset(Dataset):
    def __init__(self, df: pd.DataFrame, validation: bool = False, remove_invalid: bool = False, check_data: bool = False, batch_size: int = 32, shuffle: bool = True):
        super(SegmentationDataset, self).__init__()
        self.validation = validation
        self.batch_size = batch_size
        self.shuffle = shuffle
        self.remove_invalid = remove_invalid

        self.sources = []
        self.targets = []
        self.nets = []
        self.valid_lengths = []

        self.normalize = transforms.Normalize(mean=(0.485, 0.456, 0.406), std=(0.229, 0.224, 0.225))
        self.seq_transform = ShuffleSequencePair()
        self.transforms = transforms.Compose([
            transforms.RandomHorizontalFlip(0.0 if self.validation else 0.5), 
            transforms.RandomVerticalFlip(0.0 if self.validation else 0.5),
            transforms.RandomChoice([
                transforms.RandomRotation((90, 90)),
                transforms.RandomRotation((180, 180)),
                transforms.RandomRotation((270, 270))
            ]) if not self.validation else transforms.Lambda(lambda x: x)
        ])

        for _, row in tqdm(df.iterrows(), total=len(df), desc="Prepare data", leave=False):
            if check_data:
                nets = [self._process_net(row['net'])]
                source_h_np = np.expand_dims(np.load(row['source_h']).astype(np.float32), axis=0)
                source_v_np = np.expand_dims(np.load(row['source_v']).astype(np.float32), axis=0)
                target_np   = np.expand_dims(np.transpose(np.load(row['target_path']).astype(np.float32), (2, 0, 1)), axis=0)
                overall_result, general_result = net_connectivity.check_connectivity(nets, target_np)

                if overall_result != 1.0 or general_result != 1.0:
                    print(row['source_h'])
                    print(nets)

                    fig, axes = plt.subplots(1, 3, figsize=(15, 5))
                    axes[0].imshow(source_h_np[0][..., 0], cmap='plasma')
                    axes[0].set_title("Source H")
                    axes[0].axis('off')
                    axes[1].imshow(source_v_np[0][..., 0], cmap='plasma')
                    axes[1].set_title("Source V")
                    axes[1].axis('off')
                    axes[2].imshow(target_np[0][0], cmap='plasma')
                    axes[2].set_title("Target Path")
                    axes[2].axis('off')

                    plt.tight_layout()
                    plt.show()
                    plt.close()

                    raise ValueError(f"Error in training data: {row['source_h']} with overall result - {overall_result}")

            if remove_invalid:
                nets = [self._process_net(row['net'])]
                target_np = np.expand_dims(np.transpose(np.load(row['target_path']).astype(np.float32), (2, 0, 1)), axis=0)
                overall_result, general_result = net_connectivity.check_connectivity(nets, target_np)

                if overall_result != 1.0 or general_result != 1.0:
                    continue

            self.sources.append((row['source_h'], row['source_v']))
            self.targets.append(row['target_path'])

            if self.validation:
                self.nets.append(self._process_net(row['net']))

        print(f"{'Validation' if self.validation else 'Training'} data length: {len(self.sources)}")
    
    def get_nets(self, idx):
        start = self.batch_size * idx
        end = self.batch_size * (idx + 1)
        return self.nets[start:end]

    def _process_net(self, path_txt: str):
        netlist = []

        with open(path_txt, 'r') as f:
            lines = f.readlines()

        current_net = None

        for line in lines[:-3]:
            line = line.strip()

            if line == "BEGIN":
                current_net = None
            elif current_net is None:
                current_net = line
                netlist.append([])
            elif line == "END":
                current_net = None
            else:
                coordinates = tuple(map(int, line.split(',')))
                netlist[-1].append(coordinates)

        return netlist
    
    def _load_data(self, source_h_path, source_v_path, target_path):
        source_h = load_numpy(source_h_path, dtype=torch.float32)
        source_v = load_numpy(source_v_path, dtype=torch.float32)
        target = load_numpy(target_path, dtype=torch.float32)

        source_h, source_v, target = self.transforms(source_h, source_v, target)

        source_h = pad_and_resize(source_h).unsqueeze(-1)
        source_v = pad_and_resize(source_v).unsqueeze(-1)
        target = pad_and_resize(target).squeeze(0).long()

        third_channel = source_h * source_v
        source = torch.cat([source_h, source_v, third_channel], dim=-1).permute(0, 3, 1, 2)  # shape: (T, 3, H, W)

        source = self.seq_transform(source)
        source, target = self.normalize(source, target.squeeze(1))
        
        return source, target
    
    def __len__(self):
        return len(self.sources)
    
    def __getitem__(self, idx):
        source = self.sources[idx]
        target = self.targets[idx]

        return self._load_data(source[0], source[1], target)
