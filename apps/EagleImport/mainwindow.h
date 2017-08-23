#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QtCore>
#include <QtWidgets>
#include <librepcb/common/uuid.h>
#include <librepcb/common/fileio/filepath.h>
#include <librepcb/common/fileio/domelement.h>

namespace Ui {
class MainWindow;
}

namespace librepcb {

class MainWindow : public QMainWindow
{
        Q_OBJECT

    public:

        explicit MainWindow(QWidget *parent = 0);
        ~MainWindow();

    private slots:

        void on_inputBtn_clicked();
        void on_outputBtn_clicked();
        void on_btnAbort_clicked();
        void on_btnConvertSymbols_clicked();
        void on_btnConvertDevices_clicked();
        void on_pushButton_2_clicked();
        void on_btnPathsFromIni_clicked();
        void on_toolButton_clicked();
        void on_toolButton_2_clicked();

        void on_toolButton_3_clicked();

        void on_toolButton_4_clicked();

    private:

        enum class ConvertFileType_t {
            Symbols_to_Symbols,
            Packages_to_PackagesAndDevices,
            Devices_to_Components
        };

        void reset();
        void addError(const QString& msg, const librepcb::FilePath& inputFile = librepcb::FilePath(), int inputLine = 0);
        librepcb::Uuid getOrCreateUuid(QSettings& outputSettings,
                                       const librepcb::FilePath& filepath,
                                       const QString& cat, const QString& key1,
                                       const QString& key2 = QString());
        QString createDescription(const librepcb::FilePath& filepath, const QString& name);
        QString convertSchematicLayer(int eagleLayerId);
        QString convertBoardLayer(int eagleLayerId);
        void convertAllFiles(ConvertFileType_t type);
        void convertFile(ConvertFileType_t type, QSettings& outputSettings,
                         const librepcb::FilePath& filepath);
        bool convertSymbol(QSettings& outputSettings, const librepcb::FilePath& filepath,
                           librepcb::DomElement* node);
        bool convertPackage(QSettings& outputSettings, const librepcb::FilePath& filepath,
                            librepcb::DomElement* node);
        bool convertDevice(QSettings& outputSettings, const librepcb::FilePath& filepath,
                           librepcb::DomElement* node);

        // Attributes
        Ui::MainWindow *ui;
        bool mAbortConversion;
        QString mlastInputDirectory;
        int mReadedElementsCount;
        int mConvertedElementsCount;
};

}

#endif // MAINWINDOW_H
