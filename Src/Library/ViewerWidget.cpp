#include <QApplication>
#include <QDebug>
#include <QPainter>
#include <QPalette>
#include <QTransform>

#include "Include/ViewerWidget.hpp"

namespace gui::viewer
{

Widget::Widget(QWidget* parent)
    : QOpenGLWidget(parent), m_data(), m_is_dragging(false), m_zoom_factor(1.0f), m_pan_x(0.0f), m_pan_y(0.0f)
{
  setMouseTracking(true);
  setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

void
Widget::initializeGL()
{
  initializeOpenGLFunctions();
  glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void
Widget::resizeGL(int w, int h)
{
  glViewport(0, 0, w, h);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(0, w, h, 0, -1, 1);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
}

void
Widget::paintGL()
{
  glClearColor(0.05f, 0.05f, 0.05f, 1.0f);

  glPushMatrix();
  glTranslatef(m_pan_x, m_pan_y, 0.0f);
  glScalef(m_zoom_factor, m_zoom_factor, 1.0f);

  for(const auto& batch : m_data.m_batches)
    {
      for(const auto& pair : batch.m_rects)
        {
          const auto& [rect, color] = pair;

          const double red          = color.redF();
          const double green        = color.greenF();
          const double blue         = color.blueF();

          if(red == 1.0 && green == 1.0 && blue == 1.0)
            {
              glColor4f(red, green, blue, 1.00f);
              glBegin(GL_LINE_LOOP);
              glVertex2f(rect[0], rect[1]);
              glVertex2f(rect[2], rect[3]);
              glVertex2f(rect[4], rect[5]);
              glVertex2f(rect[6], rect[7]);
              glEnd();
            }
          else
            {
              glColor4f(red, green, blue, 0.25f);
              glBegin(GL_QUADS);
              glVertex2f(rect[0], rect[1]);
              glVertex2f(rect[2], rect[3]);
              glVertex2f(rect[4], rect[5]);
              glVertex2f(rect[6], rect[7]);
              glEnd();
            }
        }
    }

  glPopMatrix();
}

void
Widget::wheelEvent(QWheelEvent* event)
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
Widget::mousePressEvent(QMouseEvent* event)
{
  if(event->button() == Qt::LeftButton)
    {
      m_last_mouse_position = event->pos();
      m_is_dragging         = true;
    }
}

void
Widget::mouseMoveEvent(QMouseEvent* event)
{
  if(m_is_dragging)
    {
      QPoint delta = event->pos() - m_last_mouse_position;
      m_pan_x += delta.x();
      m_pan_y += delta.y();
    }

  m_last_mouse_position = event->pos();

  update();
}

void
Widget::mouseReleaseEvent(QMouseEvent* event)
{
  if(event->button() == Qt::LeftButton)
    {
      m_is_dragging = false;
    }
}

void
Widget::set_viewer_data(const Data& data)
{
  m_data                 = data;

  double bounding_width  = (m_data.m_box[2] - m_data.m_box[0]);
  double bounding_height = (m_data.m_box[3] - m_data.m_box[1]);

  m_zoom_factor          = std::min(width() / bounding_width, height() / bounding_height);
  m_pan_x                = (width() - bounding_width * m_zoom_factor) / 2.0f;
  m_pan_y                = (height() - bounding_height * m_zoom_factor) / 2.0f;

  update();
}

} // namespace gui
