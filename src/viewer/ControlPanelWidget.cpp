#include <QHBoxLayout>

#include "viewer/ControlPanelWidget.hpp"

ControlPanelWidget::ControlPanelWidget(QWidget* t_parent)
    : QMenuBar(t_parent)
{
    setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed));

    createActions();
    createMenu();
}

void ControlPanelWidget::createMenu()
{
    QMenu* m_displayMenu = new QMenu("&Display mode", this);

    m_displayMenu->addAction(m_displayScaled);
    m_displayMenu->addAction(m_displayUnScaled);
    m_displayMenu->addAction(m_displayTensor);

    addMenu(m_displayMenu);
}

void ControlPanelWidget::createActions()
{
    m_displayScaled = new QAction("&Scaled", this);
    m_displayScaled->setCheckable(true);
    m_displayScaled->setChecked(true);
    connect(m_displayScaled, &QAction::triggered, this, [this]() { m_displayUnScaled->setChecked(false); m_displayTensor->setChecked(false); this->setDisplayMode(0); });

    m_displayUnScaled = new QAction("&Unscaled", this);
    m_displayUnScaled->setCheckable(true);
    connect(m_displayUnScaled, &QAction::triggered, this, [this]() { m_displayScaled->setChecked(false); m_displayTensor->setChecked(false); this->setDisplayMode(1); });

    m_displayTensor = new QAction("&Tensor", this);
    m_displayTensor->setCheckable(true);
    connect(m_displayTensor, &QAction::triggered, this, [this]() { m_displayScaled->setChecked(false); m_displayUnScaled->setChecked(false); this->setDisplayMode(2); });
}