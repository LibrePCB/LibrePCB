#include <QtCore>
#include <QtWidgets>
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <librepcb/common/fileio/smartxmlfile.h>
#include <librepcb/common/fileio/domdocument.h>
#include <librepcb/common/fileio/fileutils.h>
#include <librepcb/library/sym/symbol.h>
#include <librepcb/library/pkg/footprint.h>
#include <librepcb/library/pkg/package.h>
#include <librepcb/library/dev/device.h>
#include <librepcb/library/cmp/component.h>
#include <librepcb/common/graphics/graphicslayer.h>
#include "polygonsimplifier.h"

namespace librepcb {
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

QString MainWindow::convertSchematicLayer(int eagleLayerId)
{
    switch (eagleLayerId)
    {
        case 93: return GraphicsLayer::sSymbolPinNames;
        case 94: return GraphicsLayer::sSymbolOutlines;
        case 95: return GraphicsLayer::sSymbolNames;
        case 96: return GraphicsLayer::sSymbolValues;
        case 99: return GraphicsLayer::sSchematicReferences; // ???
        default: throw Exception(__FILE__, __LINE__, QString("Invalid schematic layer: %1").arg(eagleLayerId));
    }
}

QString MainWindow::convertBoardLayer(int eagleLayerId)
{
    switch (eagleLayerId)
    {
        case 1:  return GraphicsLayer::sTopCopper;
        case 16: return GraphicsLayer::sBotCopper;
        case 20: return GraphicsLayer::sBoardOutlines;
        case 21: return GraphicsLayer::sTopPlacement;
        case 22: return GraphicsLayer::sBotPlacement;
        case 25: return GraphicsLayer::sTopNames;
        case 27: return GraphicsLayer::sTopValues;
        case 29: return GraphicsLayer::sTopStopMask;
        case 31: return GraphicsLayer::sTopSolderPaste;
        case 35: return GraphicsLayer::sTopGlue;
        case 39: return GraphicsLayer::sTopCourtyard;
        //case 41: return Layer::sTopCopperRestrict;
        //case 42: return Layer::sBotCopperRestrict;
        //case 43: return Layer::sViaRestrict;
        case 46: return GraphicsLayer::sBoardMillingPth;
        case 48: return GraphicsLayer::sBoardDocumentation;
        case 49: return GraphicsLayer::sBoardDocumentation; // reference
        case 51: return GraphicsLayer::sTopDocumentation;
        case 52: return GraphicsLayer::sBotDocumentation;
        default: throw Exception(__FILE__, __LINE__, QString("Invalid board layer: %1").arg(eagleLayerId));
    }
}

void MainWindow::convertAllFiles(ConvertFileType_t type)
{
    reset();

    // create output directory
    FilePath outputDir(ui->output->text());
    try {
        FileUtils::makePath(outputDir); // can throw
    } catch (const Exception& e) {
        addError("Fatal Error: " % e.getMsg());
    }

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
        std::unique_ptr<DomDocument> doc = file.parseFileAndBuildDomTree();
        DomElement* node = doc->getRoot().getFirstChild("drawing/library", true, true);

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
        foreach (DomElement* child, node->getChilds()) {
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
        addError(e.getMsg());
        return;
    }
}

bool MainWindow::convertSymbol(QSettings& outputSettings, const FilePath& filepath, DomElement* node)
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
        Symbol* symbol = new Symbol(uuid, Version("0.1"), "LibrePCB", name, desc, "");

        foreach (DomElement* child, node->getChilds()) {
            if (child->getName() == "wire")
            {
                QString layerName = convertSchematicLayer(child->getAttribute<uint>("layer", true));
                bool fill = false;
                bool isGrabArea = true;
                Length lineWidth = child->getAttribute<Length>("width", true);
                Point startpos = Point(child->getAttribute<Length>("x1", true), child->getAttribute<Length>("y1", true));
                Point endpos = Point(child->getAttribute<Length>("x2", true), child->getAttribute<Length>("y2", true));
                Angle angle = child->hasAttribute("curve") ? child->getAttribute<Angle>("curve", true) : Angle(0);
                if (rotate180) {
                    startpos = Point(-startpos.getX(), -startpos.getY());
                    endpos = Point(-endpos.getX(), -endpos.getY());
                }
                symbol->getPolygons().append(std::shared_ptr<Polygon>(Polygon::createCurve(
                    layerName, lineWidth, fill, isGrabArea, startpos, endpos, angle)));
            }
            else if (child->getName() == "rectangle")
            {
                QString layerName = convertSchematicLayer(child->getAttribute<uint>("layer", true));
                bool fill = true;
                bool isGrabArea = true;
                Length lineWidth(0);
                if (child->hasAttribute("width")) lineWidth = child->getAttribute<Length>("width", true);
                Point p1(child->getAttribute<Length>("x1", true), child->getAttribute<Length>("y1", true));
                Point p2(child->getAttribute<Length>("x2", true), child->getAttribute<Length>("y1", true));
                Point p3(child->getAttribute<Length>("x2", true), child->getAttribute<Length>("y2", true));
                Point p4(child->getAttribute<Length>("x1", true), child->getAttribute<Length>("y2", true));
                std::shared_ptr<Polygon> polygon(new Polygon(layerName, lineWidth, fill, isGrabArea, p1));
                polygon->getSegments().append(std::make_shared<PolygonSegment>(p2, Angle::deg0()));
                polygon->getSegments().append(std::make_shared<PolygonSegment>(p3, Angle::deg0()));
                polygon->getSegments().append(std::make_shared<PolygonSegment>(p4, Angle::deg0()));
                polygon->getSegments().append(std::make_shared<PolygonSegment>(p1, Angle::deg0()));
                symbol->getPolygons().append(polygon);
            }
            else if (child->getName() == "polygon")
            {
                QString layerName = convertSchematicLayer(child->getAttribute<uint>("layer", true));
                bool fill = false;
                bool isGrabArea = true;
                Length lineWidth(0);
                if (child->hasAttribute("width")) lineWidth = child->getAttribute<Length>("width", true);
                std::shared_ptr<Polygon> polygon(new Polygon(layerName, lineWidth, fill, isGrabArea, Point(0, 0)));
                foreach (DomElement* vertex, child->getChilds()) {
                    Point p(vertex->getAttribute<Length>("x", true), vertex->getAttribute<Length>("y", true));
                    if (vertex == child->getFirstChild())
                        polygon->setStartPos(p);
                    else
                        polygon->getSegments().append(std::make_shared<PolygonSegment>(p, Angle::deg0()));
                }
                polygon->close();
                symbol->getPolygons().append(polygon);
            }
            else if (child->getName() == "circle")
            {
                QString layerName = convertSchematicLayer(child->getAttribute<uint>("layer", true));
                Length radius(child->getAttribute<Length>("radius", true));
                Point center(child->getAttribute<Length>("x", true), child->getAttribute<Length>("y", true));
                Length lineWidth = child->getAttribute<Length>("width", true);
                bool fill = (lineWidth == 0);
                bool isGrabArea = true;
                symbol->getEllipses().append(std::make_shared<Ellipse>(layerName,
                    lineWidth, fill, isGrabArea, center, radius, radius, Angle::deg0()));
            }
            else if (child->getName() == "text")
            {
                QString layerName = convertSchematicLayer(child->getAttribute<uint>("layer", true));
                QString textStr = child->getText<QString>(true);
                Length height = child->getAttribute<Length>("size", true)*2;
                if (textStr == ">NAME") {
                    textStr = "${NAME}";
                    height = Length::fromMm(3.175);
                } else if (textStr == ">VALUE") {
                    textStr = "${VALUE}";
                    height = Length::fromMm(2.5);
                }
                Point pos = Point(child->getAttribute<Length>("x", true), child->getAttribute<Length>("y", true));
                int angleDeg = 0;
                if (child->hasAttribute("rot")) angleDeg = child->getAttribute<QString>("rot", true).remove("R").toInt();
                if (rotate180) {
                    pos = Point(-pos.getX(), -pos.getY());
                    angleDeg += 180;
                }
                Angle rot = Angle::fromDeg(angleDeg);
                Alignment align(HAlign::left(), VAlign::bottom());
                symbol->getTexts().append(std::make_shared<Text>(
                    layerName, textStr, pos, rot, height, align));
            }
            else if (child->getName() == "pin")
            {
                Uuid pinUuid = getOrCreateUuid(outputSettings, filepath, "symbol_pins", uuid.toStr(), child->getAttribute<QString>("name", true));
                QString name = child->getAttribute<QString>("name", true);
                Point pos = Point(child->getAttribute<Length>("x", true), child->getAttribute<Length>("y", true));
                Length len(7620000);
                if (child->hasAttribute("length")) {
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
                if (rotate180) {
                    pos = Point(-pos.getX(), -pos.getY());
                    angleDeg += 180;
                }
                Angle rot = Angle::fromDeg(angleDeg);
                symbol->getPins().append(std::make_shared<SymbolPin>(pinUuid, name,
                                                                     pos, len, rot));
            }
            else
            {
                addError(QString("Unknown node name: %1/%2").arg(node->getName()).arg(child->getName()), filepath);
                return false;
            }
        }

        // convert line rects to polygon rects
        PolygonSimplifier<Symbol> polygonSimplifier(*symbol);
        polygonSimplifier.convertLineRectsToPolygonRects(false, true);

        // save symbol to file
        symbol->saveIntoParentDirectory(FilePath(QString("%1/sym").arg(ui->output->text())));
        delete symbol;
    }
    catch (Exception& e)
    {
        addError(e.getMsg());
        return false;
    }

    return true;
}

bool MainWindow::convertPackage(QSettings& outputSettings, const FilePath& filepath, DomElement* node)
{
    try
    {
        QString name = node->getAttribute<QString>("name", true);
        QString desc = node->getFirstChild("description", false) ? node->getFirstChild("description", true)->getText<QString>(false) : "";
        desc.append(createDescription(filepath, name));
        bool rotate180 = false;
        //if (filepath.getFilename() == "con-lsta.lbr" && name.startsWith("FE")) rotate180 = true;
        //if (filepath.getFilename() == "con-lstb.lbr" && name.startsWith("MA")) rotate180 = true;

        // create footprint
        Uuid fptUuid = getOrCreateUuid(outputSettings, filepath, "packages_to_footprints", name);
        Footprint footprint(fptUuid, "default", "");

        // create package
        Uuid pkgUuid = getOrCreateUuid(outputSettings, filepath, "packages_to_packages", name);
        Package* package = new Package(pkgUuid, Version("0.1"), "LibrePCB", name, desc, "");

        foreach (DomElement* child, node->getChilds()) {
            if (child->getName() == "description")
            {
                // nothing to do
            }
            else if (child->getName() == "wire")
            {
                QString layerName = convertBoardLayer(child->getAttribute<uint>("layer", true));
                bool fill = false;
                bool isGrabArea = true;
                Length lineWidth = child->getAttribute<Length>("width", true);
                Point startpos = Point(child->getAttribute<Length>("x1", true), child->getAttribute<Length>("y1", true));
                Point endpos = Point(child->getAttribute<Length>("x2", true), child->getAttribute<Length>("y2", true));
                Angle angle = child->hasAttribute("curve") ? child->getAttribute<Angle>("curve", true) : Angle(0);
                if (rotate180) {
                    startpos = Point(-startpos.getX(), -startpos.getY());
                    endpos = Point(-endpos.getX(), -endpos.getY());
                }
                footprint.getPolygons().append(std::shared_ptr<Polygon>(Polygon::createCurve(
                    layerName, lineWidth, fill, isGrabArea, startpos, endpos, angle)));
            }
            else if (child->getName() == "rectangle")
            {
                QString layerName = convertBoardLayer(child->getAttribute<uint>("layer", true));
                bool fill = true;
                bool isGrabArea = true;
                Length lineWidth(0);
                if (child->hasAttribute("width")) lineWidth = child->getAttribute<Length>("width", true);
                Point p1(child->getAttribute<Length>("x1", true), child->getAttribute<Length>("y1", true));
                Point p2(child->getAttribute<Length>("x2", true), child->getAttribute<Length>("y1", true));
                Point p3(child->getAttribute<Length>("x2", true), child->getAttribute<Length>("y2", true));
                Point p4(child->getAttribute<Length>("x1", true), child->getAttribute<Length>("y2", true));
                std::shared_ptr<Polygon> polygon(new Polygon(layerName, lineWidth, fill, isGrabArea, p1));
                polygon->getSegments().append(std::make_shared<PolygonSegment>(p2, Angle::deg0()));
                polygon->getSegments().append(std::make_shared<PolygonSegment>(p3, Angle::deg0()));
                polygon->getSegments().append(std::make_shared<PolygonSegment>(p4, Angle::deg0()));
                polygon->getSegments().append(std::make_shared<PolygonSegment>(p1, Angle::deg0()));
                footprint.getPolygons().append(polygon);
            }
            else if (child->getName() == "polygon")
            {
                QString layerName = convertBoardLayer(child->getAttribute<uint>("layer", true));
                bool fill = false;
                bool isGrabArea = true;
                Length lineWidth(0);
                if (child->hasAttribute("width")) lineWidth = child->getAttribute<Length>("width", true);
                std::shared_ptr<Polygon> polygon(new Polygon(layerName, lineWidth, fill, isGrabArea, Point(0, 0)));
                foreach (DomElement* vertex, child->getChilds()) {
                    Point p(vertex->getAttribute<Length>("x", true), vertex->getAttribute<Length>("y", true));
                    if (vertex == child->getFirstChild())
                        polygon->setStartPos(p);
                    else
                        polygon->getSegments().append(std::make_shared<PolygonSegment>(p, Angle::deg0()));
                }
                polygon->close();
                footprint.getPolygons().append(polygon);
            }
            else if (child->getName() == "circle")
            {
                QString layerName = convertBoardLayer(child->getAttribute<uint>("layer", true));
                Length radius(child->getAttribute<Length>("radius", true));
                Point center(child->getAttribute<Length>("x", true), child->getAttribute<Length>("y", true));
                Length lineWidth = child->getAttribute<Length>("width", true);
                bool fill = (lineWidth == 0);
                bool isGrabArea = true;
                std::shared_ptr<Ellipse> ellipse(new Ellipse(layerName, lineWidth, fill, isGrabArea,
                                                 center, radius, radius, Angle::deg0()));
                footprint.getEllipses().append(ellipse);
            }
            else if (child->getName() == "text")
            {
                QString layerName = convertBoardLayer(child->getAttribute<uint>("layer", true));
                QString textStr = child->getText<QString>(true);
                Length height = child->getAttribute<Length>("size", true)*2;
                if (textStr == ">NAME") {
                    textStr = "${NAME}";
                    height = Length::fromMm(2.5);
                } else if (textStr == ">VALUE") {
                    textStr = "${VALUE}";
                    height = Length::fromMm(2.0);
                }
                Point pos = Point(child->getAttribute<Length>("x", true), child->getAttribute<Length>("y", true));
                int angleDeg = 0;
                if (child->hasAttribute("rot")) angleDeg = child->getAttribute<QString>("rot", true).remove("R").toInt();
                if (rotate180) {
                    pos = Point(-pos.getX(), -pos.getY());
                    angleDeg += 180;
                }
                Angle rot = Angle::fromDeg(angleDeg);
                Alignment align(HAlign::left(), VAlign::bottom());
                std::shared_ptr<Text> text(new Text(layerName, textStr, pos, rot, height, align));
                footprint.getTexts().append(text);
            }
            else if (child->getName() == "pad")
            {
                Uuid padUuid = getOrCreateUuid(outputSettings, filepath, "package_pads", fptUuid.toStr(), child->getAttribute<QString>("name", true));
                QString name = child->getAttribute<QString>("name", true);
                // add package pad
                package->getPads().append(std::make_shared<PackagePad>(padUuid, name));
                // add footprint pad
                Point pos = Point(child->getAttribute<Length>("x", true), child->getAttribute<Length>("y", true));
                Length drillDiameter = child->getAttribute<Length>("drill", true);
                Length padDiameter = drillDiameter * 2;
                if (child->hasAttribute("diameter")) padDiameter = child->getAttribute<Length>("diameter", true);
                Length width = padDiameter;
                Length height = padDiameter;
                FootprintPad::Shape shape;
                QString shapeStr = child->hasAttribute("shape") ? child->getAttribute<QString>("shape", true) : "round";
                if (shapeStr == "square") {
                    shape = FootprintPad::Shape::RECT;
                } else if (shapeStr == "octagon") {
                    shape = FootprintPad::Shape::OCTAGON;
                } else if (shapeStr == "round") {
                    shape = FootprintPad::Shape::ROUND;
                } else if (shapeStr == "long") {
                    shape = FootprintPad::Shape::ROUND;
                    width = padDiameter * 2;
                } else {
                    throw Exception(__FILE__, __LINE__, "Invalid shape: " % shapeStr % " :: " % filepath.toStr());
                }
                int angleDeg = 0;
                if (child->hasAttribute("rot")) angleDeg = child->getAttribute<QString>("rot", true).remove("R").toInt();
                if (rotate180) {
                    pos = Point(-pos.getX(), -pos.getY());
                    angleDeg += 180;
                }
                Angle rot = Angle::fromDeg(angleDeg);
                std::shared_ptr<FootprintPad> fptPad(new FootprintPad(
                    padUuid, pos, rot, shape, width, height, drillDiameter,
                    FootprintPad::BoardSide::THT));
                footprint.getPads().append(fptPad);
            }
            else if (child->getName() == "smd")
            {
                Uuid padUuid = getOrCreateUuid(outputSettings, filepath, "package_pads", fptUuid.toStr(), child->getAttribute<QString>("name", true));
                QString name = child->getAttribute<QString>("name", true);
                // add package pad
                package->getPads().append(std::make_shared<PackagePad>(padUuid, name));
                // add footprint pad
                QString layerName = convertBoardLayer(child->getAttribute<uint>("layer", true));
                FootprintPad::BoardSide side;
                if (layerName == GraphicsLayer::sTopCopper) {
                    side = FootprintPad::BoardSide::TOP;
                } else if (layerName == GraphicsLayer::sBotCopper) {
                    side = FootprintPad::BoardSide::BOTTOM;
                } else {
                    throw Exception(__FILE__, __LINE__, QString("Invalid pad layer: %1").arg(layerName));
                }
                Point pos = Point(child->getAttribute<Length>("x", true), child->getAttribute<Length>("y", true));
                int angleDeg = 0;
                if (child->hasAttribute("rot")) angleDeg = child->getAttribute<QString>("rot", true).remove("R").toInt();
                if (rotate180) {
                    pos = Point(-pos.getX(), -pos.getY());
                    angleDeg += 180;
                }
                Angle rot = Angle::fromDeg(angleDeg);
                Length width = child->getAttribute<Length>("dx", true);
                Length height = child->getAttribute<Length>("dy", true);
                std::shared_ptr<FootprintPad> fptPad(new FootprintPad(
                    padUuid, pos, rot, FootprintPad::Shape::RECT, width, height,
                    Length(0), side));
                footprint.getPads().append(fptPad);
            }
            else if (child->getName() == "hole")
            {
                Point pos(child->getAttribute<Length>("x", true), child->getAttribute<Length>("y", true));
                Length diameter(child->getAttribute<Length>("drill", true));
                footprint.getHoles().append(std::make_shared<Hole>(pos, diameter));
            }
            else
            {
                addError(QString("Unknown node name: %1/%2").arg(node->getName()).arg(child->getName()), filepath);
                return false;
            }
        }

        // convert line rects to polygon rects
        PolygonSimplifier<Footprint> polygonSimplifier(footprint);
        polygonSimplifier.convertLineRectsToPolygonRects(false, true);

        // add footprint to package
        package->getFootprints().append(std::make_shared<Footprint>(footprint));

        // save package to file
        package->saveIntoParentDirectory(FilePath(QString("%1/pkg").arg(ui->output->text())));

        // clean up
        delete package;
    }
    catch (Exception& e)
    {
        addError(e.getMsg());
        return false;
    }

    return true;
}

bool MainWindow::convertDevice(QSettings& outputSettings, const FilePath& filepath, DomElement* node)
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
        Component* component = new Component(uuid, Version("0.1"), "LibrePCB", name, desc, "");

        // properties
        component->getPrefixes().insert("", node->hasAttribute("prefix") ? node->getAttribute<QString>("prefix", false) : "");

        // symbol variant
        Uuid symbVarUuid = getOrCreateUuid(outputSettings, filepath, "component_symbolvariants", uuid.toStr());
        std::shared_ptr<ComponentSymbolVariant> symbvar(new ComponentSymbolVariant(symbVarUuid, "", "default", ""));
        component->getSymbolVariants().append(symbvar);

        // signals
        DomElement* device = node->getFirstChild("devices/device", true, true);
        for (DomElement* connect = device->getFirstChild("connects/connect", false, false);
             connect; connect = connect->getNextSibling())
        {
            QString gateName = connect->getAttribute<QString>("gate", true);
            QString pinName = connect->getAttribute<QString>("pin", true);
            if (pinName.contains("@")) pinName.truncate(pinName.indexOf("@"));
            if (pinName.contains("#")) pinName.truncate(pinName.indexOf("#"));
            Uuid signalUuid = getOrCreateUuid(outputSettings, filepath, "gatepins_to_componentsignals", uuid.toStr(), gateName % pinName);

            if (!component->getSignals().contains(signalUuid))
            {
                // create signal
                component->getSignals().append(std::make_shared<ComponentSignal>(signalUuid, pinName));
            }
        }

        // symbol variant items
        foreach (DomElement* gate, node->getFirstChild("gates", true)->getChilds()) {
            QString gateName = gate->getAttribute<QString>("name", true);
            QString symbolName = gate->getAttribute<QString>("symbol", true);
            Uuid symbolUuid = getOrCreateUuid(outputSettings, filepath, "symbols", symbolName);

            // create symbol variant item
            Uuid symbVarItemUuid = getOrCreateUuid(outputSettings, filepath, "symbolgates_to_symbvaritems", uuid.toStr(), gateName);
            std::shared_ptr<ComponentSymbolVariantItem> item(new ComponentSymbolVariantItem(symbVarItemUuid, symbolUuid, true, (gateName == "G$1") ? "" : gateName));

            // connect pins
            for (DomElement* connect = device->getFirstChild("connects/connect", false, false);
                 connect; connect = connect->getNextSibling())
            {
                if (connect->getAttribute<QString>("gate", true) == gateName)
                {
                    QString pinName = connect->getAttribute<QString>("pin", true);
                    Uuid pinUuid = getOrCreateUuid(outputSettings, filepath, "symbol_pins", symbolUuid.toStr(), pinName);
                    if (pinName.contains("@")) pinName.truncate(pinName.indexOf("@"));
                    if (pinName.contains("#")) pinName.truncate(pinName.indexOf("#"));
                    Uuid signalUuid = getOrCreateUuid(outputSettings, filepath, "gatepins_to_componentsignals", uuid.toStr(), gateName % pinName);
                    item->getPinSignalMap().append(
                        std::make_shared<ComponentPinSignalMapItem>(pinUuid,
                            signalUuid, CmpSigPinDisplayType::componentSignal()));
                }
            }

            symbvar->getSymbolItems().append(item);
        }

        // create devices
        foreach (DomElement* deviceNode, node->getFirstChild("devices", true)->getChilds()) {
            if (!deviceNode->hasAttribute("package")) continue;

            QString deviceName = deviceNode->getAttribute<QString>("name", false);
            QString packageName = deviceNode->getAttribute<QString>("package", true);
            Uuid pkgUuid = getOrCreateUuid(outputSettings, filepath, "packages_to_packages", packageName);
            Uuid fptUuid = getOrCreateUuid(outputSettings, filepath, "packages_to_footprints", packageName);

            Uuid compUuid = getOrCreateUuid(outputSettings, filepath, "devices_to_devices", name, deviceName);
            QString compName = deviceName.isEmpty() ? name : QString("%1_%2").arg(name, deviceName);
            Device* device = new Device(compUuid, Version("0.1"), "LibrePCB", compName, desc, "");
            device->setComponentUuid(component->getUuid());
            device->setPackageUuid(pkgUuid);

            // connect pads
            for (DomElement* connect = deviceNode->getFirstChild("connects/*", false, false);
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
                    device->getPadSignalMap().append(
                        std::make_shared<DevicePadSignalMapItem>(padUuid, signalUuid));
                }
            }

            // save device
            device->saveIntoParentDirectory(FilePath(QString("%1/dev").arg(ui->output->text())));
            delete device;
        }

        // save component to file
        component->saveIntoParentDirectory(FilePath(QString("%1/cmp").arg(ui->output->text())));
        delete component;
    }
    catch (Exception& e)
    {
        addError(e.getMsg());
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

}
