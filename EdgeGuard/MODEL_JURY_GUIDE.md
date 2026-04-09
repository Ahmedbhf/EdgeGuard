# EdgeGuard Model Explanation and Jury Guide

## 1. What the Model Does

The model classifies the machine condition into:

- `NORMAL`
- `IMBALANCE`
- `BEARING_FAULT`

It works in two stages:

1. `NanoEdge similarity` checks whether the behavior is normal or suspicious.
2. If the similarity is low, a `vibration-based decision tree` identifies the likely fault type.

This makes the system easier to explain:

- NanoEdge answers: "Is the machine behaving normally?"
- The decision tree answers: "If not normal, what fault pattern does it resemble?"

---

## 2. End-to-End Flow

```text
STM32 sensor data -> UART -> Qt app -> rolling vibration buffer
-> feature extraction -> decision tree rules -> stable fault output -> UI
```

Detailed path:

1. STM32 sends live values over UART:
   - similarity score
   - `x`
   - `y`
   - `z`
   - temperature

2. The Qt app receives them in:
   - `backend/services/serial_service.cpp`

3. The app stores recent `x/y/z` values in rolling buffers in:
   - `backend/controllers/app_controller.cpp`

4. The app computes vibration features from the rolling buffer:
   - `rms`
   - `peak`
   - `std`
   - `dominantFreqHz`

5. If similarity is high, the displayed state is:
   - `NORMAL`

6. If similarity is low, the decision tree rules classify the fault type.

7. A smoothing and cooldown layer prevents rapid switching of labels.

8. The final result is displayed on the dashboard.

---

## 3. What Data the Model Uses

### Runtime Data

At runtime, the classifier uses only vibration-derived data:

- `x`
- `y`
- `z`

Temperature is still available in the application, but the current fault classifier does not use it.

### Training Data

The offline training data comes from three labeled CSV files from the same motor setup:

- `normalMachine.csv` -> `NORMAL`
- `desequilibre.csv` -> `IMBALANCE`
- `blocakge.csv` -> `BEARING_FAULT`

The label is taken from the filename, so the dataset acts as the training source of truth.

---

## 4. How Features Are Computed

The live classifier does not use raw `x`, `y`, and `z` directly.

Instead, it builds a combined vibration signal:

```text
combined = sqrt((x² + y² + z²) / 3)
```

From this combined signal, the model computes:

### 4.1 RMS

Overall vibration level:

```text
rms = sqrt(mean(combined²))
```

### 4.2 Peak

Maximum vibration amplitude:

```text
peak = max(abs(combined))
```

### 4.3 Standard Deviation

Measures how spread or unstable the vibration is:

```text
std = standard_deviation(combined)
```

### 4.4 Dominant Frequency in Hz

The app computes an FFT on the combined signal and selects the frequency with the highest magnitude.

This is expressed in real Hz using:

- sampling rate = `100 Hz`

This is important because the rule thresholds can then be reused directly in Qt.

---

## 5. How the Decision Tree Works

The trained decision tree was exported from Python and translated into C++ rules.

Current runtime logic:

```cpp
if (std <= 48.784777f)
{
    if (dominantFreqHz <= 20.3125f)
    {
        return "NORMAL";
    }
    else
    {
        if (std <= 42.846477f)
        {
            if (peak <= 692.026589f)
                return "NORMAL";
            else
                return "IMBALANCE";
        }
        else
        {
            return "IMBALANCE";
        }
    }
}
else
{
    return "BEARING_FAULT";
}
```

Simple interpretation:

- High `std` means vibration is unstable -> likely `BEARING_FAULT`
- Lower `std` with low dominant frequency -> likely `NORMAL`
- Otherwise, strong off-normal frequency behavior with higher peak -> likely `IMBALANCE`

---

## 6. Why We Chose a Decision Tree

We chose a decision tree because it is:

- lightweight
- easy to train
- easy to explain
- easy to convert into C++
- suitable for real-time embedded-style logic

This is better for a demo and jury defense than a black-box model.

You can show the exact rules and explain why the app made a decision.

---

## 7. Where Training Happens

Training happens offline in Python inside:

- `ml_training/train.py`
- `ml_training/features.py`

Steps:

1. Clean CSV files
2. Build labeled dataset from filenames
3. Extract vibration windows
4. Compute features
5. Train a `DecisionTreeClassifier`
6. Export readable rules
7. Copy the rules into the Qt backend

Important:

- `scikit-learn` is not used inside the Qt runtime
- only the exported rules are used in C++

So the app stays lightweight and deployment stays simple.

---

## 8. How Real-Time Stability Is Handled

The app does not display every raw prediction immediately.

It uses:

### 8.1 Similarity Gate

If similarity is high:

- output = `NORMAL`

This prevents false fault classification in clearly normal states.

### 8.2 Majority Vote

The app keeps a short history of recent predictions and uses the most frequent one.

This reduces flickering.

### 8.3 Cooldown

A new fault type must persist before replacing the displayed one.

This prevents rapid switching caused by transient noise.

---

## 9. Main Strengths

- Uses real motor data from the same setup
- Uses vibration-only fault classification
- Easy to explain and defend
- Lightweight enough for real-time Qt integration
- Transparent rule-based decisions
- Stable runtime behavior because of smoothing and similarity gating

---

## 10. Current Limitations

These are important to say honestly during defense.

### 10.1 Small Number of Classes

The current model only supports:

- `NORMAL`
- `IMBALANCE`
- `BEARING_FAULT`

It does not currently identify other faults such as:

- commutator fault
- brush wear
- shaft misalignment

### 10.2 Small Dataset

The model was trained on a limited set of labeled recordings.

So high accuracy on this dataset does not automatically guarantee strong generalization to all operating conditions.

### 10.3 Filename-Based Labels

The labels come from the file names.

This is acceptable only if each file really contains one pure condition.

### 10.4 Fixed Sampling Rate Assumption

The frequency feature uses:

- `100 Hz`

If the true runtime sampling behavior changes significantly, FFT-based thresholds may drift.

### 10.5 Manual Rule Transfer

The model is trained in Python, but the Qt app uses manually transferred rules.

That means:

- retraining requires updating the C++ rules again

### 10.6 Not a Full End-to-End ML Pipeline

The final application combines:

- NanoEdge similarity
- hand-deployed decision tree rules

So it is a hybrid system, not a single monolithic ML model.

---

## 11. Best Jury Explanation in One Minute

You can say:

> Our system first receives live vibration data from the motor through STM32 over UART. In the Qt application, we keep a rolling buffer of the X, Y, and Z vibration axes. From that buffer, we compute meaningful signal features such as RMS, peak, standard deviation, and dominant frequency using FFT. We trained a lightweight decision tree offline using labeled datasets from the same motor. Then we converted the learned rules into C++ so the application can classify faults in real time. To improve reliability, we only classify faults when NanoEdge similarity indicates abnormal behavior, and we also use smoothing and cooldown to avoid unstable label switching.

---

## 12. Jury Questions and Good Answers

### Q1. Why not use raw x, y, z directly?

Good answer:

Raw axes are noisy and less meaningful by themselves. Signal features like RMS, peak, standard deviation, and dominant frequency better describe vibration behavior and make the model easier to interpret and transfer into C++.

### Q2. Why did you choose a decision tree?

Good answer:

Because it is lightweight, transparent, easy to explain, and easy to deploy in C++. It is more suitable for a demo-oriented real-time desktop application than a complex black-box model.

### Q3. Why do you still use NanoEdge similarity if you already have a classifier?

Good answer:

Similarity is used as the first safety gate. If the machine is clearly normal, we do not force a fault label. Only suspicious states go into fault-type classification. This reduces false positives.

### Q4. Why is the model not directly loaded from sklearn inside Qt?

Good answer:

For deployment simplicity and reliability. Instead of embedding Python or sklearn into the Qt app, we export the trained decision rules and implement them directly in C++.

### Q5. Is the dataset really the source of truth?

Good answer:

For training, yes. The labeled recordings from the same motor are the source of truth used to learn the model. For runtime inference, the source of truth becomes the live sensor data.

### Q6. What are the most important features?

Good answer:

In the current trained model, the most important features are vibration standard deviation, dominant frequency, and peak amplitude.

### Q7. Why does bearing fault correspond to high standard deviation?

Good answer:

Because bearing defects often create unstable and irregular vibration patterns. That increases the spread of the signal, which is captured by the standard deviation.

### Q8. What is the role of FFT here?

Good answer:

FFT transforms the time-domain vibration signal into the frequency domain. This allows us to detect the dominant vibration frequency, which helps separate normal behavior from imbalance-like behavior.

### Q9. What happens if the similarity is high but the vibration classifier would predict a fault?

Good answer:

We prioritize normal behavior in that case. High similarity means the system matches the learned healthy reference closely, so fault classification is intentionally blocked.

### Q10. What are the limitations of your model?

Good answer:

The current model supports only a small number of fault classes, uses a limited dataset, and assumes a fixed sampling rate. It is a strong prototype, but more data and more fault conditions would improve robustness.

### Q11. What would you improve next?

Good answer:

I would collect more labeled data under different loads and speeds, add more fault classes, validate the real sampling rate precisely, and automate the export of trained rules into the C++ backend.

### Q12. Why is this suitable for real-time use?

Good answer:

Because the runtime model is just a small set of simple rules applied to lightweight vibration features. It is computationally cheap and well-suited for live monitoring.

---

## 13. Files Related to the Model

### Training

- `ml_training/train.py`
- `ml_training/features.py`
- `ml_training/dataset/`

### Runtime Classification

- `backend/controllers/app_controller.h`
- `backend/controllers/app_controller.cpp`
- `backend/services/serial_service.cpp`

### UI Display

- `Dashboard.qml`

---

## 14. Final Honest Summary

This is a practical hybrid predictive-maintenance prototype.

It is not a giant AI system.

It is a clear and defendable engineering pipeline:

- collect vibration data
- compute informative features
- train a lightweight model offline
- transfer the learned rules into C++
- classify faults in real time
- stabilize the result for a usable dashboard

That makes it strong for:

- demonstration
- jury explanation
- industrial prototype discussion
- future extension
