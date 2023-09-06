#include <QGuiApplication>
#include <QPainter>
#include <math.h>

#include "DrawWidget.hpp"

#define __BORDERS__ 0.9

DrawWidget::DrawWidget(Def* t_def, QWidget* t_parent)
    : m_def(t_def)
    , QWidget(t_parent)
{

    double newInitial = std::min((width() * __BORDERS__) / m_def->dieAria.first, (height() * __BORDERS__) / m_def->dieAria.second);

    m_scale.current = m_scale.current / m_scale.initial * newInitial;
    m_scale.initial = newInitial;
}

void DrawWidget::paintEvent(QPaintEvent* t_event)
{
    Q_UNUSED(t_event);

    QPainter* painter = new QPainter();

    painter->begin(this);

    painter->translate(width() / 2.0, height() / 2.0);
    painter->scale(m_scale.current, m_scale.current);

    painter->setPen(QPen(Qt::black, 1.0 / m_scale.current, Qt::SolidLine, Qt::RoundCap));
    painter->drawRect(QRect(0 - m_def->dieAria.first / 2.0, 0 - m_def->dieAria.second / 2.0, m_def->dieAria.first, m_def->dieAria.second));

    painter->end();
}

void DrawWidget::wheelEvent(QWheelEvent* t_event)
{
    if (QGuiApplication::queryKeyboardModifiers().testFlag(Qt::ControlModifier)) {
        double sigmoid = 1.0 / (1 + std::pow(2.718281282846, -1 * m_scale.scroll));

        if ((sigmoid > 0.5 || t_event->angleDelta().y() > 0) && (sigmoid < 0.995 || t_event->angleDelta().y() < 0)) {
            m_scale.scroll += 0.1 * (t_event->angleDelta().y() > 0 ? 1 : -1);
            m_scale.current = m_scale.initial / (1.0 / (std::pow(2.718281282846, 1 * m_scale.scroll)));
        }

        update();
    }
}

void DrawWidget::resizeEvent(QResizeEvent* t_event)
{
    Q_UNUSED(t_event);

    double newInitial = std::min((width() * __BORDERS__) / m_def->dieAria.first, (height() * __BORDERS__) / m_def->dieAria.second);

    m_scale.current = m_scale.current / m_scale.initial * newInitial;
    m_scale.initial = newInitial;
}
