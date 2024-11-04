#include <QVBoxLayout>

#include "Include/MainWindow.hpp"

namespace gui
{

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
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

  {
    const ini::Config config      = ini::parse("./config.ini");

    const std::string pdk_folder  = config.at("PDK").get_as<std::string>("PATH");
    const std::string design_path = config.at("DESIGN").get_as<std::string>("PATH");

    const lef::LEF    lef;
    const lef::Data   lef_data = lef.parse(pdk_folder);

    const def::DEF    def;
    def::Data         def_data = def.parse(design_path);

    process::fill_gcells(def_data, lef_data);

    viewer::Data viewer_data;
    viewer_data.m_box = def_data.m_box;

    for(std::size_t y = 0, end_y = def_data.m_gcells.size(); y < end_y; ++y)
      {
        for(std::size_t x = 0, end_x = def_data.m_gcells[0].size(); x < end_x; ++x)
          {
            def::GCell* gcell = def_data.m_gcells[y][x];

            viewer_data.m_rects.emplace_back(gcell->m_box, types::Metal::NONE);

            for(const auto& obs : gcell->m_obstacles)
              {
                viewer_data.m_rects.emplace_back(obs);
              }

            for(const auto& [_, pin] : gcell->m_pins)
              {
                for(const auto& port : pin->m_ports)
                  {
                    viewer_data.m_rects.emplace_back(port.m_rect, port.m_metal);
                  }
              }
          }
      }

    emit send_viewer_data(viewer_data);
  }
}

MainWindow::~MainWindow() {};

} // namespace gui
