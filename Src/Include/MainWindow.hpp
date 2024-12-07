#ifndef __MAIN_WINDOW_HPP__
#define __MAIN_WINDOW_HPP__

#include <QMainWindow>

#include "Include/Ini.hpp"
#include "Include/Process.hpp"

#include "Include/ProjectSettingsWidget.hpp"
#include "Include/ViewerWidget.hpp"

namespace gui
{

class MainWindow : public QMainWindow
{
  Q_OBJECT

public:
  explicit MainWindow(QWidget* parent = nullptr);
  ~MainWindow();

signals:
  void
  send_viewer_data(const viewer::Data& data);

private:
  void
  create_menu();

private slots:
  void
  open_project();

  void
  create_project();

  void
  start_routing();

private:
  viewer::Widget* m_viewer_widget;
  ProjectSettings m_settings;
};

} // namespace gui

#endif