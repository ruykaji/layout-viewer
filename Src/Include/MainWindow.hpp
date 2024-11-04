#ifndef __MAIN_WINDOW_HPP__
#define __MAIN_WINDOW_HPP__

#include <QMainWindow>

#include "Include/Ini.hpp"
#include "Include/Process.hpp"

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
  /** Widget */
  viewer::Widget* m_viewer_widget;
};

} // namespace gui

#endif