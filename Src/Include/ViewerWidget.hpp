#ifndef __VIEWER_WIDGET_HPP__
#define __VIEWER_WIDGET_HPP__

#include <QOpenGLFunctions>
#include <QOpenGLWidget>
#include <QPaintEvent>
#include <QPixmap>
#include <QResizeEvent>
#include <QWheelEvent>
#include <QWidget>

#include "Include/Types.hpp"

namespace gui::viewer
{

struct Data
{
  struct Batch
  {
    std::size_t                                    m_width;
    std::size_t                                    m_height;
    std::vector<std::pair<types::Polygon, QColor>> m_rects;
  };

  std::array<uint32_t, 4UL> m_box;
  std::vector<Batch>        m_batches;
};

class Widget : public QOpenGLWidget, protected QOpenGLFunctions
{
  Q_OBJECT

public:
  explicit Widget(QWidget* parent = nullptr);

protected:
  void
  initializeGL() override;

  void
  resizeGL(int w, int h) override;

  void
  paintGL() override;

  void
  wheelEvent(QWheelEvent* event) override;

  void
  mousePressEvent(QMouseEvent* event) override;

  void
  mouseMoveEvent(QMouseEvent* event) override;

  virtual void
  mouseReleaseEvent(QMouseEvent* event) override;

public slots:
  void
  set_viewer_data(const Data& data);

private:
  Data   m_data;

  bool   m_is_dragging;
  double m_zoom_factor;
  double m_pan_x;
  double m_pan_y;
  QPoint m_last_mouse_position;
};

} // namespace gui

#endif