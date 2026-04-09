.pragma library

// Return true when the chart has at least one point to display.
function hasPoints(points) {
    return points && points.length > 0
}

// Clear the temporary hover marker and tooltip UI.
function resetHover(highlightSeries, hoverLine, hoverCard) {
    highlightSeries.clear()
    hoverLine.visible = false
    hoverCard.visible = false
}

// Copy the QML point array into the Qt Charts line series.
function populateSeries(series, points) {
    series.clear()
    if (!hasPoints(points))
        return

    for (var i = 0; i < points.length; ++i)
        series.append(points[i].x, points[i].y)
}

// Apply a visible time range to one history chart panel.
function setVisibleRange(axisX, axisY, points, axisMinY, axisMaxY, startMs, endMs) {
    if (!hasPoints(points))
        return false

    axisX.min = new Date(startMs)
    axisX.max = new Date(endMs)
    axisY.min = axisMinY
    axisY.max = axisMaxY
    return true
}

// Rebuild the line series and reset the chart to the full visible range.
function refreshSeries(lineSeries,
                       points,
                       highlightSeries,
                       hoverLine,
                       hoverCard,
                       axisX,
                       axisY,
                       axisMinY,
                       axisMaxY,
                       fullStartMs,
                       fullEndMs) {
    resetHover(highlightSeries, hoverLine, hoverCard)
    populateSeries(lineSeries, points)
    if (hasPoints(points))
        setVisibleRange(axisX, axisY, points, axisMinY, axisMaxY, fullStartMs, fullEndMs)
}

// Find the closest history point on the X axis with a binary search.
function nearestIndex(points, targetX) {
    if (!hasPoints(points))
        return -1

    var left = 0
    var right = points.length - 1
    while (left < right) {
        var mid = Math.floor((left + right) / 2)
        if (points[mid].x < targetX)
            left = mid + 1
        else
            right = mid
    }

    var candidate = left
    if (candidate > 0 && Math.abs(points[candidate - 1].x - targetX) < Math.abs(points[candidate].x - targetX))
        candidate -= 1
    return candidate
}

// Check whether the mouse is still inside the plot area.
function inPlotArea(plot, mouseX, mouseY) {
    return mouseX >= plot.x && mouseX <= plot.x + plot.width
            && mouseY >= plot.y && mouseY <= plot.y + plot.height
}

// Keep an interaction value inside a safe range.
function clamp(value, minValue, maxValue) {
    return Math.max(minValue, Math.min(maxValue, value))
}

// Keep the visible time range inside the full loaded history span.
function normalizeVisibleRange(startMs, endMs, fullStartMs, fullEndMs, minimumWindowMs) {
    var fullSpan = Math.max(1, fullEndMs - fullStartMs)
    var span = Math.max(minimumWindowMs, endMs - startMs)

    if (span >= fullSpan) {
        startMs = fullStartMs
        endMs = fullEndMs
    } else {
        var center = (startMs + endMs) / 2
        startMs = center - span / 2
        endMs = center + span / 2

        if (startMs < fullStartMs) {
            startMs = fullStartMs
            endMs = startMs + span
        }

        if (endMs > fullEndMs) {
            endMs = fullEndMs
            startMs = endMs - span
        }
    }

    return { startMs: startMs, endMs: endMs }
}

// Read the current axis range, clamp it to the loaded data, and write it back.
function constrainVisibleRange(axisX, axisY, points, axisMinY, axisMaxY, fullStartMs, fullEndMs, minimumWindowMs) {
    if (!hasPoints(points))
        return null

    var constrained = normalizeVisibleRange(axisX.min.getTime(),
                                            axisX.max.getTime(),
                                            fullStartMs,
                                            fullEndMs,
                                            minimumWindowMs)
    setVisibleRange(axisX, axisY, points, axisMinY, axisMaxY, constrained.startMs, constrained.endMs)
    return constrained
}

// Update the hover line and tooltip for the point nearest to the cursor.
function updateHover(chart,
                     lineSeries,
                     points,
                     interactiveEnabled,
                     highlightSeries,
                     hoverLine,
                     hoverCard,
                     hoverTime,
                     hoverValue,
                     valueLabel,
                     valueDecimals,
                     contentAreaWidth,
                     contentAreaHeight,
                     mouseX,
                     mouseY) {
    if (!interactiveEnabled || !hasPoints(points)) {
        resetHover(highlightSeries, hoverLine, hoverCard)
        return
    }

    var plot = chart.plotArea
    if (!inPlotArea(plot, mouseX, mouseY)) {
        resetHover(highlightSeries, hoverLine, hoverCard)
        return
    }

    var mapped = chart.mapToValue(Qt.point(mouseX, mouseY), lineSeries)
    var index = nearestIndex(points, mapped.x)
    if (index < 0) {
        resetHover(highlightSeries, hoverLine, hoverCard)
        return
    }

    var point = points[index]
    var scenePoint = chart.mapToPosition(Qt.point(point.x, point.y), lineSeries)
    var plotLeft = chart.x + plot.x
    var plotTop = chart.y + plot.y

    highlightSeries.clear()
    highlightSeries.append(point.x, point.y)

    hoverLine.visible = true
    hoverLine.x = clamp(chart.x + scenePoint.x - hoverLine.width / 2,
                        plotLeft,
                        plotLeft + plot.width - hoverLine.width)
    hoverLine.y = plotTop
    hoverLine.height = plot.height

    hoverCard.visible = true
    hoverTime.text = Qt.formatDateTime(new Date(point.x), "HH:mm:ss")
    hoverValue.text = valueLabel + ": " + point.y.toFixed(valueDecimals)

    var desiredX = chart.x + scenePoint.x + 14
    var maxX = contentAreaWidth - hoverCard.width - 12
    if (desiredX > maxX)
        desiredX = chart.x + scenePoint.x - hoverCard.width - 14
    hoverCard.x = clamp(desiredX, 12, maxX)

    var desiredY = chart.y + scenePoint.y - hoverCard.height - 12
    if (desiredY < 12)
        desiredY = chart.y + scenePoint.y + 12
    hoverCard.y = clamp(desiredY, 12, contentAreaHeight - hoverCard.height - 12)
}

// Apply the same visible range to the sibling history panel.
function setPanelVisibleRange(panel, startMs, endMs) {
    setVisibleRange(panel.xAxis,
                    panel.yAxis,
                    panel.points,
                    panel.axisMinY,
                    panel.axisMaxY,
                    startMs,
                    endMs)
}

// Apply one visible range to sibling history panels while skipping the source panel.
function syncVisibleRange(startMs, endMs, sourcePanel, panels) {
    if (!panels || panels.length === 0)
        return

    for (var i = 0; i < panels.length; ++i) {
        var panel = panels[i]
        if (panel && panel !== sourcePanel)
            setPanelVisibleRange(panel, startMs, endMs)
    }
}
