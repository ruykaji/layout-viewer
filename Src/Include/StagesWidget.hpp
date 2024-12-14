#ifndef __STAGES_WIDGET_HPP__
#define __STAGES_WIDGET_HPP__

#include <QVBoxLayout>
#include <QWidget>

namespace gui::stages
{

enum class Stages
{
  APPLY_GLOBAL_ROUTING = 0,
  ENCODE,
  ROUTE,
  EXTRACT
};

class Widget : public QWidget
{
  Q_OBJECT

public:
  Widget(QWidget* parent = nullptr);

private:
  QPushButton*
  add_stage(const QString& label_text, const int32_t label_width, std::function<void()> callback);

  int32_t
  get_max_label_width(const QStringList& labels);

signals:
  void
  change_stage(const Stages stage);
};

} // namespace gui

#endif