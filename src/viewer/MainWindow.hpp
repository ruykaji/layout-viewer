#ifndef __MAIN_WINDOW_H__
#define __MAIN_WINDOW_H__

#include <QListWidget>
#include <QMainWindow>
#include <QMenu>

#include "viewer/ControlPanelWidget.hpp"
#include "viewer/ViewerWidget.hpp"

class MainWindow : public QMainWindow {
    Q_OBJECT

    ViewerWidget* m_viewerWidget {};
    ControlPanelWidget* m_controlPanelWidget {};
    QListWidget* m_listWidget {};

    QMenu* m_fileMenu {};
    QAction* m_openAct {};

    QString m_fileName {};

public:
    explicit MainWindow(QWidget* parent = nullptr, Qt::WindowFlags flags = Qt::WindowFlags());

private:
    void createMenus();
    void createActions();

public Q_SLOTS:
    void open();
    void showFiles(const int8_t& t_mode);
Q_SIGNALS:
    void render(QString& t_fileName);
};

#endif