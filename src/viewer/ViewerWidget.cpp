#include <QApplication>
#include <QPainter>
#include <QPalette>
#include <random>

#include "viewer/ViewerWidget.hpp"

std::vector<RGB> generateRandomUniqueColors(int n)
{
    std::set<RGB> uniqueColors;
    std::random_device rd;
    std::mt19937 eng(rd());
    std::uniform_int_distribution<> distr(0, 255);

    while (uniqueColors.size() < n) {
        RGB color = { distr(eng), distr(eng), distr(eng) };
        uniqueColors.insert(color);
    }

    return std::vector<RGB>(uniqueColors.begin(), uniqueColors.end());
}

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

void ViewerWidget::selectBrushAndPen(QPainter* t_painter, const MetalLayer& t_layer)
{
    switch (t_layer) {
    case MetalLayer::L1:
        t_painter->setPen(QPen(QColor(0, 0, 255), 1.0 / m_currentScale));
        t_painter->setBrush(QBrush(QColor(0, 0, 255, 55)));
        break;
    case MetalLayer::M1:
        t_painter->setPen(QPen(QColor(255, 0, 0), 1.0 / m_currentScale));
        t_painter->setBrush(QBrush(QColor(255, 0, 0, 55)));
        break;
    case MetalLayer::M2:
        t_painter->setPen(QPen(QColor(0, 255, 0), 1.0 / m_currentScale));
        t_painter->setBrush(QBrush(QColor(0, 255, 0, 55)));
        break;
    case MetalLayer::M3:
        t_painter->setPen(QPen(QColor(255, 255, 0), 1.0 / m_currentScale));
        t_painter->setBrush(QBrush(QColor(255, 255, 0, 55)));
        break;
    case MetalLayer::M4:
        t_painter->setPen(QPen(QColor(0, 255, 255), 1.0 / m_currentScale));
        t_painter->setBrush(QBrush(QColor(0, 255, 255, 55)));
        break;
    case MetalLayer::M5:
        t_painter->setPen(QPen(QColor(255, 0, 255), 1.0 / m_currentScale));
        t_painter->setBrush(QBrush(QColor(255, 0, 255, 55)));
        break;
    case MetalLayer::M6:
        t_painter->setPen(QPen(QColor(125, 125, 255), 1.0 / m_currentScale));
        t_painter->setBrush(QBrush(QColor(125, 125, 255, 55)));
        break;
    case MetalLayer::M7:
        t_painter->setPen(QPen(QColor(255, 125, 125), 1.0 / m_currentScale));
        t_painter->setBrush(QBrush(QColor(255, 125, 125, 55)));
        break;
    case MetalLayer::M8:
        t_painter->setPen(QPen(QColor(125, 255, 125), 1.0 / m_currentScale));
        t_painter->setBrush(QBrush(QColor(125, 255, 125, 55)));
        break;
    case MetalLayer::M9:
        t_painter->setPen(QPen(QColor(255, 75, 125), 1.0 / m_currentScale));
        t_painter->setBrush(QBrush(QColor(255, 75, 125, 55)));
        break;
    default:
        t_painter->setPen(QPen(QColor(255, 255, 255), 1.0 / m_currentScale));
        t_painter->setBrush(QBrush(QColor(255, 255, 255, 55)));
        break;
    }
};

void ViewerWidget::render(QString& t_fileName)
{
    m_encoder.readDef(std::string_view(t_fileName.toStdString()), "./skyWater130.bin", m_data);
    m_colors = generateRandomUniqueColors(m_data->totalNets);

    for (auto& [x, y] : m_data->dieArea) {
        m_max.first = std::max(x, m_max.first);
        m_max.second = std::max(y, m_max.second);

        m_min.first = std::min(x, m_min.first);
        m_min.second = std::min(y, m_min.second);
    }

    auto newInitialScale = std::min((width() * 0.8) / (std::abs(m_max.first - m_min.first)), (height() * 0.8) / (std::abs(m_max.second - m_min.second)));

    m_currentScale = m_currentScale / m_initialScale * newInitialScale;
    m_initialScale = newInitialScale;

    m_moveAxesIn = QPointF((width() / m_currentScale - (m_max.first + m_min.first)) / 2.0, (height() / m_currentScale - (m_max.second + m_min.second)) / 2.0);
    m_axesPos = m_moveAxesIn;

    update();
}

void ViewerWidget::paintEvent(QPaintEvent* t_event)
{
    Q_UNUSED(t_event);

    if (m_data != nullptr) {
        QPainter* painter = new QPainter();

        painter->begin(this);
        painter->translate(m_moveAxesIn * m_currentScale);
        painter->scale(m_currentScale, m_currentScale);

        painter->setFont(QFont("Times", 1));

        int32_t shiftX {};
        int32_t shiftY {};

        for (auto& row : m_data->cells) {
            for (auto& col : row) {
                painter->setPen(QPen(QColor(QColor(255, 255, 255, 255)), 1.0 / m_currentScale));
                painter->setBrush(QBrush(QColor(Qt::transparent)));
                QPolygon matrixPoly {};

                for (auto& [x, y] : col->originalPlace.vertex) {
                    matrixPoly.append(QPoint(x + shiftX, y + shiftY));
                }

                painter->drawPolygon(matrixPoly);

                for (auto& pin : col->pins) {
                    painter->setPen(QPen(QColor(QColor(m_colors[pin->netIndex].r, m_colors[pin->netIndex].g, m_colors[pin->netIndex].b)), 1.0 / m_currentScale));
                    painter->setBrush(QBrush(QColor(m_colors[pin->netIndex].r, m_colors[pin->netIndex].g, m_colors[pin->netIndex].b, 55)));

                    QPolygon poly {};

                    poly.append(QPoint(pin->vertex[0].x + shiftX, pin->vertex[0].y + shiftY));
                    poly.append(QPoint(pin->vertex[1].x + shiftX, pin->vertex[1].y + shiftY));
                    poly.append(QPoint(pin->vertex[2].x + shiftX, pin->vertex[2].y + shiftY));
                    poly.append(QPoint(pin->vertex[3].x + shiftX, pin->vertex[3].y + shiftY));

                    painter->drawPolygon(poly);
                    painter->setPen(QPen(QColor(QColor(255, 255, 255)), 1.0 / m_currentScale));
                    painter->drawText(QPoint(pin->vertex[0].x + shiftX, pin->vertex[0].y + shiftY), QString::fromStdString(pin->name));
                }

                // for (auto& route : col->routes) {
                //     selectBrushAndPen(painter, route->layer);

                //     QPolygon poly {};

                //     poly.append(QPoint(route->vertex[0].x + shiftX, route->vertex[0].y + shiftY));
                //     poly.append(QPoint(route->vertex[1].x + shiftX, route->vertex[1].y + shiftY));
                //     poly.append(QPoint(route->vertex[2].x + shiftX, route->vertex[2].y + shiftY));
                //     poly.append(QPoint(route->vertex[3].x + shiftX, route->vertex[3].y + shiftY));

                //     painter->drawPolygon(poly);
                // }

                for (auto& geom : col->geometries) {
                    selectBrushAndPen(painter, geom->layer);

                    QPolygon poly {};

                    poly.append(QPoint(geom->vertex[0].x + shiftX, geom->vertex[0].y + shiftY));
                    poly.append(QPoint(geom->vertex[1].x + shiftX, geom->vertex[1].y + shiftY));
                    poly.append(QPoint(geom->vertex[2].x + shiftX, geom->vertex[2].y + shiftY));
                    poly.append(QPoint(geom->vertex[3].x + shiftX, geom->vertex[3].y + shiftY));

                    painter->drawPolygon(poly);
                }

                shiftX += 50;
            }

            shiftX = 0;
            shiftY += 50;
        }

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