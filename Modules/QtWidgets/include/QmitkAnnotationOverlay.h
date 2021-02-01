#ifndef AnnotationOverlay_h
#define AnnotationOverlay_h

#include <QObject>

#include <vector>

#include <vtkCornerAnnotation.h>

#include <QmitkRenderWindow.h>

#include <mitkDataStorage.h>

#include "MitkQtWidgetsExports.h"

class MITKQTWIDGETS_EXPORT AnnotationOverlay : public QObject
{
    Q_OBJECT
public:

    using TRenderWindows = std::vector<QmitkRenderWindow *>;
    using TRelationships = std::vector<uint32_t>;

    ~AnnotationOverlay();

    bool initialize(const TRenderWindows &, const TRelationships &, uint32_t fontSize);
    void deinitialize();

    mitk::DataNode::Pointer getNode(mitk::DataStorage::Pointer);
    bool render(mitk::DataNode::Pointer);

    mitk::DataNode::Pointer getTopLayerNode(mitk::DataStorage::SetOfObjects::ConstPointer nodes);

    void resetImageMTime();

    void setDisplayPositionInfo(bool);
    void setDisplayMetaInfo(bool);
    void setDisplayMetaInfoEx(bool);
    void setDisplayPatientInfo(bool);
    void setDisplayPatientInfoEx(bool);
    void setDisplayDirectionOnly(bool);

private:

    void setViewDirectionAnnontation(mitk::Image* image, int index);

    const mitk::Point3D getCrossPosition(const TRenderWindows &) const;

    void setCornerAnnotation(vtkCornerAnnotation::TextPosition , int i, const char *text);
    void setCornerAnnotationMaxText(vtkCornerAnnotation::TextPosition , int i, const char *text);

    TRenderWindows m_renderWindows;
    TRelationships m_relationships;

    std::vector<vtkCornerAnnotation *> m_cornerText;
    std::vector<vtkTextProperty *> m_textProp;
    std::vector<vtkRenderer *> m_ren;

    unsigned long m_imageMTime = 0;
    std::string m_imageName;

    uint32_t m_fontSize = 20;

    bool m_displayPositionInfo = true;
    bool m_displayMetaInfo = false;
    bool m_displayMetaInfoEx = false;
    bool m_displayPatientInfo = true;
    bool m_displayPatientInfoEx = false;
    bool m_displayDirectionOnly = false;
};

#endif //AnnotationOverlay_h