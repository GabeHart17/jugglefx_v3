#include <iostream>
#include <string>
#include <iomanip>
#include <sstream>
#include <list>
#include <random>

#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/videoio.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/core/types.hpp>

#include "locator.hpp"
#include "particle.hpp"

std::vector<cv::Point2f> get_points(cv::Mat&);

int main(int argc, char const *argv[]) {
  std::stringstream conv;
  conv << argv[1];
  int n_objects;
  conv >> n_objects;
  const std::string source = argv[2];
  int frameCounter = -1;
  int delay = 5;

  const std::string particle_path = argv[3];
  cv::Mat particle_image = cv::imread(particle_path, cv::IMREAD_COLOR);

  cv::VideoCapture capt(source);
  capt.set(cv::CAP_PROP_FPS, 60);
  int fps = (int) capt.get(cv::CAP_PROP_FPS);
  // delay = 1000 / fps;
  std::cout << "fps: " << fps << std::endl;

  if(!capt.isOpened()) {
    std::cout << "could not open source file" << std::endl;
    return -1;
  }

  capt.set(cv::CAP_PROP_FRAME_WIDTH, 1920);
  capt.set(cv::CAP_PROP_FRAME_HEIGHT, 1080);
  cv::Size size = cv::Size((int) capt.get(cv::CAP_PROP_FRAME_WIDTH), (int) capt.get(cv::CAP_PROP_FRAME_HEIGHT));
  std::cout << "size: " << size << std::endl;

  int original_brightness = capt.get(cv::CAP_PROP_BRIGHTNESS);

  const char* WIN_NAME = "output window";

  cv::namedWindow(WIN_NAME, cv::WINDOW_NORMAL & cv::WINDOW_FREERATIO);
  // cv::setWindowProperty(WIN_NAME, cv::WND_PROP_FULLSCREEN, cv::WINDOW_FULLSCREEN);

  int threshold = 225;
  // double downscale = 0.2;
  double downscale = 0.05;
  // double downscale = 1.0;
  double out_scale = 1.0;
  double to_out_ratio = out_scale / downscale;
  cv::Mat frame;
  cv::Mat output;
  cv::Mat gray;
  cv::Mat processed;
  cv::Mat small;
  KMeansLocator tracker(
    n_objects,
    100,
    2,
    5
  );
  std::vector<cv::Point2f> centers;

  std::mt19937 prng;
  std::uniform_real_distribution<> angle_dist(0.0, 360.0);
  std::uniform_real_distribution<> velocity_dist(0.0, 10.0);
  std::normal_distribution<> size_dist(80.0, 20.0);

  int max_particle_age = 10;

  // CircleTextureProvider provider(10, cv::Scalar(255,128,0,0));
  ImageTextureProvider provider(particle_image);
  // IntensityProgressionUpdater intensity_updater(1.025, 0.95);
  IntensityProgressionUpdater intensity_updater(1.1, 0.80);
  // IntensityProgressionUpdater intensity_updater(1.05, 1.0);
  std::list<Particle> particles;

  while (1) {
    capt >> frame;
    if (frame.empty()) {
      std::cout << "empty frame" << std::endl;
      break;
    }
    frameCounter++;

    cv::resize(
      frame,
      output,
      cv::Size(0,0),
      out_scale,
      out_scale,
      cv::INTER_LINEAR
    );

    cv::cvtColor(
      frame,
      gray,
      cv::COLOR_BGR2GRAY
    );

    cv::resize(
      gray,
      small,
      cv::Size(0,0),
      downscale,
      downscale,
      cv::INTER_LINEAR
    );

    cv::threshold(
      small,
      processed,
      threshold,
      255,
      cv::THRESH_BINARY
    );

    centers = tracker.track(processed);
    for (int i = 0; i < centers.size(); i++) {
      float x = centers[i].x * output.cols;
      float y = centers[i].y * output.rows;
      // std::cout << centers[i] << " " << x << " " << y << std::endl;
      // cv::Scalar color = CV_RGB(0,255,0);
      // cv::Scalar color(0,255,0,0);
      // cv::circle(output, cv::Point(x,y), 10, color, cv::FILLED);
      // ParticleUpdater* updater = new AllocationWrapperUpdater{&intensity_updater};
      // ParticleUpdater* updater = new IntensityProgressionUpdater (1.025, 0.95);
      float start_size = size_dist(prng);
      Particle particle(
        cv::Point2f(x,y),
        cv::Point2f(start_size,start_size),
        angle_dist(prng),
        1.0,
        &provider,
        &intensity_updater
      );
      particles.push_back(particle);
    }

    for (auto it = particles.begin(); it != particles.end();) {
      it->update();
      if (it->age > max_particle_age) {
        it = particles.erase(it);
      } else {
        it->render(output);
        it++;
      }
    }

    cv::imshow(WIN_NAME, output);

    int old_threshold = threshold;
    int brightness = capt.get(cv::CAP_PROP_BRIGHTNESS);
    int old_brightness = brightness;
    char c = (char) cv::waitKey(delay);
    if (c==27) break;  // ESC
    else if (c=='[') threshold -= 5;  // [
    else if (c==']') threshold += 5;  // ]
    else if (c==',') brightness -= 5;  // <
    else if (c=='.') brightness += 5;  // >
    else if ('0' < c && c <= '9') {
      n_objects = c - '0';
      tracker.set_n_objects(n_objects);
      std::cout << "objects: " << n_objects << std::endl;
    }
    if (threshold > 255) threshold = 255;
    if (threshold < 0) threshold = 0;
    if (brightness > 255) brightness = 255;
    if (brightness < 0) brightness = 0;
    if (old_brightness != brightness) capt.set(cv::CAP_PROP_BRIGHTNESS, brightness);
    if (old_threshold != threshold) {
      std::cout << "threshold: " << threshold << std::endl;
    }
  }

  capt.set(cv::CAP_PROP_BRIGHTNESS, original_brightness);

  return 0;
}
