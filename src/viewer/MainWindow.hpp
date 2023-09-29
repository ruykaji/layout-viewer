#ifndef __MAIN_WINDOW_H__
#define __MAIN_WINDOW_H__

#include <QMainWindow>
#include <QMenu>

#include "DEFViewerWidget.hpp"

class MainWindow : public QMainWindow {
    Q_OBJECT

    DEFViewerWidget* m_defViewerWidget {};

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