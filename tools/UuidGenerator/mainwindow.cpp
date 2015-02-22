#include <QtCore>
#include <QClipboard>
#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    QSettings s;
    restoreGeometry(s.value("geometry").toByteArray());
}

MainWindow::~MainWindow()
{
    QSettings s;
    s.setValue("geometry", saveGeometry());
    delete ui;
}

void MainWindow::on_pushButton_clicked()
{
    QUuid uuid = QUuid::createUuid();
    ui->lineEdit->setText(uuid.toString());
    QApplication::clipboard()->setText(uuid.toString());
}
