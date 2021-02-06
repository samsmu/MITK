#ifndef QmitkActiveOverlayLineHandler_h
#define QmitkActiveOverlayLineHandler_h

#include <QObject>
#include <QTimer>

#include <vtkRenderer.h>
#include <vtkCornerAnnotation.h>
#include <vtkTextProperty.h>

#include <QmitkRenderWindow.h>
#include <mitkMouseModeSwitcher.h>

#include "MitkQtWidgetsExports.h"

class MITKQTWIDGETS_EXPORT ActiveOverlayLineHandler : public QObject
{
    Q_OBJECT

public:
    ActiveOverlayLineHandler(vtkSmartPointer<vtkRenderer> vtkRender, 
        QmitkRenderWindow *handledWidget, vtkCornerAnnotation::TextPosition corner, 
        mitk::MouseModeSwitcher::MouseMode mode,
        uint32_t fontSize, int lineNumber, QObject *parent = nullptr);

    bool isActive() { return m_isContainMousePos; };
    void addText(const std::string &text);
    bool isContainMousePos(QPoint globalMousePos);

protected:
    void changeActiveElements(QPoint globalMousePos);
    bool eventFilter(QObject* object, QEvent* event);
    void updateOverlay();

private:
    mitk::MouseModeSwitcher::MouseMode m_mode;
    vtkSmartPointer<vtkRenderer> m_vtkRender;
    vtkCornerAnnotation* m_cornerAnnotation;
    vtkTextProperty* m_textProperty;

    QmitkRenderWindow* m_handledWidget;
    QTimer m_clickEventTimer;

    std::string m_endls;

    int m_corner = 0;
    int m_numberLine = 0;
    int m_textLength = 0;

    bool m_isContainMousePos = false;
    bool m_wasWLActive = false;
    bool m_isDoubleClicked = false;

signals:
    void restoreActiveMode();
    void needShowDialog();
    void setActiveMode(mitk::MouseModeSwitcher::MouseMode mode);
    void lightActiveMode(mitk::MouseModeSwitcher::MouseMode mode);
};

#endif //QmitkActiveOverlayLineHandler_h