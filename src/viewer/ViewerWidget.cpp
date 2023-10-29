#include <QApplication>
#include <QPalette>
#include <random>

#include "viewer/ViewerWidget.hpp"

bool PaintBufferObject::operator<(const PaintBufferObject& other) const
{
    return static_cast<int32_t>(layer) < static_cast<int32_t>(other.layer);
};

ViewerWidget::ViewerWidget(QWidget* t_parent)
    : QWidget(t_parent)
{
    m_data = std::make_shared<Data>();

    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    QPalette pal;
    pal.setColor(QPalette::Window, QColor(11, 11, 11));
    setAutoFillBackground(true);
    setPalette(pal);
};

void ViewerWidget::setup()
{
    if (m_displayMode != DisplayMode::TENSOR) {
        m_max = { 0, 0 };
        m_min = { INT32_MAX, INT32_MAX };

        int32_t scale = m_displayMode == DisplayMode::SCALED ? 1 : m_data->pdk.scale;

        for (auto& [x, y] : m_data->dieArea) {
            m_max.first = std::max(x * scale, m_max.first);
            m_max.second = std::max(y * scale, m_max.second);

            m_min.first = std::min(x * scale, m_min.first);
            m_min.second = std::min(y * scale, m_min.second);
        }

        double newInitialScale = std::min((width() * 0.8) / (std::abs(m_max.first - m_min.first)), (height() * 0.8) / (std::abs(m_max.second - m_min.second)));

        m_currentScale = m_currentScale / m_initialScale * newInitialScale;
        m_initialScale = newInitialScale;

        m_moveAxesIn = QPointF((width() / m_currentScale - (m_max.first + m_min.first)) / 2.0, (height() / m_currentScale - (m_max.second + m_min.second)) / 2.0);
        m_axesPos = m_moveAxesIn;

        // Setup paint buffer
        // ======================================================================================

        m_paintBuffer.clear();

        int32_t shiftStep = m_displayMode == DisplayMode::SCALED ? 100 : 0;
        int32_t shiftX {};
        int32_t shiftY {};

        for (auto& row : m_data->cells) {
            for (auto& col : row) {

                if (m_displayMode == DisplayMode::SCALED) {
                    QPolygon poly {};

                    for (auto& [x, y] : col->originalPlace.vertex) {
                        poly.append(QPoint(x * scale + shiftX, y * scale + shiftY));
                    }

                    m_paintBuffer.insert(PaintBufferObject { poly, QColor(255, 255, 255, 255), QColor(Qt::transparent), MetalLayer::NONE });
                }

                for (auto& pin : col->pins) {
                    QPolygon poly {};

                    poly.append(QPoint(pin->vertex[0].x * scale + shiftX, pin->vertex[0].y * scale + shiftY));
                    poly.append(QPoint(pin->vertex[1].x * scale + shiftX, pin->vertex[1].y * scale + shiftY));
                    poly.append(QPoint(pin->vertex[2].x * scale + shiftX, pin->vertex[2].y * scale + shiftY));
                    poly.append(QPoint(pin->vertex[3].x * scale + shiftX, pin->vertex[3].y * scale + shiftY));

                    std::pair<QColor, QColor> penBrushColor = selectBrushAndPen(pin->layer);

                    m_paintBuffer.insert(PaintBufferObject { poly, penBrushColor.first, penBrushColor.second, pin->layer });
                }

                for (auto& geom : col->geometries) {
                    QPolygon poly {};

                    poly.append(QPoint(geom->vertex[0].x * scale + shiftX, geom->vertex[0].y * scale + shiftY));
                    poly.append(QPoint(geom->vertex[1].x * scale + shiftX, geom->vertex[1].y * scale + shiftY));
                    poly.append(QPoint(geom->vertex[2].x * scale + shiftX, geom->vertex[2].y * scale + shiftY));
                    poly.append(QPoint(geom->vertex[3].x * scale + shiftX, geom->vertex[3].y * scale + shiftY));

                    std::pair<QColor, QColor> penBrushColor = selectBrushAndPen(geom->layer);

                    m_paintBuffer.insert(PaintBufferObject { poly, penBrushColor.first, penBrushColor.second, geom->layer });
                }

                shiftX += shiftStep;
            }

            shiftX = 0;
            shiftY += shiftStep;
        }
    }
}

std::pair<QColor, QColor> ViewerWidget::selectBrushAndPen(const MetalLayer& t_layer)
{
    switch (t_layer) {
    case MetalLayer::L1:
        return std::pair<QColor, QColor>(QColor(0, 0, 255), QColor(0, 0, 255, 55));
    case MetalLayer::M1:
        return std::pair<QColor, QColor>(QColor(255, 0, 0), QColor(255, 0, 0, 55));
    case MetalLayer::M2:
        return std::pair<QColor, QColor>(QColor(0, 255, 0), QColor(0, 255, 0, 55));
        break;
    case MetalLayer::M3:
        return std::pair<QColor, QColor>(QColor(255, 255, 0), QColor(255, 255, 0, 55));
        break;
    case MetalLayer::M4:
        return std::pair<QColor, QColor>(QColor(0, 255, 255), QColor(0, 255, 255, 55));
        break;
    case MetalLayer::M5:
        return std::pair<QColor, QColor>(QColor(255, 0, 255), QColor(255, 0, 255, 55));
        break;
    case MetalLayer::M6:
        return std::pair<QColor, QColor>(QColor(125, 125, 255), QColor(125, 125, 255, 55));
        break;
    case MetalLayer::M7:
        return std::pair<QColor, QColor>(QColor(255, 125, 125), QColor(255, 125, 125, 55));
        break;
    case MetalLayer::M8:
        return std::pair<QColor, QColor>(QColor(125, 255, 125), QColor(125, 255, 125, 55));
        break;
    case MetalLayer::M9:
        return std::pair<QColor, QColor>(QColor(255, 75, 125), QColor(255, 75, 125, 55));
        break;
    default:
        return std::pair<QColor, QColor>(QColor(255, 255, 255), QColor(255, 255, 255, 55));
        break;
    }
};

void ViewerWidget::paintEvent(QPaintEvent* t_event)
{
    Q_UNUSED(t_event);

    if (m_data != nullptr) {
        QPainter* painter = new QPainter();

        painter->begin(this);
        painter->translate(m_moveAxesIn * m_currentScale);
        painter->scale(m_currentScale, m_currentScale);

        double left = -m_moveAxesIn.x() - 100.0;
        double top = -m_moveAxesIn.y() - 100.0;
        double right = left + width() / m_currentScale + 100.0;
        double bottom = top + height() / m_currentScale + 100.0;

        for (auto& bufferObject : m_paintBuffer) {
            if ((bufferObject.poly[0].x() >= left && bufferObject.poly[0].y() >= top) || (bufferObject.poly[2].x() <= right && bufferObject.poly[2].y() <= bottom)) {
                painter->setPen(QPen(bufferObject.penColor, 1.0 / m_currentScale));
                painter->setBrush(QBrush(bufferObject.brushColor));
                painter->drawPolygon(bufferObject.poly);
            }
        };

        painter->end();
    }
}

void ViewerWidget::resizeEvent(QResizeEvent* t_event)
{
    Q_UNUSED(t_event);

    if (m_data != nullptr) {
        auto newInitialScale = std::min((width() * 0.8) / (std::abs(m_max.first - m_min.first)), (height() * 0.8) / (std::abs(m_max.second - m_min.second)));

        m_currentScale = m_currentScale / m_initialScale * newInitialScale;
        m_initialScale = newInitialScale;
    }
}

void ViewerWidget::mousePressEvent(QMouseEvent* t_event)
{
    switch (t_event->button()) {

    case Qt::LeftButton: {
        m_mode = Mode::DRAGING;
        m_mouseTriggerPos = t_event->pos();
        m_mouseCurrentPos = m_mouseTriggerPos;
        break;
    }
    default:
        break;
    }

    update();
}

void ViewerWidget::mouseMoveEvent(QMouseEvent* t_event)
{
    switch (m_mode) {
    case Mode::DRAGING: {
        m_moveAxesIn = m_axesPos - (m_mouseTriggerPos - t_event->pos()) / m_currentScale;
        update();
        break;
    }
    default:
        break;
    }
}

void ViewerWidget::mouseReleaseEvent(QMouseEvent* t_event)
{
    switch (m_mode) {
    case Mode::DRAGING: {
        m_mode = Mode::DEFAULT;
        m_axesPos = m_moveAxesIn;
        setCursor(QCursor(Qt::ArrowCursor));
        update();
        break;
    }
    default:
        break;
    }
}

void ViewerWidget::wheelEvent(QWheelEvent* t_event)
{
    if (m_mode == Mode::DEFAULT && QGuiApplication::queryKeyboardModifiers().testFlag(Qt::ControlModifier)) {
        m_scroll += 0.1 * (t_event->angleDelta().y() > 0 ? 1 : -1);

        m_prevCurrenScale = m_currentScale;
        m_currentScale = m_initialScale / (1.0 / (std::pow(2.718281282846, 1 * m_scroll)));

        m_moveAxesIn = m_axesPos - (m_currentScale / m_prevCurrenScale - 1) * t_event->position() / m_currentScale;
        m_axesPos = m_moveAxesIn;

        update();
    }
}

void ViewerWidget::render(QString& t_fileName)
{
    m_encoder.readDef(std::string_view(t_fileName.toStdString()), "./skyWater130.bin", m_data);

    setup();
    update();
}

void ViewerWidget::setDisplayMode(const int8_t& t_mode)
{
    m_displayMode = static_cast<DisplayMode>(t_mode);

    setup();
    update();
}
