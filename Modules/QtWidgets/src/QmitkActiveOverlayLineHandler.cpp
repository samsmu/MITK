#include "QmitkActiveOverlayLineHandler.h"

#include <QDebug>
#include <QMouseEvent>

#include <mitkVtkLayerController.h>

ActiveOverlayLineHandler::ActiveOverlayLineHandler(vtkSmartPointer<vtkRenderer> vtkRener, 
    QmitkRenderWindow * handledWidget, vtkCornerAnnotation::TextPosition corner,
    mitk::MouseModeSwitcher::MouseMode mode, uint32_t fontSize,
    int numberLine, QObject *parent)
    : QObject(parent)
    , m_handledWidget(handledWidget)
    , m_corner(corner)
    , m_vtkRender(vtkRener)
    , m_numberLine(numberLine)
    , m_mode(mode)
{
    if (m_numberLine > 0)
    {
        std::fill_n(std::back_inserter(m_endls), m_numberLine, '\n');
    }

    m_cornerAnnotation = vtkCornerAnnotation::New();
    m_cornerAnnotation->SetMaximumFontSize(fontSize);

    m_vtkRender->AddActor(m_cornerAnnotation);
    m_clickEventTimer.setSingleShot(true);

    connect(&m_clickEventTimer, &QTimer::timeout, [this]
    {
        if (!m_isDoubleClicked)
        {
            emit needShowDialog();
        }

        m_isDoubleClicked = false;
    });

    m_textProperty = vtkTextProperty::New();
    m_textProperty->SetFontFamilyToArial();
    m_textProperty->SetColor(0, 0, 1);
    m_textProperty->SetBold(false);
    m_cornerAnnotation->SetTextProperty(m_textProperty);
    m_cornerAnnotation->QueueFontUpdate();

    QTimer * activeOverlay = new QTimer;
    activeOverlay->start(50);

    connect(activeOverlay, &QTimer::timeout, [this]
    {
        changeActiveElements(QCursor::pos());
    });

    m_handledWidget->installEventFilter(this);
}

void ActiveOverlayLineHandler::addText(const std::string &text)
{
    std::string textWithEndls;
    if (!m_endls.empty())
    {
        if (m_corner == vtkCornerAnnotation::LowerLeft ||
            m_corner == vtkCornerAnnotation::LowerRight ||
            m_corner == vtkCornerAnnotation::LowerEdge)
        {
            textWithEndls = text + m_endls;
        }
        else
        {
            textWithEndls = m_endls + text;
        }
    }
    else
    {
        textWithEndls = text;
    }

    m_cornerAnnotation->SetText(m_corner, textWithEndls.c_str());
    m_cornerAnnotation->SetMaximumLengthText(m_corner, text.c_str());

    m_textLength = text.length();
}

bool ActiveOverlayLineHandler::eventFilter(QObject* object, QEvent* event)
{
    if (event->type() == QEvent::MouseButtonPress)
    {
        QMouseEvent *me = dynamic_cast<QMouseEvent *>(event);

        if (isContainMousePos(me->globalPos()))
        {
            m_clickEventTimer.start(1000);
        }
    }

    if (event->type() == QEvent::MouseButtonDblClick)
    {
        QMouseEvent *me = dynamic_cast<QMouseEvent *>(event);

        if (isContainMousePos(me->globalPos()))
        {
            m_isDoubleClicked = true;
            emit setActiveMode(m_mode);

            return true;
        }
        else
        {
            return false;
        }
    }

    return false;
}

bool ActiveOverlayLineHandler::isContainMousePos(QPoint globalMousePos)
{
    if (!m_handledWidget->underMouse())
    {
        return false;
    }

    int fontSize = m_textProperty->GetFontSize();
    int lineWidth = m_textLength * fontSize / 1.2;
    int heigtLine = fontSize + 4;
    int diffLinesSumm = heigtLine * m_numberLine;

    int globalX = globalMousePos.x();
    int globalWidgetX = m_handledWidget->mapToGlobal(QPoint(0, 0)).x();
    int lLeft = 0;
    int lRight = lineWidth;

    int globalY = globalMousePos.y();
    int globalWidgetY = m_handledWidget->mapToGlobal(QPoint(0, 0)).y();
    int hBottom = m_handledWidget->height() - diffLinesSumm - heigtLine;
    int hTop = m_handledWidget->height() - diffLinesSumm;

    if (m_corner == vtkCornerAnnotation::vtkCornerAnnotation::UpperRight
        || m_corner == vtkCornerAnnotation::vtkCornerAnnotation::LowerRight)
    {
        lLeft = m_handledWidget->width() - lineWidth;
        lRight = m_handledWidget->width();
    }

    if (m_corner == vtkCornerAnnotation::vtkCornerAnnotation::UpperLeft
        || m_corner == vtkCornerAnnotation::vtkCornerAnnotation::UpperRight)
    {
        hBottom = diffLinesSumm;
        hTop = diffLinesSumm + heigtLine;
    }

    if (globalY > globalWidgetY + hBottom && globalY < globalWidgetY + hTop &&
        globalX > globalWidgetX + lLeft && globalX < globalWidgetX + lRight)
    {
        return true;
    }
    else
    {
        return false;
    }
}

void ActiveOverlayLineHandler::changeActiveElements(QPoint globalMousePos)
{
    bool newValue = false;

    if (m_isContainMousePos = isContainMousePos(globalMousePos))
    {
        emit lightActiveMode(m_mode);
        if (!m_wasWLActive)
        {
            newValue = true;
        }

        m_wasWLActive = true;
        if (newValue)
        {
            m_textProperty->SetBold(true);
            updateOverlay();
        }
    }
    else
    {
        if (m_handledWidget->underMouse())
        {
            emit restoreActiveMode();
        }

        if (m_wasWLActive)
        {
            newValue = true;
        }

        m_wasWLActive = false;
        if (newValue)
        {
            m_textProperty->SetBold(false);
            updateOverlay();
        }
    }
}

void ActiveOverlayLineHandler::updateOverlay()
{
    mitk::VtkLayerController::GetInstance(m_handledWidget->GetRenderWindow())->UpdateLayers();
    m_handledWidget->GetRenderer()->RequestUpdate();
}

