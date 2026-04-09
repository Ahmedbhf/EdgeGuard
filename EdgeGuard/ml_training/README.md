This folder is used for offline machine learning training.
It does not affect the Qt application.

The current training script uses vibration and motor temperature features only.
If the dataset does not contain explicit fault-type labels, it falls back to a heuristic label mapping so the pipeline can still be prototyped offline.

Raw CSV logs placed in `dataset/` are preserved as-is.
Cleaned copies with headers and a `label` column derived from each filename are generated into `dataset/cleaned/`.
