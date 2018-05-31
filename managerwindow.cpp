#include "managerwindow.h"
#include "ui_managerwindow.h"

ManagerWindow::ManagerWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::ManagerWindow)
{
    ui->setupUi(this);

    this->setWindowTitle("AnManager v1.0");
    configureTableView();



}

ManagerWindow::~ManagerWindow()
{
    delete ui;
}

void ManagerWindow::configureTableView()
{
    QTableView* m_view = ui->mainTableView;
    m_view->verticalHeader()->setVisible(false);
    m_view->horizontalHeader()->setStretchLastSection(true);
    m_view->resizeColumnsToContents();
    m_view->setModel(m_model = new FileInfo);
    m_view->setShowGrid(true);
    m_view->setSelectionMode(QAbstractItemView::SingleSelection); //maybe multiple
    m_view->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->mainTableView->resizeColumnsToContents();
    this->resize(750, 750);
}
