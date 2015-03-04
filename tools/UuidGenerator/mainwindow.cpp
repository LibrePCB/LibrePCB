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

    timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(on_timer_timeout()));

    //QUuid nullUuid = QUuid(0, 0, 16385, 128, 0, 0, 0, 0, 0, 0, 0);
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

void MainWindow::on_timer_timeout()
{
    if (QApplication::clipboard()->text() != ui->lineEdit->text())
    {
        ui->checkBox->setChecked(false);
        timer->stop();
        return;
    }

    on_pushButton_clicked();
}

void MainWindow::on_checkBox_toggled(bool checked)
{
    if (checked)
    {
        on_pushButton_clicked();
        timer->start(1000);
    }
    else
    {
        timer->stop();
    }
}
