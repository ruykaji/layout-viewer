#include <QFileDialog>
#include <QMenu>
#include <QMenuBar>
#include <QVBoxLayout>

#include "Include/MainWindow.hpp"

namespace gui
{

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
  create_menu();

  m_viewer_widget = new viewer::Widget();

  connect(this, &MainWindow::send_viewer_data, m_viewer_widget, &viewer::Widget::set_viewer_data);

  QVBoxLayout* const vbox = new QVBoxLayout();
  vbox->addWidget(m_viewer_widget);
  vbox->setSpacing(0);
  vbox->setContentsMargins(0, 0, 0, 0);

  QWidget* const central_widget = new QWidget(this);
  central_widget->setLayout(vbox);

  setCentralWidget(central_widget);
  setWindowTitle("Viewer");
  setMinimumSize(1280, 1080);
  resize(1280, 1080);
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
  /** Process menu */
  QMenu*   process_menu  = menuBar()->addMenu("Process");

  QAction* start_routing = new QAction("Start routing");
  process_menu->addAction(start_routing);
  connect(start_routing, &QAction::triggered, this, &MainWindow::start_routing);
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
  ProjectSettingsWidget dialog(this);

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
MainWindow::start_routing()
{
  const lef::LEF                lef;
  const lef::Data               lef_data = lef.parse(m_settings.m_pdk_folder);

  const def::DEF                def;
  def::Data                     def_data   = def.parse(m_settings.m_def_file);

  const std::vector<guide::Net> guide_nets = guide::read(m_settings.m_guide_file);

  process::apply_global_routing(def_data, lef_data, guide_nets);

  viewer::Data viewer_data;
  viewer_data.m_box = def_data.m_box;

  for(std::size_t y = 0, end_y = def_data.m_gcells.size(); y < end_y; ++y)
    {
      for(std::size_t x = 0, end_x = def_data.m_gcells[y].size(); x < end_x; ++x)
        {
          def::GCell* gcell = def_data.m_gcells[y][x];

          viewer_data.m_rects.emplace_back(gcell->m_box, types::Metal::NONE);

          for(const auto& obs : gcell->m_obstacles)
            {
              viewer_data.m_rects.emplace_back(obs);
            }

          for(const auto& [_, pin] : gcell->m_pins)
            {
              for(const auto& [rect, metal] : pin->m_ports)
                {
                  viewer_data.m_rects.emplace_back(rect, metal);
                }
            }
        }
    }

  std::sort(viewer_data.m_rects.begin(), viewer_data.m_rects.end(), [](const auto& lhs, const auto& rhs) { return static_cast<uint8_t>(lhs.second) < static_cast<uint8_t>(rhs.second); });

  emit send_viewer_data(viewer_data);
}

} // namespace gui
