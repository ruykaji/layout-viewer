#include <GL/glu.h>
#include <QActionGroup>
#include <QApplication>
#include <QDebug>
#include <QMenu>
#include <QMenuBar>
#include <QPainter>
#include <QPalette>
#include <QTransform>

#include "Include/GUI/MainScene.hpp"
#include "Include/GlobalUtils.hpp"

namespace gui::main_scene::details
{

QColor
get_metal_color(types::Metal metal)
{
  const auto [r, g, b] = utils::get_metal_color(metal);
  return QColor(r, g, b);
}

void
tess_vertex_callback(const GLvoid* data)
{
  const GLdouble* vertex = static_cast<const GLdouble*>(data);
  glVertex2d(vertex[0], vertex[1]);
}

void
draw_polygon(const geom::Polygon& poly)
{
  static GLdouble vertices[100][3] = { 0 };
  std::size_t     i                = 0;

  auto            color            = details::get_metal_color(poly.m_metal);
  glColor4f(color.redF(), color.greenF(), color.blueF(), 0.25f);

  GLUtesselator* tess = gluNewTess();
  gluTessCallback(tess, GLU_TESS_BEGIN, (void (*)())glBegin);
  gluTessCallback(tess, GLU_TESS_VERTEX, (void (*)())details::tess_vertex_callback);
  gluTessCallback(tess, GLU_TESS_END, (void (*)())glEnd);

  gluTessBeginPolygon(tess, nullptr);
  gluTessBeginContour(tess);

  for(const auto& point : poly.m_points)
    {
      vertices[i][0] = point.x;
      vertices[i][1] = point.y;
      vertices[i][2] = 0.0;

      gluTessVertex(tess, vertices[i], vertices[i]);
      ++i;
    }

  gluTessEndContour(tess);
  gluTessEndPolygon(tess);

  gluDeleteTess(tess);
}

std::pair<QToolButton*, QAction*>
create_tool_button(QWidget* parent, const QString& icon, const QString tooltip, bool is_checked = false)
{
  QAction*     action = new QAction(QIcon(icon), tooltip, parent);

  QToolButton* button = new QToolButton(parent);
  button->setDefaultAction(action);
  button->setCheckable(true);
  button->setChecked(is_checked);

  return std::make_pair(button, action);
}

} // namespace gui::viewer::details

namespace gui::main_scene
{

/** ======================= Scene methods ======================= */

Scene::Scene(QWidget* parent)
    : QOpenGLWidget(parent), m_data(nullptr), m_cursor_mode(CursorMode::NONE), m_view_mode(ViewMode::STANDARD), m_is_gcell_hovered(false), m_is_dragging(false), m_zoom_factor(1.0f), m_pan_x(0.0f), m_pan_y(0.0f)
{
  for(uint8_t i = 0; i < uint8_t(types::Metal::SIZE); i += 2) /** +2 to skip via layers */
    {
      m_metal_layers[types::Metal(i + 1)] = true;
    }

  setMouseTracking(true);
  setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

void
Scene::initializeGL()
{
  initializeOpenGLFunctions();
  glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void
Scene::resizeGL(int w, int h)
{
  glViewport(0, 0, w, h);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(0, w, h, 0, -1, 1);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
}

void
Scene::paintGL()
{
  if(m_data == nullptr)
    {
      return;
    }

  glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  glPushAttrib(GL_ALL_ATTRIB_BITS);

  QPainter painter(this);
  painter.setPen(Qt::white);
  painter.setFont(QFont("Arial", 12));

  painter.setRenderHint(QPainter::Antialiasing);
  QString  text_coords = QString("X: %1, Y: %2").arg(m_last_mouse_scene_position.x(), 0, 'd', 0).arg(m_last_mouse_scene_position.y(), 0, 'd', 0);
  painter.drawText(10, height() - 40, text_coords);

  QString  text_proj_coords = QString("X: %1, Y: %2").arg(std::floor(m_last_mouse_scene_position.x() / 6900), 0, 'd', 0).arg(std::floor(m_last_mouse_scene_position.y() / 6900), 0, 'd', 0);
  painter.drawText(10, height() - 20, text_proj_coords);

  painter.end();

  glPopAttrib();

  if(m_data == nullptr)
    {
      return;
    }

  glPushMatrix();
  glTranslatef(m_pan_x, m_pan_y, 0.0f);
  glScalef(m_zoom_factor, m_zoom_factor, 1.0f);

  /** Draw gcells */
  const auto& gcells                                = m_data->m_gcells;

  m_is_gcell_hovered                                = false;
  std::pair<std::size_t, std::size_t> hovered_gcell = { 0, 0 };

  for(std::size_t y = 0, end_y = gcells.size(); y < end_y; ++y)
    {
      for(std::size_t x = 0, end_x = gcells[y].size(); x < end_x; ++x)
        {
          if(m_cursor_mode == CursorMode::SELECT && gcells[y][x]->m_box.probe_point({ m_last_mouse_scene_position.x(), m_last_mouse_scene_position.y() }))
            {
              glColor4f(0.0, 1.0, 0.0, 1.00f);
              m_is_gcell_hovered = true;
              hovered_gcell      = { y, x };
            }
          else
            {
              glColor4f(1.0, 1.0, 1.0, 0.25f);
            }

          glBegin(GL_LINE_LOOP);
          glVertex2d(gcells[y][x]->m_box.m_points[0].x, gcells[y][x]->m_box.m_points[0].y);
          glVertex2d(gcells[y][x]->m_box.m_points[1].x, gcells[y][x]->m_box.m_points[1].y);
          glVertex2d(gcells[y][x]->m_box.m_points[2].x, gcells[y][x]->m_box.m_points[2].y);
          glVertex2d(gcells[y][x]->m_box.m_points[3].x, gcells[y][x]->m_box.m_points[3].y);
          glEnd();

          for(const auto& poly : gcells[y][x]->m_obstacles)
            {
              if(poly.m_metal != types::Metal::NONE && !m_metal_layers[poly.m_metal])
                {
                  continue;
                }

              details::draw_polygon(poly);
            }
        }
    }

  if(m_is_gcell_hovered)
    {
      m_hovered_gcell = std::move(hovered_gcell);
    }

  if(!m_is_dragging)
    {
      if(m_is_gcell_hovered)
        {
          setCursor(Qt::PointingHandCursor);
        }
      else
        {

          setCursor(Qt::ArrowCursor);
        }
    }

  /** Draw pins */
  const auto& pins = m_data->m_pins;

  for(const auto& [name, net] : m_data->m_nets)
    {
      for(const auto& pin_name : net->m_pins)
        {
          const auto& pin = m_data->m_pins.at(pin_name);

          for(const auto& poly : pin->m_obs)
            {
              if(poly.m_metal != types::Metal::NONE && !m_metal_layers[poly.m_metal])
                {
                  continue;
                }

              details::draw_polygon(poly);
            }

          const auto& poly = pin->m_ports[0];

          if(poly.m_metal != types::Metal::NONE && !m_metal_layers[poly.m_metal])
            {
              continue;
            }

          details::draw_polygon(poly);
        }
    }

  glPopMatrix();
}

void
Scene::wheelEvent(QWheelEvent* event)
{
  if(QGuiApplication::queryKeyboardModifiers().testFlag(Qt::ControlModifier))
    {
      const double scale_factor    = 1.1;
      const double old_zoom_factor = m_zoom_factor;

      if(event->angleDelta().y() > 0)
        {
          m_zoom_factor *= scale_factor;
        }
      else
        {
          m_zoom_factor /= scale_factor;
        }

      const QPointF mouse_pos            = event->position();
      const double  mouse_pos_in_scene_x = (mouse_pos.x() - m_pan_x) / old_zoom_factor;
      const double  mouse_pos_in_scene_y = (mouse_pos.y() - m_pan_y) / old_zoom_factor;

      m_pan_x                            = mouse_pos.x() - mouse_pos_in_scene_x * m_zoom_factor;
      m_pan_y                            = mouse_pos.y() - mouse_pos_in_scene_y * m_zoom_factor;

      update();
    }
}

void
Scene::mousePressEvent(QMouseEvent* event)
{
  if(event->button() == Qt::MiddleButton)
    {
      m_last_mouse_position = event->pos();
      m_is_dragging         = true;

      setCursor(Qt::DragMoveCursor);
    }

  if(m_cursor_mode == CursorMode::SELECT && event->button() == Qt::RightButton && m_is_gcell_hovered)
    {
      show_context_menu(event->pos());
    }
}

void
Scene::mouseMoveEvent(QMouseEvent* event)
{
  if(m_is_dragging)
    {
      QPointF delta = event->pos() - m_last_mouse_position;
      m_pan_x += delta.x();
      m_pan_y += delta.y();
    }

  m_last_mouse_position = event->pos();

  QTransform transform;
  transform.translate(m_pan_x, m_pan_y);
  transform.scale(m_zoom_factor, m_zoom_factor);
  m_last_mouse_scene_position = transform.inverted().map(m_last_mouse_position);

  update();
}

void
Scene::mouseReleaseEvent(QMouseEvent* event)
{
  if(event->button() == Qt::MiddleButton)
    {
      m_is_dragging = false;

      setCursor(Qt::ArrowCursor);
    }
}

void
Scene::show_context_menu(const QPoint& pos)
{
  QMenu    menu(this);
  QAction* examine_gcell_action = menu.addAction("Examine gcell");

  connect(examine_gcell_action, &QAction::triggered, this, [this]() {
    const auto gcell = m_data->m_gcells[m_hovered_gcell.first][m_hovered_gcell.second];
    emit       send_examine(gcell);
  });

  menu.exec(mapToGlobal(pos));
}

void
Scene::recv_viewer_data(def::Data const* data)
{
  m_data                   = data;
  double bounding_width    = (m_data->m_box[2] - m_data->m_box[0]);
  double bounding_height   = (m_data->m_box[3] - m_data->m_box[1]);

  m_zoom_factor            = std::min((width() * 0.85) / bounding_width, (height() * 0.85) / bounding_height);

  double box_center_x      = (m_data->m_box[0] + m_data->m_box[2]) / 2.0;
  double box_center_y      = (m_data->m_box[1] + m_data->m_box[3]) / 2.0;

  double viewport_center_x = width() / 2.0;
  double viewport_center_y = height() / 2.0;

  m_pan_x                  = viewport_center_x - box_center_x * m_zoom_factor;
  m_pan_y                  = viewport_center_y - box_center_y * m_zoom_factor;

  update();
}

void
Scene::recv_cursor_mode(const CursorMode mode)
{
  m_cursor_mode = mode;
  update();
}

void
Scene::recv_view_mode(const ViewMode mode)
{
  m_view_mode = mode;
  update();
}

void
Scene::recv_metal_checked(const types::Metal metal, const bool status)
{
  m_metal_layers[metal] = status;
  update();
}

/** ======================= Widget methods ======================= */

Widget::Widget()
{
  Scene* scene = new Scene();
  connect(this, &Widget::send_viewer_data, scene, &Scene::recv_viewer_data);

  QVBoxLayout* layout = new QVBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(0);
  layout->addWidget(create_tool_bar());
  layout->addWidget(scene);

  connect(this, &Widget::send_cursor_mode, scene, &Scene::recv_cursor_mode);
  connect(this, &Widget::send_view_mode, scene, &Scene::recv_view_mode);
  connect(this, &Widget::send_metal_checked, scene, &Scene::recv_metal_checked);
  connect(scene, &Scene::send_examine, this, &Widget::send_examine);
}

QToolBar*
Widget::create_tool_bar()
{
  /** Cursor buttons */
  auto [cursor_none_button, cursor_none_action]     = details::create_tool_button(this, ":Icons/cursor-none.png", "Default", true);
  auto [cursor_select_button, cursor_select_action] = details::create_tool_button(this, ":Icons/cursor-select.png", "Select one or more gcells");

  connect(cursor_none_action, &QAction::triggered, [this, cursor_none_button, cursor_select_button]() {
    emit send_cursor_mode(CursorMode::NONE);

    cursor_none_button->setChecked(true);
    cursor_select_button->setChecked(false);
  });
  connect(cursor_select_action, &QAction::triggered, [this, cursor_none_button, cursor_select_button]() {
    emit send_cursor_mode(CursorMode::SELECT);

    cursor_none_button->setChecked(false);
    cursor_select_button->setChecked(true);
  });

  /** View menu button */
  QToolButton* view_dropdown_button = create_view_menu();

  /** Tool bar */
  QToolBar*    tool_bar             = new QToolBar(this);
  tool_bar->setIconSize({ 20, 20 });
  tool_bar->addWidget(cursor_none_button);
  tool_bar->addWidget(cursor_select_button);
  tool_bar->addSeparator();
  tool_bar->addWidget(view_dropdown_button);

  return tool_bar;
}

QToolButton*
Widget::create_view_menu()
{
  QMenu*   view_menu     = new QMenu(this);

  QAction* standard_view = view_menu->addAction("Standard");
  standard_view->setCheckable(true);
  standard_view->setChecked(true);

  connect(standard_view, &QAction::triggered, [this]() { emit send_view_mode(ViewMode::STANDARD); });

  QAction* pins_density_view = view_menu->addAction("Pins density");
  pins_density_view->setCheckable(true);

  connect(pins_density_view, &QAction::triggered, [this]() { emit send_view_mode(ViewMode::PINS_DENSITY); });

  QAction* tracks_density_view = view_menu->addAction("Tracks density");
  tracks_density_view->setCheckable(true);

  connect(tracks_density_view, &QAction::triggered, [this]() { emit send_view_mode(ViewMode::TRACKS_DENSITY); });

  QActionGroup* action_group = new QActionGroup(this);
  action_group->addAction(standard_view);
  action_group->addAction(pins_density_view);
  action_group->addAction(tracks_density_view);
  action_group->setExclusive(true);

  auto [view_dropdown_button, _] = details::create_tool_button(this, ":Icons/view-mode.png", "Select view mode");
  view_dropdown_button->setPopupMode(QToolButton::MenuButtonPopup);
  view_dropdown_button->setMenu(view_menu);

  connect(view_dropdown_button, &QToolButton::clicked, [view_dropdown_button]() {
    if(view_dropdown_button->menu())
      {
        view_dropdown_button->menu()->exec(view_dropdown_button->mapToGlobal(QPoint(0, view_dropdown_button->height())));
      }
  });

  return view_dropdown_button;
}

} // namespace gui
