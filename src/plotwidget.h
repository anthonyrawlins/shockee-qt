#ifndef PLOTWIDGET_H
#define PLOTWIDGET_H

#include <QWidget>
#include <QPainter>
#include <QPainterPath>
#include <QTimer>
#include <QVector>
#include <QPointF>
#include <QColor>
#include <QFont>
#include <QFontMetrics>
#include <QPen>
#include <QBrush>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QLinearGradient>
#include <QRadialGradient>
#include <QConicalGradient>
#include <QtMath>

#include "serialcommunicator.h"

class PlotWidget : public QWidget
{
    Q_OBJECT

public:
    enum PlotType {
        Position,
        Force,
        Encoder,
        ForceVsPosition,
        Comparison
    };

    explicit PlotWidget(PlotType type, QWidget *parent = nullptr);
    
    void addDataPoint(const SensorData& data);
    void addDataSeries(const QVector<SensorData>& data, const QString& label = "");
    void clearData();
    void setTimeWindow(double seconds);
    void setAutoScale(bool enable);
    void setGridVisible(bool visible);
    void setPolarMode(bool enable);
    void exportToPdf(const QString& filename);
    void exportToPng(const QString& filename);
    
    // For comparison plots
    void setOverlayMode(bool enable);
    void addOverlayData(const QVector<SensorData>& data, const QString& label);
    void clearOverlayData();

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    void drawAxes(QPainter& painter);
    void drawGrid(QPainter& painter);
    void drawData(QPainter& painter);
    void drawDataSeries(QPainter& painter, const QVector<SensorData>& data, const QColor& color);
    void drawPolarData(QPainter& painter);
    void drawPolarDataSeries(QPainter& painter, const QVector<SensorData>& data, const QColor& baseColor, double opacity = 1.0);
    void drawPolarAxes(QPainter& painter);
    void drawPolarGrid(QPainter& painter);
    void drawLabels(QPainter& painter);
    void drawPolarLabels(QPainter& painter);
    void drawLegend(QPainter& painter);
    void drawTitle(QPainter& painter);
    void calculateBounds();
    void updateScales();
    QColor getViridisColor(double value, double minValue, double maxValue) const;
    QColor getCividisColor(double value, double minValue, double maxValue) const;
    
    QPointF dataToScreen(double x, double y) const;
    QPointF screenToData(const QPointF& screen) const;
    
    PlotType m_plotType;
    QVector<SensorData> m_data;
    QVector<QVector<SensorData>> m_overlaySeries;
    QStringList m_overlayLabels;
    
    // Plot settings
    double m_timeWindow;
    bool m_autoScale;
    bool m_gridVisible;
    bool m_overlayMode;
    bool m_polarMode;
    
    // Bounds and scales
    double m_minX, m_maxX, m_minY, m_maxY;
    double m_scaleX, m_scaleY;
    QRectF m_plotArea;
    QPointF m_polarCenter;
    double m_polarRadius;
    double m_minForce, m_maxForce;
    
    // Interaction
    bool m_isDragging;
    QPointF m_lastMousePos;
    double m_zoomFactor;
    
    // Appearance
    QColor m_backgroundColor;
    QColor m_gridColor;
    QColor m_axisColor;
    QColor m_dataColor;
    QVector<QColor> m_overlayColors;
    QPen m_dataPen;
    QPen m_gridPen;
    QPen m_axisPen;
    QFont m_labelFont;
    
    // Constants
    static const int MARGIN = 60;
    static const int LEGEND_HEIGHT = 30;
    static constexpr double DEFAULT_TIME_WINDOW = 30.0; // seconds
};

#endif // PLOTWIDGET_H