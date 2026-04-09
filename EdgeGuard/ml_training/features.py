from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path
from typing import Iterable
import csv
import xml.etree.ElementTree as ET
import zipfile

import numpy as np
import pandas as pd

try:
    from sklearn.tree import DecisionTreeClassifier as SklearnDecisionTreeClassifier
except ImportError:
    SklearnDecisionTreeClassifier = None


FAULT_TYPES = ["NORMAL", "BEARING_FAULT", "IMBALANCE"]
VIBRATION_FEATURES = [
    "1x[Vibration]",
    "2x[Vibration]",
    "3x[Vibration]",
    "0-4k_Hz[Vibration]",
    "temp_mot",
]

RAW_CSV_COLUMNS = [
    "device_id",
    "score",
    "x",
    "y",
    "z",
    "temperature",
    "ambient_temperature",
]

CLEANED_CSV_COLUMNS = [
    "source_file",
    "label",
    *RAW_CSV_COLUMNS,
]

CSV_MODEL_FEATURES = [
    "rms",
    "peak",
    "crest",
    "std",
    "dominant_freq_hz",
]

CSV_WINDOW_SIZE = 32
CSV_SAMPLING_RATE_HZ = 100.0


def find_dataset_file(dataset_dir: Path) -> Path:
    excel_files = sorted(dataset_dir.glob("*.xlsx"))
    if not excel_files:
        raise FileNotFoundError(f"No .xlsx file found in {dataset_dir}")
    return excel_files[0]


def list_raw_csv_files(dataset_dir: Path) -> list[Path]:
    return sorted(
        path for path in dataset_dir.glob("*.csv")
        if path.is_file() and path.parent.name.lower() != "cleaned"
    )


def clean_csv_datasets(dataset_dir: Path) -> list[Path]:
    raw_csv_files = list_raw_csv_files(dataset_dir)
    if not raw_csv_files:
        return []

    cleaned_dir = dataset_dir / "cleaned"
    cleaned_dir.mkdir(exist_ok=True)

    cleaned_paths: list[Path] = []
    combined_frames: list[pd.DataFrame] = []

    for csv_path in raw_csv_files:
        cleaned_df = _clean_single_csv(csv_path)
        cleaned_path = cleaned_dir / f"{csv_path.stem}_clean.csv"
        cleaned_df.to_csv(cleaned_path, index=False)
        cleaned_paths.append(cleaned_path)
        combined_frames.append(cleaned_df)

    combined_df = pd.concat(combined_frames, ignore_index=True)
    combined_df.to_csv(cleaned_dir / "combined_labeled_dataset.csv", index=False)
    return cleaned_paths


def load_csv_dataset(dataset_dir: Path) -> pd.DataFrame:
    cleaned_paths = clean_csv_datasets(dataset_dir)
    if not cleaned_paths:
        raise FileNotFoundError(f"No raw CSV files found in {dataset_dir}")
    frames = [pd.read_csv(path) for path in cleaned_paths]
    return pd.concat(frames, ignore_index=True)


def _clean_single_csv(csv_path: Path) -> pd.DataFrame:
    label = normalize_filename_label(csv_path.stem)
    rows: list[dict[str, object]] = []

    with csv_path.open("r", encoding="utf-8", errors="ignore", newline="") as handle:
        reader = csv.reader(handle)
        for raw_row in reader:
            row = [value.strip() for value in raw_row if value is not None]
            if not row:
                continue

            first_cell = row[0].strip()
            if first_cell.startswith("=~=~=~=~=~"):
                continue

            normalized = _normalize_raw_csv_row(row)
            if normalized is None:
                continue

            record = {
                "source_file": csv_path.name,
                "label": label,
            }
            record.update(normalized)
            rows.append(record)

    cleaned_df = pd.DataFrame(rows, columns=CLEANED_CSV_COLUMNS)
    for numeric_column in ["score", "x", "y", "z", "temperature", "ambient_temperature"]:
        cleaned_df[numeric_column] = pd.to_numeric(cleaned_df[numeric_column], errors="coerce")
    return cleaned_df.dropna(subset=["score", "x", "y", "z", "temperature"]).reset_index(drop=True)


def normalize_filename_label(raw_label: str) -> str:
    normalized = raw_label.strip().lower().replace(" ", "").replace("-", "").replace("_", "")

    if normalized in {"normalmachine", "normal"}:
        return "NORMAL"
    if normalized in {"desequilibre", "imbalance"}:
        return "IMBALANCE"
    if normalized in {"blocakge", "blockage", "bearing", "bearingfault", "bearingproblem"}:
        return "BEARING_FAULT"

    return raw_label.strip().upper()


def _normalize_raw_csv_row(row: list[str]) -> dict[str, object] | None:
    useful_values = row[-7:] if len(row) >= 7 else row[-5:]

    if len(useful_values) == 7:
        device_id, score, x, y, z, temperature, ambient_temperature = useful_values
    elif len(useful_values) == 6:
        device_id = ""
        score, x, y, z, temperature, ambient_temperature = useful_values
    elif len(useful_values) == 5:
        device_id = ""
        ambient_temperature = np.nan
        score, x, y, z, temperature = useful_values
    else:
        return None

    return {
        "device_id": device_id,
        "score": score,
        "x": x,
        "y": y,
        "z": z,
        "temperature": temperature,
        "ambient_temperature": ambient_temperature,
    }


def load_dataset(excel_path: Path) -> pd.DataFrame:
    try:
        return pd.read_excel(excel_path)
    except ImportError:
        return _load_xlsx_without_openpyxl(excel_path)


def _load_xlsx_without_openpyxl(excel_path: Path) -> pd.DataFrame:
    ns = {"a": "http://schemas.openxmlformats.org/spreadsheetml/2006/main"}

    def read_shared_strings(handle: zipfile.ZipFile) -> list[str]:
        sst = ET.fromstring(handle.read("xl/sharedStrings.xml"))
        return [
            "".join((node.text or "") for node in item.iterfind(".//a:t", ns))
            for item in sst.findall("a:si", ns)
        ]

    with zipfile.ZipFile(excel_path) as archive:
        shared = read_shared_strings(archive)
        sheet = ET.fromstring(archive.read("xl/worksheets/sheet1.xml"))
        rows = sheet.find("a:sheetData", ns)
        parsed_rows: list[dict[str, object]] = []

        for row in rows:
            values: dict[str, object] = {}
            for cell in row.findall("a:c", ns):
                ref = cell.attrib["r"]
                column_id = "".join(ch for ch in ref if ch.isalpha())
                raw_value = cell.find("a:v", ns)

                if raw_value is None:
                    value: object = np.nan
                elif cell.attrib.get("t") == "s":
                    value = shared[int(raw_value.text)]
                else:
                    value = float(raw_value.text)

                values[column_id] = value

            parsed_rows.append(values)

    headers = parsed_rows[0]
    records = [
        {headers[key]: row.get(key, np.nan) for key in headers}
        for row in parsed_rows[1:]
    ]
    return pd.DataFrame.from_records(records)


def select_feature_columns(df: pd.DataFrame) -> list[str]:
    return [column for column in VIBRATION_FEATURES if column in df.columns]


def prepare_feature_matrix(df: pd.DataFrame, feature_names: Iterable[str]) -> pd.DataFrame:
    feature_frame = df.loc[:, list(feature_names)].copy()
    for column in feature_frame.columns:
        feature_frame[column] = pd.to_numeric(feature_frame[column], errors="coerce")
        feature_frame[column] = feature_frame[column].fillna(feature_frame[column].median())
    return feature_frame


def build_csv_feature_frame(df: pd.DataFrame) -> pd.DataFrame:
    working = df.copy()
    for column in ["x", "y", "z"]:
        working[column] = pd.to_numeric(working[column], errors="coerce")

    working = working.dropna(subset=["x", "y", "z"]).reset_index(drop=True)

    window_records: list[dict[str, float | str]] = []
    for _, group in working.groupby(["source_file", "label"], sort=False):
        window_records.extend(_extract_window_features(group))

    feature_frame = pd.DataFrame.from_records(window_records)
    for column in CSV_MODEL_FEATURES:
        feature_frame[column] = pd.to_numeric(feature_frame[column], errors="coerce")
        feature_frame[column] = feature_frame[column].fillna(feature_frame[column].median())
    return feature_frame


def _extract_window_features(group: pd.DataFrame) -> list[dict[str, float | str]]:
    x_values = group["x"].to_numpy(dtype=float)
    y_values = group["y"].to_numpy(dtype=float)
    z_values = group["z"].to_numpy(dtype=float)
    label = group["label"].iloc[0]
    source_file = group["source_file"].iloc[0]

    records: list[dict[str, float | str]] = []
    for start in range(0, len(group) - CSV_WINDOW_SIZE + 1, CSV_WINDOW_SIZE):
        x_window = x_values[start:start + CSV_WINDOW_SIZE]
        y_window = y_values[start:start + CSV_WINDOW_SIZE]
        z_window = z_values[start:start + CSV_WINDOW_SIZE]
        combined_signal = np.sqrt((x_window ** 2 + y_window ** 2 + z_window ** 2) / 3.0)

        rms = float(np.sqrt(np.mean(combined_signal ** 2)))
        peak = float(np.max(np.abs(combined_signal)))
        crest = float(peak / rms) if rms > 1e-9 else 0.0
        std = float(np.std(combined_signal))
        dominant_freq_hz = float(_dominant_frequency_hz(combined_signal))

        records.append({
            "source_file": source_file,
            "label": label,
            "rms": rms,
            "peak": peak,
            "crest": crest,
            "std": std,
            "dominant_freq_hz": dominant_freq_hz,
        })

    return records


def _dominant_frequency_hz(signal: np.ndarray) -> float:
    if signal.size < 4:
        return 0.0

    centered = signal - np.mean(signal)
    spectrum = np.fft.rfft(centered)
    magnitudes = np.abs(spectrum)
    if magnitudes.size <= 1:
        return 0.0

    magnitudes[0] = 0.0
    dominant_index = int(np.argmax(magnitudes))
    frequencies = np.fft.rfftfreq(signal.size, d=1.0 / CSV_SAMPLING_RATE_HZ)
    return float(frequencies[dominant_index])


def build_fault_type_labels(df: pd.DataFrame) -> tuple[pd.Series, str]:
    if "label" in df.columns:
        return df["label"].astype(str), "filename"

    direct_labels = _extract_labels_from_text_columns(df)
    if direct_labels.notna().any():
        filled = direct_labels.ffill().bfill()
        return filled.astype(str), "metadata"

    return _build_heuristic_fault_labels(df), "heuristic_vibration_signature"


def _extract_labels_from_text_columns(df: pd.DataFrame) -> pd.Series:
    text_columns = [
        column
        for column in df.columns
        if df[column].dtype == object or "file" in column.lower() or "fault" in column.lower()
    ]
    labels = pd.Series(index=df.index, dtype="object")

    for column in text_columns:
        values = df[column].fillna("").astype(str).str.upper()
        labels = labels.combine_first(values.map(_map_text_to_fault_type))

    return labels


def _map_text_to_fault_type(text: str) -> str | None:
    if "BRUSH" in text:
        return "BRUSH_WEAR"
    if "COMM" in text:
        return "COMMUTATOR_FAULT"
    if "BEARING" in text:
        return "BEARING_FAULT"
    return None


def _build_heuristic_fault_labels(df: pd.DataFrame) -> pd.Series:
    band_energy = pd.to_numeric(df["0-4k_Hz[Vibration]"], errors="coerce")
    one_x = pd.to_numeric(df["1x[Vibration]"], errors="coerce")
    two_x = pd.to_numeric(df["2x[Vibration]"], errors="coerce")
    three_x = pd.to_numeric(df["3x[Vibration]"], errors="coerce")

    high_band_threshold = band_energy.quantile(0.66)
    harmonic_sum = one_x + two_x + three_x
    harmonic_threshold = harmonic_sum.quantile(0.45)

    labels = pd.Series("BRUSH_WEAR", index=df.index, dtype="object")
    labels[band_energy >= high_band_threshold] = "BEARING_FAULT"
    commutator_mask = (
        (band_energy < high_band_threshold)
        & ((three_x >= one_x * 1.15) | (harmonic_sum >= harmonic_threshold))
    )
    labels[commutator_mask] = "COMMUTATOR_FAULT"
    return labels


def deterministic_stratified_split(y: pd.Series, train_ratio: float = 0.8) -> tuple[np.ndarray, np.ndarray]:
    train_idx: list[int] = []
    test_idx: list[int] = []

    for label in sorted(y.unique()):
        class_indices = y.index[y == label].to_list()
        split_at = max(1, int(round(len(class_indices) * train_ratio)))
        split_at = min(split_at, len(class_indices) - 1)
        train_idx.extend(class_indices[:split_at])
        test_idx.extend(class_indices[split_at:])

    return np.array(train_idx), np.array(test_idx)


@dataclass
class SimpleTreeNode:
    feature_index: int | None = None
    threshold: float | None = None
    left: "SimpleTreeNode | None" = None
    right: "SimpleTreeNode | None" = None
    class_index: int | None = None


class SimpleDecisionTreeClassifier:
    def __init__(self, max_depth: int = 4, min_samples_leaf: int = 5):
        self.max_depth = max_depth
        self.min_samples_leaf = min_samples_leaf
        self.tree_: SimpleTreeNode | None = None
        self.classes_: np.ndarray | None = None

    def fit(self, X: np.ndarray, y: np.ndarray) -> "SimpleDecisionTreeClassifier":
        self.classes_ = np.unique(y)
        self.tree_ = self._build_tree(X, y, depth=0)
        return self

    def predict(self, X: np.ndarray) -> np.ndarray:
        return np.array([self._predict_row(row, self.tree_) for row in X])

    def _predict_row(self, row: np.ndarray, node: SimpleTreeNode) -> object:
        if node.class_index is not None:
            return self.classes_[node.class_index]
        if row[node.feature_index] <= node.threshold:
            return self._predict_row(row, node.left)
        return self._predict_row(row, node.right)

    def _build_tree(self, X: np.ndarray, y: np.ndarray, depth: int) -> SimpleTreeNode:
        values, counts = np.unique(y, return_counts=True)
        majority_value = values[np.argmax(counts)]
        majority_index = int(np.where(self.classes_ == majority_value)[0][0])

        if (
            depth >= self.max_depth
            or len(values) == 1
            or len(y) <= self.min_samples_leaf * 2
        ):
            return SimpleTreeNode(class_index=majority_index)

        split = self._best_split(X, y)
        if split is None:
            return SimpleTreeNode(class_index=majority_index)

        feature_index, threshold, left_mask, right_mask = split
        return SimpleTreeNode(
            feature_index=feature_index,
            threshold=threshold,
            left=self._build_tree(X[left_mask], y[left_mask], depth + 1),
            right=self._build_tree(X[right_mask], y[right_mask], depth + 1),
        )

    def _best_split(self, X: np.ndarray, y: np.ndarray) -> tuple[int, float, np.ndarray, np.ndarray] | None:
        best_gain = 0.0
        best_split: tuple[int, float, np.ndarray, np.ndarray] | None = None
        parent_gini = self._gini(y)

        for feature_index in range(X.shape[1]):
            values = np.unique(X[:, feature_index])
            if len(values) < 2:
                continue

            thresholds = (values[:-1] + values[1:]) / 2.0
            for threshold in thresholds:
                left_mask = X[:, feature_index] <= threshold
                right_mask = ~left_mask

                if left_mask.sum() < self.min_samples_leaf or right_mask.sum() < self.min_samples_leaf:
                    continue

                gain = parent_gini - (
                    left_mask.mean() * self._gini(y[left_mask])
                    + right_mask.mean() * self._gini(y[right_mask])
                )

                if gain > best_gain:
                    best_gain = gain
                    best_split = (feature_index, float(threshold), left_mask, right_mask)

        return best_split

    @staticmethod
    def _gini(y: np.ndarray) -> float:
        if len(y) == 0:
            return 0.0
        _, counts = np.unique(y, return_counts=True)
        probabilities = counts / counts.sum()
        return 1.0 - np.sum(probabilities**2)


def build_classifier(max_depth: int = 4, min_samples_leaf: int = 5):
    if SklearnDecisionTreeClassifier is not None:
        return SklearnDecisionTreeClassifier(
            max_depth=max_depth,
            min_samples_leaf=min_samples_leaf,
            random_state=42,
        )
    return SimpleDecisionTreeClassifier(
        max_depth=max_depth,
        min_samples_leaf=min_samples_leaf,
    )


def export_rules(model, feature_names: list[str], class_names: list[str]) -> str:
    if hasattr(model, "tree_") and hasattr(model.tree_, "feature"):
        return _export_sklearn_rules(model, feature_names, class_names)
    return _export_simple_rules(model.tree_, feature_names, class_names)


def _export_sklearn_rules(model, feature_names: list[str], class_names: list[str]) -> str:
    tree = model.tree_

    def walk(node_id: int, depth: int) -> list[str]:
        indent = "    " * depth
        if tree.feature[node_id] == -2:
            class_index = int(np.argmax(tree.value[node_id][0]))
            return [f"{indent}return {class_names[class_index]}"]

        feature_name = feature_names[tree.feature[node_id]]
        threshold = tree.threshold[node_id]
        lines = [f"{indent}if {feature_name} <= {threshold:.6f}:"]
        lines.extend(walk(tree.children_left[node_id], depth + 1))
        lines.append(f"{indent}else:")
        lines.extend(walk(tree.children_right[node_id], depth + 1))
        return lines

    return "\n".join(walk(0, 0))


def _export_simple_rules(node: SimpleTreeNode, feature_names: list[str], class_names: list[str], depth: int = 0) -> str:
    indent = "    " * depth
    if node.class_index is not None:
        return f"{indent}return {class_names[node.class_index]}"

    lines = [f"{indent}if {feature_names[node.feature_index]} <= {node.threshold:.6f}:"]
    lines.append(_export_simple_rules(node.left, feature_names, class_names, depth + 1))
    lines.append(f"{indent}else:")
    lines.append(_export_simple_rules(node.right, feature_names, class_names, depth + 1))
    return "\n".join(lines)
