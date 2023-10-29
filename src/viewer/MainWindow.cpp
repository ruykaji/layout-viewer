#include <QFileDialog>
#include <QMenuBar>
#include <QVBoxLayout>

#include "viewer/MainWindow.hpp"

MainWindow::MainWindow(QWidget* t_parent, Qt::WindowFlags t_flags)
    : QMainWindow(t_parent, t_flags)
{
    m_viewerWidget = new ViewerWidget();
    m_controlPanelWidget = new ControlPanelWidget();

    connect(this, &MainWindow::render, m_viewerWidget, &ViewerWidget::render);
    connect(m_controlPanelWidget, &ControlPanelWidget::setDisplayMode, m_viewerWidget, &ViewerWidget::setDisplayMode);

    QVBoxLayout* vbox = new QVBoxLayout();

    vbox->addWidget(m_controlPanelWidget);
    vbox->addWidget(m_viewerWidget);
    vbox->setSpacing(0);
    vbox->setContentsMargins(0, 0, 0, 0);

    QWidget* central = new QWidget();

    central->setLayout(vbox);

    setCentralWidget(central);

    createActions();
    createMenus();

    setWindowTitle("Viewer");
    setMinimumSize(1280, 1080);
    resize(1280, 1080);
}

void MainWindow::createMenus()
{
    m_fileMenu = menuBar()->addMenu("&File");
    m_fileMenu->addAction(m_openAct);
}

void MainWindow::createActions()
{
    m_openAct = new QAction("&Open", this);
    m_openAct->setShortcut(QKeySequence::Open);
    m_openAct->setStatusTip("Open a file.");
    connect(m_openAct, &QAction::triggered, this, &MainWindow::open);
}

void MainWindow::open()
{
    QString fileName = QFileDialog::getOpenFileName(this, "Open DEF", "/home", "(*.def)");

    if (!fileName.isEmpty()) {
        render(fileName);
    }
}