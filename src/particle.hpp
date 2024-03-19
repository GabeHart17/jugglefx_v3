#ifndef PARTICLE_HEADER
#define PARTICLE_HEADER

#include <list>

#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/core/types.hpp>


class Particle;

class TextureProvider {
public:
  TextureProvider () {}
  virtual ~TextureProvider () {}
  virtual cv::Mat get() = 0;
};


class CircleTextureProvider : public TextureProvider {
private:
  int radius_;
  cv::Scalar color_;

public:
  CircleTextureProvider (int radius, cv::Scalar color):radius_{radius},color_{color} {}
  virtual ~CircleTextureProvider () {}
  cv::Mat get() override {
    cv::Mat result = cv::Mat::zeros(radius_*2, radius_*2, CV_8UC3);
    cv::circle(result, cv::Point2f(radius_,radius_),radius_,color_,cv::FILLED);
    return result;
  }
};

class ImageTextureProvider : public TextureProvider {
private:
  cv::Mat img_;
public:
  ImageTextureProvider (cv::Mat& source):img_{source} {}
  virtual ~ImageTextureProvider() {}
  cv::Mat get() override {
    return img_;
  }
};


class ParticleUpdater {
public:
  ParticleUpdater() {}
  virtual ~ParticleUpdater() {}
  virtual void update(Particle& particle) {}
};


class Particle {
private:
  TextureProvider* texture_provider_;
  ParticleUpdater* updater_;

  cv::Mat transformed() {
    cv::Mat sprite = texture_provider_->get();
    cv::Point2f sprite_size(sprite.size().width, sprite.size().height);
    cv::Mat rot = cv::getRotationMatrix2D(sprite_size * 0.5, rotation, 1.0);
    cv::Mat rotated;
    cv::warpAffine(sprite, rotated, rot, cv::Size(sprite.rows, sprite.cols));
    cv::Mat output;
    cv::Size the_size(static_cast<int>(size.x),static_cast<int>(size.y));
    cv::resize(rotated, output, the_size);
    return output;
  }

public:
  int age = 0;
  cv::Point2f location;
  cv::Point2f size;
  float rotation;
  float alpha;

  Particle (
    cv::Point2f location,
    cv::Point2f size,
    float rotation,
    float alpha,
    TextureProvider* texture_provider,
    ParticleUpdater* updater
  ):location{location},size{size},rotation{rotation},alpha{alpha},
    texture_provider_{texture_provider},updater_{updater} {}

  virtual ~Particle () {
    // delete updater_;
  }

  void update() {
    age++;
    updater_->update(*this);
  }

  void render(cv::Mat& canvas) {
    cv::Mat sprite = transformed();
    cv::Point2f start = location - 0.5*size;
    cv::Point2f end = start + size;
    cv::Rect sprite_restriction(0,0,sprite.rows,sprite.cols);
    if (start.x < 0) {
      sprite_restriction.x = -start.x;
      sprite_restriction.width += start.x;
      start.x = 0;
    }
    if (start.y < 0) {
      sprite_restriction.y = -start.y;
      sprite_restriction.height += start.y;
      start.y = 0;
    }
    if (end.x > canvas.cols) {
      sprite_restriction.width -= end.x - canvas.cols;
      end.x = canvas.cols;
    }
    if (end.y > canvas.rows) {
      sprite_restriction.height -= end.y - canvas.rows;
      end.y = canvas.rows;
    }
    sprite = sprite(sprite_restriction);
    cv::Mat dest = canvas(
      cv::Range(static_cast<int>(start.y), static_cast<int>(start.y) + sprite.rows),
      cv::Range(static_cast<int>(start.x), static_cast<int>(start.x) + sprite.cols)
    );
    cv::addWeighted(
      sprite,
      alpha,
      dest,
      1.0,
      0.0,
      dest
    );
  }
};

/*
encapsulated updaters must be heap-allocated
CompositeUpdater manages deletion of contained updaters
*/
// class CompositeUpdater : public ParticleUpdater {
// private:
//   std::list<ParticleUpdater*> updaters_;
//
// public:
//   CompositeUpdater() {}
//   virtual ~CompositeUpdater() {
//     for (auto it = updaters_.begin(); it != updaters_.end(); it++) {
//       delete *it;
//     }
//   }
//   void add_updater(ParticleUpdater* updater) {  // updater must be heap-allocated
//     updaters_.push_back(updater);
//   }
//   void update(Particle& particle) override {
//     for (auto it = updaters_.begin(); it != updaters_.end(); it++) {
//       (*it)->update(particle);
//     }
//   }
// };


// wrapper for using stack-allocated updater instances
// class AllocationWrapperUpdater : public ParticleUpdater {
// private:
//   ParticleUpdater* updater_;
// public:
//   AllocationWrapperUpdater (ParticleUpdater* updater):updater_{updater} {}
//   virtual ~AllocationWrapperUpdater() {}
//   void update(Particle& particle) override {
//     updater_->update(particle);
//   }
// };


class IntensityProgressionUpdater : public ParticleUpdater {
private:
  float size_rate_;
  float alpha_rate_;

public:
  IntensityProgressionUpdater(
    float size_rate,
    float alpha_rate
  ):size_rate_{size_rate},alpha_rate_{alpha_rate} {}
  virtual ~IntensityProgressionUpdater() {}
  void update(Particle& particle) override {
    particle.size *= size_rate_;
    particle.alpha *= alpha_rate_;
  }
};


class VelocityUpdater : public ParticleUpdater {
private:
  cv::Point2f velocity_;
  float progression_;

public:
  VelocityUpdater(
    cv::Point2f velocity_vector,
    float geometric_progression
  ):velocity_{velocity_vector},progression_{geometric_progression} {}
  virtual ~VelocityUpdater() {}
  void update(Particle& particle) override {
    particle.location += velocity_;
    velocity_ *= progression_;
  }
};

#endif
