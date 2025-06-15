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
    , m_minX(0), m_maxX(10), m_minY(-10), m_maxY(10)
    , m_scaleX(1), m_scaleY(1)
    , m_isDragging(false)
    , m_zoomFactor(1.0)
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
    
    // Draw axes
    drawAxes(painter);
    
    // Draw data
    drawData(painter);
    
    // Draw labels
    drawLabels(painter);
    
    // Draw legend if needed
    if (m_overlayMode && !m_overlayLabels.isEmpty()) {
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
                x = point.timestamp / 1000.0;
                y = point.position; // Default to position, can be changed
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
    
    // Axis titles
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

void PlotWidget::drawLegend(QPainter& painter)
{
    if (m_overlayLabels.isEmpty()) return;
    
    painter.setPen(m_axisColor);
    painter.setFont(m_labelFont);
    
    double legendY = m_plotArea.bottom() + 30;
    double legendX = m_plotArea.left();
    
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

void PlotWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    updateScales();
}