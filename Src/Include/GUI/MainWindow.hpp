#ifndef __MAIN_WINDOW_HPP__
#define __MAIN_WINDOW_HPP__

#include <QMainWindow>

#include "Include/GUI/ProjectSettings.hpp"
#include "Include/Process.hpp"

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
  encode();

signals:
  void
  send_viewer_data(def::Data const* data);

  void
  send_design(def::Data const* data);

private slots:
  void
  open_project();

  void
  create_project();

private:
  ProjectSettings m_settings;

  lef::Data*      m_lef_data;
  def::Data*      m_def_data;
};

} // namespace gui

#endif