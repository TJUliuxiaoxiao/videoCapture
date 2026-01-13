#include "ffcapwindow.h"
#include "ui_ffcapwindow.h"

FFCapWindow::FFCapWindow(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::FFCapWindow)
{
    ui->setupUi(this);
}

FFCapWindow::~FFCapWindow()
{
    delete ui;
}
