#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QtCore>
#include <QtWidgets>
#include <librepcb/common/fileio/filepath.h>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
        Q_OBJECT

    public:

        explicit MainWindow(QWidget *parent = 0);
        ~MainWindow();

    private slots:

        void on_libBtn_clicked();
        void on_addProjectBtn_clicked();
        void on_removeProjectBtn_clicked();
        void on_clrProjectBtn_clicked();
        void on_pushButton_2_clicked();
        void on_rescanlib_clicked();

    private:

        // Attributes
        Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H
