#ifndef __DEF_VIEWER_WIDGET_H__
#define __DEF_VIEWER_WIDGET_H__

#include <QImage>
#include <QListWidget>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QResizeEvent>
#include <QString>
#include <QWheelEvent>
#include <QWidget>
#include <set>

#include "encoder/encoder.hpp"

struct PaintBufferObject {
    QPolygon poly {};
    QColor penColor {};
    QColor brushColor {};
    MetalLayer layer {};

    // For tensor display mode
    bool isTensorMode { false };
    Point position {};
    QImage tensor {};

    PaintBufferObject() = default;
    ~PaintBufferObject() = default;

    PaintBufferObject(const QPolygon& t_poly, const QColor& t_penColor, const QColor& t_brushColor, const MetalLayer& t_layer)
        : poly(t_poly)
        , penColor(t_penColor)
        , brushColor(t_brushColor)
        , layer(t_layer) {};
    PaintBufferObject(const Point& t_position, const QImage& t_image)
        : isTensorMode(true)
        , position(t_position)
        , tensor(t_image) {};

    bool operator<(const PaintBufferObject& other) const;
};

class ViewerWidget : public QWidget {
    Q_OBJECT

    enum class Mode {
        DEFAULT,
        DRAGING,
    };

    enum class DisplayMode {
        DEFAULT,
        TENSOR
    };

    QListWidget* m_listWidget {};

    Mode m_mode {};
    DisplayMode m_displayMode { DisplayMode::DEFAULT };

    torch::Tensor m_source {};
    std::shared_ptr<PDK> m_pdk {};
    std::shared_ptr<Config> m_config {};
    std::shared_ptr<Data> m_data {};
    std::unique_ptr<Encoder> m_encoder {};
    std::multiset<PaintBufferObject> m_paintBuffer {};

    std::pair<int32_t, int32_t> m_max { 0, 0 };
    std::pair<int32_t, int32_t> m_min { INT32_MAX, INT32_MAX };

    double m_initialScale { 1.0 };
    double m_currentScale { 1.0 };
    double m_prevCurrenScale { 1.0 };
    double m_scroll { 0.0 };

    QPointF m_mouseTriggerPos {};
    QPointF m_mouseCurrentPos {};
    QPointF m_moveAxesIn {};
    QPointF m_axesPos {};

public:
    explicit ViewerWidget(QWidget* t_parent = nullptr);

private:
    void setup();
    std::pair<QColor, QColor> selectBrushAndPen(const MetalLayer& t_layer);
    QImage torchMatrixToQImage(const torch::Tensor& t_matrix, const QColor& t_fillColor);

protected:
    void paintEvent(QPaintEvent* t_event);
    void resizeEvent(QResizeEvent* t_event);
    void mousePressEvent(QMouseEvent* t_event);
    void mouseMoveEvent(QMouseEvent* t_event);
    void mouseReleaseEvent(QMouseEvent* t_event);
    void wheelEvent(QWheelEvent* t_event);

public Q_SLOTS:
    void render(QString& t_fileName);
    void setDisplayMode(const int8_t& t_mode);
};

#endif