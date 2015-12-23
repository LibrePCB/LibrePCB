#include <QtCore>
#include <QtWidgets>
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <librepcbcommon/fileio/smartxmlfile.h>
#include <librepcbcommon/fileio/xmldomdocument.h>
#include <librepcbcommon/fileio/xmldomelement.h>
#include <librepcblibrary/sym/symbol.h>
#include <librepcblibrary/pkg/footprint.h>
#include <librepcblibrary/pkg/package.h>
#include <librepcblibrary/dev/device.h>
#include <librepcblibrary/cmp/component.h>
#include <librepcbcommon/boardlayer.h>
#include <librepcbcommon/schematiclayer.h>
#include "polygonsimplifier.h"

using namespace library;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->lblUuidList->setText(UUID_LIST_FILEPATH);

    QSettings s;
    restoreGeometry(s.value("mainwindow/geometry").toByteArray());
    restoreState(s.value("mainwindow/state").toByteArray());
    mlastInputDirectory = s.value("mainwindow/last_input_directory").toString();
    ui->edtDuplicateFolders->setText(s.value("mainwindow/last_library_directory").toString());
    ui->input->addItems(s.value("mainwindow/input").toStringList());
    ui->output->setText(s.value("mainwindow/output").toString());

    reset();
}

MainWindow::~MainWindow()
{
    QStringList inputList;
    for (int i = 0; i < ui->input->count(); i++)
        inputList.append(ui->input->item(i)->text());

    QSettings s;
    s.setValue("mainwindow/geometry", saveGeometry());
    s.setValue("mainwindow/state", saveState());
    s.setValue("mainwindow/last_input_directory", mlastInputDirectory);
    s.setValue("mainwindow/last_library_directory", ui->edtDuplicateFolders->text());
    s.setValue("mainwindow/input", QVariant::fromValue(inputList));
    s.setValue("mainwindow/output", ui->output->text());

    delete ui;
}

void MainWindow::reset()
{
    mAbortConversion = false;
    mReadedElementsCount = 0;
    mConvertedElementsCount = 0;

    ui->errors->clear();
    ui->pbarElements->setValue(0);
    ui->pbarElements->setMaximum(0);
    ui->pbarFiles->setValue(0);
    ui->pbarFiles->setMaximum(ui->input->count());
    ui->lblConvertedElements->setText("0 of 0");
}

void MainWindow::addError(const QString& msg, const FilePath& inputFile, int inputLine)
{
    ui->errors->addItem(QString("%1 (%2:%3)").arg(msg).arg(inputFile.toNative()).arg(inputLine));
}

Uuid MainWindow::getOrCreateUuid(QSettings& outputSettings, const FilePath& filepath,
                                  const QString& cat, const QString& key1, const QString& key2)
{
    QString allowedChars("_-.0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz");

    QString settingsKey = filepath.getFilename() % '_' % key1 % '_' % key2;
    settingsKey.replace("{", "");
    settingsKey.replace("}", "");
    settingsKey.replace(" ", "_");
    for (int i=0; i<settingsKey.length(); i++)
    {
        if (!allowedChars.contains(settingsKey[i]))
            settingsKey.replace(i, 1, QString("__U%1__").arg(QString::number(settingsKey[i].unicode(), 16).toUpper()));
    }
    settingsKey.prepend(cat % '/');

    Uuid uuid = Uuid::createRandom();
    QString value = outputSettings.value(settingsKey).toString();
    if (!value.isEmpty()) uuid = Uuid(value); //Uuid(QString("{%1}").arg(value));

    if (uuid.isNull())
    {
        addError("Invalid UUID in *.ini file: " % settingsKey, filepath);
        return Uuid::createRandom();
    }
    outputSettings.setValue(settingsKey, uuid.toStr());
    return uuid;
}

QString MainWindow::createDescription(const FilePath& filepath, const QString& name)
{
    return QString("\n\nThis element was automatically imported from Eagle\n"
                   "Filepath: %1\nName: %2\n"
                   "NOTE: Remove this text after manual rework!")
            .arg(filepath.getFilename(), name);
}

void MainWindow::convertAllFiles(ConvertFileType_t type)
{
    reset();

    // create output directory
    FilePath outputDir(ui->output->text());
    outputDir.mkPath();

    QSettings outputSettings(UUID_LIST_FILEPATH, QSettings::IniFormat);

    for (int i = 0; i < ui->input->count(); i++)
    {
        FilePath filepath(ui->input->item(i)->text());
        if (!filepath.isExistingFile())
        {
            addError("File not found: " % filepath.toNative());
            continue;
        }

        convertFile(type, outputSettings, filepath);
        ui->pbarFiles->setValue(i + 1);

        if (mAbortConversion)
            break;
    }
}

void MainWindow::convertFile(ConvertFileType_t type, QSettings& outputSettings, const FilePath& filepath)
{
    try
    {
        // Check input file and read XML content
        SmartXmlFile file(filepath, false, true);
        QSharedPointer<XmlDomDocument> doc = file.parseFileAndBuildDomTree(false);
        XmlDomElement* node = doc->getRoot().getFirstChild("drawing/library", true, true);

        switch (type)
        {
            case ConvertFileType_t::Symbols_to_Symbols:
                node = node->getFirstChild("symbols", true);
                break;
            case ConvertFileType_t::Packages_to_PackagesAndDevices:
                node = node->getFirstChild("packages", true);
                break;
            case ConvertFileType_t::Devices_to_Components:
                node = node->getFirstChild("devicesets", true);
                break;
            default:
                throw Exception(__FILE__, __LINE__);
        }

        ui->pbarElements->setValue(0);
        ui->pbarElements->setMaximum(node->getChildCount());

        // Convert Elements
        for (XmlDomElement* child = node->getFirstChild(); child; child = child->getNextSibling())
        {
            bool success;
            if (child->getName() == "symbol")
                success = convertSymbol(outputSettings, filepath, child);
            else if (child->getName() == "package")
                success = convertPackage(outputSettings, filepath, child);
            else if (child->getName() == "deviceset")
                success = convertDevice(outputSettings, filepath, child);
            else
                throw Exception(__FILE__, __LINE__, child->getName());

            mReadedElementsCount++;
            if (success) mConvertedElementsCount++;
            ui->pbarElements->setValue(ui->pbarElements->value() + 1);
            ui->lblConvertedElements->setText(QString("%1 of %2").arg(mConvertedElementsCount)
                                                                 .arg(mReadedElementsCount));
        }
    }
    catch (Exception& e)
    {
        addError(e.getUserMsg() % " [" % e.getDebugMsg() % "]");
        return;
    }
}

bool MainWindow::convertSymbol(QSettings& outputSettings, const FilePath& filepath, XmlDomElement* node)
{
    try
    {
        QString name = node->getAttribute<QString>("name", true);
        QString desc = createDescription(filepath, name);
        Uuid uuid = getOrCreateUuid(outputSettings, filepath, "symbols", name);
        bool rotate180 = false;
        if (filepath.getFilename() == "con-lsta.lbr" && name.startsWith("FE")) rotate180 = true;
        if (filepath.getFilename() == "con-lstb.lbr" && name.startsWith("MA")) rotate180 = true;

        // create symbol
        Symbol* symbol = new Symbol(uuid, Version("0.1"), "LibrePCB", name, desc);

        for (XmlDomElement* child = node->getFirstChild(); child; child = child->getNextSibling())
        {
            if (child->getName() == "wire")
            {
                SymbolPolygon* polygon = new SymbolPolygon();
                switch (child->getAttribute<uint>("layer", true))
                {
                    case 94: polygon->setLayerId(SchematicLayer::LayerID::SymbolOutlines); break;
                    case 95: polygon->setLayerId(SchematicLayer::LayerID::ComponentNames); break;
                    default: throw Exception(__FILE__, __LINE__, "Invalid layer: " % child->getAttribute<QString>("layer", false));
                }
                polygon->setIsFilled(false);
                polygon->setWidth(child->getAttribute<Length>("width", true));
                polygon->setIsGrabArea(true);
                Point startpos = Point(child->getAttribute<Length>("x1", true), child->getAttribute<Length>("y1", true));
                Point endpos = Point(child->getAttribute<Length>("x2", true), child->getAttribute<Length>("y2", true));
                Angle angle = child->hasAttribute("curve") ? child->getAttribute<Angle>("curve", true) : Angle(0);
                if (rotate180)
                {
                    startpos = Point(-startpos.getX(), -startpos.getY());
                    endpos = Point(-endpos.getX(), -endpos.getY());
                }
                polygon->setStartPos(startpos);
                polygon->appendSegment(new SymbolPolygonSegment(endpos, angle));
                symbol->addPolygon(polygon);
            }
            else if (child->getName() == "rectangle")
            {
                SymbolPolygon* polygon = new SymbolPolygon();
                switch (child->getAttribute<uint>("layer", true))
                {
                    case 94: polygon->setLayerId(SchematicLayer::LayerID::SymbolOutlines); break;
                    default: throw Exception(__FILE__, __LINE__, "Invalid layer: " % child->getAttribute<QString>("layer", false));
                }
                polygon->setIsFilled(true);
                if (child->hasAttribute("width"))
                    polygon->setWidth(child->getAttribute<Length>("width", true));
                polygon->setIsGrabArea(true);
                polygon->setStartPos(Point(child->getAttribute<Length>("x1", true), child->getAttribute<Length>("y1", true)));
                polygon->appendSegment(new SymbolPolygonSegment(Point(child->getAttribute<Length>("x2", true), child->getAttribute<Length>("y1", true))));
                polygon->appendSegment(new SymbolPolygonSegment(Point(child->getAttribute<Length>("x2", true), child->getAttribute<Length>("y2", true))));
                polygon->appendSegment(new SymbolPolygonSegment(Point(child->getAttribute<Length>("x1", true), child->getAttribute<Length>("y2", true))));
                polygon->appendSegment(new SymbolPolygonSegment(Point(child->getAttribute<Length>("x1", true), child->getAttribute<Length>("y1", true))));
                symbol->addPolygon(polygon);
            }
            else if (child->getName() == "polygon")
            {
                SymbolPolygon* polygon = new SymbolPolygon();
                switch (child->getAttribute<uint>("layer", true))
                {
                    case 94: polygon->setLayerId(SchematicLayer::LayerID::SymbolOutlines); break;
                    default: throw Exception(__FILE__, __LINE__, "Invalid layer: " % child->getAttribute<QString>("layer", false));
                }
                polygon->setIsFilled(false);
                if (child->hasAttribute("width"))
                    polygon->setWidth(child->getAttribute<Length>("width", true));
                polygon->setIsGrabArea(true);
                for (XmlDomElement* vertex = child->getFirstChild(); vertex; vertex = vertex->getNextSibling())
                {
                    Point p(vertex->getAttribute<Length>("x", true), vertex->getAttribute<Length>("y", true));
                    if (vertex == child->getFirstChild())
                        polygon->setStartPos(p);
                    else
                        polygon->appendSegment(new SymbolPolygonSegment(p));
                }
                polygon->appendSegment(new SymbolPolygonSegment(polygon->getStartPos()));
                symbol->addPolygon(polygon);
            }
            else if (child->getName() == "circle")
            {
                Length radius(child->getAttribute<Length>("radius", true));
                Point center(child->getAttribute<Length>("x", true), child->getAttribute<Length>("y", true));
                SymbolEllipse* ellipse = new SymbolEllipse();
                switch (child->getAttribute<uint>("layer", true))
                {
                    case 94: ellipse->setLayerId(SchematicLayer::LayerID::SymbolOutlines); break;
                    default: throw Exception(__FILE__, __LINE__, "Invalid layer: " % child->getAttribute<QString>("layer", false));
                }
                ellipse->setLineWidth(child->getAttribute<Length>("width", true));
                ellipse->setIsFilled(ellipse->getLineWidth() == 0);
                ellipse->setIsGrabArea(true);
                ellipse->setCenter(center);
                ellipse->setRadiusX(radius);
                ellipse->setRadiusY(radius);
                symbol->addEllipse(ellipse);
            }
            else if (child->getName() == "text")
            {
                SymbolText* text = new SymbolText();
                switch (child->getAttribute<uint>("layer", true))
                {
                    case 93: text->setLayerId(SchematicLayer::LayerID::SymbolPinNames); break;
                    case 94: text->setLayerId(SchematicLayer::LayerID::SymbolOutlines); break;
                    case 95: text->setLayerId(SchematicLayer::LayerID::ComponentNames); break;
                    case 96: text->setLayerId(SchematicLayer::LayerID::ComponentValues); break;
                    case 99: text->setLayerId(SchematicLayer::LayerID::OriginCrosses); break; // ???
                    default: throw Exception(__FILE__, __LINE__, "Invalid layer: " % child->getAttribute<QString>("layer", false));
                }
                QString textStr = child->getText<QString>(true);
                if (textStr == ">NAME")
                {
                    textStr = "${SYM::NAME}";
                    text->setHeight(Length::fromMm(3.175));
                }
                else if (textStr == ">VALUE")
                {
                    textStr = "${CMP::VALUE}";
                    text->setHeight(Length::fromMm(2.5));
                }
                else
                    text->setHeight(child->getAttribute<Length>("size", true)*2);
                text->setText(textStr);
                Point pos = Point(child->getAttribute<Length>("x", true), child->getAttribute<Length>("y", true));
                int angleDeg = 0;
                if (child->hasAttribute("rot")) angleDeg = child->getAttribute<QString>("rot", true).remove("R").toInt();
                if (rotate180)
                {
                    pos = Point(-pos.getX(), -pos.getY());
                    angleDeg += 180;
                }
                text->setPosition(pos);
                text->setRotation(Angle::fromDeg(angleDeg));
                text->setAlign(Alignment(HAlign::left(), VAlign::bottom()));
                symbol->addText(text);
            }
            else if (child->getName() == "pin")
            {
                Uuid pinUuid = getOrCreateUuid(outputSettings, filepath, "symbol_pins", uuid.toStr(), child->getAttribute<QString>("name", true));
                SymbolPin* pin = new SymbolPin(pinUuid, child->getAttribute<QString>("name", true));
                Point pos = Point(child->getAttribute<Length>("x", true), child->getAttribute<Length>("y", true));
                Length len(7620000);
                if (child->hasAttribute("length"))
                {
                    if (child->getAttribute<QString>("length", true) == "point")
                        len.setLengthNm(0);
                    else if (child->getAttribute<QString>("length", true) == "short")
                        len.setLengthNm(2540000);
                    else if (child->getAttribute<QString>("length", true) == "middle")
                        len.setLengthNm(5080000);
                    else if (child->getAttribute<QString>("length", true) == "long")
                        len.setLengthNm(7620000);
                    else
                        throw Exception(__FILE__, __LINE__, "Invalid symbol pin length: " % child->getAttribute<QString>("length", false));
                }
                int angleDeg = 0;
                if (child->hasAttribute("rot")) angleDeg = child->getAttribute<QString>("rot", true).remove("R").toInt();
                if (rotate180)
                {
                    pos = Point(-pos.getX(), -pos.getY());
                    angleDeg += 180;
                }
                pin->setPosition(pos);
                pin->setLength(len);
                pin->setRotation(Angle::fromDeg(angleDeg));
                symbol->addPin(pin);
            }
            else
            {
                addError(QString("Unknown node name: %1/%2").arg(node->getName()).arg(child->getName()), filepath);
                return false;
            }
        }

        // convert line rects to polygon rects
        PolygonSimplifier<Symbol, SymbolPolygon, SymbolPolygonSegment> polygonSimplifier(*symbol);
        polygonSimplifier.convertLineRectsToPolygonRects(false, true);

        // save symbol to file
        symbol->saveTo(FilePath(QString("%1/sym").arg(ui->output->text())));
        delete symbol;
    }
    catch (Exception& e)
    {
        addError(e.getUserMsg() % " [" % e.getDebugMsg() % "]");
        return false;
    }

    return true;
}

bool MainWindow::convertPackage(QSettings& outputSettings, const FilePath& filepath, XmlDomElement* node)
{
    try
    {
        QString name = node->getAttribute<QString>("name", true);
        QString desc = node->getFirstChild("description", false) ? node->getFirstChild("description", true)->getText<QString>(false) : "";
        desc.append(createDescription(filepath, name));
        bool rotate180 = false;
        //if (filepath.getFilename() == "con-lsta.lbr" && name.startsWith("FE")) rotate180 = true;
        //if (filepath.getFilename() == "con-lstb.lbr" && name.startsWith("MA")) rotate180 = true;

        // create package
        Uuid pkgUuid = getOrCreateUuid(outputSettings, filepath, "packages_to_packages", name);
        Package* package = new Package(pkgUuid, Version("0.1"), "LibrePCB", name, desc);

        // create default footprint
        Uuid fptUuid = getOrCreateUuid(outputSettings, filepath, "packages_to_footprints", name);
        Footprint* footprint = new Footprint(fptUuid, "default", "", true);

        for (XmlDomElement* child = node->getFirstChild(); child; child = child->getNextSibling())
        {
            if (child->getName() == "description")
            {
                // nothing to do
            }
            else if (child->getName() == "wire")
            {
                FootprintPolygon* polygon = new FootprintPolygon();
                switch (child->getAttribute<uint>("layer", true))
                {
                    case 20: polygon->setLayerId(BoardLayer::LayerID::BoardOutline); break;
                    case 21: polygon->setLayerId(BoardLayer::LayerID::TopOverlay); break;
                    case 25: polygon->setLayerId(BoardLayer::LayerID::TopOverlayNames); break;
                    case 27: polygon->setLayerId(BoardLayer::LayerID::TopOverlayValues); break;
                    case 39: polygon->setLayerId(BoardLayer::LayerID::TopDeviceKeepout); break;
                    case 46: polygon->setLayerId(BoardLayer::LayerID::BoardOutline); break; // milling
                    case 48: polygon->setLayerId(BoardLayer::LayerID::TopDeviceOutlines); break; // document
                    case 49: polygon->setLayerId(BoardLayer::LayerID::OriginCrosses); break; // reference
                    case 51: polygon->setLayerId(BoardLayer::LayerID::TopDeviceOutlines); break;
                    case 52: polygon->setLayerId(BoardLayer::LayerID::BottomDeviceOutlines); break;
                    default: throw Exception(__FILE__, __LINE__, "Invalid layer: " % child->getAttribute<QString>("layer", false));
                }
                polygon->setIsFilled(false);
                polygon->setWidth(child->getAttribute<Length>("width", true));
                polygon->setIsGrabArea(true);
                Point startpos = Point(child->getAttribute<Length>("x1", true), child->getAttribute<Length>("y1", true));
                Point endpos = Point(child->getAttribute<Length>("x2", true), child->getAttribute<Length>("y2", true));
                Angle angle = child->hasAttribute("curve") ? child->getAttribute<Angle>("curve", true) : Angle(0);
                if (rotate180)
                {
                    startpos = Point(-startpos.getX(), -startpos.getY());
                    endpos = Point(-endpos.getX(), -endpos.getY());
                }
                polygon->setStartPos(startpos);
                polygon->appendSegment(new FootprintPolygonSegment(endpos, angle));
                footprint->addPolygon(polygon);
            }
            else if (child->getName() == "rectangle")
            {
                bool valid = true;
                FootprintPolygon* polygon = new FootprintPolygon();
                switch (child->getAttribute<uint>("layer", true))
                {
                    case 1: polygon->setLayerId(BoardLayer::LayerID::TopCopper); break;
                    case 16: polygon->setLayerId(BoardLayer::LayerID::BottomCopper); break;
                    case 21: polygon->setLayerId(BoardLayer::LayerID::TopOverlay); break;
                    case 29: polygon->setLayerId(BoardLayer::LayerID::TopStopMask); break;
                    case 31: polygon->setLayerId(BoardLayer::LayerID::TopPaste); break;
                    case 35: polygon->setLayerId(BoardLayer::LayerID::TopGlue); break;
                    case 39: polygon->setLayerId(BoardLayer::LayerID::TopDeviceKeepout); break;
                    case 41: polygon->setLayerId(BoardLayer::LayerID::TopCopperRestrict); break;
                    case 42: polygon->setLayerId(BoardLayer::LayerID::BottomCopperRestrict); break;
                    case 43: polygon->setLayerId(BoardLayer::LayerID::ViaRestrict); break;
                    case 51: polygon->setLayerId(BoardLayer::LayerID::TopDeviceOutlines); break;
                    case 52: polygon->setLayerId(BoardLayer::LayerID::BottomDeviceOutlines); break;
                    default: throw Exception(__FILE__, __LINE__, "Invalid layer: " % child->getAttribute<QString>("layer", false));
                }
                polygon->setIsFilled(true);
                if (child->hasAttribute("width"))
                    polygon->setWidth(child->getAttribute<Length>("width", true));
                polygon->setIsGrabArea(true);
                polygon->setStartPos(Point(child->getAttribute<Length>("x1", true), child->getAttribute<Length>("y1", true)));
                polygon->appendSegment(new FootprintPolygonSegment(Point(child->getAttribute<Length>("x2", true), child->getAttribute<Length>("y1", true))));
                polygon->appendSegment(new FootprintPolygonSegment(Point(child->getAttribute<Length>("x2", true), child->getAttribute<Length>("y2", true))));
                polygon->appendSegment(new FootprintPolygonSegment(Point(child->getAttribute<Length>("x1", true), child->getAttribute<Length>("y2", true))));
                polygon->appendSegment(new FootprintPolygonSegment(Point(child->getAttribute<Length>("x1", true), child->getAttribute<Length>("y1", true))));
                if (valid)
                    footprint->addPolygon(polygon);
                else
                    delete polygon;
            }
            else if (child->getName() == "polygon")
            {
                FootprintPolygon* polygon = new FootprintPolygon();
                switch (child->getAttribute<uint>("layer", true))
                {
                    case 1: polygon->setLayerId(BoardLayer::LayerID::TopCopper); break;
                    case 16: polygon->setLayerId(BoardLayer::LayerID::BottomCopper); break;
                    case 21: polygon->setLayerId(BoardLayer::LayerID::TopOverlay); break;
                    case 29: polygon->setLayerId(BoardLayer::LayerID::TopStopMask); break;
                    case 31: polygon->setLayerId(BoardLayer::LayerID::TopPaste); break;
                    case 51: polygon->setLayerId(BoardLayer::LayerID::TopDeviceOutlines); break;
                    default: throw Exception(__FILE__, __LINE__, "Invalid layer: " % child->getAttribute<QString>("layer", false));
                }
                polygon->setIsFilled(false);
                if (child->hasAttribute("width"))
                    polygon->setWidth(child->getAttribute<Length>("width", true));
                polygon->setIsGrabArea(true);
                for (XmlDomElement* vertex = child->getFirstChild(); vertex; vertex = vertex->getNextSibling())
                {
                    Point p(vertex->getAttribute<Length>("x", true), vertex->getAttribute<Length>("y", true));
                    if (vertex == child->getFirstChild())
                        polygon->setStartPos(p);
                    else
                        polygon->appendSegment(new FootprintPolygonSegment(p));
                }
                polygon->appendSegment(new FootprintPolygonSegment(polygon->getStartPos()));
                footprint->addPolygon(polygon);
            }
            else if (child->getName() == "circle")
            {
                bool valid = true;
                Length radius(child->getAttribute<Length>("radius", true));
                Point center(child->getAttribute<Length>("x", true), child->getAttribute<Length>("y", true));
                FootprintEllipse* ellipse = new FootprintEllipse();
                switch (child->getAttribute<uint>("layer", true))
                {
                    case 1: ellipse->setLayerId(BoardLayer::LayerID::TopCopper); break;
                    case 20: ellipse->setLayerId(BoardLayer::LayerID::BoardOutline); break;
                    case 21: ellipse->setLayerId(BoardLayer::LayerID::TopOverlay); break;
                    case 22: ellipse->setLayerId(BoardLayer::LayerID::BottomDeviceOutlines); break;
                    case 27: ellipse->setLayerId(BoardLayer::LayerID::TopOverlayValues); break;
                    case 29: ellipse->setLayerId(BoardLayer::LayerID::TopStopMask); break;
                    case 31: ellipse->setLayerId(BoardLayer::LayerID::TopPaste); break;
                    case 41: ellipse->setLayerId(BoardLayer::LayerID::TopCopperRestrict); break;
                    case 42: ellipse->setLayerId(BoardLayer::LayerID::BottomCopperRestrict); break;
                    case 43: ellipse->setLayerId(BoardLayer::LayerID::ViaRestrict); break;
                    case 49: ellipse->setLayerId(BoardLayer::LayerID::OriginCrosses); break; // reference
                    case 51: ellipse->setLayerId(BoardLayer::LayerID::TopDeviceOutlines); break;
                    case 52: ellipse->setLayerId(BoardLayer::LayerID::BottomDeviceOutlines); break;
                    default: throw Exception(__FILE__, __LINE__, "Invalid layer: " % child->getAttribute<QString>("layer", false));
                }
                ellipse->setLineWidth(child->getAttribute<Length>("width", true));
                ellipse->setIsFilled(ellipse->getLineWidth() == 0);
                ellipse->setIsGrabArea(true);
                ellipse->setCenter(center);
                ellipse->setRadiusX(radius);
                ellipse->setRadiusY(radius);
                if (valid)
                    footprint->addEllipse(ellipse);
                else
                    delete ellipse;
            }
            else if (child->getName() == "text")
            {
                FootprintText* text = new FootprintText();
                switch (child->getAttribute<uint>("layer", true))
                {
                    case 21: text->setLayerId(BoardLayer::LayerID::TopOverlay); break;
                    case 25: text->setLayerId(BoardLayer::LayerID::TopOverlayNames); break;
                    case 27: text->setLayerId(BoardLayer::LayerID::TopOverlayValues); break;
                    case 48: text->setLayerId(BoardLayer::LayerID::TopDeviceOutlines); break; // document
                    case 51: text->setLayerId(BoardLayer::LayerID::TopDeviceOutlines); break;
                    default: throw Exception(__FILE__, __LINE__, "Invalid layer: " % child->getAttribute<QString>("layer", false));
                }
                QString textStr = child->getText<QString>(true);
                if (textStr == ">NAME")
                {
                    textStr = "${CMP::NAME}";
                    text->setHeight(Length::fromMm(2.5));
                }
                else if (textStr == ">VALUE")
                {
                    textStr = "${CMP::VALUE}";
                    text->setHeight(Length::fromMm(2));
                }
                else
                    text->setHeight(child->getAttribute<Length>("size", true)*2);
                text->setText(textStr);
                Point pos = Point(child->getAttribute<Length>("x", true), child->getAttribute<Length>("y", true));
                int angleDeg = 0;
                if (child->hasAttribute("rot")) angleDeg = child->getAttribute<QString>("rot", true).remove("R").toInt();
                if (rotate180)
                {
                    pos = Point(-pos.getX(), -pos.getY());
                    angleDeg += 180;
                }
                text->setPosition(pos);
                text->setRotation(Angle::fromDeg(angleDeg));
                text->setAlign(Alignment(HAlign::left(), VAlign::bottom()));
                footprint->addText(text);
            }
            else if (child->getName() == "pad")
            {
                Uuid padUuid = getOrCreateUuid(outputSettings, filepath, "package_pads", fptUuid.toStr(), child->getAttribute<QString>("name", true));
                // add package pad
                PackagePad* pkgPad = new PackagePad(padUuid, child->getAttribute<QString>("name", true));
                package->addPad(*pkgPad);
                // add footprint pad
                FootprintPad* fptPad = new FootprintPad(padUuid);
                Length drill = child->getAttribute<Length>("drill", true);
                fptPad->setDrillDiameter(drill);
                Length diameter = child->hasAttribute("diameter") ? child->getAttribute<Length>("diameter", true) : drill * 2;
                int angleDeg = 0;
                if (child->hasAttribute("rot")) angleDeg = child->getAttribute<QString>("rot", true).remove("R").toInt();
                QString shape = child->hasAttribute("shape") ? child->getAttribute<QString>("shape", true) : "round";
                if (shape == "square")
                {
                    fptPad->setType(FootprintPad::Type_t::ThtRect);
                    fptPad->setWidth(diameter);
                    fptPad->setHeight(diameter);
                }
                else if (shape == "octagon")
                {
                    fptPad->setType(FootprintPad::Type_t::ThtOctagon);
                    fptPad->setWidth(diameter);
                    fptPad->setHeight(diameter);
                }
                else if (shape == "round")
                {
                    fptPad->setType(FootprintPad::Type_t::ThtRound);
                    fptPad->setWidth(diameter);
                    fptPad->setHeight(diameter);
                }
                else if (shape == "long")
                {
                    fptPad->setType(FootprintPad::Type_t::ThtRound);
                    fptPad->setWidth(diameter * 2);
                    fptPad->setHeight(diameter);
                }
                else
                {
                    throw Exception(__FILE__, __LINE__, "Invalid shape: " % shape % " :: " % filepath.toStr());
                }
                Point pos = Point(child->getAttribute<Length>("x", true), child->getAttribute<Length>("y", true));
                if (rotate180)
                {
                    pos = Point(-pos.getX(), -pos.getY());
                    angleDeg += 180;
                }
                fptPad->setPosition(pos);
                fptPad->setRotation(Angle::fromDeg(angleDeg));
                footprint->addPad(fptPad);
            }
            else if (child->getName() == "smd")
            {
                Uuid padUuid = getOrCreateUuid(outputSettings, filepath, "package_pads", fptUuid.toStr(), child->getAttribute<QString>("name", true));
                // add package pad
                PackagePad* pkgPad = new PackagePad(padUuid, child->getAttribute<QString>("name", true));
                package->addPad(*pkgPad);
                // add footprint pad
                FootprintPad* fptPad = new FootprintPad(padUuid);
                fptPad->setType(FootprintPad::Type_t::SmtRect);
                Point pos = Point(child->getAttribute<Length>("x", true), child->getAttribute<Length>("y", true));
                int angleDeg = 0;
                if (child->hasAttribute("rot")) angleDeg = child->getAttribute<QString>("rot", true).remove("R").toInt();
                if (rotate180)
                {
                    pos = Point(-pos.getX(), -pos.getY());
                    angleDeg += 180;
                }
                fptPad->setPosition(pos);
                fptPad->setRotation(Angle::fromDeg(angleDeg));
                fptPad->setWidth(child->getAttribute<Length>("dx", true));
                fptPad->setHeight(child->getAttribute<Length>("dy", true));
                switch (child->getAttribute<uint>("layer", true))
                {
                    case 1: fptPad->setLayerId(BoardLayer::LayerID::TopCopper); break;
                    case 16: fptPad->setLayerId(BoardLayer::LayerID::BottomCopper); break;
                    default: throw Exception(__FILE__, __LINE__, "Invalid layer: " % child->getAttribute<QString>("layer", false));
                }

                footprint->addPad(fptPad);
            }
            else if (child->getName() == "hole")
            {
                Footprint::FootprintHole_t* hole = new Footprint::FootprintHole_t();
                hole->pos = Point(child->getAttribute<Length>("x", true), child->getAttribute<Length>("y", true));
                hole->diameter = child->getAttribute<Length>("drill", true);
                footprint->addHole(hole);
            }
            else
            {
                addError(QString("Unknown node name: %1/%2").arg(node->getName()).arg(child->getName()), filepath);
                return false;
            }
        }

        // convert line rects to polygon rects
        PolygonSimplifier<Footprint, FootprintPolygon, FootprintPolygonSegment> polygonSimplifier(*footprint);
        polygonSimplifier.convertLineRectsToPolygonRects(false, true);

        // add footprint to package
        package->addFootprint(*footprint);

        // save package to file
        package->saveTo(FilePath(QString("%1/pkg").arg(ui->output->text())));

        // clean up
        delete package;
    }
    catch (Exception& e)
    {
        addError(e.getUserMsg() % " [" % e.getDebugMsg() % "]");
        return false;
    }

    return true;
}

bool MainWindow::convertDevice(QSettings& outputSettings, const FilePath& filepath, XmlDomElement* node)
{
    try
    {
        QString name = node->getAttribute<QString>("name", true);

        // abort if device name ends with "-US"
        if (name.endsWith("-US")) return false;

        Uuid uuid = getOrCreateUuid(outputSettings, filepath, "devices_to_components", name);
        QString desc = node->getFirstChild("description", false) ? node->getFirstChild("description", true)->getText<QString>(false) : "";
        desc.append(createDescription(filepath, name));

        // create  component
        Component* component = new Component(uuid, Version("0.1"), "LibrePCB", name, desc);

        // properties
        component->addDefaultValue("en_US", "");
        component->addPrefix("", node->hasAttribute("prefix") ? node->getAttribute<QString>("prefix", false) : "", true);

        // symbol variant
        Uuid symbVarUuid = getOrCreateUuid(outputSettings, filepath, "component_symbolvariants", uuid.toStr());
        ComponentSymbolVariant* symbvar = new ComponentSymbolVariant(symbVarUuid, QString(), true);
        symbvar->setName("en_US", "default");
        symbvar->setDescription("en_US", "");
        component->addSymbolVariant(*symbvar);

        // signals
        XmlDomElement* device = node->getFirstChild("devices/device", true, true);
        for (XmlDomElement* connect = device->getFirstChild("connects/connect", false, false);
             connect; connect = connect->getNextSibling())
        {
            QString gateName = connect->getAttribute<QString>("gate", true);
            QString pinName = connect->getAttribute<QString>("pin", true);
            if (pinName.contains("@")) pinName.truncate(pinName.indexOf("@"));
            if (pinName.contains("#")) pinName.truncate(pinName.indexOf("#"));
            Uuid signalUuid = getOrCreateUuid(outputSettings, filepath, "gatepins_to_componentsignals", uuid.toStr(), gateName % pinName);

            if (!component->getSignalByUuid(signalUuid))
            {
                // create signal
                ComponentSignal* signal = new ComponentSignal(signalUuid, pinName);
                component->addSignal(*signal);
            }
        }

        // symbol variant items
        for (XmlDomElement* gate = node->getFirstChild("gates/*", true, true); gate; gate = gate->getNextSibling())
        {
            QString gateName = gate->getAttribute<QString>("name", true);
            QString symbolName = gate->getAttribute<QString>("symbol", true);
            Uuid symbolUuid = getOrCreateUuid(outputSettings, filepath, "symbols", symbolName);

            // create symbol variant item
            Uuid symbVarItemUuid = getOrCreateUuid(outputSettings, filepath, "symbolgates_to_symbvaritems", uuid.toStr(), gateName);
            ComponentSymbolVariantItem* item = new ComponentSymbolVariantItem(symbVarItemUuid, symbolUuid, true, (gateName == "G$1") ? "" : gateName);

            // connect pins
            for (XmlDomElement* connect = device->getFirstChild("connects/connect", false, false);
                 connect; connect = connect->getNextSibling())
            {
                if (connect->getAttribute<QString>("gate", true) == gateName)
                {
                    QString pinName = connect->getAttribute<QString>("pin", true);
                    Uuid pinUuid = getOrCreateUuid(outputSettings, filepath, "symbol_pins", symbolUuid.toStr(), pinName);
                    if (pinName.contains("@")) pinName.truncate(pinName.indexOf("@"));
                    if (pinName.contains("#")) pinName.truncate(pinName.indexOf("#"));
                    Uuid signalUuid = getOrCreateUuid(outputSettings, filepath, "gatepins_to_componentsignals", uuid.toStr(), gateName % pinName);
                    item->addPinSignalMapping(pinUuid, signalUuid, ComponentSymbolVariantItem::PinDisplayType_t::ComponentSignal);
                }
            }

            symbvar->addItem(*item);
        }

        // create devices
        for (XmlDomElement* deviceNode = node->getFirstChild("devices/*", true, true); deviceNode; deviceNode = deviceNode->getNextSibling())
        {
            if (!deviceNode->hasAttribute("package")) continue;

            QString deviceName = deviceNode->getAttribute<QString>("name", false);
            QString packageName = deviceNode->getAttribute<QString>("package", true);
            Uuid pkgUuid = getOrCreateUuid(outputSettings, filepath, "packages_to_packages", packageName);
            Uuid fptUuid = getOrCreateUuid(outputSettings, filepath, "packages_to_footprints", packageName);

            Uuid compUuid = getOrCreateUuid(outputSettings, filepath, "devices_to_devices", name, deviceName);
            QString compName = deviceName.isEmpty() ? name : QString("%1_%2").arg(name, deviceName);
            Device* device = new Device(compUuid, Version("0.1"), "LibrePCB", compName, desc);
            device->setComponentUuid(component->getUuid());
            device->setPackageUuid(pkgUuid);

            // connect pads
            for (XmlDomElement* connect = deviceNode->getFirstChild("connects/*", false, false);
                 connect; connect = connect->getNextSibling())
            {
                QString gateName = connect->getAttribute<QString>("gate", true);
                QString pinName = connect->getAttribute<QString>("pin", true);
                QString padNames = connect->getAttribute<QString>("pad", true);
                if (pinName.contains("@")) pinName.truncate(pinName.indexOf("@"));
                if (pinName.contains("#")) pinName.truncate(pinName.indexOf("#"));
                if (connect->hasAttribute("route"))
                {
                    if (connect->getAttribute<QString>("route", true) != "any")
                        addError(QString("Unknown connect route: %1/%2").arg(node->getName()).arg(connect->getAttribute<QString>("route", false)), filepath);
                }
                foreach (const QString& padName, padNames.split(" ", QString::SkipEmptyParts))
                {
                    Uuid padUuid = getOrCreateUuid(outputSettings, filepath, "package_pads", fptUuid.toStr(), padName);
                    Uuid signalUuid = getOrCreateUuid(outputSettings, filepath, "gatepins_to_componentsignals", uuid.toStr(), gateName % pinName);
                    device->addPadSignalMapping(padUuid, signalUuid);
                }
            }

            // save device
            device->saveTo(FilePath(QString("%1/dev").arg(ui->output->text())));
            delete device;
        }

        // save component to file
        component->saveTo(FilePath(QString("%1/cmp").arg(ui->output->text())));
        delete component;
    }
    catch (Exception& e)
    {
        addError(e.getUserMsg() % " [" % e.getDebugMsg() % "]");
        return false;
    }

    return true;
}

void MainWindow::on_inputBtn_clicked()
{
    ui->input->addItems(QFileDialog::getOpenFileNames(this, "Select Eagle Library Files",
                                                    mlastInputDirectory, "*.lbr"));
    ui->pbarFiles->setMaximum(ui->input->count());

    if (ui->input->count() > 0)
        mlastInputDirectory = QFileInfo(ui->input->item(0)->text()).absolutePath();
}

void MainWindow::on_outputBtn_clicked()
{
    ui->output->setText(QFileDialog::getExistingDirectory(this, "Select Output Directory",
                                                          ui->output->text()));
}

void MainWindow::on_btnAbort_clicked()
{
    mAbortConversion = true;
}

void MainWindow::on_btnConvertSymbols_clicked()
{
    convertAllFiles(ConvertFileType_t::Symbols_to_Symbols);
}

void MainWindow::on_btnConvertDevices_clicked()
{
    convertAllFiles(ConvertFileType_t::Devices_to_Components);
}

void MainWindow::on_pushButton_2_clicked()
{
    convertAllFiles(ConvertFileType_t::Packages_to_PackagesAndDevices);
}

void MainWindow::on_btnPathsFromIni_clicked()
{
    FilePath inputDir(QFileDialog::getExistingDirectory(this, "Select Input Folder", mlastInputDirectory));
    if (!inputDir.isExistingDir()) return;

    QSettings outputSettings(UUID_LIST_FILEPATH, QSettings::IniFormat);

    foreach (QString key, outputSettings.allKeys())
    {
        key.remove(0, key.indexOf("/")+1);
        key.remove(key.indexOf(".lbr")+4, key.length() - key.indexOf(".lbr") - 4);
        QString filepath = inputDir.getPathTo(key).toNative();

        bool exists = false;
        for (int i = 0; i < ui->input->count(); i++)
        {
            FilePath fp(ui->input->item(i)->text());
            if (fp.toNative() == filepath) exists = true;
        }
        if (!exists) ui->input->addItem(filepath);
    }
}

void MainWindow::on_toolButton_clicked()
{
    ui->input->clear();
}

void MainWindow::on_toolButton_2_clicked()
{
    qDeleteAll(ui->input->selectedItems());
}

void MainWindow::on_toolButton_3_clicked()
{
    QString dir = QFileDialog::getExistingDirectory(this, "Select Library Folder", ui->edtDuplicateFolders->text());
    if (dir.isEmpty()) return;
    ui->edtDuplicateFolders->setText(dir);
}

void MainWindow::on_toolButton_4_clicked()
{
    if (ui->edtDuplicateFolders->text().isEmpty()) return;
    if (ui->output->text().isEmpty()) return;

    QDir libDir(ui->edtDuplicateFolders->text());
    QDir outDir(ui->output->text());

    uint count = 0;

    foreach (const QString& subdir1, libDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot))
    {
        if (subdir1 == "EagleImport") continue;
        if (subdir1 == "Staging_Area") continue;
        QDir repoDir = libDir.absoluteFilePath(subdir1);
        foreach (const QString& subdir2, repoDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot))
        {
            QDir repoSubDir = repoDir.absoluteFilePath(subdir2);
            foreach (const QString& elementDir, repoSubDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot))
            {
                Uuid elementUuid(elementDir);
                if (elementUuid.isNull()) continue;

                QDir outDirElement = outDir.absoluteFilePath(subdir2 % "/" % elementDir);
                if (outDirElement.exists())
                {
                    qDebug() << outDirElement.absolutePath();
                    if (outDirElement.removeRecursively())
                        count++;
                    else
                        qDebug() << "Failed!";
                }
            }
        }
    }

    QMessageBox::information(this, "Duplicates Removed", QString("%1 duplicates removed.").arg(count));
}
