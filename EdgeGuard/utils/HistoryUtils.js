.pragma library

// Parse one timestamp cell from the CSV history file.
// Accepts either a directly parseable ISO date string or a simple HH:mm[:ss] value.
function parseTimestamp(text, sampleIndex) {
    var trimmed = text.trim()
    var direct = Date.parse(trimmed)
    if (!isNaN(direct))
        return direct

    var parts = trimmed.split(":")
    if (parts.length < 2)
        return sampleIndex * 1000

    var h = parseInt(parts[0], 10)
    var m = parseInt(parts[1], 10)
    var s = parts.length > 2 ? parseFloat(parts[2]) : 0
    if (isNaN(h) || isNaN(m) || isNaN(s))
        return sampleIndex * 1000

    var base = new Date()
    base.setHours(0, 0, 0, 0)
    return base.getTime() + (((h * 60) + m) * 60 + s) * 1000
}

function computeRms(x, y, z) {
    return Math.sqrt(((x * x) + (y * y) + (z * z)) / 3.0)
}

// Convert the CSV text into chart-ready arrays.
// This keeps file parsing and simple derived metrics out of QML UI code.
function parseCsv(csvText) {
    if (!csvText || csvText.length === 0)
        return { ok: false, error: "No stored history is available yet." }

    var lines = csvText.split(/\r?\n/)
    if (lines.length < 2)
        return { ok: false, error: "No samples stored in the last 24 hours yet." }

    var headers = lines[0].split(",")
    var timeIndex = headers.indexOf("timestamp")
    var anomalyIndex = headers.indexOf("anomaly")
    var xIndex = headers.indexOf("x")
    var yIndex = headers.indexOf("y")
    var zIndex = headers.indexOf("z")
    var tempIndex = headers.indexOf("temp")
    if (timeIndex < 0 || anomalyIndex < 0 || xIndex < 0 || yIndex < 0 || zIndex < 0 || tempIndex < 0)
        return { ok: false, error: "Stored CSV must contain timestamp, anomaly, x, y, z, and temp columns." }

    var rmsValues = []
    var anomalyValues = []
    var tempValues = []
    var anomalyPoints = []
    var rmsPoints = []
    var tempPoints = []
    var firstMs = -1
    for (var i = 1; i < lines.length; ++i) {
        var line = lines[i].trim()
        if (!line)
            continue

        var fields = line.split(",")
        if (fields.length <= Math.max(timeIndex, anomalyIndex, xIndex, yIndex, zIndex, tempIndex))
            continue

        var anomalyValue = parseFloat(fields[anomalyIndex].trim())
        var xValue = parseFloat(fields[xIndex].trim())
        var yValue = parseFloat(fields[yIndex].trim())
        var zValue = parseFloat(fields[zIndex].trim())
        var tempValue = parseFloat(fields[tempIndex].trim())
        if (isNaN(anomalyValue) || isNaN(xValue) || isNaN(yValue) || isNaN(zValue) || isNaN(tempValue))
            continue

        var rawMs = parseTimestamp(fields[timeIndex], rmsValues.length)
        var pointMs = rawMs
        if (firstMs < 0)
            firstMs = pointMs

        var rmsValue = computeRms(xValue, yValue, zValue)

        anomalyPoints.push({ x: pointMs, y: anomalyValue })
        rmsPoints.push({ x: pointMs, y: rmsValue })
        tempPoints.push({ x: pointMs, y: tempValue })
        anomalyValues.push(anomalyValue)
        rmsValues.push(rmsValue)
        tempValues.push(tempValue)
    }

    if (anomalyValues.length === 0 || rmsValues.length === 0 || tempValues.length === 0)
        return { ok: false, error: "No valid samples found in the stored 24h history." }

    var fullStartMs = firstMs
    var fullEndMs = rmsPoints[rmsPoints.length - 1].x > firstMs ? rmsPoints[rmsPoints.length - 1].x : firstMs + 1000

    return {
        ok: true,
        sampleCount: rmsValues.length,
        anomalyValues: anomalyValues,
        rmsValues: rmsValues,
        tempValues: tempValues,
        anomalyPoints: anomalyPoints,
        rmsPoints: rmsPoints,
        tempPoints: tempPoints,
        fullStartMs: fullStartMs,
        fullEndMs: fullEndMs,
        minWindowMs: Math.max(1000, (fullEndMs - fullStartMs) / Math.min(20, rmsValues.length))
    }
}
