#include <QApplication>

#include "include/DrawWidget.hpp"
#include "reader.hpp"

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);

    Reader reader("C:/Users/IlyaShafeev/Documents/GitHub/s27.def");
    DrawWidget widget(&reader.def);

    widget.show();

    return app.exec();
}