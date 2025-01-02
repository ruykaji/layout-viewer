#include <QPainter>
#include <QPixmap>
#include <QStandardItem>
#include <QStandardItemModel>
#include <QTabWidget>
#include <QTreeView>

#include "Include/GUI/Information.hpp"
#include "Include/Utils.hpp"

namespace gui::information::details
{

QColor
get_metal_color(types::Metal metal)
{
  const auto [r, g, b] = utils::get_metal_color(metal);
  return QColor(r, g, b, 125);
}

void
clear_widget(QWidget* parent)
{
  if(QLayout* layout = parent->layout())
    {
      QLayoutItem* item;

      while((item = layout->takeAt(0)) != nullptr)
        {
          if(QWidget* widget = item->widget())
            {
              delete widget;
            }

          delete item;
        }

      delete layout;
    }

  for(QObject* child : parent->children())
    {
      if(QWidget* childWidget = qobject_cast<QWidget*>(child))
        {
          delete childWidget;
        }
    }
}

QIcon
create_icon_form_color(const QColor color)
{
  static int32_t size = 12;

  QPixmap        pixmap(size, size);
  pixmap.fill(Qt::transparent);

  QPainter painter(&pixmap);
  painter.setBrush(color);
  painter.setPen(Qt::NoPen);
  painter.drawRect(0, 0, size, size);
  painter.end();

  return QIcon(pixmap);
}

std::pair<QTreeView*, QStandardItemModel*>
create_metal_list()
{
  QStandardItemModel* standard_model = new QStandardItemModel();
  standard_model->setHorizontalHeaderLabels({ "Metal" });

  QStandardItem* root_item = standard_model->invisibleRootItem();

  for(uint8_t i = 1; i < uint8_t(types::Metal::SIZE); i += 2) /** +2 to skip via layers, i=1 to skip NONE */
    {
      const types::Metal metal      = types::Metal(i);
      const QString      metal_name = std::move(QString::fromStdString(utils::skywater130_metal_to_string(types::Metal(i))));

      if(metal_name == "NONE")
        {
          break;
        }

      QStandardItem* item  = new QStandardItem(std::move(metal_name));
      QColor         color = get_metal_color(metal);

      item->setIcon(create_icon_form_color(color));
      item->setCheckable(true);
      item->setCheckState(Qt::CheckState::Checked);
      root_item->appendRow(item);
    }

  QTreeView* tree_view = new QTreeView();
  tree_view->setModel(standard_model);
  tree_view->setIndentation(0);
  tree_view->setStyleSheet("QHeaderView::section {padding:5px;} QTreeView::item {margin-top: 5px;}");

  return std::make_pair(tree_view, standard_model);
}

std::pair<QTreeView*, QStandardItemModel*>
create_gcell_net_list(def::GCell const* gcell)
{
  QStandardItemModel* standard_model = new QStandardItemModel();
  standard_model->setHorizontalHeaderLabels({ "Nets" });

  QStandardItem* root_item = standard_model->invisibleRootItem();

  for(const auto& [name, _] : gcell->m_nets)
    {
      QStandardItem* item = new QStandardItem(QString::fromStdString(name));
      QColor         color{ QString::fromStdString(utils::get_color_from_string(name)) };

      item->setIcon(create_icon_form_color(color));
      item->setCheckable(true);
      item->setCheckState(Qt::CheckState::Checked);
      root_item->appendRow(item);
    }

  QTreeView* tree_view = new QTreeView();
  tree_view->setModel(standard_model);
  tree_view->setIndentation(0);
  tree_view->setStyleSheet("QHeaderView::section {padding:5px;} QTreeView::item {margin-top: 5px;}");

  return std::make_pair(tree_view, standard_model);
}

std::pair<QTreeView*, QStandardItemModel*>
create_gcell_tracks_list(def::GCell const* gcell)
{
  QStandardItemModel* standard_model = new QStandardItemModel();
  standard_model->setHorizontalHeaderLabels({ "Tracks" });

  QStandardItem* root_item = standard_model->invisibleRootItem();

  for(std::size_t i = 0, end = gcell->m_tracks_x.size(); i < end; ++i)
    {
      const types::Metal metal = gcell->m_tracks_x[i].m_metal;

      if(i % 2 == 0)
        {
          QStandardItem* item_x = new QStandardItem(std::move(QString::fromStdString(utils::skywater130_metal_to_string(metal)) + "_x"));
          QColor         color  = get_metal_color(metal);

          item_x->setIcon(create_icon_form_color(color));
          item_x->setCheckable(true);
          item_x->setCheckState(Qt::CheckState::Unchecked);
          root_item->appendRow(item_x);
        }

      if(i % 2 != 0 || metal == types::Metal::M1)
        {
          QStandardItem* item_y = new QStandardItem(std::move(QString::fromStdString(utils::skywater130_metal_to_string(metal)) + "_y"));
          QColor         color  = get_metal_color(metal);

          item_y->setIcon(create_icon_form_color(color));
          item_y->setCheckable(true);
          item_y->setCheckState(Qt::CheckState::Unchecked);
          root_item->appendRow(item_y);
        }
    }

  QTreeView* tree_view = new QTreeView();
  tree_view->setModel(standard_model);
  tree_view->setIndentation(0);
  tree_view->setStyleSheet("QHeaderView::section {padding:5px;} QTreeView::item {margin-top: 5px;}");

  return std::make_pair(tree_view, standard_model);
}

} // namespace gui::information::details

namespace gui::information
{

Widget::Widget(QWidget* parent)
    : QWidget(parent)
{
  m_info_widget            = new QWidget(this);

  QVBoxLayout* main_layout = new QVBoxLayout(this);
  main_layout->addWidget(m_info_widget);
  main_layout->setSpacing(0);
  main_layout->setContentsMargins(0, 24, 0, 0);

  setLayout(main_layout);
  setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
  setFixedWidth(200);
}

void
Widget::recv_design(def::Data const* data)
{
  details::clear_widget(m_info_widget);

  const auto [metal_tree_view, metal_standard_model] = details::create_metal_list();
  metal_tree_view->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);

  connect(metal_standard_model, &QStandardItemModel::itemChanged, [this](QStandardItem* item) {
    const types::Metal metal = utils::get_skywater130_metal(item->text().toStdString());
    emit               send_metal_checked(metal, item->checkState() == Qt::CheckState::Checked);
  });

  QVBoxLayout* vbox = new QVBoxLayout(m_info_widget);
  vbox->addWidget(metal_tree_view);
  vbox->setSpacing(0);
  vbox->setContentsMargins(0, 0, 0, 0);

  m_info_widget->setLayout(vbox);
  m_info_widget->update();
}

void
Widget::recv_examine(def::GCell const* gcell)
{
  details::clear_widget(m_info_widget);

  const auto [metal_tree_view, metal_standard_model] = details::create_metal_list();
  metal_tree_view->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);

  connect(metal_standard_model, &QStandardItemModel::itemChanged, [this](QStandardItem* item) {
    const types::Metal metal = utils::get_skywater130_metal(item->text().toStdString());
    emit               send_metal_checked(metal, item->checkState() == Qt::CheckState::Checked);
  });

  const auto [track_tree_view, track_standard_model] = details::create_gcell_tracks_list(gcell);
  track_tree_view->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);

  connect(track_standard_model, &QStandardItemModel::itemChanged, [this](QStandardItem* item) {
    const std::string  text      = item->text().toStdString();
    const std::size_t  separator = text.find_last_of('_');
    const types::Metal metal     = utils::get_skywater130_metal(text.substr(0, separator));
    const char         direction = text.back();

    emit               send_track_checked(metal, direction, item->checkState() == Qt::CheckState::Checked);
  });

  const auto [net_tree_view, net_standard_model] = details::create_gcell_net_list(gcell);

  connect(net_standard_model, &QStandardItemModel::itemChanged, [this](QStandardItem* item) {
    emit send_net_checked(item->text().toStdString(), item->checkState() == Qt::CheckState::Checked);
  });

  QVBoxLayout* vbox = new QVBoxLayout(m_info_widget);
  vbox->addWidget(metal_tree_view);
  vbox->addWidget(track_tree_view);
  vbox->addWidget(net_tree_view);
  vbox->setSpacing(0);
  vbox->setContentsMargins(0, 0, 0, 0);

  m_info_widget->setLayout(vbox);
  m_info_widget->update();
}

} // namespace gui