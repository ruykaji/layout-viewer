#ifndef __VIEWER_WIDGET_HPP__
#define __VIEWER_WIDGET_HPP__

#include <QPaintEvent>
#include <QResizeEvent>
#include <QWheelEvent>
#include <QWidget>

#include "Include/Types.hpp"

namespace gui::viewer
{

struct Data
{
  std::array<uint32_t, 4UL>                              m_box;
  std::vector<std::pair<types::Rectangle, types::Metal>> m_rects;
};

class Widget : public QWidget
{
  Q_OBJECT

public:
  explicit Widget(QWidget* parent = nullptr);

public slots:
  void
  set_viewer_data(const Data& data);

protected:
  virtual void
  paintEvent(QPaintEvent* event) override;

  virtual void
  resizeEvent(QResizeEvent* event) override;

  virtual void
  wheelEvent(QWheelEvent* event) override;

  virtual void
  mousePressEvent(QMouseEvent* event) override;

  virtual void
  mouseMoveEvent(QMouseEvent* event) override;

  virtual void
  mouseReleaseEvent(QMouseEvent* event) override;

private:
  Data    m_data;

  bool    m_is_dragging;
  qreal   m_scale_factor;
  QPointF m_last_mouse_pos;
  QPointF m_transition_offset;
  QPointF m_mouse_scene_pos;
};

} // namespace gui

#endif