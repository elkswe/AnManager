#include "managerwindow.h"
#include "ui_managerwindow.h"

ManagerWindow::ManagerWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::ManagerWindow)
{
    ui->setupUi(this);

    this->setWindowTitle("AnManager v1.0");
    mountSections();
    configureTableView();

    ui->hBoxForDisks->setAlignment(Qt::AlignLeft);
    this->resize(750, 750);
}

ManagerWindow::~ManagerWindow()
{
    delete ui;
    this->sections.clear();
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
    m_view->resizeColumnsToContents();
}

void ManagerWindow::mountSections()
{
#ifdef __linux__
    addSection(path(L"/"), "Root");
    char* username = getenv("USER");
    std::string home_dir("/home/");
    home_dir += username;
    addSection(path(home_dir), "Home dir");

    std::string mount_devices("/media/");
    mount_devices += username;
    path mount_devices_path(mount_devices);
    for(directory_entry d_entry
        :directory_iterator(mount_devices_path))
    {
        if(is_directory(d_entry.path()) &&
                exists(d_entry.path()))
        {
            addSection(d_entry.path(),
                       QString::fromStdWString(
                           d_entry.path().filename().wstring()));
        }
    }

    this->curr_path = this->sections.first()->diskPath;
#elif _WIN32 | _WIN64

#endif
}

void ManagerWindow::addSection(path _path, QString _name)
{
    if(_name == NULL) _name = QString::fromStdWString(_path.filename().wstring());
    DiskButton *d_btn = new DiskButton(_path, _name);
    //connect signal
    ui->hBoxForDisks->addWidget(d_btn, 0, Qt::AlignLeft);
    this->sections.push_back(d_btn);
}
