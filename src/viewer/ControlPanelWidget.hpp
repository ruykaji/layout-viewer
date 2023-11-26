#ifndef __CONTROL_PANEL_WIDGET_H__
#define __CONTROL_PANEL_WIDGET_H__

#include <QMenuBar>
#include <QWidget>

class ControlPanelWidget : public QMenuBar {
    Q_OBJECT

    QMenu* m_displayMenu {};
    QAction* m_displayDefault {};
    QAction* m_displayTensor {};

public:
    explicit ControlPanelWidget(QWidget* t_parent = nullptr);

private:
    void createMenu();
    void createActions();

public Q_SLOTS:
Q_SIGNALS:
    void setDisplayMode(const int8_t& t_mode);
};

#endif