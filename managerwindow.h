#ifndef MANAGERWINDOW_H
#define MANAGERWINDOW_H

#include <QMainWindow>
#include <QMessageBox>
#include <FileInfo.h>
#include <DiskButton.h>
#include <Utils.h>

namespace fs  = boost::filesystem;

#define DEBUG

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
    void addSection(const path _path, QString _name = NULL);
    bool fillWithData(const path & _path);
    void readThePath(path & _path);
private slots:
    void disk_button_clicked_handler(const path & path);
    void mainTableView_dbClicked_handler(const QModelIndex & index);
};

#endif // MANAGERWINDOW_H
