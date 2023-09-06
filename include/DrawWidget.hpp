#ifndef __DRAW_WIDGET_H__
#define __DRAW_WIDGET_H__

#include "def.hpp"

#include <QWheelEvent>
#include <QWidget>

struct Scale {
    double initial { 1.0 };
    double current { 1.0 };
    double scroll { 0.0 };
};

class DrawWidget : public QWidget {
    Q_OBJECT

    Scale m_scale {};
    Def* m_def {};

public:
    DrawWidget(Def* t_def, QWidget* t_parent = 0);

protected:
    void paintEvent(QPaintEvent* t_event);
    void wheelEvent(QWheelEvent* t_event);
    void resizeEvent(QResizeEvent* event);
};

#endif