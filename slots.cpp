#include <managerwindow.h>

void ManagerWindow::disk_button_clicked_handler(const path &_path)
{
    this->curr_path = _path;
    fillWithData(_path);

    //CHANGE
    ui->curr_path_label->setText(QString::fromStdWString(this->curr_path.wstring()));
    ui->curr_path_label->repaint();
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

void ManagerWindow::cmd_returnPressed_handler()
{
    boost::thread thr(&ManagerWindow::run_cmd, this);
}

void ManagerWindow::run_cmd()
{
#ifdef _WIN32 | _ WIN64
    ui->cmd->selectAll();
    QString cmd('\"');
    cmd += this->curr_path.root_name().string().c_str();
    cmd += " &&";
    cmd += " cd ";
    cmd += this->curr_path.string().c_str();
    if (!ui->cmd->text().isEmpty())
    {
        cmd += " && ";
        cmd += ui->cmd->text();
    }
    cmd += '\"';
    system(cmd.toLocal8Bit());
#elif __linux__
    //ADD
#endif
}


void ManagerWindow::slotCustomMenuRequested(QPoint pos)
{
    QMenu *menu = new QMenu(this);

    QAction *renameFile = new QAction("Rename", this);
    QAction *copyFile = new QAction("Copy", this);
    QAction *cutFile = new QAction("Cut", this);
    QAction *pasteFile = new QAction("Paste", this);
    QAction *deleteFile = new QAction("Delete", this);
    QAction *newSmth = new QAction("New", this);
    //New sub menu
    QMenu *newSubMenu = new QMenu(menu);
    QAction *createDir = new QAction("Folder", this);
    QAction *createTxtFile = new QAction("Text document (.txt)", this);
    QAction *createDocxFile = new QAction("Microsoft Word Document (.docx)", this);

    connect(renameFile, SIGNAL(triggered()),
        this, SLOT(slotRenameFile()));
    connect(copyFile, SIGNAL(triggered()),
        this, SLOT(slotCopyFile()));
    connect(cutFile, SIGNAL(triggered()),
        this, SLOT(slotCutFile()));
    connect(pasteFile, SIGNAL(triggered()),
        this, SLOT(slotPasteFile()));
    connect(deleteFile, SIGNAL(triggered()),
        this, SLOT(slotDeleteFile()));

    //New sub menu
    newSubMenu->addAction(createDir);
    newSubMenu->addAction(createTxtFile);
    newSubMenu->addAction(createDocxFile);

    connect(createDir, SIGNAL(triggered()),
        this, SLOT(slotCreateDir()));
    connect(createTxtFile, SIGNAL(triggered()),
        this, SLOT(slotCreateTxtFile()));
    connect(createDocxFile, SIGNAL(triggered()),
        this, SLOT(slotCreateDocxFile()));

    newSmth->setMenu(newSubMenu);

    menu->addAction(renameFile);
    menu->addAction(copyFile);
    menu->addAction(cutFile);
    menu->addAction(pasteFile);
    menu->addAction(deleteFile);
    menu->addSeparator();
    menu->addAction(newSmth);

    menu->popup(ui->mainTableView->viewport()->mapToGlobal(pos));
}

void ManagerWindow::slotRenameFile()
{
    QModelIndex index = ui->mainTableView->selectionModel()->currentIndex();
    if (!index.isValid()) return;
    QModelIndex ind = ui->mainTableView->model()->index(index.row(), 0);
    path old_path(curr_path /
        ind.data(Qt::DisplayRole).toString().toStdWString());
    if(old_path.filename_is_dot_dot()) return;

    bool bOK;
    QString new_name = QInputDialog::getText(
        0, "Renaming",
        "New name:", QLineEdit::Normal,
        QString::fromStdWString(old_path.filename().wstring()),
        &bOK);

    if (bOK)
    {
        path new_path(curr_path /
            new_name.toStdWString());
        boost::system::error_code ec;
        fs::rename(old_path, new_path, ec);
        if (ec.value() == boost::system::errc::success)
            m_model->setData(ind, QString::fromStdWString(new_path.filename().wstring()), Qt::EditRole);
        else
            utils::showErrorCode(old_path, ec);
    }
}

void ManagerWindow::slotCopyFile()
{
    QModelIndex index = ui->mainTableView->selectionModel()->currentIndex();
    if (!index.isValid()) return;
    QModelIndex ind = ui->mainTableView->model()->index(index.row(), 0);
    path path(curr_path /
        ind.data(Qt::DisplayRole).toString().toStdWString());

    if(path.filename_is_dot_dot()) return;

    fOps.from = path;
    fOps.op = COPY;
}

void ManagerWindow::slotCutFile()
{
    QModelIndex index = ui->mainTableView->selectionModel()->currentIndex();
    if (!index.isValid()) return;
    QModelIndex ind = ui->mainTableView->model()->index(index.row(), 0);
    path path(curr_path /
        ind.data(Qt::DisplayRole).toString().toStdWString());

    if(path.filename_is_dot_dot()) return;

    fOps.from = path;
    fOps.op = CUT;
    fOps.index = index;
}

void ManagerWindow::slotPasteFile()
{
    boost::thread thr(boost::bind(&ManagerWindow::doPaste, this));
}

void ManagerWindow::doPaste()
{
    if (fOps.op == NULL || this->fOps.from.empty()) return;
    path to(curr_path / fOps.from.filename());

    boost::system::error_code ec;
    if (!exists(fOps.from, ec))
    {
        QMessageBox::information(0, "Ooops", "The file does not exist.");
        return;
    }

    if (ec.value() != boost::system::errc::success)
    {
        utils::showErrorCode(to, ec);
        return;
    }
    ec.clear();

    switch (fOps.op)
    {
    case COPY:
    {
        if(copyFromTo(fOps.from, to))
        {
            if(curr_path == to.parent_path())
            {
                last_write_time(to, pt::to_time_t(pt::second_clock::local_time()));
                FileInfo::FileData file = fillFileData(to);
                m_model->appendData(file);
                m_model->sort(
                            curr_path == curr_path.root_path()
                            ? 0
                            : 1);
            }
        }
        else
            QMessageBox::information(0, "Copying", "I can not do this :(");
        break;
    }
    case CUT:
    {
        if(copyFromTo(fOps.from, to))
        {
            if(curr_path == to.parent_path())
            {
                last_write_time(to, pt::to_time_t(pt::second_clock::local_time()));
                FileInfo::FileData file = fillFileData(to);
                m_model->appendData(file);
                m_model->sort(
                            curr_path == curr_path.root_path()
                            ? 0
                            : 1);
            }
            deleteFile(fOps.from, fOps.index, 0);
        }
        else
            QMessageBox::information(0, "Cutting", "I can not do this :(");

        fOps.from = path();
        fOps.index = QModelIndex();
        fOps.op = NULL;

        break;
    }
    default:
        break;
    }
}

void ManagerWindow::slotDeleteFile()
{
    QModelIndex index = ui->mainTableView->selectionModel()->currentIndex();
    if (!index.isValid()) return;

    QModelIndex ind = ui->mainTableView->model()->index(index.row(), 0);
    path path(curr_path /
        ind.data(Qt::DisplayRole).toString().toStdWString());

    if(path.filename_is_dot_dot()) return;

    boost::system::error_code ec;

    if (!exists(path, ec))
        QMessageBox::information(0, "Ooops", "The file does not exist.");

    if (ec.value() != boost::system::errc::success)
    {
        utils::showErrorCode(path, ec);
        return;
    }

    boost::thread thr(&ManagerWindow::deleteFile, this, path, index, 1);
}

void ManagerWindow::slotCreateDir()
{
    bool bOK;
    QString new_name = QInputDialog::getText(
        0, "New folder",
        "Name:", QLineEdit::Normal,
        "New folder",
        &bOK);

    if (bOK)
    {
        path dir_path(curr_path /
            new_name.toStdWString());
        boost::system::error_code ec;
        fs::create_directory(dir_path, ec);
        if (ec.value() == boost::system::errc::success)
        {
            last_write_time(dir_path, pt::to_time_t(pt::second_clock::local_time()));
            FileInfo::FileData file = fillFileData(dir_path);
            m_model->appendData(file);
            m_model->sort(
                curr_path == curr_path.root_path()
                ? 0
                : 1);
        }
        else
            utils::showErrorCode(dir_path, ec);
    }
}

void ManagerWindow::slotCreateTxtFile()
{
    createFileWithExtension(".txt");
}

void ManagerWindow::slotCreateDocxFile()
{
    createFileWithExtension(".docx");
}

void ManagerWindow::slotChngHiddenState(int state)
{
    fHidden = (state == Qt::Checked) ? true : false;
    fillWithData(curr_path);
}
