#ifndef __MAIN_WINDOW_H__
#define __MAIN_WINDOW_H__

#include <QMainWindow>
#include <QMenu>

#include "viewer/ControlPanelWidget.hpp"
#include "viewer/ViewerWidget.hpp"

class MainWindow : public QMainWindow {
    Q_OBJECT

    ViewerWidget* m_viewerWidget {};
    ControlPanelWidget* m_controlPanelWidget {};

    QMenu* m_fileMenu {};
    QAction* m_openAct {};

public:
    explicit MainWindow(QWidget* parent = nullptr, Qt::WindowFlags flags = Qt::WindowFlags());

private:
    void createMenus();
    void createActions();

public slots:
    void open();
signals:
    void render(QString& t_fileName);
};

#endif