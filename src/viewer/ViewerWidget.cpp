#include <QApplication>
#include <QPainter>
#include <QPalette>
#include <random>

#include "DEFViewerWidget.hpp"

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

DEFViewerWidget::DEFViewerWidget(QWidget* t_parent)
    : QWidget(t_parent)
{
    m_def = std::make_shared<Def>();

    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    QPalette pal;
    pal.setColor(QPalette::Window, QColor(11, 11, 11));
    setAutoFillBackground(true);
    setPalette(pal);
};

void DEFViewerWidget::selectBrushAndPen(QPainter* t_painter, const ML& t_layer)
{
    switch (t_layer) {
    case ML::L1:
        t_painter->setPen(QPen(QColor(0, 0, 255), 1.0 / m_currentScale));
        t_painter->setBrush(QBrush(QColor(0, 0, 255, 55)));
        break;
    case ML::M1:
        t_painter->setPen(QPen(QColor(255, 0, 0), 1.0 / m_currentScale));
        t_painter->setBrush(QBrush(QColor(255, 0, 0, 55)));
        break;
    case ML::M2:
        t_painter->setPen(QPen(QColor(0, 255, 0), 1.0 / m_currentScale));
        t_painter->setBrush(QBrush(QColor(0, 255, 0, 55)));
        break;
    case ML::M3:
        t_painter->setPen(QPen(QColor(255, 255, 0), 1.0 / m_currentScale));
        t_painter->setBrush(QBrush(QColor(255, 255, 0, 55)));
        break;
    case ML::M4:
        t_painter->setPen(QPen(QColor(0, 255, 255), 1.0 / m_currentScale));
        t_painter->setBrush(QBrush(QColor(0, 255, 255, 55)));
        break;
    case ML::M5:
        t_painter->setPen(QPen(QColor(255, 0, 255), 1.0 / m_currentScale));
        t_painter->setBrush(QBrush(QColor(255, 0, 255, 55)));
        break;
    case ML::M6:
        t_painter->setPen(QPen(QColor(125, 125, 255), 1.0 / m_currentScale));
        t_painter->setBrush(QBrush(QColor(125, 125, 255, 55)));
        break;
    case ML::M7:
        t_painter->setPen(QPen(QColor(255, 125, 125), 1.0 / m_currentScale));
        t_painter->setBrush(QBrush(QColor(255, 125, 125, 55)));
        break;
    case ML::M8:
        t_painter->setPen(QPen(QColor(125, 255, 125), 1.0 / m_currentScale));
        t_painter->setBrush(QBrush(QColor(125, 255, 125, 55)));
        break;
    case ML::M9:
        t_painter->setPen(QPen(QColor(255, 75, 125), 1.0 / m_currentScale));
        t_painter->setBrush(QBrush(QColor(255, 75, 125, 55)));
        break;
    default:
        t_painter->setPen(QPen(QColor(255, 255, 255), 1.0 / m_currentScale));
        t_painter->setBrush(QBrush(QColor(255, 255, 255, 55)));
        break;
    }
};

void DEFViewerWidget::render(QString& t_fileName)
{
    m_encoder.readDef(std::string_view(t_fileName.toStdString()), m_data);
    m_colors = generateRandomUniqueColors(m_data->totalNets);

    std::sort(m_data->geometries.begin(), m_data->geometries.end(), [](auto& t_left, auto& t_right) { return static_cast<int>(t_left->layer) < static_cast<int>(t_right->layer); });

    for (auto& [x, y] : m_data->dieArea) {
        m_max.first = std::max(x, m_max.first);
        m_max.second = std::max(y, m_max.second);

        m_min.first = std::min(x, m_min.first);
        m_min.second = std::min(y, m_min.second);
    }

    auto newInitialScale = std::min((width() * 0.8) / std::abs(m_max.first - m_min.first), (height() * 0.8) / std::abs(m_max.second - m_min.second));

    m_currentScale = m_currentScale / m_initialScale * newInitialScale;
    m_initialScale = newInitialScale;

    m_moveAxesIn = QPointF((width() / m_currentScale - (m_max.first + m_min.first)) / 2.0, (height() / m_currentScale - (m_max.second + m_min.second)) / 2.0);
    m_axesPos = m_moveAxesIn;

    update();
}

void DEFViewerWidget::paintEvent(QPaintEvent* t_event)
{
    Q_UNUSED(t_event);

    if (m_def != nullptr) {
        QPainter* painter = new QPainter();

        painter->begin(this);
        painter->translate(m_moveAxesIn * m_currentScale);
        painter->scale(m_currentScale, m_currentScale);

        // Geometries drawing
        //======================================================================

        // for (auto& geom : m_def->geometries) {
        //     selectBrushAndPen(painter, geom->layer);

        //     std::shared_ptr<Rectangle> rect = std::static_pointer_cast<Rectangle>(geom);

        //     QPolygon poly {};

        //     poly.append(QPoint(rect->vertex[0].x, rect->vertex[0].y));
        //     poly.append(QPoint(rect->vertex[1].x, rect->vertex[1].y));
        //     poly.append(QPoint(rect->vertex[2].x, rect->vertex[2].y));
        //     poly.append(QPoint(rect->vertex[3].x, rect->vertex[3].y));

        //     painter->drawPolygon(poly);
        // }

        // DieArea drawing
        //======================================================================

        painter->setFont(QFont("Times", 32));
        painter->setPen(QPen(QColor(QColor(255, 255, 255, 255)), 2.0 / m_currentScale));
        painter->setBrush(QBrush(QColor(Qt::transparent)));

        QPolygon dieAreaPoly {};

        for (auto& [x, y] : m_data->dieArea) {
            dieAreaPoly.append(QPoint(x, y));
        }

        painter->drawPolygon(dieAreaPoly);

        for (auto& row : m_data->cells) {
            for (auto& col : row) {
                QPolygon matrixPoly {};

                for (auto& [x, y] : col->originalPlace.vertex) {
                    matrixPoly.append(QPoint(x, y));
                }

                painter->drawPolygon(matrixPoly);
            }
        }

        for (auto& row : m_data->cells) {
            for (auto& col : row) {
                for (auto& pin : col->pins) {
                    painter->setPen(QPen(QColor(QColor(m_colors[pin->netIndex].r, m_colors[pin->netIndex].g, m_colors[pin->netIndex].b)), 1.0 / m_currentScale));
                    painter->setBrush(QBrush(QColor(m_colors[pin->netIndex].r, m_colors[pin->netIndex].g, m_colors[pin->netIndex].b, 55)));

                    QPolygon poly {};

                    poly.append(QPoint(pin->vertex[0].x, pin->vertex[0].y));
                    poly.append(QPoint(pin->vertex[1].x, pin->vertex[1].y));
                    poly.append(QPoint(pin->vertex[2].x, pin->vertex[2].y));
                    poly.append(QPoint(pin->vertex[3].x, pin->vertex[3].y));

                    painter->drawPolygon(poly);
                    painter->setPen(QPen(QColor(QColor(255, 255, 255)), 1.0 / m_currentScale));
                    painter->drawText(QPoint(pin->vertex[0].x, pin->vertex[0].y - 10), QString::fromStdString(pin->name));
                }

                for (auto& route : col->routes) {
                    selectBrushAndPen(painter, route->layer);

                    QPolygon poly {};

                    poly.append(QPoint(route->vertex[0].x, route->vertex[0].y));
                    poly.append(QPoint(route->vertex[1].x, route->vertex[1].y));
                    poly.append(QPoint(route->vertex[2].x, route->vertex[2].y));
                    poly.append(QPoint(route->vertex[3].x, route->vertex[3].y));

                    painter->drawPolygon(poly);
                }

                for (auto& geom : col->geometries) {
                    selectBrushAndPen(painter, geom->layer);

                    QPolygon poly {};

                    poly.append(QPoint(geom->vertex[0].x, geom->vertex[0].y));
                    poly.append(QPoint(geom->vertex[1].x, geom->vertex[1].y));
                    poly.append(QPoint(geom->vertex[2].x, geom->vertex[2].y));
                    poly.append(QPoint(geom->vertex[3].x, geom->vertex[3].y));

                    painter->drawPolygon(poly);
                }
            }
        }

        painter->end();
    }
}

void DEFViewerWidget::resizeEvent(QResizeEvent* t_event)
{
    Q_UNUSED(t_event);

    if (m_def != nullptr) {
        auto newInitialScale = std::min((width() * 0.8) / std::abs(m_max.first - m_min.first), (height() * 0.8) / std::abs(m_max.second - m_min.second));

        m_currentScale = m_currentScale / m_initialScale * newInitialScale;
        m_initialScale = newInitialScale;
    }
}

void DEFViewerWidget::mousePressEvent(QMouseEvent* t_event)
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

void DEFViewerWidget::mouseMoveEvent(QMouseEvent* t_event)
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

void DEFViewerWidget::mouseReleaseEvent(QMouseEvent* t_event)
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

void DEFViewerWidget::wheelEvent(QWheelEvent* t_event)
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