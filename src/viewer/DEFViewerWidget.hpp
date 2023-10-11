#ifndef __DEF_VIEWER_WIDGET_H__
#define __DEF_VIEWER_WIDGET_H__

#include <QPaintEvent>
#include <QResizeEvent>
#include <QWheelEvent>
#include <QMouseEvent>
#include <QString>
#include <QWidget>

#include "encoder.hpp"

class DEFViewerWidget : public QWidget {
    Q_OBJECT

    enum class Mode {
        DEFAULT,
        DRAGING,
    };

    Mode m_mode {};

    std::shared_ptr<Def> m_def {};
    Encoder m_encoder {};

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
    explicit DEFViewerWidget(QWidget* t_parent = nullptr);

private:
    void selectBrushAndPen(QPainter* t_painter, const ML& t_layer);

protected:
    void paintEvent(QPaintEvent* t_event);
    void resizeEvent(QResizeEvent* t_event);
    void mousePressEvent(QMouseEvent* t_event);
    void mouseMoveEvent(QMouseEvent* t_event);
    void mouseReleaseEvent(QMouseEvent* t_event);
    void wheelEvent(QWheelEvent* t_event);

public slots:
    void render(QString& t_fileName);
};

#endif