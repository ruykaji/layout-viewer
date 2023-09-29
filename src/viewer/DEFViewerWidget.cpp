#include <QPainter>

#include "DEFViewerWidget.hpp"

DEFViewerWidget::DEFViewerWidget(QWidget* t_parent)
    : QWidget(t_parent)
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
};

void DEFViewerWidget::render(QString& t_fileName)
{
    m_def = m_defEncoder.read(std::string_view(t_fileName.toStdString()));
}

void DEFViewerWidget::paintEvent(QPaintEvent* t_event)
{
    Q_UNUSED(t_event);

    if (m_def != nullptr) {
        QPainter* painter = new QPainter();

        painter->begin(this);

        painter->setPen(QPen(QColor(Qt::black)));
        painter->setBrush(QBrush(QColor(Qt::transparent)));

        painter->drawRect(QRect(m_def->dieArea.left, m_def->dieArea.top, m_def->dieArea.width, m_def->dieArea.height));

        painter->end();
    }
}