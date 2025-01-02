#ifndef __INFORMATION_HPP__
#define __INFORMATION_HPP__

#include <QVBoxLayout>
#include <QWidget>

#include "Include/DEF.hpp"

namespace gui::information
{

class Widget : public QWidget
{
  Q_OBJECT

public:
  Widget(QWidget* parent = nullptr);

public slots:
  void
  recv_design(def::Data const* data);

  void
  recv_examine(def::GCell const* gcell);

signals:
  void
  send_metal_checked(const types::Metal metal, const bool status);

  void
  send_net_checked(const std::string& name, const bool status);

  void
  send_track_checked(const types::Metal metal, const char direction, const bool status);

private:
  QWidget* m_info_widget;
};

} // namespace gui

#endif