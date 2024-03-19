#ifndef TRACKER_HEADER
#define TRACKER_HEADER

#include <vector>
#include <queue>

#include <opencv2/core.hpp>
#include <opencv2/core/types.hpp>

class AssignmentSolver {
public:
  AssignmentSolver () {}
  virtual ~AssignmentSolver ();

  // result is index in previous to which current is assigned
  // negative index means no assignment
  virtual std::vector<int> assign(
    std::vector<cv::Point2f>& previous,
    std::vector<cv::Point2f>& current
  );
};


class BruteForceAssignmentSolver : public AssignmentSolver {
private:
  double square_dist(cv::Point2f& a, cv::Point2f& b) {
    cv::Point2f diff = a - b;
    return a.ddot(b);
  }

public:
  BruteForceAssignmentSolver() {}
  virtual ~BruteForceAssignmentSolver() {}
  std::vector<int> assign(
    std::vector<cv::Point2f>& previous,
    std::vector<cv::Point2f>& current
  ) override {
    std::vector<cv::Point2f> result;
    //TODO
  }
};


class Tracker {
private:
  AssignmentSolver* solver_;
  int max_skip_;
  int max_history_;
  int max_dist_;

public:
  std::vector<std::queue<cv::Point2f>> tracks;

  Tracker (
    AssignmentSolver* solver;
    int max_skip,
    int max_history
  ):solver_{solver},max_skip_{max_skip},max_history_{max_history} {}
  virtual ~Tracker () {}

  std::vector<cv::Point2f> latest() {
    std::vector<cv::Point2f> result;
    for (auto it = tracks.begin(); it != tracks.end(); it++) {
      result.push_back(it->back());
    }
    return result;
  }

  void track(std::vector<cv::Point2f>& positions) {
    std::vector<cv::Point2f> latest = latest_();
    std::vector<int> assignment = solver_->assign(latest_, positions);
    for (int i = 0; i < positions.size(); i++) {
      if (assignment[i] < 0) {
        std::queue<cv::Point2f> new_track;
        new_track.push(positions[i]);
        tracks.push_back(new_track);
      } else {
        tracks[assignment[i]].push_back(positions[i]);
        if (tracks[i].size() > max_history_) {
          tracks[i].pop();
        }
      }
    }
  }
};

#endif
