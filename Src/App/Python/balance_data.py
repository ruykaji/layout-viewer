import argparse
import os
import pandas as pd
from imblearn.under_sampling import RandomUnderSampler
from scipy.spatial.distance import directed_hausdorff
import numpy as np
from tqdm import tqdm
import net_similarity

def load_data(input_dir):
    """
    Loads CSV files from each folder in the input directory.
    Expects a file "gcells/old_data.csv" inside each folder.
    """
    print("Loading data...")

    all_dfs = []

    for folder in os.listdir(input_dir):
        folder_path = os.path.join(input_dir, folder)
        if not os.path.isdir(folder_path):
            continue

        csv_path = os.path.join(folder_path, "gcells", "data.csv")

        if os.path.exists(csv_path):
            df = pd.read_csv(csv_path)
            all_dfs.append(df)
        else:
            print(f"Warning: {csv_path} does not exist. Skipping folder '{folder}'.")

    if not all_dfs:
        raise ValueError("No CSV files loaded. Please check your directory and file structure.")

    return pd.concat(all_dfs, ignore_index=True)


def balance_undersampling(df, min_samples, column):
    """
    Balance the DataFrame to keep only those classes that have at least 'min_samples' samples.
    """
    class_counts = df[column].value_counts()
    valid_classes = class_counts[class_counts >= min_samples].index.tolist()
    df_filtered = df[df[column].isin(valid_classes)]

    return df_filtered.reset_index(drop=True)


def balance_oversampling(df, target_samples, column):
    """
    Balances the DataFrame by undersampling each class according to the target number of samples.
    """
    non_numeric_columns = df.select_dtypes(include=['object']).columns.tolist()

    X_numeric = df.drop(columns=non_numeric_columns + [column])
    y = df[column]

    class_counts = y.value_counts()
    sampling_strategy = {cls: min(count, target_samples) for cls, count in class_counts.items()}

    undersampler = RandomUnderSampler(sampling_strategy=sampling_strategy)
    X_resampled, y_resampled = undersampler.fit_resample(X_numeric, y)

    df_balanced = pd.DataFrame(X_resampled, columns=X_numeric.columns)
    df_balanced[column] = y_resampled

    df_balanced[non_numeric_columns] = df.sample(n=len(df_balanced), replace=True)[non_numeric_columns].values

    return df_balanced.reset_index(drop=True)


def balance_undersampling_values(df:pd.DataFrame, min_samples, column):
    return df.loc[df[column] >= min_samples].reset_index(drop=True)

def remove_similar_data(df:pd.DataFrame, threshold):
    return net_similarity.remove_similar_data(df, threshold).reset_index(drop=True)

def main():
    parser = argparse.ArgumentParser(description="Balance dataset by undersampling classes.")
    parser.add_argument("--input_dir", type=str, required=True, help="Path to the directory containing folders with CSV files.")
    parser.add_argument("--output_file", type=str, default="data.csv", help="Filename for the balanced dataset CSV output.")
    args = parser.parse_args()

    df = load_data(args.input_dir)
    print("Initial class distribution:")
    print(df["nets_count"].value_counts())

    df_balanced = balance_undersampling_values(df, 3, "nets_count")

    df_balanced = balance_undersampling(df_balanced, 1000, "nets_count")
    df_balanced = balance_oversampling(df_balanced, 100000, "nets_count")

    df_balanced = remove_similar_data(df_balanced, 0.90)

    df_balanced.to_csv(args.output_file, index=False)
    print(f"\nBalanced dataset saved to '{args.output_file}'")
    print("New class distribution:")
    print(df_balanced["nets_count"].value_counts())


if __name__ == "__main__":
    main()
