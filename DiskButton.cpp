#include "DiskButton.h"

DiskButton::DiskButton(const path & path, QWidget * parent) : QPushButton(parent)
{
    this->diskPath = path;
    connect(this, SIGNAL(clicked()),
            this, SLOT(resendSignal()));
}

DiskButton::DiskButton(const path & path, const QString & text, QWidget * parent)
    :QPushButton(text, parent)
{
    this->diskPath = path;
    connect(this, SIGNAL(clicked()),
            this, SLOT(resendSignal()));
}

DiskButton::DiskButton(const path & path, const QIcon & icon, const QString & text, QWidget * parent)
    :QPushButton(icon, text, parent)
{
    this->diskPath = path;
    connect(this, SIGNAL(clicked()),
            this, SLOT(resendSignal()));
}

DiskButton::DiskButton(const path & path, QPushButtonPrivate & dd, QWidget * parent)
    :QPushButton(dd, parent)
{
    this->diskPath = path;
    connect(this, SIGNAL(clicked()),
            this, SLOT(resendSignal()));
}

DiskButton::~DiskButton()
{
}

void DiskButton::resendSignal()
{
    emit diskClicked(diskPath);
}
