#include <QFrame>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWidget>

#include "Include/StagesWidget.hpp"

namespace gui::stages
{

Widget::Widget(QWidget* parent)
    : QWidget(parent)
{
  const int32_t max_label_width             = get_max_label_width({ "Apply Global Routing", "Make Tasks", "Encode", "Route", "Extract" });

  QPushButton*  apply_global_routing_button = add_stage("Apply Global Routing", max_label_width, [this]() { emit change_stage(Stages::APPLY_GLOBAL_ROUTING); });
  QPushButton*  make_tasks_button           = add_stage("Make Tasks", max_label_width, [this]() { emit change_stage(Stages::MAKE_TASKS); });
  QPushButton*  encode_button               = add_stage("Encode", max_label_width, [this]() { emit change_stage(Stages::ENCODE); });
  QPushButton*  route_button                = add_stage("Route", max_label_width, [this]() { emit change_stage(Stages::ROUTE); });
  QPushButton*  extract_button              = add_stage("Extract", max_label_width, [this]() { emit change_stage(Stages::EXTRACT); });

  QVBoxLayout*  layout                      = new QVBoxLayout(this);
  layout->setSpacing(0);
  layout->setContentsMargins(0, 0, 0, 0);

  layout->setAlignment(Qt::AlignTop);

  layout->addWidget(apply_global_routing_button);
  layout->addWidget(make_tasks_button);
  layout->addWidget(encode_button);
  layout->addWidget(route_button);
  layout->addWidget(extract_button);

  setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
  setFixedWidth(200);
}

QPushButton*
Widget::add_stage(const QString& label_text, const int32_t label_width, std::function<void()> callback)
{
  static QIcon play_icon(":Icons/play.png");

  QPushButton* button = new QPushButton();
  button->setFixedHeight(40);
  button->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

  button->setIcon(play_icon);
  button->setText(label_text);
  button->setIconSize(QSize(16, 16));
  button->setStyleSheet("text-align: left; padding-left: 10px;");

  QObject::connect(button, &QPushButton::clicked, callback);

  return button;
}

int32_t
Widget::get_max_label_width(const QStringList& labels)
{
  int32_t            maxWidth = 0;
  const QFontMetrics metrics(font());

  for(const QString& label : labels)
    {
      maxWidth = qMax(maxWidth, metrics.horizontalAdvance(label));
    }

  return maxWidth + 10;
}

} // namespace gui