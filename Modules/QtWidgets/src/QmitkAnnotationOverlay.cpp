#include "QmitkAnnotationOverlay.h"

#include <sstream>

#include <vtkTextProperty.h>
#include <vtkRenderer.h>

#include <mitkNodePredicateDataType.h>
#include <mitkImage.h>
#include <mitkVtkLayerController.h>
#include <mitkLine.h>
#include <mitkLevelWindowProperty.h>

#include <PathUtilities.h>

AnnotationOverlay::~AnnotationOverlay()
{
    deinitialize();
}

bool AnnotationOverlay::initialize(const TRenderWindows &renderers, const TRelationships &relationships,
    uint32_t fontSize)
{
    if (renderers.size() != relationships.size())
    {
        return false;
    }

    for (auto relationship : relationships)
    {
        auto cornerText = vtkCornerAnnotation::New();

        auto vtkRen = vtkRenderer::New();
        vtkRen->AddActor(cornerText);
        vtkRen->InteractiveOff();

        mitk::VtkLayerController::GetInstance(renderers[relationship]->GetRenderWindow())->InsertForegroundRenderer(vtkRen, true);

        m_textProp.emplace_back(vtkTextProperty::New());
        m_cornerText.emplace_back(cornerText);
        m_ren.emplace_back(vtkRen);
    }

    for (uint32_t i = 0; i < renderers.size(); i++)
    {
        if (!renderers[i]->isWindow3d())
        {
            m_renderWindows.push_back(renderers[i]);
            m_relationships.push_back(relationships[i]);
        }
    }

    m_fontSize = fontSize;

    return true;
}

void AnnotationOverlay::deinitialize()
{
    for (uint32_t i = 0; i < m_renderWindows.size(); i++)
    {
        mitk::VtkLayerController::GetInstance(m_renderWindows[m_relationships[i]]->GetRenderWindow())->RemoveRenderer(m_ren[i]);
        m_cornerText[i]->Delete();
        m_textProp[i]->Delete();
        m_ren[i]->Delete();
    }

    m_renderWindows.clear();
    m_relationships.clear();
    m_cornerText.clear();
    m_ren.clear();
}

mitk::DataNode::Pointer AnnotationOverlay::getNode(mitk::DataStorage::Pointer dataStorage)
{
    if (dataStorage.IsNull() || m_renderWindows.empty())
    {
        return mitk::DataNode::Pointer();
    }

    // find image with highest layer
    mitk::TNodePredicateDataType<mitk::Image>::Pointer isImageData = mitk::TNodePredicateDataType<mitk::Image>::New();
    mitk::DataStorage::SetOfObjects::ConstPointer nodes = dataStorage->GetSubset(isImageData).GetPointer();

    return getTopLayerNode(nodes);
}

bool AnnotationOverlay::render(mitk::DataNode::Pointer node)
{
    mitk::Image::Pointer image;
    if (node.IsNotNull())
    {
        image = dynamic_cast<mitk::Image*>(node->GetData());
    }
    else
    {
        return false;
    }

    mitk::Point3D crosshairPos = getCrossPosition(m_renderWindows);
    std::stringstream stream;
    itk::Index<3> p;
    auto baseRenderer = m_renderWindows[0]->GetSliceNavigationController()->GetRenderer();
    unsigned int timestep = baseRenderer->GetTimeStep();
    auto timeSteps = image.IsNotNull() ? image->GetTimeSteps() : 0;
    unsigned int component = baseRenderer->GetComponent();
    unsigned int componentMax = image.IsNotNull() ? image->GetPixelType().GetNumberOfComponents() : 0;

    if (image.IsNotNull() && (timeSteps > timestep))
    {
        image->GetGeometry()->WorldToIndex(crosshairPos, p);
        stream.precision(2);

        std::vector<std::stringstream> _infoStringStream(m_renderWindows.size());
        std::vector<std::string> cornerimgtext(m_renderWindows.size());

        mitk::PropertyList::Pointer imageProperties = image->GetPropertyList();
        std::string seriesNumber;
        imageProperties->GetStringProperty("dicom.series.SeriesNumber", seriesNumber);

        auto secondaryProp = dynamic_cast<mitk::IntProperty*>(static_cast<mitk::BaseProperty*>(image->GetProperty("autoplan.secondaryAxisIndex")));
        auto tertiaryProp = dynamic_cast<mitk::IntProperty*>(static_cast<mitk::BaseProperty*>(image->GetProperty("autoplan.tertiaryAxisIndex")));
        auto mainProp = dynamic_cast<mitk::IntProperty*>(static_cast<mitk::BaseProperty*>(image->GetProperty("autoplan.mainAxisIndex")));

        int axisIndices[3] = 
        {
          secondaryProp ? secondaryProp->GetValue() : 0,
          tertiaryProp ? tertiaryProp->GetValue() : 1,
          mainProp ? mainProp->GetValue() : 2
        };

        std::vector<mitk::BaseRenderer *> baseRenderes;
        for (auto relationship : m_relationships)
        {
            baseRenderes.push_back(m_renderWindows[relationship]->GetSliceNavigationController()->GetRenderer());
        }

        for (int i = 0; i < m_renderWindows.size(); i++)
        {
            auto geometry = baseRenderes[i]->GetCurrentWorldPlaneGeometryNode();
            int thickslices = 0;
            geometry->GetIntProperty("reslice.thickslices.num", thickslices);
            thickslices = thickslices == 0 ? 1 : thickslices;

            const auto pos = baseRenderes[i]->GetSliceNavigationController()->GetSlice()->GetPos() + 1;
            const auto max = baseRenderes[i]->GetSliceNavigationController()->GetSlice()->GetSteps();

            if (m_displayPositionInfo && !m_displayDirectionOnly)
            {
                _infoStringStream[axisIndices[i]] << "Im: " << pos << "/" << max;
                if (timeSteps > 1)
                {
                    _infoStringStream[axisIndices[i]] << "\nT: " << (timestep + 1) << "/" << timeSteps;
                }
                if (componentMax > 1)
                {
                    _infoStringStream[axisIndices[i]] << "\nV: " << (component + 1) << "/" << componentMax;
                }
                if (seriesNumber != "")
                {
                    _infoStringStream[axisIndices[i]] << "\nSe: " << seriesNumber;
                }
                if (thickslices > 0)
                {
                    _infoStringStream[axisIndices[i]] << "\nWidth: " << thickslices;
                }
            }
            else
            {
                _infoStringStream[axisIndices[i]].clear();
            }

            cornerimgtext[axisIndices[i]] = _infoStringStream[axisIndices[i]].str();

            setCornerAnnotation(vtkCornerAnnotation::UpperLeft, axisIndices[i], cornerimgtext[axisIndices[i]].c_str());

            std::string maxPosText = "";

            maxPosText += "Im: " + std::to_string(pos) + "/" + std::to_string(max);
            if (timeSteps > 1)
            {
                maxPosText += "\nT: " + std::to_string(timeSteps) + "/" + std::to_string(timeSteps);
            }
            if (componentMax > 1)
            {
                maxPosText += "\nV: " + std::to_string(componentMax) + "/" + std::to_string(componentMax);
            }
            if (seriesNumber != "")
            {
                maxPosText += "\nSe: " + seriesNumber;
            }

            setCornerAnnotationMaxText(vtkCornerAnnotation::UpperLeft, axisIndices[i], maxPosText.c_str());
            // Left Right annotiation
            setViewDirectionAnnontation(image, axisIndices[i]);

            auto wlProperty = dynamic_cast<mitk::LevelWindowProperty*>(node->GetProperty("levelwindow"));

            if (wlProperty)
            {
                setCornerAnnotation(vtkCornerAnnotation::LowerLeft, axisIndices[i], wlProperty->GetValueAsString().c_str());
            }
        }

        bool value = false;
        node->GetBoolProperty("volumerendering", value);
        if (value)
        {
            std::string text = "";
            if (m_displayPositionInfo && timeSteps > 1)
            {
                text += "T: " + std::to_string(timestep + 1) + "/" + std::to_string(timeSteps);
            }
            setCornerAnnotation(vtkCornerAnnotation::UpperLeft, 3, text.c_str());

            std::string maxPosText = "";
            maxPosText += "T: " + std::to_string(timeSteps) + "/" + std::to_string(timeSteps);
            setCornerAnnotationMaxText(vtkCornerAnnotation::UpperLeft, 3, maxPosText.c_str());
        }

        unsigned long newImageMTime = image->GetMTime();
        std::string newNodeName = node->GetName();

        // check if image is changed or node is renamed
        if (m_imageMTime != newImageMTime || m_imageName != newNodeName)
        {
            m_imageMTime = newImageMTime;
            m_imageName = newNodeName;

            // seriesDescription replaced by newNodeName
            std::string patient, patientId,
                birthday, sex, institution, studyDate, studyTime,
                studiId/*, seriesDescription*/, studyDescription, exInfo,
                magneticFieldStrength, dicomTR, dicomTE, bodyPart, protocolName,
                sliceThickness, xrayTubeCurrent, kvp, imagePosition, windowCenter, windowWidth;

            imageProperties->GetStringProperty("dicom.patient.PatientsName", patient);
            imageProperties->GetStringProperty("dicom.patient.PatientID", patientId);
            imageProperties->GetStringProperty("dicom.patient.PatientsBirthDate", birthday);
            imageProperties->GetStringProperty("dicom.patient.PatientsSex", sex);
            imageProperties->GetStringProperty("dicom.study.InstitutionName", institution);
            imageProperties->GetStringProperty("dicom.acquisition.Date", studyDate);
            imageProperties->GetStringProperty("dicom.acquisition.Time", studyTime);
            if (studyDate.empty() || studyTime.empty())
            {
                imageProperties->GetStringProperty("dicom.study.StudyDate", studyDate);
                imageProperties->GetStringProperty("dicom.study.StudyTime", studyTime);
            }
            imageProperties->GetStringProperty("dicom.study.StudyID", studiId);
            imageProperties->GetStringProperty("dicom.study.StudyDescription", studyDescription);
            //imageProperties->GetStringProperty("dicom.series.SeriesDescription", seriesDescription);
            imageProperties->GetStringProperty("dicom.ExInfo", exInfo);
            imageProperties->GetStringProperty("dicom.MagneticFieldStrength", magneticFieldStrength);
            imageProperties->GetStringProperty("dicom.TR", dicomTR);
            imageProperties->GetStringProperty("dicom.TE", dicomTE);
            imageProperties->GetStringProperty("dicom.series.BodyPartExamined", bodyPart);
            imageProperties->GetStringProperty("dicom.series.ProtocolName", protocolName);
            imageProperties->GetStringProperty("dicom.SliceThickness", sliceThickness);
            imageProperties->GetStringProperty("dicom.XrayTubeCurrent", xrayTubeCurrent);
            imageProperties->GetStringProperty("dicom.Kvp", kvp);
            imageProperties->GetStringProperty("dicom.ImagePosition", imagePosition);
            imageProperties->GetStringProperty("dicom.voilut.WindowCenter", windowCenter);
            imageProperties->GetStringProperty("dicom.voilut.WindowWidth", windowWidth);

            auto it = imagePosition.rfind('\\');
            if (std::string::npos != it)
            {
                imagePosition.erase(0, it + 1);
            }

            std::stringstream infoStringStream[2];

            char yy[5]; yy[4] = 0;
            char mm[3]; mm[2] = 0;
            char dd[3]; dd[2] = 0;
            char hh[3]; hh[2] = 0;
            char mi[3]; mi[2] = 0;
            char ss[3]; ss[2] = 0;

            if (m_displayMetaInfo && !m_displayDirectionOnly)
            {
                if (m_displayPatientInfo)
                {
                    infoStringStream[0]
                        << patient.c_str()
                        << "\n" << patientId.c_str();
                }
                else
                {
                    infoStringStream[0] << "***\n***";
                }

                if (birthday != "")
                {
                    sscanf(birthday.c_str(), "%4c%2c%2c", yy, mm, dd);
                    infoStringStream[0] << "\n" << dd << "." << mm << "." << yy << " " << sex.c_str();
                }
                else
                {
                    infoStringStream[0] << '\n';
                }

                infoStringStream[0]
                    << "\n" << institution.c_str()
                    << "\n" << studiId;
                
                if (m_displayPatientInfoEx)
                {
                    infoStringStream[0]
                        << "\n" << studyDescription
                        << "\n" << newNodeName
                        << "\n" << exInfo
                        << "\n" << bodyPart
                        << "\n" << protocolName;
                }
            }
            else
            {
                infoStringStream[0].clear();
            }

            if (m_displayMetaInfo && !m_displayDirectionOnly)
            {
                if (m_displayMetaInfoEx)
                {
                    if (!magneticFieldStrength.empty())
                    {
                        infoStringStream[1]
                            << "FS: " << magneticFieldStrength;
                    }
                    if (!xrayTubeCurrent.empty() || !kvp.empty())
                    {
                        infoStringStream[1] << '\n';
                    }
                    if (!xrayTubeCurrent.empty())
                    {
                        infoStringStream[1] << xrayTubeCurrent << "mA";
                    }
                    if (!kvp.empty())
                    {
                        infoStringStream[1] << ' ' << kvp << " kV";
                    }
                    if (!dicomTR.empty() || !dicomTE.empty())
                    {
                        infoStringStream[1] << '\n';
                        if (!dicomTR.empty())
                        {
                            infoStringStream[1] << "TR: " << dicomTR;
                        }
                        if (!dicomTE.empty())
                        {
                            infoStringStream[1] << " TE: " << dicomTE;
                        }
                    }

                    if (!windowCenter.empty() && !windowWidth.empty())
                    {
                        infoStringStream[1] << '\n';
                        auto pos = windowCenter.find('\\');
                        if (std::string::npos != pos)
                        {
                            windowCenter.resize(pos);
                        }
                        pos = windowWidth.find('\\');
                        if (std::string::npos != pos)
                        {
                            windowWidth.resize(pos);
                        }
                        if (!windowCenter.empty())
                        {
                            infoStringStream[1] << "WL: " << windowCenter;
                        }
                        if (!windowWidth.empty())
                        {
                            infoStringStream[1] << " WW: " << windowWidth;
                        }
                    }
                    if (!sliceThickness.empty() || !imagePosition.empty())
                    {
                        infoStringStream[1] << '\n';
                    }
                    if (!sliceThickness.empty())
                    {
                        infoStringStream[1] << "T: " << sliceThickness << "mm";
                    }
                    if (!imagePosition.empty())
                    {
                        infoStringStream[1] << " L: " << imagePosition << "mm";
                    }
                }
                if (!studyDate.empty() && !studyTime.empty())
                {
                    sscanf(studyDate.c_str(), "%4c%2c%2c", yy, mm, dd);
                    sscanf(studyTime.c_str(), "%2c%2c%2c", hh, mi, ss);
                    infoStringStream[1]
                        << "\n" << dd << "." << mm << "." << yy
                        << " " << hh << ":" << mi << ":" << ss;
                }
            }
            else
            {
                infoStringStream[1].clear();
            }

            auto render_annotation = [&](int j, vtkCornerAnnotation::TextPosition corner)
            {
                const std::string infoString = infoStringStream[j].str();
                for (int i = 0; i < m_renderWindows.size(); i++)
                {
                    setCornerAnnotation(corner, i, infoString.c_str());
                    setCornerAnnotationMaxText(corner, i, infoString.c_str());
                }
            };

            render_annotation(0, vtkCornerAnnotation::UpperRight);
            render_annotation(1, vtkCornerAnnotation::LowerRight);

            for (auto renderer : m_renderWindows)
            {
                renderer->GetRenderer()->RequestUpdate();
            }
        }
    }

    return true;
}

mitk::DataNode::Pointer AnnotationOverlay::getTopLayerNode(mitk::DataStorage::SetOfObjects::ConstPointer nodes)
{
    mitk::Point3D crosshairPos = getCrossPosition(m_renderWindows);
    mitk::DataNode::Pointer node;
    int  maxlayer = -32768;

    if (nodes.IsNotNull())
    {
        mitk::BaseRenderer* baseRenderer = m_renderWindows[0]->GetSliceNavigationController()->GetRenderer();
        // find node with largest layer, that is the node shown on top in the render window
        for (unsigned int x = 0; x < nodes->size(); x++)
        {
            if ((nodes->at(x)->GetData()->GetGeometry() != NULL) &&
                nodes->at(x)->GetData()->GetGeometry()->IsInside(crosshairPos))
            {
                int layer = 0;
                if (!(nodes->at(x)->GetIntProperty("layer", layer)))
                {
                    continue;
                }

                bool isBinary = false;
                nodes->at(x)->GetBoolProperty("binary", isBinary);
                if (isBinary) continue;

                if (layer > maxlayer)
                {
                    if (static_cast<mitk::DataNode::Pointer>(nodes->at(x))->IsVisible(baseRenderer))
                    {
                        node = nodes->at(x);
                        maxlayer = layer;
                    }
                }
            }
        }
    }
    return node;
}

void AnnotationOverlay::resetImageMTime()
{
    m_imageMTime = -1;
}

void AnnotationOverlay::setDisplayPositionInfo(bool value)
{
    m_displayPositionInfo = value;
}

void AnnotationOverlay::setDisplayMetaInfo(bool value)
{
    m_displayMetaInfo = value;
}

void AnnotationOverlay::setDisplayMetaInfoEx(bool value)
{
    m_displayMetaInfoEx = value;
}

void AnnotationOverlay::setDisplayPatientInfo(bool value)
{
    m_displayPatientInfo = value;
}

void AnnotationOverlay::setDisplayPatientInfoEx(bool value)
{
    m_displayPatientInfoEx = value;
}

void AnnotationOverlay::setDisplayDirectionOnly(bool value)
{
    m_displayDirectionOnly = value;
}

void AnnotationOverlay::setViewDirectionAnnontation(mitk::Image* image, int i)
{
    auto mainAxis = 2;
    auto secondAxis = 0;
    auto tertiaryAxis = 1;

    auto mainSign = true;
    auto secondSign = true;
    auto tertiarySign = true;

    if (!m_displayMetaInfo)
    {
        mainAxis = secondAxis = -1;
    }
    else
    {
        switch (i)
        {
        case 0:
            std::swap(mainAxis, tertiaryAxis);
            std::swap(mainSign, tertiarySign);
            secondAxis = mainAxis;
            secondSign = mainSign;
            break;
        case 1:
            std::swap(mainAxis, tertiaryAxis);
            std::swap(mainSign, tertiarySign);
            break;
        }
    }

    switch (mainAxis)
    {
    case 0:
        setCornerAnnotation(vtkCornerAnnotation::UpperEdge, i, mainSign ? "I" : "S");
        setCornerAnnotation(vtkCornerAnnotation::LowerEdge, i, mainSign ? "S" : "I");
        break;
    case 1:
        setCornerAnnotation(vtkCornerAnnotation::UpperEdge, i, mainSign ? "S" : "I");
        setCornerAnnotation(vtkCornerAnnotation::LowerEdge, i, mainSign ? "I" : "S");
        break;
    case 2:
        setCornerAnnotation(vtkCornerAnnotation::UpperEdge, i, mainSign ? "A" : "P");
        setCornerAnnotation(vtkCornerAnnotation::LowerEdge, i, mainSign ? "P" : "A");
        break;
    default:
        setCornerAnnotation(vtkCornerAnnotation::UpperEdge, i, " ");
        setCornerAnnotation(vtkCornerAnnotation::LowerEdge, i, " ");
        break;
    }

    switch (secondAxis) 
    {
    case 0:
        setCornerAnnotation(vtkCornerAnnotation::RightEdge, i, secondSign ? "L" : "R");
        setCornerAnnotation(vtkCornerAnnotation::LeftEdge, i, secondSign ? "R" : "L");
        break;
    case 1:
        setCornerAnnotation(vtkCornerAnnotation::RightEdge, i, secondSign ? "P" : "A");
        setCornerAnnotation(vtkCornerAnnotation::LeftEdge, i, secondSign ? "A" : "P");
        break;
    case 2:
        setCornerAnnotation(vtkCornerAnnotation::RightEdge, i, secondSign ? "L" : "R");
        setCornerAnnotation(vtkCornerAnnotation::LeftEdge, i, secondSign ? "R" : "L");
        break;
    default:
        setCornerAnnotation(vtkCornerAnnotation::RightEdge, i, " ");
        setCornerAnnotation(vtkCornerAnnotation::LeftEdge, i, " ");
        break;
    }
}

const mitk::Point3D AnnotationOverlay::getCrossPosition(const TRenderWindows &baseRenders) const
{
    mitk::Point3D point;
    point.Fill(0);

    if (baseRenders.size() < 3)
    {
        return point;
    }

    std::vector<const mitk::PlaneGeometry *> planes;
    for (auto baseRender : baseRenders)
    {
        planes.push_back(baseRender->GetSliceNavigationController()->GetCurrentPlaneGeometry());
    }

    mitk::Line3D line;
    if ((planes[0] != nullptr) && (planes[1] != nullptr)
        && (planes[0]->IntersectionLine(planes[1], line)))
    {
        if ((planes[2] != nullptr)
            && (planes[2]->IntersectionPoint(line, point)))
        {
            return point;
        }
    }

    return point;
}

void AnnotationOverlay::setCornerAnnotation(vtkCornerAnnotation::TextPosition corner, int i, const char* text)
{
    std::string fontPath = Utilities::preferredPath(Utilities::absPath(std::string("Fonts\\DejaVuSans.ttf")));

    // empty or NULL string breaks renderer
    // and white square appears
    if ((text == nullptr) || (text[0] == 0))
    {
        text = " ";
    }

    m_cornerText[i]->SetMaximumFontSize(m_fontSize);
    m_textProp[i]->SetFontFamily(VTK_FONT_FILE);
    m_textProp[i]->SetFontFile(fontPath.c_str());
    m_textProp[i]->SetColor(1.0, 1.0, 7.0);
    m_cornerText[i]->SetTextProperty(m_textProp[i]);
    m_cornerText[i]->QueueFontUpdate();

    m_cornerText[i]->SetText(corner, text);
}

void AnnotationOverlay::setCornerAnnotationMaxText(vtkCornerAnnotation::TextPosition corner, int i, const char* text)
{
    m_cornerText[i]->SetMaximumLengthText(corner, text);
}