from tqdm import tqdm
import torch
import torch.nn as nn
from torch.utils.data import Dataset, DataLoader
import pandas as pd
import numpy as np


DATASET_PATH = "/content/drive/MyDrive/Colab Notebooks/complete_fixed.txt"
PMAT_INDEX = 0


class PositionDataset(Dataset):
    def __init__(self, file):
        data = torch.tensor(pd.read_csv(file, header=None).astype(np.float32).values)
        cols = data.shape[1]
        assert cols % 2 == 1, "Must print heuristic value for both sides"
        self.num_heuristics = (cols - 1) // 2
        heuristics = (
            data[:, : self.num_heuristics]
            - data[:, self.num_heuristics : 2 * self.num_heuristics]
        )
        self.scale = 10000
        self.data = torch.cat(
            (heuristics / self.scale, data[:, 2 * self.num_heuristics :]), dim=1
        )

    def __len__(self):
        return len(self.data)

    def __getitem__(self, idx):
        return self.data[idx]


TRAIN_SPLIT = 0.8
BS = 16384
LR = 0.001
# games_output should be csv file with no header, 21 values per row
# <10 eval scores for white>,<10 eval scores for black>,<game outcome (0, 0.5, or 1)>
dataset = PositionDataset(DATASET_PATH)
num_heuristics = dataset.num_heuristics
train_size = int(TRAIN_SPLIT * len(dataset))
test_size = len(dataset) - train_size
train_dataset, test_dataset = torch.utils.data.random_split(
    dataset, [train_size, test_size]
)
train_loader, test_loader = DataLoader(
    train_dataset, batch_size=BS, shuffle=True, num_workers=8
), DataLoader(test_dataset, batch_size=BS, shuffle=True, num_workers=8)
model = nn.Linear(num_heuristics, 1, bias=False)
criterion = nn.BCEWithLogitsLoss()
optim = torch.optim.Adam(model.parameters(), lr=LR)
# usually 20-30 epochs is sufficient
# train/test loss should be around 4e-5 (or lower)
for epoch in range(30):
    tot_train_loss = 0
    tot_test_loss = 0
    tot_train, tot_test = 0, 0
    for i, data in enumerate(tqdm(train_loader)):
        tot_train += len(data)
        x = data[:, :num_heuristics]
        y = data[:, -1]
        optim.zero_grad()
        out = model(x).squeeze(-1)
        loss = criterion(out, y)
        # L2 regularization
        loss += torch.sum(torch.square(model.weight)) * 3e-6
        loss.backward()
        optim.step()
        tot_train_loss += loss.item()
    print(f"Epoch {epoch}: train loss {tot_train_loss/tot_train}")
    with torch.no_grad():
        for i, data in enumerate(test_loader):
            tot_test += len(data)
            x = data[:, :num_heuristics]
            y = data[:, -1]
            out = model(x).squeeze(-1)
            loss = criterion(out, y)
            tot_test_loss += loss.item()
    print(f"Epoch {epoch}: test loss {tot_test_loss/tot_test}")
    torch.set_printoptions(sci_mode=False)

    # in eval() PMAT is implicitly 1 so we scale everything to make this true
    weights = (model.weight * dataset.scale).squeeze() / 10000
    print("Weights: ", (weights / weights[PMAT_INDEX]).data)
    # print("Bias: ", bias.data)
