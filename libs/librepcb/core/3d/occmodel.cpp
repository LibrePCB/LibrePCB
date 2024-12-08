/*
 * LibrePCB - Professional EDA for everyone!
 * Copyright (C) 2013 LibrePCB Developers, see AUTHORS.md for contributors.
 * https://librepcb.org/
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "occmodel.h"

#include "../exceptions.h"
#include "../fileio/filepath.h"
#include "../fileio/fileutils.h"
#include "../utils/toolbox.h"
#include "../utils/transform.h"
#include "librepcb_build_env.h"

// clang-format off
#if USE_OPENCASCADE
#include <APIHeaderSection_MakeHeader.hxx>
#include <BRep_Tool.hxx>
#include <BRepAlgoAPI_Cut.hxx>
#include <BRepBuilderAPI.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepBuilderAPI_MakeWire.hxx>
#include <BRepPrimAPI_MakePrism.hxx>
#include <BRepMesh_IncrementalMesh.hxx>
#include <gp_Circ.hxx>
#include <gp_Pnt.hxx>
#include <Message.hxx>
#include <Message_Gravity.hxx>
#include <Message_Messenger.hxx>
#include <Message_Printer.hxx>
#include <Message_SequenceOfPrinters.hxx>
#include <Poly_Triangulation.hxx>
#include <Quantity_Color.hxx>
#include <Standard_Version.hxx>
#include <STEPCAFControl_Reader.hxx>
#include <STEPCAFControl_Writer.hxx>
#include <STEPControl_Reader.hxx>
#include <TColgp_Array1OfPnt.hxx>
#include <TCollection_HAsciiString.hxx>
#include <TDataStd_Name.hxx>
#include <TDF_ChildIterator.hxx>
#include <TDF_Label.hxx>
#include <TDF_LabelSequence.hxx>
#include <TDocStd_Document.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS.hxx>
#include <XCAFApp_Application.hxx>
#include <XCAFDoc_ColorTool.hxx>
#include <XCAFDoc_DocumentTool.hxx>
#include <XCAFDoc_ShapeTool.hxx>
#endif
// clang-format on

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

bool OccModel::sOutputVerbosityConfigured = false;

/*******************************************************************************
 *  Data
 ******************************************************************************/

struct OccModel::Data {
#if USE_OPENCASCADE
  Handle(TDocStd_Document) doc;
  TDF_Label assemblyLabel;
#else
  int dummy;
#endif
};

/*******************************************************************************
 *  Helper Functions
 ******************************************************************************/

#if USE_OPENCASCADE

static bool tryGetColor(Handle(XCAFDoc_ColorTool) colorTool,
                        const TopoDS_Shape& shape, Quantity_Color& color) {
  return colorTool->GetColor(shape, XCAFDoc_ColorSurf, color) ||
      colorTool->GetColor(shape, XCAFDoc_ColorGen, color) ||
      colorTool->GetColor(shape, XCAFDoc_ColorCurv, color);
}

static bool tryGetColor(Handle(XCAFDoc_ColorTool) colorTool,
                        const TDF_Label& label, Quantity_Color& color) {
  return colorTool->GetColor(label, XCAFDoc_ColorSurf, color) ||
      colorTool->GetColor(label, XCAFDoc_ColorGen, color) ||
      colorTool->GetColor(label, XCAFDoc_ColorCurv, color);
}

static bool tesselateFace(Handle(XCAFDoc_ColorTool) colorTool,
                          const TopoDS_Face& face, const gp_Trsf& transform,
                          Quantity_Color color,
                          QMap<OccModel::Color, QVector<QVector3D>>& result) {
  if (face.IsNull()) return false;

  const Standard_Real deflectionAngle = 20. * 3.141 / 180.;
  const Standard_Real deflection = 0.01;

  TopLoc_Location loc;
  Handle(Poly_Triangulation) triangulation =
      BRep_Tool::Triangulation(face, loc);
  if (triangulation.IsNull() ||
      (triangulation->Deflection() > (deflection + Precision::Confusion()))) {
    BRepMesh_IncrementalMesh(face, deflection, Standard_False, deflectionAngle);
    triangulation = BRep_Tool::Triangulation(face, loc);
  }
  if (triangulation.IsNull()) return false;

  tryGetColor(colorTool, face, color);
  const OccModel::Color colorTuple =
      std::make_tuple(color.Red(), color.Green(), color.Blue());
  QVector<QVector3D>& triangles = result[colorTuple];

  Standard_Integer n1, n2, n3;
  for (Standard_Integer i = 1; i < (triangulation->NbTriangles() + 1); ++i) {
    triangulation->Triangles().Value(i).Get(n1, n2, n3);
#if OCC_VERSION_HEX >= 0x070600
    gp_XYZ p1 = triangulation->Node(n1).XYZ();
    gp_XYZ p2 = triangulation->Node(n2).XYZ();
    gp_XYZ p3 = triangulation->Node(n3).XYZ();
#else
    gp_XYZ p1 = triangulation->Nodes().Value(n1).XYZ();
    gp_XYZ p2 = triangulation->Nodes().Value(n2).XYZ();
    gp_XYZ p3 = triangulation->Nodes().Value(n3).XYZ();
#endif
    transform.Transforms(p1);
    transform.Transforms(p2);
    transform.Transforms(p3);
    triangles.append(QVector3D(p1.X(), p1.Y(), p1.Z()));
    triangles.append(QVector3D(p2.X(), p2.Y(), p2.Z()));
    triangles.append(QVector3D(p3.X(), p3.Y(), p3.Z()));
  }
  return true;
}

static void tesselateShell(Handle(XCAFDoc_ColorTool) colorTool,
                           const TopoDS_Shape& shell, const gp_Trsf& transform,
                           Quantity_Color color,
                           QMap<OccModel::Color, QVector<QVector3D>>& result) {
  tryGetColor(colorTool, shell, color);

  TopoDS_Iterator it;
  for (it.Initialize(shell); it.More(); it.Next()) {
    const TopoDS_Face& face = TopoDS::Face(it.Value());
    tesselateFace(colorTool, face, transform, color, result);
  }
}

static void tesselateSolid(Handle(XCAFDoc_ColorTool) colorTool,
                           const TopoDS_Shape& solid, const gp_Trsf& transform,
                           Quantity_Color color,
                           QMap<OccModel::Color, QVector<QVector3D>>& result) {
  tryGetColor(colorTool, solid, color);

  TopoDS_Iterator it;
  for (it.Initialize(solid); it.More(); it.Next()) {
    const TopoDS_Shape& subShape = it.Value();
    if (subShape.ShapeType() == TopAbs_SHELL) {
      tesselateShell(colorTool, TopoDS::Shell(subShape), transform, color,
                     result);
    }
  }
}

static void tesselateLabel(Handle(XCAFDoc_ShapeTool) shapeTool,
                           Handle(XCAFDoc_ColorTool) colorTool,
                           gp_Trsf transform, Quantity_Color color,
                           TDF_Label label,
                           QMap<OccModel::Color, QVector<QVector3D>>& result) {
  if (!colorTool->IsVisible(label)) {
    return;
  }

  TopoDS_Shape shape;
  if (!shapeTool->GetShape(label, shape)) {
    return;
  }
  if (!shape.Location().IsIdentity()) {
    transform *= shape.Location().Transformation();
  }

  if (shapeTool->GetReferredShape(label, label)) {
    if (!shapeTool->GetShape(label, shape)) {
      return;
    }
  }

  switch (shape.ShapeType()) {
    case TopAbs_COMPOUND: {
      if (!shapeTool->IsAssembly(label)) {
        TopExp_Explorer ex;
        tryGetColor(colorTool, shape, color);
        for (ex.Init(shape, TopAbs_SOLID); ex.More(); ex.Next()) {
          tesselateSolid(colorTool, ex.Current(), transform, color, result);
        }
        for (ex.Init(shape, TopAbs_SHELL, TopAbs_SOLID); ex.More(); ex.Next()) {
          tesselateShell(colorTool, ex.Current(), transform, color, result);
        }
        for (ex.Init(shape, TopAbs_FACE, TopAbs_SHELL); ex.More(); ex.Next()) {
          const TopoDS_Face& face = TopoDS::Face(ex.Current());
          tesselateFace(colorTool, face, transform, color, result);
        }
      }
      break;
    }
    case TopAbs_SOLID: {
      tesselateSolid(colorTool, shape, transform, color, result);
      break;
    }
    case TopAbs_SHELL: {
      tesselateShell(colorTool, shape, transform, color, result);
      break;
    }
    case TopAbs_FACE: {
      tesselateFace(colorTool, TopoDS::Face(shape), transform, color, result);
      break;
    }
    default:
      break;
  }

  if ((!shapeTool->IsSimpleShape(label)) && label.HasChild()) {
    tryGetColor(colorTool, shape, color);
    TDF_ChildIterator it;
    for (it.Initialize(label); it.More(); it.Next()) {
      tesselateLabel(shapeTool, colorTool, transform, color, it.Value(),
                     result);
    }
  }
}

static TopoDS_Face pathToFace(const Path& path, const Length& z) {
  BRepBuilderAPI_MakeWire wire;
  for (int i = 1; i < path.getVertices().count(); ++i) {
    const Vertex& v0 = path.getVertices().at(i - 1);
    const Vertex& v1 = path.getVertices().at(i);
    const gp_Pnt p0(v0.getPos().getX().toMm(), v0.getPos().getY().toMm(),
                    z.toMm());
    const gp_Pnt p1(v1.getPos().getX().toMm(), v1.getPos().getY().toMm(),
                    z.toMm());
    TopoDS_Edge edge;
    if (auto center =
            Toolbox::arcCenter(v0.getPos(), v1.getPos(), v0.getAngle())) {
      // Arc segment.
      const UnsignedLength radiusAbs = (v0.getPos() - (*center)).getLength();
      gp_Circ arc(
          gp_Ax2(gp_Pnt(center->getX().toMm(), center->getY().toMm(), z.toMm()),
                 gp_Dir(0.0, 0.0, (v0.getAngle() < 0) ? -1.0 : 1.0)),
          radiusAbs->toMm());
      edge = BRepBuilderAPI_MakeEdge(arc, p0, p1);
    } else {
      // Straight segment.
      edge = BRepBuilderAPI_MakeEdge(p0, p1);
    }
    wire.Add(edge);
  }
  return BRepBuilderAPI_MakeFace(wire);
}

#endif

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

OccModel::OccModel(std::unique_ptr<Data> data) : mImpl(std::move(data)) {
  Q_ASSERT(mImpl);
}

OccModel::~OccModel() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void OccModel::addToAssembly(const OccModel& model, const Point3D& pos,
                             const Angle3D& rot, const Transform& transform,
                             const QString& name) {
#if USE_OPENCASCADE
  try {
    Handle(XCAFDoc_ShapeTool) assemblyShapeTool =
        XCAFDoc_DocumentTool::ShapeTool(mImpl->doc->Main());
    Handle(XCAFDoc_ShapeTool) modelShapeTool =
        XCAFDoc_DocumentTool::ShapeTool(model.mImpl->doc->Main());
    Handle(XCAFDoc_ColorTool) assemblyColorTool =
        XCAFDoc_DocumentTool::ColorTool(model.mImpl->doc->Main());
    Handle(XCAFDoc_ColorTool) modelColorTool =
        XCAFDoc_DocumentTool::ColorTool(mImpl->doc->Main());
    TopExp_Explorer assemblyExplorer;
    TopExp_Explorer modelExplorer;

    TDF_Label newLabel = assemblyShapeTool->NewShape();
    TCollection_ExtendedString newName(cleanString(name).toStdString().c_str());
    TDataStd_Name::Set(newLabel, newName);

    TDF_LabelSequence modelShapes;
    modelShapeTool->GetFreeShapes(modelShapes);
    for (int i = 1; i <= modelShapes.Length(); ++i) {
      TopoDS_Shape shape = modelShapeTool->GetShape(modelShapes.Value(i));
      if (shape.IsNull()) continue;
      TDF_Label shapeLabel = assemblyShapeTool->AddShape(shape, Standard_False);
      const QString shapeName = QString("%1:%2").arg(cleanString(name)).arg(i);
      TDataStd_Name::Set(shapeLabel, shapeName.toStdString().c_str());
      // ATTENTION: Until LibrePCB 1.1.0 we passed shape.Location() instead of
      // TopLoc_Location(), but this caused wrong placement in rare cases.
      // Although TopLoc_Location() sounds wrong(?), it fixes the issue without
      // observing any negative consequences so far. If any issues are observed
      // in future, we might have to reconsider this change.
      // See details in https://github.com/LibrePCB/LibrePCB/issues/1387.
      TDF_Label cmpLabel = assemblyShapeTool->AddComponent(newLabel, shapeLabel,
                                                           TopLoc_Location());

      // Copy face colors.
      modelExplorer.Init(shape, TopAbs_FACE);
      assemblyExplorer.Init(assemblyShapeTool->GetShape(cmpLabel), TopAbs_FACE);
      while (modelExplorer.More() && assemblyExplorer.More()) {
        Quantity_Color color;
        TDF_Label label;
        if (modelShapeTool->FindShape(modelExplorer.Current(), label)) {
          if (tryGetColor(assemblyColorTool, label, color)) {
            modelColorTool->SetColor(assemblyExplorer.Current(), color,
                                     XCAFDoc_ColorSurf);
          }
        } else if (tryGetColor(assemblyColorTool, modelExplorer.Current(),
                               color)) {
          modelColorTool->SetColor(assemblyExplorer.Current(), color,
                                   XCAFDoc_ColorSurf);
        }
        modelExplorer.Next();
        assemblyExplorer.Next();
      }

      // Copy solid colors.
      modelExplorer.Init(shape, TopAbs_SOLID, TopAbs_FACE);
      assemblyExplorer.Init(assemblyShapeTool->GetShape(cmpLabel), TopAbs_SOLID,
                            TopAbs_FACE);
      while (modelExplorer.More() && assemblyExplorer.More()) {
        Quantity_Color color;
        TDF_Label label;
        if (modelShapeTool->FindShape(modelExplorer.Current(), label)) {
          if (tryGetColor(assemblyColorTool, label, color)) {
            modelColorTool->SetColor(assemblyExplorer.Current(), color,
                                     XCAFDoc_ColorSurf);
          }
        } else if (tryGetColor(assemblyColorTool, modelExplorer.Current(),
                               color)) {
          modelColorTool->SetColor(assemblyExplorer.Current(), color,
                                   XCAFDoc_ColorSurf);
        }
        modelExplorer.Next();
        assemblyExplorer.Next();
      }
    }

    gp_Trsf t, tTmp;
    t.SetTranslation(gp_Vec(transform.getPosition().getX().toMm(),
                            transform.getPosition().getY().toMm(), 0));
    tTmp.SetRotation(gp_Ax1(gp_Pnt(0, 0, 0), gp_Dir(0, 0, 1)),
                     transform.getRotation().toRad());
    t *= tTmp;
    if (transform.getMirrored()) {
      tTmp.SetRotation(gp_Ax1(gp_Pnt(0, 0, 0), gp_Dir(0, 1, 0)),
                       Angle::deg180().toRad());
      t *= tTmp;
    }
    tTmp.SetTranslation(gp_Vec(std::get<0>(pos).toMm(), std::get<1>(pos).toMm(),
                               std::get<2>(pos).toMm()));
    t *= tTmp;
    tTmp.SetRotation(gp_Ax1(gp_Pnt(0, 0, 0), gp_Dir(0, 0, 1)),
                     std::get<2>(rot).toRad());
    t *= tTmp;
    tTmp.SetRotation(gp_Ax1(gp_Pnt(0, 0, 0), gp_Dir(0, 1, 0)),
                     std::get<1>(rot).toRad());
    t *= tTmp;
    tTmp.SetRotation(gp_Ax1(gp_Pnt(0, 0, 0), gp_Dir(1, 0, 0)),
                     std::get<0>(rot).toRad());
    t *= tTmp;
    assemblyShapeTool->AddComponent(mImpl->assemblyLabel, newLabel,
                                    TopLoc_Location(t));

#if OCC_VERSION_HEX >= 0x070200
    assemblyShapeTool->UpdateAssemblies();
#endif
  } catch (const Standard_Failure& e) {
    qCritical() << "OpenCascade error:" << e.GetMessageString();
    throw RuntimeError(
        __FILE__, __LINE__,
        QString("STEP assembly failed: %1").arg(e.GetMessageString()));
  }
#else
  Q_UNUSED(model);
  Q_UNUSED(pos);
  Q_UNUSED(rot);
  Q_UNUSED(transform);
  Q_UNUSED(name);
#endif
}

void OccModel::saveAsStep(const QString& name, const FilePath& fp) const {
#if USE_OPENCASCADE
  try {
    STEPCAFControl_Writer writer;
    writer.SetColorMode(Standard_True);
    writer.SetNameMode(Standard_True);

    APIHeaderSection_MakeHeader hdr(writer.ChangeWriter().Model());
    hdr.SetName(
        new TCollection_HAsciiString(cleanString(name).toStdString().c_str()));
    hdr.SetAuthorValue(1, new TCollection_HAsciiString(""));
    hdr.SetOrganizationValue(1, new TCollection_HAsciiString(""));
    hdr.SetOriginatingSystem(new TCollection_HAsciiString("LibrePCB"));
    hdr.SetDescriptionValue(1, new TCollection_HAsciiString("PCB Assembly"));

    FileUtils::makePath(fp.getParentDir());
    if (writer.Perform(mImpl->doc, qPrintable(fp.toStr())) != Standard_True) {
      throw RuntimeError(__FILE__, __LINE__, tr("Failed to write STEP file."));
    }
  } catch (const Standard_Failure& e) {
    qCritical() << "OpenCascade error:" << e.GetMessageString();
    throw RuntimeError(
        __FILE__, __LINE__,
        QString("STEP export failed: %1").arg(e.GetMessageString()));
  }
#else
  Q_UNUSED(name);
  Q_UNUSED(fp);
  throwNotAvailable();
#endif
}

QMap<OccModel::Color, QVector<QVector3D>> OccModel::tesselate() const {
  QMap<OccModel::Color, QVector<QVector3D>> result;
#if USE_OPENCASCADE
  try {
    Handle(XCAFDoc_ShapeTool) shapeTool =
        XCAFDoc_DocumentTool::ShapeTool(mImpl->doc->Main());
    Handle(XCAFDoc_ColorTool) colorTool =
        XCAFDoc_DocumentTool::ColorTool(mImpl->doc->Main());
    TDF_LabelSequence labels;
    shapeTool->GetFreeShapes(labels);
    for (Standard_Integer i = 1; i <= labels.Length(); ++i) {
      tesselateLabel(shapeTool, colorTool, gp_Trsf(),
                     Quantity_Color(0.5, 0.5, 0.5, Quantity_TOC_RGB),
                     labels.Value(i), result);
    }
  } catch (const Standard_Failure& e) {
    qCritical() << "OpenCascade error:" << e.GetMessageString();
    throw RuntimeError(
        __FILE__, __LINE__,
        QString("STEP tesselation failed: %1").arg(e.GetMessageString()));
  }
#endif
  return result;
}

/*******************************************************************************
 *  Static Methods
 ******************************************************************************/

bool OccModel::isAvailable() noexcept {
  return (USE_OPENCASCADE != 0);
}

QString OccModel::getOccVersionString() noexcept {
  QString s = OCC_EDITION_NAME;
#if USE_OPENCASCADE
  s += " ";
  s += OCC_VERSION_COMPLETE;
#endif
  return s;
}

void OccModel::setVerboseOutput(bool verbose) noexcept {
#if USE_OPENCASCADE
  const Message_SequenceOfPrinters& printers =
      Message::DefaultMessenger()->Printers();
  for (int i = 1; i <= printers.Length(); ++i) {
    printers.Value(i)->SetTraceLevel(verbose ? Message_Trace : Message_Alarm);
  }
#else
  Q_UNUSED(verbose);
#endif
  sOutputVerbosityConfigured = true;
}

std::unique_ptr<OccModel> OccModel::createAssembly(const QString& name) {
  std::unique_ptr<OccModel> result;
#if USE_OPENCASCADE
  try {
    initOpenCascade();

    Handle(XCAFApp_Application) app = XCAFApp_Application::GetApplication();
    Handle(TDocStd_Document) doc;
    app->NewDocument("MDTV-XCAF", doc);
    Handle(XCAFDoc_ShapeTool) shapeTool =
        XCAFDoc_DocumentTool::ShapeTool(doc->Main());
    TDF_Label label = shapeTool->NewShape();

    TCollection_ExtendedString shapeName(
        cleanString(name).toStdString().c_str());
    TDataStd_Name::Set(label, shapeName);

    result.reset(new OccModel(std::unique_ptr<Data>(new Data{doc, label})));
  } catch (const Standard_Failure& e) {
    qCritical() << "OpenCascade error:" << e.GetMessageString();
    throw RuntimeError(
        __FILE__, __LINE__,
        QString("Failed to create 3D assembly: %1").arg(e.GetMessageString()));
  }
#else
  Q_UNUSED(name);
  throwNotAvailable();
#endif
  return result;
}

std::unique_ptr<OccModel> OccModel::createBoard(const librepcb::Path& outline,
                                                const QVector<Path>& holes,
                                                const PositiveLength& thickness,
                                                const QColor& color) {
  std::unique_ptr<OccModel> result;
#if USE_OPENCASCADE
  try {
    initOpenCascade();

    Handle(XCAFApp_Application) app = XCAFApp_Application::GetApplication();
    Handle(TDocStd_Document) doc;
    app->NewDocument("MDTV-XCAF", doc);
    Handle(XCAFDoc_ShapeTool) shapeTool =
        XCAFDoc_DocumentTool::ShapeTool(doc->Main());

    TopoDS_Shape face;
    if (holes.isEmpty()) {
      // Cutting fails if there are no holes.
      face = pathToFace(outline.cleaned(), Length(0));
    } else {
      TopTools_ListOfShape boardFaces;
      boardFaces.Append(pathToFace(outline.cleaned(), Length(0)));
      TopTools_ListOfShape holeFaces;
      foreach (const Path& hole, holes) {
        holeFaces.Append(pathToFace({hole.cleaned()}, Length(0)));
      }
      BRepAlgoAPI_Cut cutter;
      cutter.SetArguments(boardFaces);
      cutter.SetTools(holeFaces);
      cutter.SetRunParallel(Standard_True);
      cutter.Build();
      face = cutter.Shape();
    }
    if (face.IsNull()) {
      // Handle error to avoid segfault in code below.
      throw LogicError(__FILE__, __LINE__, "OCC failed to build board shape.");
    }
    TopoDS_Shape shape =
        BRepPrimAPI_MakePrism(face, gp_Vec(0, 0, thickness->toMm()));
    TDF_Label label = shapeTool->AddShape(shape, false);

    if (!label.IsNull()) {
      Quantity_Color shapeColor(color.redF(), color.greenF(), color.blueF(),
                                Quantity_TOC_RGB);
      Handle(XCAFDoc_ColorTool) colorTool =
          XCAFDoc_DocumentTool::ColorTool(doc->Main());
      colorTool->SetColor(label, shapeColor, XCAFDoc_ColorSurf);
      TopExp_Explorer explorer;
      explorer.Init(shape, TopAbs_SOLID);
      while (explorer.More()) {
        colorTool->SetColor(explorer.Current(), shapeColor, XCAFDoc_ColorSurf);
        explorer.Next();
      }
    } else {
      qWarning() << "Failed to apply color to PCB 3D model.";
    }

#if OCC_VERSION_HEX >= 0x070200
    shapeTool->UpdateAssemblies();
#endif

    result.reset(
        new OccModel(std::unique_ptr<Data>(new Data{doc, TDF_Label()})));
  } catch (const Standard_Failure& e) {
    qCritical() << "OpenCascade error:" << e.GetMessageString();
    throw RuntimeError(__FILE__, __LINE__,
                       QString("Failed to create board 3D model: %1")
                           .arg(e.GetMessageString()));
  }
#else
  Q_UNUSED(outline);
  Q_UNUSED(holes);
  Q_UNUSED(thickness);
  Q_UNUSED(color);
  throwNotAvailable();
#endif
  return result;
}

std::unique_ptr<OccModel> OccModel::loadStep(const QByteArray content) {
  std::unique_ptr<OccModel> result;
#if USE_OPENCASCADE
  try {
    initOpenCascade();

    Handle(XCAFApp_Application) app = XCAFApp_Application::GetApplication();
    Handle(TDocStd_Document) doc;
    app->NewDocument("MDTV-XCAF", doc);
    STEPCAFControl_Reader stepReader;
    stepReader.SetColorMode(Standard_True);
    stepReader.SetNameMode(Standard_False);
    stepReader.SetLayerMode(Standard_False);

    STEPControl_Reader& reader = stepReader.ChangeReader();
#if OCC_VERSION_HEX >= 0x070500
    std::istringstream is(content.data());
    const IFSelect_ReturnStatus ret = reader.ReadStream("stream.step", is);
#else
    FilePath tmp = FilePath::getRandomTempPath();
    FileUtils::writeFile(tmp, content);  // can throw
    const IFSelect_ReturnStatus ret = reader.ReadFile(qPrintable(tmp.toStr()));
    FileUtils::removeFile(tmp);  // can throw
#endif
    if (ret != IFSelect_RetDone) {
      throw RuntimeError(__FILE__, __LINE__, tr("Failed to read STEP file!"));
    }

    if (!stepReader.Transfer(doc)) {
      doc->Close();
      throw RuntimeError(__FILE__, __LINE__, "Failed to transfer STEP model.");
    }
    result.reset(
        new OccModel(std::unique_ptr<Data>(new Data{doc, TDF_Label()})));
  } catch (const Standard_Failure& e) {
    qCritical() << "OpenCascade error:" << e.GetMessageString();
    throw RuntimeError(
        __FILE__, __LINE__,
        QString("Failed to read STEP file: %1").arg(e.GetMessageString()));
  }
#else
  Q_UNUSED(content);
  throwNotAvailable();
#endif
  return result;
}

QByteArray OccModel::minifyStep(const QByteArray& content) {
  QElapsedTimer timer;
  timer.start();

  // Split linhes and clean whitespaces.
  QStringList lines;
  foreach (QString line, content.split('\n')) {
    const QString trimmed = line.trimmed();
    if (!trimmed.startsWith('*')) {
      line = trimmed;
    }
    line.replace("\r", "");
    while (line.endsWith(" ;")) {
      line.chop(2);
      line.append(';');
    }
    if (!line.isEmpty()) {
      lines.append(line);
    }
  }

  // Split header, data and footer.
  const int dataStart = lines.indexOf("DATA;");
  if (dataStart < 0) {
    throw RuntimeError(__FILE__, __LINE__, "STEP data section not found.");
  }
  const int dataEnd = lines.indexOf("ENDSEC;", dataStart + 1);
  if (dataEnd < 0) {
    throw RuntimeError(__FILE__, __LINE__, "STEP data section end not found.");
  }
  const QStringList headerLines = lines.mid(0, dataStart + 1);
  const QStringList footerLines = lines.mid(dataEnd);

  // Unwrap multi-line data items and replace "-0." by "0." to allow
  // eliminating more duplicates.
  QRegularExpression re("\\-0\\.([^0-9])");
  const QStringList dataLines =
      lines.mid(dataStart + 1, dataEnd - dataStart - 1)
          .join("")
          .replace(re, "0.\\1")
          .split(';', Qt::SkipEmptyParts);

  // Parse data into key-value structure.
  // Note: The last item of the QList<int> is not part of the data, but only
  // used internally to make particular entries unique.
  typedef std::pair<QStringList, QList<int>> Value;
  QMap<int, Value> data;
  re = QRegularExpression("#([0-9]+)");
  foreach (const QString& line, dataLines) {
    const int equalPos = line.indexOf("=");
    bool ok = false;
    const int id = line.mid(1, equalPos - 1).trimmed().toInt(&ok);
    if (!ok) {
      throw RuntimeError(__FILE__, __LINE__,
                         "Failed to parse data section of STEP file.");
    }
    const QString valueStr = line.mid(equalPos + 1).trimmed();
    Value value;
    int consumed = 0;
    auto it = re.globalMatch(valueStr);
    while (it.hasNext()) {
      auto match = it.next();
      value.first.append(
          valueStr.mid(consumed, match.capturedStart(1) - consumed));
      value.second.append(match.captured(1).toInt());
      consumed = match.capturedEnd(1);
    }
    value.first.append(valueStr.mid(consumed));
    // Important: It seems some entries must not be merged even if they are
    // identical. When merged, the STEP model won't be rendered anymore
    // and FreeCAD displays a wrong shape object tree. We add a unique
    // number to these entries to ensure they are left untouched.
    // See also https://github.com/LibrePCB/LibrePCB/issues/1286.
    if (valueStr.contains("PRODUCT_DEFINITION") ||
        valueStr.contains("REPRESENTATION")) {
      value.second.append(data.count() + 1);
    } else {
      value.second.append(0);
    }
    data.insert(id, value);
  }

  // Eliminate duplicate data items.
  QHash<Value, int> uniqueData;
  QHash<int, int> idMap;
  while (true) {
    uniqueData.clear();
    idMap.clear();
    for (auto it = data.begin(); it != data.end(); it++) {
      auto newIt = uniqueData.find(it.value());
      if (newIt == uniqueData.end()) {
        newIt = uniqueData.insert(it.value(), uniqueData.count() + 1);
      }
      idMap.insert(it.key(), newIt.value());
    }
    if (uniqueData.count() == data.count()) {
      break;
    }
    data.clear();
    for (auto it = uniqueData.begin(); it != uniqueData.end(); it++) {
      data.insert(it.value(), it.key());
    }
    for (auto& value : data) {
      for (auto& id : value.second) {
        id = idMap.value(id, id);
      }
    }
  }

  // Build new STEP file.
  QByteArray output;
  output += headerLines.join('\n').toUtf8() + '\n';
  for (auto it = data.begin(); it != data.end(); it++) {
    output += QString("#%1=").arg(it.key()).toUtf8();
    for (int i = 0; i < it.value().first.count(); ++i) {
      output += it.value().first.at(i).toUtf8();
      if (i < (it.value().second.count() - 1)) {  // Ignore unique ID.
        output += QString::number(it.value().second.at(i)).toUtf8();
      }
    }
    output += ";\n";
  }
  output += footerLines.join('\n').toUtf8() + '\n';
  qDebug() << "Minified STEP file from" << (content.size() / 1024.0) << "kB to"
           << (output.size() / 1024.0) << "kB in" << timer.elapsed() << "ms.";
  return output;
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void OccModel::initOpenCascade() {
#if USE_OPENCASCADE
  auto initOnce = []() {
    // Make console output less verbose.
    if (!sOutputVerbosityConfigured) {
      setVerboseOutput(false);
    }

    // Apply global settings.
    XCAFDoc_ShapeTool::SetAutoNaming(false);
    BRepBuilderAPI::Precision(1.0e-6);
    return true;
  };
  static const bool done = initOnce();
  Q_UNUSED(done);
#endif
}

QString OccModel::cleanString(const QString& str) {
  const QString validChars("-a-zA-Z0-9_+/!?<>(){}.|&@# :");
  return str.normalized(QString::NormalizationForm_KD)
      .remove(QRegularExpression(QString("[^%1]").arg(validChars)));
}

void OccModel::throwNotAvailable() {
  throw LogicError(__FILE__, __LINE__,
                   "Attempted to work with STEP file, but LibrePCB was "
                   "compiled without OpenCascade.");
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
