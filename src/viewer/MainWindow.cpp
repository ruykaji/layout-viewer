#include <QFileDialog>
#include <QMenuBar>

#include "viewer/MainWindow.hpp"

MainWindow::MainWindow(QWidget* t_parent, Qt::WindowFlags t_flags)
    : QMainWindow(t_parent, t_flags)
{
    m_viewerWidget = new ViewerWidget();

    connect(this, &MainWindow::render, m_viewerWidget, &ViewerWidget::render);

    setCentralWidget(m_viewerWidget);

    createActions();
    createMenus();

    setWindowTitle("Viewer");
    setMinimumSize(480, 480);
    resize(480, 480);
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