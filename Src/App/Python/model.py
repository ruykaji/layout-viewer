import torch
import torch.nn as nn
import timm

from typing import List, Tuple
from segmentation_models_pytorch.base import SegmentationHead
from segmentation_models_pytorch.decoders.unet.decoder import UnetDecoder

class ConvGRUCell(nn.Module):
    def __init__(self, input_channels, kernel_size=3, padding=1):
        super(ConvGRUCell, self).__init__()
        self.hidden_channels = input_channels * 2

        self.conv_r = nn.Conv2d(
            in_channels=input_channels + self.hidden_channels,
            out_channels=self.hidden_channels,
            kernel_size=kernel_size,
            padding=padding
        )

        self.conv_z = nn.Conv2d(
            in_channels=input_channels + self.hidden_channels,
            out_channels=self.hidden_channels,
            kernel_size=kernel_size,
            padding=padding
        )

        self.conv_n = nn.Conv2d(
            in_channels=input_channels + self.hidden_channels,
            out_channels=self.hidden_channels,
            kernel_size=kernel_size,
            padding=padding
        )

    def forward(self, x: torch.Tensor, h_prev: torch.Tensor) -> torch.Tensor:
        if h_prev is None:
            h_prev = torch.zeros(x.shape[0], self.hidden_channels, x.shape[2], x.shape[3], device=x.device)

        combined = torch.cat([x, h_prev], dim=1)

        r = torch.sigmoid(self.conv_r(combined))
        z = torch.sigmoid(self.conv_z(combined))

        combined_reset = torch.cat([x, r * h_prev], dim=1)
        n = torch.tanh(self.conv_n(combined_reset))

        h_next = (1 - z) * n + z * h_prev
        return h_next

class ConvLSTMCell(nn.Module):
    def __init__(self, channels, kernel_size):
        super(ConvLSTMCell, self).__init__()
        self.conv = nn.Conv2d(channels * 2, channels * 4, kernel_size, padding=kernel_size // 2)

    def forward(self, x: torch.Tensor, h_c: Tuple[torch.Tensor, torch.Tensor]) -> Tuple[torch.Tensor, torch.Tensor]:
        h, c = h_c
        combined = torch.cat([x, h], dim=1)
        conv_output = self.conv(combined)
        cc_i, cc_f, cc_o, cc_g = torch.split(conv_output, h.shape[1], dim=1)
        i = torch.sigmoid(cc_i)
        f = torch.sigmoid(cc_f)
        o = torch.sigmoid(cc_o)
        g = torch.tanh(cc_g)
        c_new = f * c + i * g
        h_new = o * torch.tanh(c_new)
        return h_new, c_new
    
class EncoderResNet34(nn.Module):
    def __init__(self):
        super(EncoderResNet34, self).__init__()
        self.encoder = timm.create_model(
            model_name="resnet34", features_only=True, pretrained=True, scriptable=True
        )
        self.stages = nn.ModuleList([
            nn.Identity(),
            nn.Sequential(self.encoder.conv1, self.encoder.bn1, self.encoder.act1),
            nn.Sequential(self.encoder.maxpool, self.encoder.layer1),
            self.encoder.layer2,
            self.encoder.layer3,
            self.encoder.layer4,
        ])
        self.num_levels = len(self.stages)

    def forward(self, x: torch.Tensor, mask: torch.Tensor = None) -> List[torch.Tensor]:
        T, B, C, H, W = x.size(0), x.size(1), x.size(2), x.size(3), x.size(4)

        x = x.reshape(T * B, C, H, W)
        features = []

        for stage in self.stages:
            x = stage(x)

            C_new, H_new, W_new = x.size(1), x.size(2), x.size(3)
            stage_features = x.reshape(T, B, C_new, H_new, W_new)

            if mask is not None:
                new_stage_features = [stage_features[0]]

                for t in range(1, T):
                    mask_t = mask[t].view(B, 1, 1, 1)
                    new_feat = torch.where(mask_t > 0, stage_features[t], new_stage_features[-1])
                    new_stage_features.append(new_feat)

                stage_features = torch.stack(new_stage_features, dim=0)
            features.append(stage_features)
        return features

class SequenceEncoder(nn.Module):
    def __init__(self, in_channels: List[int]):
        super(SequenceEncoder, self).__init__()
        self.fwd_conv_lstm_cells = nn.ModuleList([ConvLSTMCell(ch, 3) for ch in in_channels])
        self.bwd_conv_lstm_cells = nn.ModuleList([ConvLSTMCell(ch, 3) for ch in in_channels])

    def forward(self, features: List[torch.Tensor], mask: torch.Tensor) -> List[torch.Tensor]:
        agg_hs: List[torch.Tensor] = []

        for level, (fwd_cell, bwd_cell) in enumerate(zip(self.fwd_conv_lstm_cells, self.bwd_conv_lstm_cells)):
            level_feats = features[level]
            batch_size, T_feat = level_feats.size(1), level_feats.size(0)

            fwd_state = (torch.zeros_like(level_feats[0]), torch.zeros_like(level_feats[0]))
            bwd_state = (torch.zeros_like(level_feats[0]), torch.zeros_like(level_feats[0]))

            for t in range(T_feat):
                feat = level_feats[t]
                mask_t = mask[t].view(batch_size, 1, 1, 1)

                new_fwd_state = fwd_cell(feat, fwd_state)
                fwd_state = (torch.where(mask_t > 0, new_fwd_state[0], fwd_state[0]), torch.where(mask_t > 0, new_fwd_state[1], fwd_state[1]))

                new_bwd_state = bwd_cell(feat, bwd_state)
                bwd_state = (torch.where(mask_t > 0, new_bwd_state[0], bwd_state[0]), torch.where(mask_t > 0, new_bwd_state[1], bwd_state[1]))
            
            agg_h = (fwd_state[0] + bwd_state[0]) / 2.0
            agg_hs.append(agg_h)
        
        return agg_hs


class RecurrentUNet(nn.Module):
    def __init__(self, num_classes=2, **kwargs):
        super(RecurrentUNet, self).__init__(**kwargs)
        self.encoder_channels = (3, 64, 64, 128, 256, 512)
        self.decoder_channels = (256, 128, 64, 32, 16)

        self.encoder = torch.jit.script(EncoderResNet34())
        self.sequence_encoder = torch.jit.script(SequenceEncoder(self.encoder_channels))

        self.decoder = UnetDecoder(
            encoder_channels=self.encoder_channels,
            decoder_channels=self.decoder_channels,
            n_blocks=len(self.encoder_channels) - 1,
            use_batchnorm=True,
            center=False,
            attention_type=None,
        )

        self.head = SegmentationHead(self.decoder_channels[-1], num_classes, kernel_size=1, activation=None)

    def forward(self, inputs, mask):
        x_seq, _ = torch.nn.utils.rnn.pad_packed_sequence(inputs, batch_first=True)
        T = x_seq.size(1)

        x_seq = x_seq.transpose(0, 1)
        mask = mask.transpose(0, 1)

        encoder_features = self.encoder(x_seq, mask)
        agg_hs = self.sequence_encoder(encoder_features, mask)
        
        x_dec = self.decoder(*agg_hs)
        output = self.head(x_dec)

        return output
