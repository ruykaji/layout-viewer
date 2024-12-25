#include <QFileDialog>
#include <QHBoxLayout>
#include <QMenu>
#include <QMenuBar>
#include <QVBoxLayout>

#include "Include/MainWindow.hpp"

namespace gui
{

namespace details
{

QColor
get_metal_color(types::Metal metal)
{
  switch(metal)
    {
    case types::Metal::L1:
      {
        return QColor(0, 0, 255);
      }
    case types::Metal::L1M1_V:
      {
        return QColor(255, 0, 0, 50);
      }
    case types::Metal::M1:
      {
        return QColor(255, 0, 0);
      }
    case types::Metal::M2:
      {
        return QColor(0, 255, 0);
      }
    case types::Metal::M3:
      {
        return QColor(255, 255, 0);
      }
    case types::Metal::M4:
      {
        return QColor(0, 255, 255);
      }
    case types::Metal::M5:
      {
        return QColor(255, 0, 255);
      }
    case types::Metal::M6:
      {
        return QColor(125, 125, 255);
      }
    case types::Metal::M7:
      {
        return QColor(255, 125, 125);
      }
    case types::Metal::M8:
      {
        return QColor(125, 255, 125);
      }
    case types::Metal::M9:
      {
        return QColor(255, 75, 125);
      }
    default:
      {
        return QColor(255, 255, 255);
      }
    }
}

types::Metal
get_metal_type(const QColor& color)
{
  if(color == QColor(0, 0, 255))
    {
      return types::Metal::L1;
    }
  if(color == QColor(255, 0, 0, 50))
    {
      return types::Metal::L1M1_V;
    }
  if(color == QColor(255, 0, 0))
    {
      return types::Metal::M1;
    }
  if(color == QColor(0, 255, 0))
    {
      return types::Metal::M2;
    }
  if(color == QColor(255, 255, 0))
    {
      return types::Metal::M3;
    }
  if(color == QColor(0, 255, 255))
    {
      return types::Metal::M4;
    }
  if(color == QColor(255, 0, 255))
    {
      return types::Metal::M5;
    }
  if(color == QColor(125, 125, 255))
    {
      return types::Metal::M6;
    }
  if(color == QColor(255, 125, 125))
    {
      return types::Metal::M7;
    }
  if(color == QColor(125, 255, 125))
    {
      return types::Metal::M8;
    }
  if(color == QColor(255, 75, 125))
    {
      return types::Metal::M9;
    }

  return types::Metal::NONE;
}

} // namespace

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
  create_menu();

  m_stages_widget = new stages::Widget();
  connect(m_stages_widget, &stages::Widget::change_stage, this, &MainWindow::change_stage);

  m_viewer_widget = new viewer::Widget();
  connect(this, &MainWindow::send_viewer_data, m_viewer_widget, &viewer::Widget::set_viewer_data);

  QHBoxLayout* const hbox = new QHBoxLayout();
  hbox->addWidget(m_stages_widget);
  hbox->addWidget(m_viewer_widget);
  hbox->setSpacing(0);
  hbox->setContentsMargins(0, 0, 0, 0);

  QVBoxLayout* const vbox = new QVBoxLayout();
  vbox->addLayout(hbox);
  vbox->setSpacing(0);
  vbox->setContentsMargins(0, 0, 0, 0);

  QWidget* const central_widget = new QWidget(this);
  central_widget->setLayout(vbox);

  setCentralWidget(central_widget);
  setWindowTitle("Viewer");
  setMinimumSize(1620, 1080);
  resize(1620, 1080);
}

MainWindow::~MainWindow() {};

void
MainWindow::create_menu()
{
  /** File menu */
  QMenu*   file_menu          = menuBar()->addMenu("File");

  QAction* new_project_action = new QAction("New Project");
  file_menu->addAction(new_project_action);
  connect(new_project_action, &QAction::triggered, this, &MainWindow::create_project);

  QAction* open_project_action = new QAction("Open Project");
  file_menu->addAction(open_project_action);
  connect(open_project_action, &QAction::triggered, this, &MainWindow::open_project);
}

void
MainWindow::apply_global_routing()
{
  const lef::LEF lef;
  m_lef_data = lef.parse(m_settings.m_pdk_folder);

  const def::DEF def;
  m_def_data                               = def.parse(m_settings.m_def_file);

  const std::vector<guide::Net> guide_nets = guide::read(m_settings.m_guide_file);

  process::apply_global_routing(m_def_data, m_lef_data, guide_nets);

  /** Make view data */
  {
    viewer::Data viewer_data;
    viewer_data.m_box = m_def_data.m_box;

    for(std::size_t y = 0, end_y = m_def_data.m_gcells.size(); y < end_y; ++y)
      {
        for(std::size_t x = 0, end_x = m_def_data.m_gcells[y].size(); x < end_x; ++x)
          {
            def::GCell*         gcell = m_def_data.m_gcells[y][x];

            viewer::Data::Batch batch;
            batch.m_width  = gcell->m_box.m_points[1].x - gcell->m_box.m_points[0].x;
            batch.m_height = gcell->m_box.m_points[2].y - gcell->m_box.m_points[1].y;

            batch.m_rects.emplace_back(gcell->m_box, details::get_metal_color(types::Metal::NONE));

            for(const auto& obs : gcell->m_obstacles)
              {
                batch.m_rects.emplace_back(obs, details::get_metal_color(obs.m_metal));
              }

            for(const auto& [name, net] : gcell->m_nets)
              {
                for(const auto& pin : net)
                  {
                    for(const auto& port : pin->m_ports)
                      {
                        batch.m_rects.emplace_back(port, details::get_metal_color(port.m_metal));
                      }
                  }
              }

            std::sort(batch.m_rects.begin(), batch.m_rects.end(), [](const auto& lhs, const auto& rhs) {
              return static_cast<uint8_t>(details::get_metal_type(lhs.second)) < static_cast<uint8_t>(details::get_metal_type(rhs.second));
            });

            viewer_data.m_batches.emplace_back(std::move(batch));
          }
      }

    emit send_viewer_data(viewer_data);
  }
}

void
MainWindow::encode()
{
  const std::vector<process::Task> tasks = process::encode(m_def_data);

  /** Make view data */
  {
    viewer::Data viewer_data;
    viewer_data.m_box = { 0, 0, uint32_t(tasks.size() * 64), uint32_t(tasks.size() * 64) };

    // for(std::size_t y = 0, end_y = tasks.size(); y < end_y; ++y)
    //   {
    //     for(std::size_t x = 0, end_x = tasks[y].size(); x < end_x; ++x)
    //       {
    //         const auto&         task  = tasks[y][x];
    //         const auto&         shape = task.m_matrix.shape();

    //         viewer::Data::Batch batch;

    //         for(std::size_t i = 0, cols = shape.m_x; i < cols; ++i)
    //           {
    //             for(std::size_t j = 0, rows = shape.m_y; j < rows; ++j)
    //               {
    //                 uint16_t color = 0;

    //                 for(std::size_t k = 0, depth = shape.m_z; k < depth; ++k)
    //                   {
    //                     color += task.m_matrix.get_at(i, j, k);
    //                   }

    //                 double         i_x = task.m_idx_x * 64 + i;
    //                 double         j_y = task.m_idx_y * 64 + j;
    //                 types::Polygon rect{ i_x, j_y, i_x + 1, j_y, i_x + 1, j_y + 1, i_x, j_y + 1 };

    //                 batch.m_rects.emplace_back(std::move(rect), QColor(color * 10, 0, 0));
    //               }
    //           }

    //         viewer_data.m_batches.emplace_back(std::move(batch));
    //       }
    //   }

    emit send_viewer_data(viewer_data);
  }
}

void
MainWindow::route()
{
}

void
MainWindow::extract()
{
}

void
MainWindow::open_project()
{
  QString file_name = QFileDialog::getOpenFileName(this, "Select Project file", "", "Project files (*.proj)");

  if(!file_name.isEmpty())
    {
      m_settings.read_from(file_name.toStdString());
    }
}

void
MainWindow::create_project()
{
  project_settings::Widget dialog(this);

  if(dialog.exec() == QDialog::Accepted)
    {
      m_settings                = dialog.get_settings();

      const auto current_path   = std::filesystem::current_path();
      const auto project_folder = current_path / m_settings.m_name;
      const auto project_file   = project_folder / (m_settings.m_name + ".proj");

      if(!std::filesystem::exists(project_folder))
        {
          std::filesystem::create_directories(project_folder);
          m_settings.save_to(project_file);
        }
    }
}

void
MainWindow::change_stage(const stages::Stages stage)
{
  switch(stage)
    {
    case stages::Stages::APPLY_GLOBAL_ROUTING:
      {
        apply_global_routing();
        break;
      }
    case stages::Stages::ENCODE:
      {
        encode();
        break;
      }
    case stages::Stages::ROUTE:
      {
        route();
        break;
      }
    case stages::Stages::EXTRACT:
      {
        extract();
        break;
      }
    default:
      break;
    }
}

} // namespace gui
