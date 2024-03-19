#ifndef TRACKER_HEADER
#define TRACKER_HEADER

#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/core/types.hpp>
#include <opencv2/video/tracking.hpp>


class KMeansLocator {
private:
  int n_objects_;
  int max_iter_;
  int epsilon_;
  int max_attempts_;
  cv::TermCriteria termCriteria_;
  cv::Mat cluster_labels_;
  cv::Mat cluster_centers_;
  std::vector<cv::Point2f> centers_;
  int mean_cluster_size_ = 0;

  std::vector<cv::Point2f> get_points(cv::Mat& img) {
    std::vector<cv::Point2f> result;
    for (int i = 0; i < img.cols; i++) {
      for (int j = 0; j < img.rows; j++) {
        int pixel = img.ptr<uchar>(j)[i];
        if (pixel) {
          result.push_back(cv::Point2f(i,j));
        }
      }
    }
    return result;
  }

public:
  KMeansLocator (int n_objects, int max_iter, int epsilon, int max_attemts):
  n_objects_{n_objects}, max_attempts_{max_attemts} {
    termCriteria_ = cv::TermCriteria(
      cv::TermCriteria::MAX_ITER,
      max_iter,
      epsilon
    );
  }

  virtual ~KMeansLocator () {}

  // k-means clustering on all non-zero pixels in single channel image
  // result returned as relative coordinates
  std::vector<cv::Point2f> track(cv::Mat& frame) {
    std::vector<cv::Point2f> points = get_points(frame);
    if (points.size() < n_objects_) {
      return std::vector<cv::Point2f>();
    }
    cv::kmeans(
      points,
      n_objects_,
      cluster_labels_,
      termCriteria_,
      max_attempts_,
      cv::KMEANS_PP_CENTERS,
      cluster_centers_
    );
    std::vector<cv::Point2f> result;
    for (int i = 0; i < n_objects_; i++) {
      float y = cluster_centers_.at<float>(i,1) / static_cast<float>(frame.rows);
      float x = cluster_centers_.at<float>(i,0) / static_cast<float>(frame.cols);
      result.push_back(cv::Point2f(x,y));
    }
    centers_ = result;
    mean_cluster_size_ = points.size() / n_objects_;
    return result;
  }

  // proportional coordinate centers from most recent call of track()
  std::vector<cv::Point2f> last_centers() {
    return centers_;
  }

  // mean cluster size from most recent call of track()
  int mean_cluster_size() {
    return mean_cluster_size_;
  }

  void set_n_objects(int n) {
    n_objects_ = n;
  }

  int get_n_objects() {
    return n_objects_;
  }
};

#endif
