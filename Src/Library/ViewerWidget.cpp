#include <QApplication>
#include <QDebug>
#include <QPainter>
#include <QPalette>
#include <QTransform>

#include "Include/ViewerWidget.hpp"

namespace gui::viewer
{

namespace details
{

std::pair<QColor, QColor>
get_metal_color(types::Metal metal)
{
  switch(metal)
    {
    case types::Metal::L1:
      {
        return { QColor(0, 0, 255), QColor(0, 0, 255, 55) };
      }
    case types::Metal::M1:
      {
        return { QColor(255, 0, 0), QColor(255, 0, 0, 55) };
      }
    case types::Metal::M2:
      {
        return { QColor(0, 255, 0), QColor(0, 255, 0, 55) };
      }
    case types::Metal::M3:
      {
        return { QColor(255, 255, 0), QColor(255, 255, 0, 55) };
      }
    case types::Metal::M4:
      {
        return { QColor(0, 255, 255), QColor(0, 255, 255, 55) };
      }
    case types::Metal::M5:
      {
        return { QColor(255, 0, 255), QColor(255, 0, 255, 55) };
      }
    case types::Metal::M6:
      {
        return { QColor(125, 125, 255), QColor(125, 125, 255, 55) };
      }
    case types::Metal::M7:
      {
        return { QColor(255, 125, 125), QColor(255, 125, 125, 55) };
      }
    case types::Metal::M8:
      {
        return { QColor(125, 255, 125), QColor(125, 255, 125, 55) };
      }
    case types::Metal::M9:
      {
        return { QColor(255, 75, 125), QColor(255, 75, 125, 55) };
      }
    default:
      {
        return { QColor(255, 255, 255), QColor(255, 255, 255, 5) };
      }
    }
}

} // namespace

Widget::Widget(QWidget* parent)
    : QWidget(parent), m_data(), m_is_dragging(false), m_scale_factor(1.0), m_last_mouse_pos(), m_transition_offset()
{
  setMouseTracking(true);
  setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

  QPalette palette;
  palette.setColor(QPalette::Window, QColor(13, 13, 13));
  setAutoFillBackground(true);
  setPalette(palette);
}

void
Widget::set_viewer_data(const Data& data)
{
  m_data = data;
  update();
}

void
Widget::paintEvent(QPaintEvent* event)
{
  QTransform transform;
  transform.translate(m_transition_offset.x(), m_transition_offset.y());
  transform.scale(m_scale_factor, m_scale_factor);

  QPainter painter(this);
  painter.setTransform(transform);

  {
    auto [pen_color, brush_color] = details::get_metal_color(types::Metal::NONE);
    painter.setPen(QPen(pen_color, 1.0 / m_scale_factor));
    painter.setBrush(QBrush(brush_color));
    painter.drawRect(QRectF(m_data.m_box[0], m_data.m_box[1], m_data.m_box[2], m_data.m_box[3]));
  }

  for(const auto& pair : m_data.m_rects)
    {
      const types::Polygon& rect    = pair.first;
      const types::Metal    metal   = pair.second;

      auto [pen_color, brush_color] = details::get_metal_color(metal);
      painter.setPen(QPen(pen_color, 1.0 / m_scale_factor));
      painter.setBrush(QBrush(brush_color));

      QPointF points[4] = {
        QPointF(rect[0], rect[1]),
        QPointF(rect[2], rect[3]),
        QPointF(rect[4], rect[5]),
        QPointF(rect[6], rect[7])
      };

      painter.drawPolygon(points, 4);
    }

  painter.resetTransform();

  QString text_coords = QString("X: %1, Y: %2").arg(m_mouse_scene_pos.x(), 0, 'd', 0).arg(m_mouse_scene_pos.y(), 0, 'f', 0);
  painter.setBrush(QColor{ 255, 255, 255 });
  painter.setPen(QColor{ 255, 255, 255 });
  painter.drawText(10, height() - 20, text_coords);
}

void
Widget::resizeEvent(QResizeEvent* event)
{
  QSize new_size = event->size();
  QSize old_size = event->oldSize();

  if(old_size.width() > 0 && old_size.height() > 0)
    {
      QPointF widgetCenter         = rect().center();
      QPointF visibleCenterInScene = (widgetCenter - m_transition_offset) / m_scale_factor;
      m_transition_offset          = widgetCenter - visibleCenterInScene * m_scale_factor;
    }
  else
    {
      m_scale_factor      = std::min(new_size.width() / static_cast<qreal>(m_data.m_box[2]), new_size.height() / static_cast<qreal>(m_data.m_box[3]));
      m_transition_offset = QPointF((new_size.width() - m_data.m_box[2] * m_scale_factor) / 2.0, (new_size.height() - m_data.m_box[3] * m_scale_factor) / 2.0);
    }

  update();
}

void
Widget::wheelEvent(QWheelEvent* event)
{
  if(QGuiApplication::queryKeyboardModifiers().testFlag(Qt::ControlModifier))
    {
      double zoom_factor      = 1.1;
      double old_scale_factor = m_scale_factor;

      if(event->angleDelta().y() > 0)
        {
          m_scale_factor *= zoom_factor;
        }
      else
        {
          m_scale_factor /= zoom_factor;
        }

      QPointF mousePos        = event->position();
      QPointF mousePosInScene = (mousePos - m_transition_offset) / old_scale_factor;
      m_transition_offset     = mousePos - mousePosInScene * m_scale_factor;

      update();
    }
}

void
Widget::mousePressEvent(QMouseEvent* event)
{
  if(event->button() == Qt::LeftButton)
    {
      m_last_mouse_pos = event->position();
      m_is_dragging    = true;
    }
}

void
Widget::mouseMoveEvent(QMouseEvent* event)
{
  QTransform transform;
  transform.translate(m_transition_offset.x(), m_transition_offset.y());
  transform.scale(m_scale_factor, m_scale_factor);

  QPointF mouse_widget_pos = event->position();
  m_mouse_scene_pos        = transform.inverted().map(mouse_widget_pos);

  if(m_is_dragging)
    {
      QPointF delta = event->position() - m_last_mouse_pos;
      m_transition_offset += delta;
      m_last_mouse_pos = event->position();
    }

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

} // namespace gui
