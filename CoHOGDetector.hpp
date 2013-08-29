#pragma once

#include <opencv2/opencv.hpp>
#include <libcohog/Detector.hpp>
#include <libcohog/CoHOGParams.hpp>
#include <linear.h>

namespace libcohog
{

class CoHOGDetector: public Detector
{
    CoHOGParams param_cohog;

    std::vector<double> weights;

    int quantitize_gradient(int level, float th, int dx, int dy) const;
    cv::Mat_<unsigned char> calc_gradient_orientation_matrix(const cv::Mat_<unsigned char>& image, unsigned level, float th) const;

public:
    CoHOGDetector(const CoHOGParams& _param_cohog = CoHOGParams(), const ScanParams& _param_scan = ScanParams())
        :Detector(_param_scan),
         param_cohog(_param_cohog),
         weights(_param_cohog.dimension(), 0)
    {
    }

    void setDetector(const std::vector<double>& _weights);
    void setDetector(model *liblinear_model);
    void setDetector(const char* liblinear_model_file);

    std::vector<float> calculate_feature(const cv::Mat_<unsigned char>& img);
    std::vector<Window> detect(const cv::Mat_<unsigned char>& img);
};

}

