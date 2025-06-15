#include "plotwidget.h"
#include <QPainter>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QResizeEvent>
#include <QPaintEvent>
#include <QApplication>
#include <QDebug>
#include <QtMath>

PlotWidget::PlotWidget(PlotType type, QWidget *parent)
    : QWidget(parent)
    , m_plotType(type)
    , m_timeWindow(DEFAULT_TIME_WINDOW)
    , m_autoScale(true)
    , m_gridVisible(true)
    , m_overlayMode(false)
    , m_polarMode(type == Comparison)
    , m_minX(0), m_maxX(10), m_minY(-10), m_maxY(10)
    , m_scaleX(1), m_scaleY(1)
    , m_isDragging(false)
    , m_zoomFactor(1.0)
    , m_polarRadius(0)
    , m_minForce(0), m_maxForce(1000)
{
    setMinimumSize(200, 150);
    setAttribute(Qt::WA_OpaquePaintEvent);
    setMouseTracking(true);
    
    // Setup appearance
    m_backgroundColor = QColor(250, 250, 250);
    m_gridColor = QColor(220, 220, 220);
    m_axisColor = QColor(100, 100, 100);
    m_dataColor = QColor(50, 150, 250);
    
    // Setup overlay colors
    m_overlayColors << QColor(255, 100, 100) << QColor(100, 255, 100) 
                    << QColor(255, 255, 100) << QColor(255, 100, 255);
    
    // Setup pens
    m_dataPen = QPen(m_dataColor, 2);
    m_gridPen = QPen(m_gridColor, 1);
    m_axisPen = QPen(m_axisColor, 2);
    
    // Setup font
    m_labelFont = QFont("Arial", 10);
    
    updateScales();
}

void PlotWidget::addDataPoint(const SensorData& data)
{
    m_data.append(data);
    
    // Keep only recent data for performance
    if (m_data.size() > 10000) {
        m_data.removeFirst();
    }
    
    if (m_autoScale) {
        calculateBounds();
        updateScales();
    }
    
    update();
}

void PlotWidget::addDataSeries(const QVector<SensorData>& data, const QString& label)
{
    if (m_overlayMode) {
        m_overlaySeries.append(data);
        m_overlayLabels.append(label);
    } else {
        m_data = data;
    }
    
    if (m_autoScale) {
        calculateBounds();
        updateScales();
    }
    
    update();
}

void PlotWidget::clearData()
{
    m_data.clear();
    m_overlaySeries.clear();
    m_overlayLabels.clear();
    update();
}

void PlotWidget::setTimeWindow(double seconds)
{
    m_timeWindow = seconds;
    update();
}

void PlotWidget::setAutoScale(bool enable)
{
    m_autoScale = enable;
    if (enable) {
        calculateBounds();
        updateScales();
        update();
    }
}

void PlotWidget::setGridVisible(bool visible)
{
    m_gridVisible = visible;
    update();
}

void PlotWidget::setOverlayMode(bool enable)
{
    m_overlayMode = enable;
    update();
}

void PlotWidget::setPolarMode(bool enable)
{
    m_polarMode = enable;
    update();
}

void PlotWidget::addOverlayData(const QVector<SensorData>& data, const QString& label)
{
    m_overlaySeries.append(data);
    m_overlayLabels.append(label);
    
    if (m_autoScale) {
        calculateBounds();
        updateScales();
    }
    
    update();
}

void PlotWidget::clearOverlayData()
{
    m_overlaySeries.clear();
    m_overlayLabels.clear();
    update();
}

void PlotWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    
    // Fill background
    painter.fillRect(rect(), m_backgroundColor);
    
    // Calculate plot area
    m_plotArea = QRectF(MARGIN, MARGIN, 
                        width() - 2 * MARGIN, 
                        height() - 2 * MARGIN - LEGEND_HEIGHT);
    
    // Draw grid if enabled
    if (m_gridVisible) {
        drawGrid(painter);
    }
    
    // Draw axes and data based on mode
    if (m_polarMode && m_plotType == Comparison) {
        // Calculate polar center and radius
        double size = qMin(m_plotArea.width(), m_plotArea.height());
        m_polarRadius = size * 0.4;
        m_polarCenter = m_plotArea.center();
        
        drawPolarGrid(painter);
        drawPolarAxes(painter);
        drawPolarData(painter);
        drawPolarLabels(painter);
    } else {
        drawAxes(painter);
        drawData(painter);
        drawLabels(painter);
    }
    
    // Draw title
    drawTitle(painter);
    
    // Draw legend if needed
    if ((m_overlayMode && !m_overlayLabels.isEmpty()) || (m_polarMode && m_plotType == Comparison)) {
        drawLegend(painter);
    }
}

void PlotWidget::drawAxes(QPainter& painter)
{
    painter.setPen(m_axisPen);
    
    // Draw axes
    painter.drawLine(m_plotArea.bottomLeft(), m_plotArea.topLeft());
    painter.drawLine(m_plotArea.bottomLeft(), m_plotArea.bottomRight());
}

void PlotWidget::drawGrid(QPainter& painter)
{
    painter.setPen(m_gridPen);
    
    // Vertical grid lines
    int numVerticalLines = 10;
    for (int i = 1; i < numVerticalLines; ++i) {
        double x = m_plotArea.left() + (m_plotArea.width() * i) / numVerticalLines;
        painter.drawLine(QPointF(x, m_plotArea.top()), 
                        QPointF(x, m_plotArea.bottom()));
    }
    
    // Horizontal grid lines
    int numHorizontalLines = 8;
    for (int i = 1; i < numHorizontalLines; ++i) {
        double y = m_plotArea.top() + (m_plotArea.height() * i) / numHorizontalLines;
        painter.drawLine(QPointF(m_plotArea.left(), y), 
                        QPointF(m_plotArea.right(), y));
    }
}

void PlotWidget::drawData(QPainter& painter)
{
    if (m_data.isEmpty() && m_overlaySeries.isEmpty()) {
        return;
    }
    
    // Draw main data series
    if (!m_data.isEmpty()) {
        painter.setPen(m_dataPen);
        drawDataSeries(painter, m_data, m_dataColor);
    }
    
    // Draw overlay series
    for (int i = 0; i < m_overlaySeries.size(); ++i) {
        QColor color = m_overlayColors[i % m_overlayColors.size()];
        QPen pen(color, 2);
        painter.setPen(pen);
        drawDataSeries(painter, m_overlaySeries[i], color);
    }
}

void PlotWidget::drawPolarData(QPainter& painter)
{
    if (m_data.isEmpty() && m_overlaySeries.isEmpty()) {
        return;
    }
    
    // Calculate force bounds for color mapping
    m_minForce = m_maxForce = 0;
    bool firstForce = true;
    
    // Find min/max force from all data
    auto updateForceBounds = [&](const QVector<SensorData>& data) {
        for (const SensorData& point : data) {
            if (firstForce) {
                m_minForce = m_maxForce = point.force;
                firstForce = false;
            } else {
                m_minForce = qMin(m_minForce, point.force);
                m_maxForce = qMax(m_maxForce, point.force);
            }
        }
    };
    
    if (!m_data.isEmpty()) {
        updateForceBounds(m_data);
    }
    for (const auto& series : m_overlaySeries) {
        updateForceBounds(series);
    }
    
    // Draw main data series with viridis colors
    if (!m_data.isEmpty()) {
        drawPolarDataSeries(painter, m_data, QColor(68, 1, 84), m_overlayMode ? 0.5 : 1.0);
    }
    
    // Draw overlay series with different color palette
    for (int i = 0; i < m_overlaySeries.size(); ++i) {
        QColor baseColor = (i == 0) ? QColor(253, 231, 37) : QColor(94, 201, 98);
        drawPolarDataSeries(painter, m_overlaySeries[i], baseColor, 0.5);
    }
}

void PlotWidget::drawPolarDataSeries(QPainter& painter, const QVector<SensorData>& data, const QColor& baseColor, double opacity)
{
    if (data.size() < 2) return;
    
    painter.setRenderHint(QPainter::Antialiasing);
    
    for (int i = 0; i < data.size() - 1; ++i) {
        const SensorData& point = data[i];
        
        // Convert encoder pulses to angle (assuming 360 degrees per full rotation)
        double angle = (point.encoderPulses % 3600) * (2.0 * M_PI / 3600.0); // 10 pulses per degree
        
        // Use position as radius (stroke length)
        double radius = (point.position + 75.0) / 150.0 * m_polarRadius; // Normalize to 0-75mm range
        
        // Calculate screen coordinates
        double x = m_polarCenter.x() + radius * qCos(angle - M_PI_2); // -PI/2 to start at top
        double y = m_polarCenter.y() + radius * qSin(angle - M_PI_2);
        
        // Get color based on force (viridis or cividis)
        QColor color;
        if (baseColor == QColor(68, 1, 84)) { // Viridis for first dataset
            color = getViridisColor(point.force, m_minForce, m_maxForce);
        } else { // Cividis for second dataset
            color = getCividisColor(point.force, m_minForce, m_maxForce);
        }
        
        color.setAlphaF(opacity);
        
        // Draw point
        painter.setPen(QPen(color, 3));
        painter.setBrush(QBrush(color));
        painter.drawEllipse(QPointF(x, y), 2, 2);
        
        // Connect to next point if close in angle
        if (i < data.size() - 1) {
            const SensorData& nextPoint = data[i + 1];
            double nextAngle = (nextPoint.encoderPulses % 3600) * (2.0 * M_PI / 3600.0);
            double angleDiff = qAbs(nextAngle - angle);
            if (angleDiff < M_PI / 6) { // Only connect if within 30 degrees
                double nextRadius = (nextPoint.position + 75.0) / 150.0 * m_polarRadius;
                double nextX = m_polarCenter.x() + nextRadius * qCos(nextAngle - M_PI_2);
                double nextY = m_polarCenter.y() + nextRadius * qSin(nextAngle - M_PI_2);
                
                QPen linePen(color, 1);
                linePen.setStyle(Qt::SolidLine);
                painter.setPen(linePen);
                painter.drawLine(QPointF(x, y), QPointF(nextX, nextY));
            }
        }
    }
}

void PlotWidget::drawPolarAxes(QPainter& painter)
{
    painter.setPen(QPen(m_axisColor, 2));
    
    // Draw radial lines every 30 degrees
    for (int angle = 0; angle < 360; angle += 30) {
        double radians = angle * M_PI / 180.0;
        double x1 = m_polarCenter.x();
        double y1 = m_polarCenter.y();
        double x2 = x1 + m_polarRadius * qCos(radians - M_PI_2);
        double y2 = y1 + m_polarRadius * qSin(radians - M_PI_2);
        
        painter.drawLine(QPointF(x1, y1), QPointF(x2, y2));
    }
}

void PlotWidget::drawPolarGrid(QPainter& painter)
{
    if (!m_gridVisible) return;
    
    painter.setPen(m_gridPen);
    
    // Draw concentric circles
    for (int i = 1; i <= 4; ++i) {
        double radius = m_polarRadius * i / 4.0;
        painter.drawEllipse(m_polarCenter, radius, radius);
    }
}

void PlotWidget::drawPolarLabels(QPainter& painter)
{
    painter.setPen(m_axisColor);
    painter.setFont(m_labelFont);
    
    // Draw angle labels
    for (int angle = 0; angle < 360; angle += 45) {
        double radians = angle * M_PI / 180.0;
        double x = m_polarCenter.x() + (m_polarRadius + 15) * qCos(radians - M_PI_2);
        double y = m_polarCenter.y() + (m_polarRadius + 15) * qSin(radians - M_PI_2);
        
        QString label = QString("%1°").arg(angle);
        QRectF textRect(x - 15, y - 10, 30, 20);
        painter.drawText(textRect, Qt::AlignCenter, label);
    }
    
    // Draw radius labels (stroke length)
    for (int i = 1; i <= 4; ++i) {
        double radius = m_polarRadius * i / 4.0;
        double strokeValue = 75.0 * i / 4.0; // 0-75mm range
        
        QString label = QString("%1mm").arg(strokeValue, 0, 'f', 1);
        QRectF textRect(m_polarCenter.x() + radius - 20, m_polarCenter.y() - 10, 40, 20);
        painter.drawText(textRect, Qt::AlignCenter, label);
    }
}

void PlotWidget::drawDataSeries(QPainter& painter, const QVector<SensorData>& data, const QColor& color)
{
    if (data.size() < 2) return;
    
    QPainterPath path;
    bool firstPoint = true;
    
    for (const SensorData& point : data) {
        double x, y;
        
        switch (m_plotType) {
            case Position:
                x = point.timestamp / 1000.0; // Convert to seconds
                y = point.position;
                break;
            case Force:
                x = point.timestamp / 1000.0;
                y = point.force;
                break;
            case Encoder:
                x = point.timestamp / 1000.0;
                y = point.encoderPulses;
                break;
            case ForceVsPosition:
                x = point.position;
                y = point.force;
                break;
            case Comparison:
                if (m_polarMode) {
                    // For polar mode, we handle this in drawPolarDataSeries
                    return;
                } else {
                    x = point.timestamp / 1000.0;
                    y = point.position;
                }
                break;
        }
        
        QPointF screenPoint = dataToScreen(x, y);
        
        if (firstPoint) {
            path.moveTo(screenPoint);
            firstPoint = false;
        } else {
            path.lineTo(screenPoint);
        }
    }
    
    painter.drawPath(path);
}

void PlotWidget::drawLabels(QPainter& painter)
{
    painter.setPen(m_axisColor);
    painter.setFont(m_labelFont);
    
    // X-axis labels
    int numXLabels = 5;
    for (int i = 0; i <= numXLabels; ++i) {
        double dataX = m_minX + (m_maxX - m_minX) * i / numXLabels;
        double screenX = m_plotArea.left() + (m_plotArea.width() * i) / numXLabels;
        
        QString label;
        if (m_plotType == ForceVsPosition) {
            label = QString::number(dataX, 'f', 1);
        } else {
            label = QString::number(dataX, 'f', 1) + "s";
        }
        
        QRectF textRect(screenX - 30, m_plotArea.bottom() + 5, 60, 20);
        painter.drawText(textRect, Qt::AlignCenter, label);
    }
    
    // Y-axis labels
    int numYLabels = 5;
    for (int i = 0; i <= numYLabels; ++i) {
        double dataY = m_minY + (m_maxY - m_minY) * (numYLabels - i) / numYLabels;
        double screenY = m_plotArea.top() + (m_plotArea.height() * i) / numYLabels;
        
        QString label = QString::number(dataY, 'f', 1);
        
        QRectF textRect(5, screenY - 10, MARGIN - 10, 20);
        painter.drawText(textRect, Qt::AlignRight | Qt::AlignVCenter, label);
    }
    
    if (!m_polarMode || m_plotType != Comparison) {
        // Axis titles for regular plots
        painter.save();
        painter.translate(15, m_plotArea.center().y());
        painter.rotate(-90);
        
        QString yTitle;
        switch (m_plotType) {
            case Position: yTitle = "Position (mm)"; break;
            case Force: yTitle = "Force (kg)"; break;
            case Encoder: yTitle = "Encoder (pulses)"; break;
            case ForceVsPosition: yTitle = "Force (kg)"; break;
            case Comparison: yTitle = "Position (mm)"; break;
        }
        
        painter.drawText(-50, 0, 100, 20, Qt::AlignCenter, yTitle);
        painter.restore();
        
        // X-axis title
        QString xTitle = (m_plotType == ForceVsPosition) ? "Position (mm)" : "Time (s)";
        QRectF xTitleRect(m_plotArea.left(), height() - 25, m_plotArea.width(), 20);
        painter.drawText(xTitleRect, Qt::AlignCenter, xTitle);
    }
}

void PlotWidget::drawLegend(QPainter& painter)
{
    painter.setPen(m_axisColor);
    painter.setFont(m_labelFont);
    
    double legendY = m_plotArea.bottom() + 40;
    double legendX = m_plotArea.left();
    
    if (m_polarMode && m_plotType == Comparison) {
        // Color scale legend for polar chart
        QString forceLabel = QString("Force Range: %1 - %2 kg")
                           .arg(m_minForce, 0, 'f', 1)
                           .arg(m_maxForce, 0, 'f', 1);
        painter.drawText(legendX, legendY, forceLabel);
        
        // Draw viridis color bar
        QRectF colorBar(legendX, legendY + 15, 200, 15);
        QLinearGradient gradient(colorBar.topLeft(), colorBar.topRight());
        for (int i = 0; i <= 10; ++i) {
            double t = i / 10.0;
            QColor color = getViridisColor(m_minForce + t * (m_maxForce - m_minForce), m_minForce, m_maxForce);
            gradient.setColorAt(t, color);
        }
        painter.fillRect(colorBar, gradient);
        painter.drawRect(colorBar);
        
        // Add dataset labels if overlaying
        if (m_overlayMode && !m_data.isEmpty() && !m_overlaySeries.isEmpty()) {
            legendY += 40;
            painter.drawText(legendX, legendY, "Dataset 1: Viridis colors");
            painter.drawText(legendX, legendY + 15, "Dataset 2: Cividis colors");
        }
    } else if (!m_overlayLabels.isEmpty()) {
        // Regular legend for overlay mode
        for (int i = 0; i < m_overlayLabels.size(); ++i) {
            QColor color = m_overlayColors[i % m_overlayColors.size()];
            
            // Draw color indicator
            QRectF colorRect(legendX, legendY, 15, 10);
            painter.fillRect(colorRect, color);
            
            // Draw label
            QRectF textRect(legendX + 20, legendY - 5, 100, 20);
            painter.drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, m_overlayLabels[i]);
            
            legendX += 130;
            if (legendX + 130 > m_plotArea.right()) {
                legendX = m_plotArea.left();
                legendY += 20;
            }
        }
    }
}

void PlotWidget::calculateBounds()
{
    if (m_data.isEmpty() && m_overlaySeries.isEmpty()) {
        return;
    }
    
    m_minX = m_maxX = m_minY = m_maxY = 0;
    bool firstPoint = true;
    
    // Process main data
    for (const SensorData& point : m_data) {
        double x, y;
        
        switch (m_plotType) {
            case Position:
                x = point.timestamp / 1000.0;
                y = point.position;
                break;
            case Force:
                x = point.timestamp / 1000.0;
                y = point.force;
                break;
            case Encoder:
                x = point.timestamp / 1000.0;
                y = point.encoderPulses;
                break;
            case ForceVsPosition:
                x = point.position;
                y = point.force;
                break;
            case Comparison:
                x = point.timestamp / 1000.0;
                y = point.position;
                break;
        }
        
        if (firstPoint) {
            m_minX = m_maxX = x;
            m_minY = m_maxY = y;
            firstPoint = false;
        } else {
            m_minX = qMin(m_minX, x);
            m_maxX = qMax(m_maxX, x);
            m_minY = qMin(m_minY, y);
            m_maxY = qMax(m_maxY, y);
        }
    }
    
    // Process overlay data
    for (const QVector<SensorData>& series : m_overlaySeries) {
        for (const SensorData& point : series) {
            double x, y;
            
            switch (m_plotType) {
                case Position:
                    x = point.timestamp / 1000.0;
                    y = point.position;
                    break;
                case Force:
                    x = point.timestamp / 1000.0;
                    y = point.force;
                    break;
                case Encoder:
                    x = point.timestamp / 1000.0;
                    y = point.encoderPulses;
                    break;
                case ForceVsPosition:
                    x = point.position;
                    y = point.force;
                    break;
                case Comparison:
                    x = point.timestamp / 1000.0;
                    y = point.position;
                    break;
            }
            
            if (firstPoint) {
                m_minX = m_maxX = x;
                m_minY = m_maxY = y;
                firstPoint = false;
            } else {
                m_minX = qMin(m_minX, x);
                m_maxX = qMax(m_maxX, x);
                m_minY = qMin(m_minY, y);
                m_maxY = qMax(m_maxY, y);
            }
        }
    }
    
    // Add some padding
    double xPadding = (m_maxX - m_minX) * 0.05;
    double yPadding = (m_maxY - m_minY) * 0.05;
    
    m_minX -= xPadding;
    m_maxX += xPadding;
    m_minY -= yPadding;
    m_maxY += yPadding;
    
    // Ensure minimum range
    if (m_maxX - m_minX < 0.1) {
        m_minX -= 0.05;
        m_maxX += 0.05;
    }
    if (m_maxY - m_minY < 0.1) {
        m_minY -= 0.05;
        m_maxY += 0.05;
    }
}

void PlotWidget::updateScales()
{
    if (m_plotArea.width() > 0 && m_plotArea.height() > 0) {
        m_scaleX = m_plotArea.width() / (m_maxX - m_minX);
        m_scaleY = m_plotArea.height() / (m_maxY - m_minY);
    }
}

QPointF PlotWidget::dataToScreen(double x, double y) const
{
    double screenX = m_plotArea.left() + (x - m_minX) * m_scaleX;
    double screenY = m_plotArea.bottom() - (y - m_minY) * m_scaleY;
    return QPointF(screenX, screenY);
}

QPointF PlotWidget::screenToData(const QPointF& screen) const
{
    double dataX = m_minX + (screen.x() - m_plotArea.left()) / m_scaleX;
    double dataY = m_minY + (m_plotArea.bottom() - screen.y()) / m_scaleY;
    return QPointF(dataX, dataY);
}

void PlotWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_isDragging = true;
        m_lastMousePos = event->position();
    }
}

void PlotWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (m_isDragging) {
        QPointF delta = event->position() - m_lastMousePos;
        
        // Pan the view
        double deltaX = -delta.x() / m_scaleX;
        double deltaY = delta.y() / m_scaleY;
        
        m_minX += deltaX;
        m_maxX += deltaX;
        m_minY += deltaY;
        m_maxY += deltaY;
        
        updateScales();
        update();
        
        m_lastMousePos = event->position();
    }
}

void PlotWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_isDragging = false;
    }
}

void PlotWidget::wheelEvent(QWheelEvent *event)
{
    double scaleFactor = 1.15;
    if (event->angleDelta().y() < 0) {
        scaleFactor = 1.0 / scaleFactor;
    }
    
    QPointF mousePos = event->position();
    QPointF dataPos = screenToData(mousePos);
    
    // Zoom around mouse position
    double rangeX = m_maxX - m_minX;
    double rangeY = m_maxY - m_minY;
    
    double newRangeX = rangeX / scaleFactor;
    double newRangeY = rangeY / scaleFactor;
    
    m_minX = dataPos.x() - newRangeX * (dataPos.x() - m_minX) / rangeX;
    m_maxX = dataPos.x() + newRangeX * (m_maxX - dataPos.x()) / rangeX;
    m_minY = dataPos.y() - newRangeY * (dataPos.y() - m_minY) / rangeY;
    m_maxY = dataPos.y() + newRangeY * (m_maxY - dataPos.y()) / rangeY;
    
    updateScales();
    update();
}

void PlotWidget::drawTitle(QPainter& painter)
{
    painter.setPen(m_axisColor);
    QFont titleFont = m_labelFont;
    titleFont.setPointSize(titleFont.pointSize() + 2);
    titleFont.setBold(true);
    painter.setFont(titleFont);
    
    QString title;
    switch (m_plotType) {
        case Position: title = "Position vs Time"; break;
        case Force: title = "Force vs Time"; break;
        case Encoder: title = "Encoder vs Time"; break;
        case ForceVsPosition: title = "Force vs Position"; break;
        case Comparison:
            if (m_polarMode) {
                title = "Polar Comparison - Encoder Angle vs Stroke Length (Force Colored)";
            } else {
                title = "Data Comparison";
            }
            break;
    }
    
    QRectF titleRect(0, 5, width(), 25);
    painter.drawText(titleRect, Qt::AlignCenter, title);
}

QColor PlotWidget::getViridisColor(double value, double minValue, double maxValue) const
{
    if (maxValue <= minValue) return QColor(68, 1, 84);
    
    double t = qBound(0.0, (value - minValue) / (maxValue - minValue), 1.0);
    
    // Viridis color map approximation
    if (t <= 0.25) {
        double s = t / 0.25;
        return QColor(
            static_cast<int>(68 + s * (59 - 68)),
            static_cast<int>(1 + s * (82 - 1)),
            static_cast<int>(84 + s * (139 - 84))
        );
    } else if (t <= 0.5) {
        double s = (t - 0.25) / 0.25;
        return QColor(
            static_cast<int>(59 + s * (33 - 59)),
            static_cast<int>(82 + s * (145 - 82)),
            static_cast<int>(139 + s * (140 - 139))
        );
    } else if (t <= 0.75) {
        double s = (t - 0.5) / 0.25;
        return QColor(
            static_cast<int>(33 + s * (94 - 33)),
            static_cast<int>(145 + s * (201 - 145)),
            static_cast<int>(140 + s * (98 - 140))
        );
    } else {
        double s = (t - 0.75) / 0.25;
        return QColor(
            static_cast<int>(94 + s * (253 - 94)),
            static_cast<int>(201 + s * (231 - 201)),
            static_cast<int>(98 + s * (37 - 98))
        );
    }
}

QColor PlotWidget::getCividisColor(double value, double minValue, double maxValue) const
{
    if (maxValue <= minValue) return QColor(0, 32, 76);
    
    double t = qBound(0.0, (value - minValue) / (maxValue - minValue), 1.0);
    
    // Cividis color map approximation
    if (t <= 0.25) {
        double s = t / 0.25;
        return QColor(
            static_cast<int>(0 + s * (35 - 0)),
            static_cast<int>(32 + s * (53 - 32)),
            static_cast<int>(76 + s * (102 - 76))
        );
    } else if (t <= 0.5) {
        double s = (t - 0.25) / 0.25;
        return QColor(
            static_cast<int>(35 + s * (86 - 35)),
            static_cast<int>(53 + s * (73 - 53)),
            static_cast<int>(102 + s * (115 - 102))
        );
    } else if (t <= 0.75) {
        double s = (t - 0.5) / 0.25;
        return QColor(
            static_cast<int>(86 + s * (144 - 86)),
            static_cast<int>(73 + s * (91 - 73)),
            static_cast<int>(115 + s * (109 - 115))
        );
    } else {
        double s = (t - 0.75) / 0.25;
        return QColor(
            static_cast<int>(144 + s * (222 - 144)),
            static_cast<int>(91 + s * (137 - 91)),
            static_cast<int>(109 + s * (96 - 109))
        );
    }
}

void PlotWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    updateScales();
}