#include "managerwindow.h"
#include "ui_managerwindow.h"

ManagerWindow::ManagerWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::ManagerWindow)
{
    ui->setupUi(this);

    this->setWindowTitle("AnManager v1.0");

    ui->hBoxForDisks->setAlignment(Qt::AlignLeft);
    this->resize(750, 750);

    configureTableView();
    mountSections();
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
//    m_view->horizontalHeader()->setCascadingSectionResizes(true);
    m_view->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
//    m_view->horizontalHeader()->setStretchLastSection(true);
    m_view->setModel(m_model = new FileInfo);
    m_view->setShowGrid(true);
    m_view->setSelectionMode(QAbstractItemView::SingleSelection); //maybe multiple
    m_view->setSelectionBehavior(QAbstractItemView::SelectRows);
    connect(m_view, SIGNAL(doubleClicked(QModelIndex)),
            this, SLOT(mainTableView_dbClicked_handler(QModelIndex)));
}

void ManagerWindow::mountSections()
{
#ifdef __linux__
    addSection(path(L"/"), "Root");
    char* username = getenv("USER");
    addSection(path("/home/") /= username, "Home dir");

    path mount_devices(L"/media/");
    mount_devices /= username;
    for(directory_entry d_entry
        :directory_iterator(mount_devices))
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
    fillWithData(curr_path);
#elif _WIN32 | _WIN64

#endif
}

void ManagerWindow::addSection(const path _path, QString _name)
{
    if(_name == NULL) _name = QString::fromStdWString(_path.filename().wstring());
    DiskButton *d_btn = new DiskButton(_path, _name);
    connect(d_btn, SIGNAL(diskClicked(const path&)),
            this, SLOT(disk_button_clicked_handler(const path&)));
    ui->hBoxForDisks->addWidget(d_btn, 0, Qt::AlignLeft);
    this->sections.push_back(d_btn);
}

bool ManagerWindow::fillWithData(const path &_path)
{
    boost::system::error_code ec;
    if(!exists(_path, ec)) return false;
    if(ec.value() != boost::system::errc::success)
        return false;

    FileInfo::Files new_files;

    for(directory_entry entry
        : directory_iterator(_path, ec))
    {
        try
        {
            path file_path = entry.path();
            FileInfo::FileData file;

            file[FileInfo::NAME] = QString::fromStdWString(
                        file_path.filename().wstring());

//            if(is_empty(file_path)) std::cout << file_path << " is empty\n";
//            if(is_other(file_path)) std::cout << file_path << " is other\n";
//            if(is_regular(file_path)) std::cout << file_path << " is regular\n";
//            if(is_regular_file(file_path)) std::cout << file_path << " is regular file\n";
//            if(is_symlink(file_path)) std::cout << file_path << " is symlink\n";
//            if(is_directory(file_path)) std::cout << file_path << " is directory\n";

            if(is_symlink(file_path) && symbolic_link_exists(file_path))
               file_path = utils::extractPathFromSymlink(file_path);

            file[FileInfo::SIZE] = utils::getSize(file_path);

            file[FileInfo::DATE] = utils::getLastWriteTime(file_path);

            new_files.append(file);
        }
        catch (filesystem_error & ex){
#ifdef DEBUG
            std::cout << ex.what() << std::endl
                      << "path1: " << ex.path1() << std::endl
                      << "path2: " << ex.path2() << std::endl;
            std::printf("perms: %o\n", status(ex.path1()).permissions());
            std::cout << ex.code().category().name() << ':'
                      << ex.code().value() << std::endl;
#endif
            QMessageBox::information(this, "Oooops",
                                     QString::fromStdString(ex.code().message()) + "\n" +
                                     QString::fromStdWString(ex.path1().wstring()));
        }
        catch (...)
        {
            QMessageBox::critical(this, "Magic", "So what did you do?");
        }
    }
    if(ec.value() == boost::system::errc::success)
    {
        m_model->resetData(new_files);
        m_model->sort();
    }
    else
    {
        utils::showErrorCode(_path, ec);
        return false;
    }

    if(_path != path("/"))
    {
        FileInfo::FileData to_past;
        to_past[FileInfo::NAME] = "..";
        to_past[FileInfo::SIZE] = "";
        to_past[FileInfo::DATE] = "";
        this->m_model->prependData(to_past);
    }

    return true;
}

void ManagerWindow::readThePath(path &_path)
{
    try
    {
        boost::system::error_code ec;
        if(!exists(_path, ec))
        {
            QMessageBox::information(this, "Error!",
                                     "The file does not exist.");
            return;
        }

        if(_path.filename() == L".." && _path.has_parent_path())
        {
            _path = _path.parent_path();
            if(exists(_path) && _path.has_parent_path())
            {
                path prev_dir = _path.parent_path();
                if(exists(prev_dir))
                {
                    if(fillWithData(prev_dir)) curr_path = prev_dir;
                    return;
                }
            }
        }

        if(is_regular_file(_path))
        {
#ifdef _WIN32 | _WIN64
            if (_path.extension() != L".lnk")
                ShellExecute(NULL, L"open", _path.wstring().c_str(), NULL, NULL, SW_SHOWNORMAL);
            else
            {
                LPCTSTR lpszShortcutPath = _path.wstring().c_str();
                TCHAR szFilePath[MAX_PATH];
                HRESULT hres = ResolveShortcut(lpszShortcutPath, szFilePath);
                if (hres == S_OK)
                    readThePath(path(szFilePath));
            }
#elif __linux__  //added
            QString command("xdg-open \"");
            command += QString::fromStdWString(_path.wstring());
            command += '\"';
            system(command.toStdString().c_str());
#endif
            return;
        }

        if(is_symlink(_path) && symbolic_link_exists(_path))
        {
            path sym_path = read_symlink(_path);
            readThePath(sym_path);
            return;
        }

        if(is_directory(_path))
        {
            if(fillWithData(_path)) curr_path = _path;
            return;
        }

        if(is_other(_path))
            QMessageBox::information(this, "Sorry",
                                     "This file can not be opened.");

    } catch (filesystem_error & ex){
#ifdef DEBUG
        std::cout << ex.what() << std::endl
                  << "path1: " << ex.path1() << std::endl
                  << "path2: " << ex.path2() << std::endl;
        std::printf("perms: %o\n", status(ex.path1()).permissions());
        std::cout << ex.code().category().name() << ':'
                  << ex.code().value() << std::endl;
#endif
        QMessageBox::information(this, "Oooops",
                                 QString::fromStdString(ex.code().message()) + "\n" +
                                 QString::fromStdWString(ex.path1().wstring()));
    }
}

//SLOTS
void ManagerWindow::disk_button_clicked_handler(const path &_path)
{
    this->curr_path = _path;
    fillWithData(_path);
}

void ManagerWindow::mainTableView_dbClicked_handler(const QModelIndex &index)
{
    if (!index.isValid()) return;

    QModelIndex ind = ui->mainTableView->model()->index(index.row(), 0);

    path new_path(curr_path /
                  ind.data(Qt::DisplayRole).toString().toStdWString());

    readThePath(new_path);

    //CHANGE
    ui->curr_path_label->setText(QString::fromStdWString(this->curr_path.wstring()));
    ui->curr_path_label->repaint();
}
