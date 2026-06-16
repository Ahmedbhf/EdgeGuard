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
    var padding = Math.max(spread * 0.18, 1)
    var nextMin = minValue - padding
    return {
        min: clampMinToZero ? Math.max(0, nextMin) : nextMin,
        max: maxValue + padding
    }
}


