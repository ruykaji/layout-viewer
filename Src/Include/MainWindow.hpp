#ifndef __MAIN_WINDOW_HPP__
#define __MAIN_WINDOW_HPP__

#include <QMainWindow>

#include "Include/Process.hpp"

#include "Include/ProjectSettingsWidget.hpp"
#include "Include/StagesWidget.hpp"
#include "Include/ViewerWidget.hpp"

namespace gui
{

class MainWindow : public QMainWindow
{
  Q_OBJECT

public:
  explicit MainWindow(QWidget* parent = nullptr);
  ~MainWindow();

private:
  void
  create_menu();

  void
  apply_global_routing();

  void
  make_tasks();

  void
  encode();

  void
  route();

  void
  extract();

signals:
  void
  send_viewer_data(const viewer::Data& data);

private slots:
  void
  open_project();

  void
  create_project();

  void
  change_stage(const stages::Stages stage);

private:
  viewer::Widget*                   m_viewer_widget;
  stages::Widget*                   m_stages_widget;

  project_settings::ProjectSettings m_settings;

  lef::Data                         m_lef_data;
  def::Data                         m_def_data;
};

} // namespace gui

#endif