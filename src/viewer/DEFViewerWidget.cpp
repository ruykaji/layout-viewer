#include <QApplication>
#include <QPainter>
#include <QPalette>

#include "DEFViewerWidget.hpp"

DEFViewerWidget::DEFViewerWidget(QWidget* t_parent)
    : QWidget(t_parent)
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    QPalette pal;
    pal.setColor(QPalette::Window, QColor(16, 16, 16));
    setAutoFillBackground(true);
    setPalette(pal);
};

void DEFViewerWidget::render(QString& t_fileName)
{
    m_def = m_defEncoder.read(std::string_view(t_fileName.toStdString()));

    for (auto& [x, y] : m_def->dieArea.points) {
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
        painter->setPen(QPen(QColor(Qt::white), 1.0 / m_currentScale));

        QPolygonF poly {};

        for (auto& [x, y] : m_def->dieArea.points) {
            poly.append(QPointF(x, y));
        }

        painter->drawPolygon(poly);

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