#ifndef __vtkMitkBlurFilter_h
#define __vtkMitkBlurFilter_h

#include "vtkImagingGeneralModule.h" // For export macro
#include "vtkSimpleImageToImageFilter.h"

#include <MitkCoreExports.h>

class MITKCORE_EXPORT vtkMitkBlurSharpnessFilter : public vtkSimpleImageToImageFilter
{
public:
    static vtkMitkBlurSharpnessFilter* New();

    vtkTypeMacro(vtkMitkBlurSharpnessFilter, vtkSimpleImageToImageFilter);

    void setBlurSharpness(int blurSharpness);

protected:
    vtkMitkBlurSharpnessFilter() = default;
    ~vtkMitkBlurSharpnessFilter() override = default;

    void SimpleExecute(vtkImageData* input, vtkImageData* output) override;

private:
    vtkMitkBlurSharpnessFilter(const vtkMitkBlurSharpnessFilter&) = delete;
    void operator=(const vtkMitkBlurSharpnessFilter&) = delete;

    template <class IT>
    void filterExecute(
        vtkImageData* input, vtkImageData* output, IT* inPtr, IT* outPtr);
    
    int m_blurSharpness = 0;
};

#endif
