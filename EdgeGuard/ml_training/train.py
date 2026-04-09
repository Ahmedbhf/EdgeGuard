from __future__ import annotations

from pathlib import Path

from features import (
    CSV_MODEL_FEATURES,
    build_classifier,
    build_csv_feature_frame,
    build_fault_type_labels,
    clean_csv_datasets,
    deterministic_stratified_split,
    export_rules,
    find_dataset_file,
    list_raw_csv_files,
    load_csv_dataset,
    load_dataset,
    prepare_feature_matrix,
    select_feature_columns,
)


def main() -> None:
    base_dir = Path(__file__).resolve().parent
    dataset_dir = base_dir / "dataset"
    raw_csv_files = list_raw_csv_files(dataset_dir)

    if raw_csv_files:
        cleaned_paths = clean_csv_datasets(dataset_dir)
        df = load_csv_dataset(dataset_dir)
        print("Loaded labeled CSV datasets:")
        for path in cleaned_paths:
            print(f"- {path.name}")
    else:
        excel_path = find_dataset_file(dataset_dir)
        df = load_dataset(excel_path)
        print(f"Loaded dataset: {excel_path.name}")

    print("Columns:")
    for column in df.columns:
        print(f"- {column}")

    if raw_csv_files:
        selected_features = CSV_MODEL_FEATURES
        X_frame = build_csv_feature_frame(df)
        y, label_source = build_fault_type_labels(X_frame)
        X_frame = X_frame.loc[:, selected_features].copy()
    else:
        selected_features = select_feature_columns(df)
        X_frame = prepare_feature_matrix(df, selected_features)
        y, label_source = build_fault_type_labels(df)

    train_idx, test_idx = deterministic_stratified_split(y)
    X_train = X_frame.loc[train_idx].to_numpy()
    X_test = X_frame.loc[test_idx].to_numpy()
    y_train = y.loc[train_idx].to_numpy()
    y_test = y.loc[test_idx].to_numpy()

    model = build_classifier(max_depth=4, min_samples_leaf=5)
    model.fit(X_train, y_train)
    accuracy = (model.predict(X_test) == y_test).mean()
    rules = export_rules(model, selected_features, list(model.classes_))

    print()
    print("Selected feature names:")
    for feature in selected_features:
        print(f"- {feature}")

    print()
    print(f"Label source: {label_source}")
    print("Label counts:")
    print(y.value_counts().to_string())

    print()
    print(f"Model accuracy: {accuracy:.4f}")
    print()
    print("Final decision rules:")
    print(rules)


if __name__ == "__main__":
    main()
