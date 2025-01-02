#include <QFileDialog>
#include <QHBoxLayout>
#include <QMenu>
#include <QMenuBar>
#include <QTabBar>
#include <QVBoxLayout>

#include "Include/GUI/CreateProject.hpp"
#include "Include/GUI/ExamineScene.hpp"
#include "Include/GUI/Information.hpp"
#include "Include/GUI/MainScene.hpp"
#include "Include/GUI/MainWindow.hpp"

namespace gui
{

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
  create_menu();

  main_scene::Widget*    main_scene_widget = new main_scene::Widget();
  examine_scene::Widget* examine_scene     = new examine_scene::Widget();

  QTabWidget*            tab               = new QTabWidget(this);
  tab->addTab(main_scene_widget, "Design");
  tab->addTab(examine_scene, "Examine");

  connect(this, &MainWindow::send_viewer_data, [main_scene_widget, tab](def::Data const* data) {
    tab->setCurrentWidget(main_scene_widget);
    main_scene_widget->send_viewer_data(data);
  });
  connect(main_scene_widget, &main_scene::Widget::send_examine, [examine_scene, tab](def::GCell const* gcell) {
    tab->setCurrentWidget(examine_scene);
    examine_scene->send_viewer_data(gcell);
  });

  QVBoxLayout* const vbox = new QVBoxLayout();
  vbox->addWidget(tab);
  vbox->setSpacing(0);
  vbox->setContentsMargins(0, 0, 0, 0);

  information::Widget* information_widget = new information::Widget();
  connect(this, &MainWindow::send_design, information_widget, &information::Widget::recv_design);
  connect(main_scene_widget, &main_scene::Widget::send_examine, information_widget, &information::Widget::recv_examine);

  /** Connection of information widget with main scene widget */
  connect(information_widget, &information::Widget::send_metal_checked, main_scene_widget, &main_scene::Widget::send_metal_checked);

  /** Connection of information widget with examine scene widget */
  connect(information_widget, &information::Widget::send_metal_checked, examine_scene, &examine_scene::Widget::send_metal_checked);
  connect(information_widget, &information::Widget::send_metal_checked, examine_scene, &examine_scene::Widget::send_metal_checked);
  connect(information_widget, &information::Widget::send_net_checked, examine_scene, &examine_scene::Widget::send_net_checked);
  connect(information_widget, &information::Widget::send_track_checked, examine_scene, &examine_scene::Widget::send_track_checked);

  QHBoxLayout* const hbox = new QHBoxLayout();
  hbox->addLayout(vbox);
  hbox->addWidget(information_widget);
  hbox->setSpacing(0);
  hbox->setContentsMargins(0, 0, 0, 0);

  QWidget* const central_widget = new QWidget(this);
  central_widget->setLayout(hbox);

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
  const def::DEF def;

  m_lef_data                               = new lef::Data(std::move(lef.parse(m_settings.m_pdk_folder)));
  m_def_data                               = new def::Data(std::move(def.parse(m_settings.m_def_file)));

  const std::vector<guide::Net> guide_nets = guide::read(m_settings.m_guide_file);
  process::apply_global_routing(*m_def_data, *m_lef_data, guide_nets);

  emit send_viewer_data(m_def_data);
  emit send_design(m_def_data);
}

void
MainWindow::encode()
{
  const std::vector<process::Task> tasks = process::encode(*m_def_data);
}

void
MainWindow::open_project()
{
  QString file_name = QFileDialog::getOpenFileName(this, "Select Project file", "", "Project files (*.proj)");

  if(!file_name.isEmpty())
    {
      m_settings.read_from(file_name.toStdString());
      apply_global_routing();
    }
}

void
MainWindow::create_project()
{
  create_project::Widget dialog(this);

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

      apply_global_routing();
    }
}

} // namespace gui
