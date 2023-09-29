#ifndef __DEF_VIEWER_WIDGET_H__
#define __DEF_VIEWER_WIDGET_H__

#include <QPaintEvent>
#include <QString>
#include <QWidget>

#include "def_encoder.hpp"

class DEFViewerWidget : public QWidget {
    Q_OBJECT

    std::shared_ptr<Def> m_def {};
    DEFEncoder m_defEncoder {};

public:
    explicit DEFViewerWidget(QWidget* t_parent = nullptr);

protected:
    void paintEvent(QPaintEvent* t_event);

public slots:
    void render(QString& t_fileName);
};

#endif