/*===================================================================

The Medical Imaging Interaction Toolkit (MITK)

Copyright (c) German Cancer Research Center,
Division of Medical and Biological Informatics.
All rights reserved.

This software is distributed WITHOUT ANY WARRANTY; without
even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE.

See LICENSE.txt or http://www.mitk.org for details.

===================================================================*/

#include <mitkImageCast.h>
#include <mitkITKImageImport.h>
#include <mitkProperties.h>
#include <mitkImage.h>
#include <mitkIOUtil.h>
#include "mitkCommandLineParser.h"
#include <boost/lexical_cast.hpp>

// Caffe2
#include <google/protobuf/text_format.h>
#include "caffe2/core/context.h"
#include "caffe2/core/operator.h"
#include "caffe2/core/predictor.h"
#include "caffe2/core/tensor.h"
#include "caffe2/utils/math.h"
#include <caffe2/core/context_gpu.h>
#include <caffe2/core/workspace.h>

#include <gtest/gtest.h>

// OpenCV
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>

template<class TensorType>
void to_tensor(cv::Mat &img, TensorType &tensor)
{
    const int num_channel = tensor.dim(1);
    const int height = tensor.dim(2);
    const int width = tensor.dim(3);

    assert(img.channels() == num_channel);
    assert(img.cols == width);
    assert(img.rows == height);

    // convert to float
    if (num_channel==3 && img.type()!=CV_32FC3)
        img.convertTo(img, CV_32FC3);
    else if(num_channel==1 && img.type()!=CV_32FC1)
        img.convertTo(img, CV_32FC1);

    // subtract mean
    img -= 128;  // necessary?

    float* tensor_data = tensor.template mutable_data<float>();

    // create array of cv::Mat where each matrix points to the tensor data
    std::vector<cv::Mat> bgr;
    for (int i = 0; i < num_channel; ++i)
    {
        bgr.push_back(cv::Mat(height, width, CV_32FC1, tensor_data));
        tensor_data += width * height;
    }

    // HWC to CHW
    // splits 3 channel matrix into three single channel matrices
    cv::split(img, bgr);
}

/*!
* \brief Minimal example for Caffe2 prediction in MITK
*
*/
int main(int argc, char* argv[])
{
  mitkCommandLineParser parser;
  parser.setTitle("Caffe2Test");
  parser.setCategory("Fiber Tracking and Processing Methods");
  parser.setContributor("MIC");
  parser.setDescription("Minimal example for Caffe2 prediction in MITK (Imagenet)");
  parser.setArgumentPrefix("--", "-");
  parser.addArgument("image", "", mitkCommandLineParser::String, "image:", "", us::Any());
  parser.addArgument("init_net", "", mitkCommandLineParser::String, "init_net:", "the network weights (.pb)", us::Any());
  parser.addArgument("predict_net", "", mitkCommandLineParser::String, "predict_net:", "the network definition (.pb)", us::Any());

  typedef caffe2::TensorCPU TensorType;
  //typedef caffe2::TensorCUDA TensorType;

  std::map<std::string, us::Any> parsedArgs = parser.parseArguments(argc, argv);
  std::string init_net_filename = us::any_cast<std::string>(parsedArgs["init_net"]);
  std::string predict_net_filename = us::any_cast<std::string>(parsedArgs["predict_net"]);
  std::string image_filename = us::any_cast<std::string>(parsedArgs["image"]);

  cv::Mat img = cv::imread(image_filename, CV_LOAD_IMAGE_COLOR);
  img = cv::Mat(30,30,CV_32FC1,129);

//  cv::namedWindow( "Display window", cv::WINDOW_AUTOSIZE );// Create a window for display.
//  cv::imshow( "Display window", img2 );                   // Show our image inside it.
//  cv::waitKey(0);                                          // Wait for a keystroke in the window

  caffe2::NetDef init_net, predict_net;
  CAFFE_ENFORCE(ReadProtoFromFile(init_net_filename, &init_net));
  CAFFE_ENFORCE(ReadProtoFromFile(predict_net_filename, &predict_net));

  // Can be large due to constant fills
  MITK_INFO << "Predict net: " << ProtoDebugString(predict_net);
  std::vector<TensorType*> outputVec;

  int w,h,c; w=30; h=30; c=1;
  TensorType input(std::vector<int>({1, c, h, w}));
  to_tensor<TensorType>(img, input);

  input.mutable_data<float>();

  std::vector<TensorType> inputVec{input};

  caffe2::Workspace ws;
  ws.RunNetOnce(init_net);
  ws.CreateNet(predict_net, true);

  for (auto i = 0; i < inputVec.size(); ++i)
  {
    auto* blob = ws.GetBlob(predict_net.external_input(i));
    TensorType* tensor = blob->template GetMutable<TensorType>();
    tensor->ResizeLike(inputVec[i]);
    tensor->ShareData(inputVec[i]);
  }

  CAFFE_ENFORCE(ws.RunNet(predict_net.name()));

  outputVec.resize(predict_net.external_output_size());
  for (auto i = 0; i < outputVec.size(); ++i)
  {
    auto* blob = ws.GetBlob(predict_net.external_output(i));
    outputVec[i] = blob->template GetMutable<TensorType>();
  }

//  auto predictor = caffe2::make_unique<caffe2::Predictor>(init_net, predict_net);
//  predictor->run(inputVec, &outputVec);

  TensorType* output = outputVec[0];

  const float* output_data =  output->data<float>();
  int max_class = -1;
  float max_prob = 0;
  for (int i=0; i<output->size(); i++)
      if(output_data[i]>max_prob)
      {
          max_prob = output_data[i];
          max_class = i;
      }
  MITK_INFO << "Class: " << max_class;
  MITK_INFO << "Probability: " << max_prob;

  return EXIT_SUCCESS;
}
