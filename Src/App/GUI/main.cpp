
#include <QApplication>

#include "Include/MainWindow.hpp"

int
main(int argc, char* argv[])
{
  QCoreApplication::setAttribute(Qt::AA_UseOpenGLES);
  QApplication    app(argc, argv);

  gui::MainWindow main_window;
  main_window.show();

  return app.exec();
}