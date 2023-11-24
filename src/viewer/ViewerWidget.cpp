#include <QApplication>
#include <QPalette>
#include <random>

#include "pdk/convertor.hpp"
#include "viewer/ViewerWidget.hpp"

bool PaintBufferObject::operator<(const PaintBufferObject& other) const
{
    return static_cast<int32_t>(layer) < static_cast<int32_t>(other.layer);
};

ViewerWidget::ViewerWidget(QWidget* t_parent)
    : QWidget(t_parent)
{
    PDK pdk;
    Convertor::deserialize("./skyWater130.bin", pdk);

    m_encoder = std::make_unique<Encoder>();
    m_pdk = std::make_shared<PDK>(pdk);
    m_config = std::make_shared<Config>();
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

        int32_t scale = m_displayMode == DisplayMode::SCALED ? 1 : m_pdk->scale;

        for (auto& [x, y] : m_data->dieArea) {
            m_max.first = std::max(x * scale, m_max.first);
            m_max.second = std::max(y * scale, m_max.second);

            m_min.first = std::min(x * scale, m_min.first);
            m_min.second = std::min(y * scale, m_min.second);
        }

        m_max.first = m_max.first / m_pdk->scale;
        m_max.second = m_max.second / m_pdk->scale;

        m_min.first = m_min.first / m_pdk->scale;
        m_min.second = m_min.second / m_pdk->scale;

        // Setup paint buffer
        // ======================================================================================

        m_paintBuffer.clear();

        for (auto& row : m_data->cells) {
            for (auto& col : row) {
                if (m_displayMode == DisplayMode::SCALED) {
                    QPolygon poly {};

                    for (auto& [x, y] : col->originalPlace.vertex) {
                        poly.append(QPoint(x * scale, y * scale));
                    }

                    m_paintBuffer.insert(PaintBufferObject { poly, QColor(255, 255, 255, 255), QColor(Qt::transparent), MetalLayer::NONE });
                }

                for (auto& geom : col->geometries) {
                    if (!(geom->type == RectangleType::TRACK)) {
                        QPolygon poly {};

                        poly.append(QPoint(geom->vertex[0].x * scale, geom->vertex[0].y * scale));
                        poly.append(QPoint(geom->vertex[1].x * scale, geom->vertex[1].y * scale));
                        poly.append(QPoint(geom->vertex[2].x * scale, geom->vertex[2].y * scale));
                        poly.append(QPoint(geom->vertex[3].x * scale, geom->vertex[3].y * scale));

                        std::pair<QColor, QColor> penBrushColor = selectBrushAndPen(geom->layer);

                        m_paintBuffer.insert(PaintBufferObject { poly, penBrushColor.first, penBrushColor.second, geom->layer });
                    }
                }

                for (auto& [_, net] : col->nets) {
                    for (auto& connection : net) {
                        QPolygon poly {};

                        poly.append(QPoint(connection.start.x * scale, connection.start.y * scale));
                        poly.append(QPoint(connection.end.x * scale, connection.end.y * scale));

                        m_paintBuffer.insert(PaintBufferObject { poly, QColor(Qt::white), QColor(Qt::white), MetalLayer::NONE });
                    }
                }
            }
        }
    }
    // else {
    //     m_max = { 0, 0 };
    //     m_min = { INT32_MAX, INT32_MAX };

    //     for (auto& [x, y] : m_data->dieArea) {
    //         m_max.first = std::max(x, m_max.first);
    //         m_max.second = std::max(y, m_max.second);

    //         m_min.first = std::min(x, m_min.first);
    //         m_min.second = std::min(y, m_min.second);
    //     }

    //     // Setup paint buffer
    //     // ======================================================================================

    //     m_paintBuffer.clear();

    //     int32_t shiftStep = 25;
    //     int32_t shiftX {};
    //     int32_t shiftY {};

    //     for (int32_t j = 0; j < m_data->numCellY; ++j) {
    //         for (int32_t i = 0; i < m_data->numCellX; ++i) {
    //             QPolygon poly(QRect(i * (m_data->cellSize + 2) + shiftX, j * (m_data->cellSize + 2) + shiftY, m_data->cellSize + 2, m_data->cellSize + 2));

    //             m_paintBuffer.insert(PaintBufferObject { poly, QColor(255, 255, 255, 255), QColor(Qt::transparent), MetalLayer::NONE });

    //             std::pair<QColor, QColor> penBrushColor {};

    //             for (int8_t k = 0; k < m_data->cells[j][i]->source.size(1); ++k) {
    //                 penBrushColor = selectBrushAndPen(static_cast<MetalLayer>(k));
    //                 m_paintBuffer.insert(PaintBufferObject { Point(poly[0].x(), poly[0].y()), torchMatrixToQImage(m_data->cells[j][i]->source[0][k], penBrushColor.first) });
    //             }

    //             // for (int8_t k = 0; k < m_data->cells[j][i]->target.size(1); ++k) {
    //             //     penBrushColor = selectBrushAndPen(static_cast<MetalLayer>(k));
    //             //     m_paintBuffer.insert(PaintBufferObject { Point(poly[0].x(), poly[0].y()), torchMatrixToQImage(m_data->cells[j][i]->target[0][k], penBrushColor.first) });
    //             // }

    //             shiftX += shiftStep;
    //         }

    //         shiftX = 0;
    //         shiftY += shiftStep;
    //     }
    // }

    double newInitialScale = std::min((width() * 0.8) / (std::abs(m_max.first - m_min.first)), (height() * 0.8) / (std::abs(m_max.second - m_min.second)));

    m_currentScale = m_currentScale / m_initialScale * newInitialScale;
    m_initialScale = newInitialScale;

    m_moveAxesIn = QPointF((width() / m_currentScale - (m_max.first + m_min.first)) / 2.0, (height() / m_currentScale - (m_max.second + m_min.second)) / 2.0);
    m_axesPos = m_moveAxesIn;
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

QImage ViewerWidget::torchMatrixToQImage(const torch::Tensor& t_matrix, const QColor& t_fillColor)
{
    int32_t height = t_matrix.size(0);
    int32_t width = t_matrix.size(1);
    QImage image(width, height, QImage::Format_RGBA64);

    for (int32_t y = 0; y < height; ++y) {
        for (int32_t x = 0; x < width; ++x) {
            if (t_matrix[y][x].item<float>() != 0.0) {
                if (t_matrix[y][x].item<float>() > 0) {
                    image.setPixelColor(x, y, t_fillColor);
                } else {
                    image.setPixelColor(x, y, QColor(255, 255, 255));
                }
            } else {
                image.setPixelColor(x, y, QColor(0, 0, 0, 0));
            }
        }
    }

    return image;
}

void ViewerWidget::paintEvent(QPaintEvent* t_event)
{
    Q_UNUSED(t_event);

    if (m_data != nullptr) {
        QPainter* painter = new QPainter();

        painter->begin(this);
        painter->translate(m_moveAxesIn * m_currentScale);
        painter->scale(m_currentScale, m_currentScale);

        for (auto& bufferObject : m_paintBuffer) {
            if (!bufferObject.isTensorMode) {
                painter->setPen(QPen(bufferObject.penColor, 1.0 / m_currentScale));
                painter->setBrush(QBrush(bufferObject.brushColor));
                painter->drawPolygon(bufferObject.poly);
            } else {
                painter->drawImage(QPoint(bufferObject.position.x, bufferObject.position.y), bufferObject.tensor);
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
    m_encoder->readDef(std::string_view(t_fileName.toStdString()), m_data, m_pdk, m_config);

    setup();
    update();
}

void ViewerWidget::setDisplayMode(const int8_t& t_mode)
{
    m_displayMode = static_cast<DisplayMode>(t_mode);

    setup();
    update();
}
