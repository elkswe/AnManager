#ifndef MANAGERWINDOW_H
#define MANAGERWINDOW_H

#include <QMainWindow>
#include <fileinfo.h>
#include <DiskButton.h>

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
    path curr_path;
    QVector<DiskButton*> sections;

    //My methods
    void configureTableView();
    void mountSections();
    void addSection(path _path, QString _name = NULL);
};

#endif // MANAGERWINDOW_H
