#include "libcohog.hpp"


libcohog::EvaluationResult libcohog::VerificationResult::to_eval() const
{
    EvaluationResult t;

    t.nTP  = std::count(TP_grouped_flags.begin(), TP_grouped_flags.end(), true);
    t.nFP  = std::count(TP_grouped_flags.begin(), TP_grouped_flags.end(), false);
    t.nFPW = std::count(TP_flags.begin(),         TP_flags.end(),         false);
    t.nWnd = total_windows;
    t.nImg = 1;

    t.nFN  = 0;
    for(int i = 0; i < ground_truth.size(); ++i)
        if(!found_flags[i] && ground_truth[i].confident)
            ++t.nFN;

    return t;
}

std::vector<cv::Rect> libcohog::thresholding(const std::vector<libcohog::Window>& windows, double th)
{
    std::vector<cv::Rect> result;
    for(unsigned i = 0; i < windows.size(); ++i)
        if(th <= windows[i].v)
            result.push_back(cv::Rect(windows[i].x, windows[i].y, windows[i].w, windows[i].h));
    return result;
}

std::vector<cv::Rect> libcohog::grouping(const std::vector<cv::Rect>& rects, int groupTh, double eps)
{
    std::vector<cv::Rect> result = rects;
    cv::groupRectangles(result, groupTh, eps);
    return result;
}

cv::Rect libcohog::normalize_rectangle(const cv::Rect& r, double height_to_width_ratio, double height_ratio)
{
    const int center_x  = r.x + r.width  / 2;
    const int center_y  = r.y + r.height / 2;
    const int height    = r.height / height_ratio;
    const int width     = height   / height_to_width_ratio;

    // Prevent to the height will be the odd number, using width*2 instead of the height
    return cv::Rect(center_x - width / 2, center_y - width, width, width * 2);
}

bool libcohog::is_equivalent(const cv::Rect& r1, const cv::Rect& r2, double overwrap_th)
{
    const int and_x1    = std::max(r1.x, r2.x);
    const int and_y1    = std::max(r1.y, r2.y);
    const int and_x2    = std::min(r1.x + r1.width,  r2.x + r2.width)  - 1;
    const int and_y2    = std::min(r1.y + r1.height, r2.y + r2.height) - 1;
    const int and_area  = (and_y2 - and_y1 + 1) * (and_x2 - and_x1 + 1);
    const int or_area   = r1.width * r1.height + r2.width * r2.height - and_area;

    // the relation between left-upper and right-bottom is inverse -> the rectangles are not crossing
    if(and_x2 <= and_x1 || and_y2 <= and_y1)
        return false;

    return overwrap_th <= 1.0 * and_area / or_area;
}

libcohog::VerificationResult libcohog::verify(const std::vector<libcohog::Window>& windows, const std::vector<libcohog::TruthRect>& normalized_truth, 
                                    double threshold, double overwrap_th, int groupTh, double eps)
{
    libcohog::VerificationResult result;

    result.total_windows    = windows.size();
    result.windows          = libcohog::thresholding(windows, threshold);
    result.windows_grouped  = libcohog::grouping(result.windows, groupTh, eps);
    result.ground_truth     = normalized_truth;

    result.TP_flags         = std::vector<bool>(result.total_windows,           false);
    result.TP_grouped_flags = std::vector<bool>(result.windows_grouped.size(),  false);
    result.found_flags      = std::vector<bool>(result.ground_truth.size(),     false);

    // Evaluation of non-grouped detection windows
    for(int i = 0; i < result.windows.size(); ++i)
    {
        const cv::Rect& r = result.windows[i];
        for(int j = 0; j < result.ground_truth.size(); ++j)
        {
            if(libcohog::is_equivalent(result.ground_truth[j].rect, r, overwrap_th))
            {
                result.TP_flags[i] = true;
                break;
            }
        }
    }

    // Evaluation of grouped detection windows
    for(int i = 0; i < result.windows_grouped.size(); ++i)
    {
        const cv::Rect& r = result.windows_grouped[i];
        for(int j = 0; j < result.ground_truth.size(); ++j)
        {
            if(!result.found_flags[j] && libcohog::is_equivalent(result.ground_truth[j].rect, r, overwrap_th))
            {
                // Mark this rectangle in the ground truth as detected correctly.
                result.found_flags[j] = true;

                // Mark this rectangle in the detection result as true positive
                result.TP_grouped_flags[i] = true;

                break;
            }
        }
    }

    return result;
}

libcohog::EvaluationResult libcohog::evaluate(const std::vector<libcohog::Window>& windows, const std::vector<libcohog::TruthRect>& normalized_truth, 
                                            double threshold, double overwrap_th, int groupTh, double eps)
{
    const VerificationResult result = libcohog::verify(windows, normalized_truth, threshold, overwrap_th, groupTh, eps);
    return result.to_eval();
}
