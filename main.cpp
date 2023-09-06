#include <QApplication>

#include "include/DrawWidget.hpp"
#include "reader.hpp"

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);

    Reader reader("F:/circuits/s27/RUN_2023.05.31_11.46.48/results/routing/s27.def");
    DrawWidget widget(&reader.def);

    widget.show();

    return app.exec();
}