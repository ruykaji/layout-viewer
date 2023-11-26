#include <QDir>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QMenuBar>
#include <QStringList>
#include <QVBoxLayout>

#include "viewer/MainWindow.hpp"

MainWindow::MainWindow(QWidget* t_parent, Qt::WindowFlags t_flags)
    : QMainWindow(t_parent, t_flags)
{
    m_viewerWidget = new ViewerWidget();
    m_controlPanelWidget = new ControlPanelWidget();

    m_listWidget = new QListWidget();
    m_listWidget->setFixedWidth(150);
    m_listWidget->hide();

    connect(this, &MainWindow::render, m_viewerWidget, &ViewerWidget::render);
    connect(m_listWidget, &QListWidget::itemClicked, [this](QListWidgetItem* item) { QString fileName = this->m_fileName + "/" + item->text(); this->m_viewerWidget->render(fileName); });
    connect(m_controlPanelWidget, &ControlPanelWidget::setDisplayMode, m_viewerWidget, &ViewerWidget::setDisplayMode);
    connect(m_controlPanelWidget, &ControlPanelWidget::setDisplayMode, this, &MainWindow::showFiles);

    QVBoxLayout* vbox = new QVBoxLayout();
    QHBoxLayout* hbox = new QHBoxLayout();

    hbox->addWidget(m_viewerWidget);
    hbox->addWidget(m_listWidget);
    hbox->setSpacing(0);
    hbox->setContentsMargins(0, 0, 0, 0);

    vbox->addWidget(m_controlPanelWidget);
    vbox->addLayout(hbox);
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
        int32_t lastSlashPos = fileName.lastIndexOf('/');
        int32_t lastDotPos = fileName.lastIndexOf('.');

        if (lastSlashPos != -1 && lastDotPos != -1 && lastDotPos > lastSlashPos) {
            m_fileName = fileName.mid(lastSlashPos + 1, lastDotPos - lastSlashPos - 1);
        } else {
            m_fileName = fileName;
        }

        render(fileName);
    }
}

void MainWindow::showFiles(const int8_t& t_mode)
{
    m_listWidget->clear();

    if (t_mode == 1) {
        QDir directory("./cache/" + m_fileName);
        QStringList files = directory.entryList(QDir::Files);

        m_listWidget->show();

        for (auto& file : files) {
            if (file.contains("source")) {
                m_listWidget->addItem(file);
            }
        }
    } else {
        m_listWidget->hide();
    }
}