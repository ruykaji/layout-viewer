#ifndef __MAIN_SCENE_HPP__
#define __MAIN_SCENE_HPP__

#include <QOpenGLFunctions>
#include <QOpenGLWidget>
#include <QPushButton>
#include <QResizeEvent>
#include <QToolBar>
#include <QToolButton>
#include <QVBoxLayout>
#include <QWheelEvent>
#include <QWidget>

#include "Include/DEF/DEF.hpp"

namespace gui::main_scene
{

enum class CursorMode
{
  NONE = 0,
  SELECT,
};

enum class ViewMode
{
  STANDARD = 0,
  PINS_DENSITY,
  TRACKS_DENSITY
};

class Scene : public QOpenGLWidget,
              protected QOpenGLFunctions
{
  Q_OBJECT

public:
  explicit Scene(QWidget* parent = nullptr);

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

private:
  void
  show_context_menu(const QPoint& pos);

public slots:
  void
  recv_viewer_data(def::Data const* data);

  void
  recv_cursor_mode(const CursorMode mode);

  void
  recv_view_mode(const ViewMode mode);

  void
  recv_metal_checked(const types::Metal metal, const bool status);

signals:
  void
  send_examine(def::GCell const* gcell);

private:
  def::Data const*                       m_data;

  CursorMode                             m_cursor_mode;
  ViewMode                               m_view_mode;
  bool                                   m_is_gcell_hovered;
  bool                                   m_is_dragging;
  double                                 m_zoom_factor;
  double                                 m_pan_x;
  double                                 m_pan_y;
  QPointF                                m_last_mouse_position;
  QPointF                                m_last_mouse_scene_position;
  std::pair<std::size_t, std::size_t>    m_hovered_gcell;
  std::unordered_map<types::Metal, bool> m_metal_layers;
};

class Widget : public QWidget
{
  Q_OBJECT

public:
  Widget();

private:
  QToolBar*
  create_tool_bar();

  QToolButton*
  create_view_menu();

signals:
  void
  send_viewer_data(def::Data const* data);

  void
  send_cursor_mode(const CursorMode mode);

  void
  send_view_mode(const ViewMode mode);

  void
  send_examine(def::GCell const* gcell);

  void
  send_metal_checked(const types::Metal metal, const bool status);
};

} // namespace gui

#endif