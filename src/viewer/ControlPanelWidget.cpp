#include <QHBoxLayout>

#include "viewer/ControlPanelWidget.hpp"

ControlPanelWidget::ControlPanelWidget(QWidget* t_parent)
    : QMenuBar(t_parent)
{
    setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred));

    createActions();
    createMenu();
}

void ControlPanelWidget::createMenu()
{
    QMenu* m_displayMenu = new QMenu("&Display mode", this);

    m_displayMenu->addAction(m_displayDefault);
    m_displayMenu->addAction(m_displayTensor);

    addMenu(m_displayMenu);
}

void ControlPanelWidget::createActions()
{
    m_displayDefault = new QAction("&Default", this);
    m_displayDefault->setCheckable(true);
    m_displayDefault->setChecked(true);
    connect(m_displayDefault, &QAction::triggered, this, [this]() { m_displayTensor->setChecked(false); this->setDisplayMode(0); });

    m_displayTensor = new QAction("&Tensor", this);
    m_displayTensor->setCheckable(true);
    connect(m_displayTensor, &QAction::triggered, this, [this]() { m_displayDefault->setChecked(false); this->setDisplayMode(1); });
}