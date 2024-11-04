
#include <QApplication>

#include "Include/MainWindow.hpp"

int
main(int argc, char* argv[])
{
  QApplication    app(argc, argv);

  gui::MainWindow main_window;
  main_window.show();

  return app.exec();
}