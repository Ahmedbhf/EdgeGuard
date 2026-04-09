.pragma library

// Compute a padded min/max range for charts that display a full data set.
function computeRange(values, clampMinToZero) {
    if (clampMinToZero === undefined)
        clampMinToZero = true

    if (!values || values.length === 0)
        return { min: 0, max: 1 }

    var minValue = values[0]
    var maxValue = values[0]
    for (var i = 1; i < values.length; ++i) {
        minValue = Math.min(minValue, values[i])
        maxValue = Math.max(maxValue, values[i])
    }

    if (minValue === maxValue) {
        var pad = Math.max(1, Math.abs(minValue) * 0.2)
        var flatMin = minValue - pad
        return {
            min: clampMinToZero ? Math.max(0, flatMin) : flatMin,
            max: maxValue + pad
        }
    }

    var spread = maxValue - minValue
    var nextMin = minValue - spread * 0.12
    return {
        min: clampMinToZero ? Math.max(0, nextMin) : nextMin,
        max: maxValue + spread * 0.12
    }
}

// Compute a readable Y-axis maximum for compact live trend charts.
function computeDynamicMax(values) {
    if (!values || values.length === 0)
        return 1.0

    var maxValue = values[0]
    for (var i = 1; i < values.length; ++i) {
        if (values[i] > maxValue)
            maxValue = values[i]
    }

    var padded = maxValue * 1.2
    if (padded <= 1)
        padded = 1
    else if (padded <= 2)
        padded = 2
    else if (padded <= 5)
        padded = 5
    else if (padded <= 10)
        padded = 10
    else if (padded <= 20)
        padded = 20
    else if (padded <= 50)
        padded = 50
    else if (padded <= 100)
        padded = 100
    else if (padded <= 200)
        padded = 200
    else
        padded = Math.ceil(padded / 50) * 50

    return padded
}
