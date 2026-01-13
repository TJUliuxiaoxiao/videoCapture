#ifndef FFCAPWINDOW_H
#define FFCAPWINDOW_H

#include <QWidget>

namespace Ui {
class FFCapWindow;
}

class FFCapWindow : public QWidget
{
    Q_OBJECT

public:
    explicit FFCapWindow(QWidget *parent = nullptr);
    ~FFCapWindow();

private:
    Ui::FFCapWindow *ui;
};

#endif // FFCAPWINDOW_H
