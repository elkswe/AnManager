#ifndef MANAGERWINDOW_H
#define MANAGERWINDOW_H

#include <QMainWindow>
#include <fileinfo.h>

namespace Ui {
class ManagerWindow;
}

class ManagerWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit ManagerWindow(QWidget *parent = 0);
    ~ManagerWindow();

private:
    Ui::ManagerWindow *ui;

    //My data
    FileInfo* m_model;

    //My methods
    void configureTableView();
};

#endif // MANAGERWINDOW_H
