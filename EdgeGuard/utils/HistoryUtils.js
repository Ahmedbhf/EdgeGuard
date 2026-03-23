.pragma library

// Parse one timestamp cell from the CSV history file.
// Accepts either a directly parseable date string or a simple HH:mm[:ss] value.
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

// Convert the CSV text into chart-ready arrays.
// This keeps file parsing and day-rollover normalization out of QML UI code.
function parseCsv(csvText) {
    if (!csvText || csvText.length === 0)
        return { ok: false, error: "Could not read the selected CSV file." }

    var lines = csvText.split(/\r?\n/)
    if (lines.length < 2)
        return { ok: false, error: "CSV file is empty." }

    var headers = lines[0].split(",")
    var timeIndex = headers.indexOf("time")
    var rmsIndex = headers.indexOf("rms")
    var tempIndex = headers.indexOf("temp")
    if (timeIndex < 0 || rmsIndex < 0 || tempIndex < 0)
        return { ok: false, error: "CSV must contain time, rms, and temp columns." }

    var rmsValues = []
    var tempValues = []
    var rmsPoints = []
    var tempPoints = []
    var firstMs = -1
    var previousMs = -1
    var dayOffsetMs = 0

    for (var i = 1; i < lines.length; ++i) {
        var line = lines[i].trim()
        if (!line)
            continue

        var fields = line.split(",")
        if (fields.length <= Math.max(timeIndex, rmsIndex, tempIndex))
            continue

        var rmsValue = parseFloat(fields[rmsIndex].trim())
        var tempValue = parseFloat(fields[tempIndex].trim())
        if (isNaN(rmsValue) || isNaN(tempValue))
            continue

        var rawMs = parseTimestamp(fields[timeIndex], rmsValues.length)
        if (previousMs >= 0 && rawMs + dayOffsetMs < previousMs)
            dayOffsetMs += 24 * 60 * 60 * 1000

        var pointMs = rawMs + dayOffsetMs
        if (firstMs < 0)
            firstMs = pointMs
        previousMs = pointMs

        rmsPoints.push({ x: pointMs, y: rmsValue })
        tempPoints.push({ x: pointMs, y: tempValue })
        rmsValues.push(rmsValue)
        tempValues.push(tempValue)
    }

    if (rmsValues.length === 0 || tempValues.length === 0)
        return { ok: false, error: "No valid samples found in the CSV file." }

    var fullStartMs = firstMs
    var fullEndMs = previousMs > firstMs ? previousMs : firstMs + 1000

    return {
        ok: true,
        sampleCount: rmsValues.length,
        rmsValues: rmsValues,
        tempValues: tempValues,
        rmsPoints: rmsPoints,
        tempPoints: tempPoints,
        fullStartMs: fullStartMs,
        fullEndMs: fullEndMs,
        minWindowMs: Math.max(1000, (fullEndMs - fullStartMs) / Math.min(20, rmsValues.length))
    }
}
