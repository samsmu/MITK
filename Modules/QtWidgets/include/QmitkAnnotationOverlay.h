#ifndef AnnotationOverlay_h
#define AnnotationOverlay_h

#include <QObject>

#include <vector>
#include <array>

#include <vtkCornerAnnotation.h>

#include <QmitkRenderWindow.h>
#include <QmitkSetWLDialog.h>

#include <mitkDataStorage.h>
#include <mitkMouseModeSwitcher.h>

#include "MitkQtWidgetsExports.h"

class ActiveOverlayLineHandler;

class MITKQTWIDGETS_EXPORT AnnotationOverlay : public QObject
{
    Q_OBJECT
public:

    using TRenderWindows = std::vector<QmitkRenderWindow *>;
    using TRelationships = std::vector<uint32_t>;

    ~AnnotationOverlay();

    bool initialize(const TRenderWindows &, const TRelationships &, uint32_t fontSize);
    void deinitialize();

    void setDataStorage(mitk::DataStorage *ds);

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

    bool isContainMousePos(QPoint globalMousePos);

signals:
    void restoreActiveMode();
    void setActiveMode(mitk::MouseModeSwitcher::MouseMode mode);
    void setLightActiveMode(mitk::MouseModeSwitcher::MouseMode mode);

private:


    template<typename T>
    static constexpr std::underlying_type_t<T> enumToIntegral(T value)
    {
        return static_cast<std::underlying_type_t<T>>(value);
    }

    struct ActiveOverlayLine
    {
        enum class Type
        {
            WL = 0,
            Im = 1,
            Scale,
            Width,

            Count,
        };

        std::array<ActiveOverlayLineHandler *, enumToIntegral(Type::Count)> Handlers;
    };

    void setViewDirectionAnnontation(mitk::Image* image, int index);

    const mitk::Point3D getCrossPosition(const TRenderWindows &) const;

    void setCornerAnnotation(vtkCornerAnnotation::TextPosition , int i, const char *text);
    void setCornerAnnotationMaxText(vtkCornerAnnotation::TextPosition , int i, const char *text);

    ActiveOverlayLineHandler *createWLOverlay(vtkSmartPointer<vtkRenderer> vtkRender,
        QmitkRenderWindow *handledWidget, vtkCornerAnnotation::TextPosition corner,
        uint32_t fontSize, int lineNumber = 0);

    ActiveOverlayLineHandler *createImOverlay(vtkSmartPointer<vtkRenderer> vtkRender,
        QmitkRenderWindow *handledWidget, vtkCornerAnnotation::TextPosition corner,
        uint32_t fontSize, int lineNumber = 0);

    ActiveOverlayLineHandler *createZoomOverlay(vtkSmartPointer<vtkRenderer> vtkRender,
        QmitkRenderWindow *handledWidget, vtkCornerAnnotation::TextPosition corner,
        uint32_t fontSize, int lineNumber = 0);

    ActiveOverlayLineHandler *createWidthOverlay(vtkSmartPointer<vtkRenderer> vtkRender,
        QmitkRenderWindow *handledWidget, vtkCornerAnnotation::TextPosition corner,
        uint32_t fontSize, int lineNumber = 0);

    TRenderWindows m_renderWindows;
    TRelationships m_relationships;

    std::vector<ActiveOverlayLine> m_activeOverlayLineHandlers;
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

    mitk::DataStorage::Pointer m_dataStorage = nullptr;

    SetWLDialog m_tSetWL;
};

#endif //AnnotationOverlay_h