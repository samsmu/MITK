#include "vtkMitkBlurSharpnessFilter.h"

#include "vtkImageData.h"
#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkMitkBlurSharpnessFilter);

template <class IT>
void vtkMitkBlurSharpnessFilter::filterExecute(
    vtkImageData* input, vtkImageData* output, IT* inPtr, IT* outPtr)
{
    int dims[3];
    input->GetDimensions(dims);
    if (input->GetScalarType() != output->GetScalarType())
    {
        vtkGenericWarningMacro(<< "Execute: input ScalarType, " << input->GetScalarType()
            << ", must match out ScalarType " << output->GetScalarType());
        return;
    }

    const auto width = dims[0];
    const auto height = dims[1];

    for (int i = 1; i < width - 1; i++)
    {
        for (int j = 1; j < height - 1; j++)
        {
            double value = 0.0;
            if (m_blurSharpness > 0)
            {
                value = input->GetScalarComponentAsFloat(i, j, 0, 0) * 3 - input->GetScalarComponentAsFloat(i - 1, j, 0, 0) - input->GetScalarComponentAsFloat(i + 1, j, 0, 0);
            }
            else if(m_blurSharpness < 0)
            {
                value = (input->GetScalarComponentAsFloat(i, j - 1, 0, 0) + input->GetScalarComponentAsFloat(i, j, 0, 0) + input->GetScalarComponentAsFloat(i, j + 1, 0, 0) +
                    input->GetScalarComponentAsFloat(i - 1, j - 1, 0, 0) + input->GetScalarComponentAsFloat(i - 1, j, 0, 0) + input->GetScalarComponentAsFloat(i - 1, j + 1, 0, 0) +
                    input->GetScalarComponentAsFloat(i + 1, j - 1, 0, 0) + input->GetScalarComponentAsFloat(i + 1, j, 0, 0) + input->GetScalarComponentAsFloat(i + 1, j + 1, 0, 0)
                    ) / 9;
            }
            else
            {
                value = input->GetScalarComponentAsFloat(i, j, 0, 0);
            }

            output->SetScalarComponentFromDouble(i, j, 0, 0, value);
        }
    }

}

void vtkMitkBlurSharpnessFilter::SimpleExecute(vtkImageData* input, vtkImageData* output)
{
    void* inPtr = input->GetScalarPointer();
    void* outPtr = output->GetScalarPointer();

    switch (output->GetScalarType())
    {
        // This is simply a #define for a big case list. It handles all
        // data types VTK supports.
        vtkTemplateMacro(filterExecute(
            input, output, static_cast<VTK_TT*>(inPtr), static_cast<VTK_TT*>(outPtr)));
    default:
        vtkGenericWarningMacro("Execute: Unknown input ScalarType");
        return;
    }
}

void vtkMitkBlurSharpnessFilter::setBlurSharpness(int blurSharpness)
{
    m_blurSharpness = blurSharpness;
}
