#include <QVBoxLayout>

#include "Include/MainWindow.hpp"

namespace gui
{

namespace details
{

void
line_to_left_point(types::Rectangle& rect, const double width)
{
  rect[2] = rect[0];
  rect[3] = rect[1];
  rect[4] = rect[0];
  rect[5] = rect[1];
  rect[6] = rect[0];
  rect[7] = rect[1];

  rect[0] -= width;
  rect[1] -= width;

  rect[2] += width;
  rect[3] -= width;

  rect[4] += width;
  rect[5] += width;

  rect[6] -= width;
  rect[7] += width;
}

void
line_to_right_point(types::Rectangle& rect, const double width)
{
  rect[0] = rect[4];
  rect[1] = rect[5];
  rect[2] = rect[4];
  rect[3] = rect[5];
  rect[6] = rect[4];
  rect[7] = rect[5];

  rect[0] -= width;
  rect[1] -= width;

  rect[2] += width;
  rect[3] -= width;

  rect[4] += width;
  rect[5] += width;

  rect[6] -= width;
  rect[7] += width;
}

} // namespace details

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
    const ini::Config             config      = ini::parse("./config.ini");

    const std::string             pdk_folder  = config.at("PDK").get_as<std::string>("PATH");
    const std::string             design_path = config.at("DESIGN").get_as<std::string>("PATH");
    const std::string             guide_path  = config.at("DESIGN").get_as<std::string>("GUIDE");

    const lef::LEF                lef;
    const lef::Data               lef_data = lef.parse(pdk_folder);

    const def::DEF                def;
    def::Data                     def_data   = def.parse(design_path);

    const std::vector<guide::Net> guide_nets = guide::read(guide_path);

    process::fill_gcells(def_data, lef_data, guide_nets);

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
                for(const auto& [rect, metal] : pin->m_ports)
                  {
                    viewer_data.m_rects.emplace_back(rect, metal);
                  }
              }

            for(const auto& track : gcell->m_tracks_x)
              {
                if(track.m_ln != 0)
                  {
                    types::Rectangle point = track.m_box;
                    details::line_to_left_point(point, lef_data.m_layers.at(track.m_metal).m_width / 2.0 * lef_data.m_database_number);
                    viewer_data.m_rects.emplace_back(point, track.m_metal);
                  }

                if(track.m_rn != 0)
                  {
                    types::Rectangle point = track.m_box;
                    details::line_to_right_point(point, lef_data.m_layers.at(track.m_metal).m_width / 2.0 * lef_data.m_database_number);
                    viewer_data.m_rects.emplace_back(point, track.m_metal);
                  }
              }

            for(const auto& track : gcell->m_tracks_y)
              {
                if(track.m_ln != 0)
                  {
                    types::Rectangle point = track.m_box;
                    details::line_to_left_point(point, lef_data.m_layers.at(track.m_metal).m_width / 2.0 * lef_data.m_database_number);
                    viewer_data.m_rects.emplace_back(point, track.m_metal);
                  }

                if(track.m_rn != 0)
                  {
                    types::Rectangle point = track.m_box;
                    details::line_to_right_point(point, lef_data.m_layers.at(track.m_metal).m_width / 2.0 * lef_data.m_database_number);
                    viewer_data.m_rects.emplace_back(point, track.m_metal);
                  }
              }
          }
      }

    std::sort(viewer_data.m_rects.begin(), viewer_data.m_rects.end(), [](const auto& lhs, const auto& rhs) { return static_cast<uint8_t>(lhs.second) < static_cast<uint8_t>(rhs.second); });

    emit send_viewer_data(viewer_data);
  }
}

MainWindow::~MainWindow() {};

} // namespace gui
