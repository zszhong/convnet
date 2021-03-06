#ifndef OPTIMIZER_H_
#define OPTIMIZER_H_
#include "util.h"
#include "matrix.h"

// Base class for all optimizers.
class Optimizer {
 public:
  Optimizer(const config::Optimizer& optimizer_config);
  ~Optimizer();

  // Allocate any memory needed.
  virtual void AllocateMemory(const int rows, const int cols);

  // Do the optimizaion. This will update parameter.
  virtual void Optimize(Matrix& gradient, Matrix& parameter) = 0;
  virtual void ReduceLearningRate(float factor);

  // Load and Save gradient history so that optimization can be restarted if it
  // gets interrupted for some reason.
  virtual void LoadParameters(hid_t file, const string& prefix);
  virtual void SaveParameters(hid_t file, const string& prefix);

  static Optimizer* ChooseOptimizer(const config::Optimizer& config);

 protected:
  float GetDecayedEpsilon() const;
  void ApplyConstraints(Matrix& parameter);

  const config::Optimizer::Decay epsilon_decay_type_;
  float epsilon_, minimum_epsilon_;
  const int epsilon_decay_timescale_;
  const float l2_decay_, weight_norm_limit_, weight_norm_constraint_;
  int step_;
};

class SGDOptimizer : public Optimizer {
 public:
  SGDOptimizer(const config::Optimizer& optimizer_config);
  virtual void AllocateMemory(const int rows, const int cols);
  virtual void Optimize(Matrix& gradient, Matrix& parameter);
  virtual void LoadParameters(hid_t file, const string& prefix);
  virtual void SaveParameters(hid_t file, const string& prefix);

 protected:
  float GetMomentum() const;

  Matrix gradient_history_;
  const float gradient_clip_;

  // Hyperparams.
  const float initial_momentum_, final_momentum_;
  const int momentum_transition_timescale_;
};

class LBFGSOptimizer : public Optimizer {
 public:
  LBFGSOptimizer(const config::Optimizer& optimizer_config);
  virtual void AllocateMemory(const int rows, const int cols);
  virtual void Optimize(Matrix& gradient, Matrix& parameter);
  virtual void LoadParameters(hid_t file, const string& prefix);
  virtual void SaveParameters(hid_t file, const string& prefix);

 protected:
  Matrix q_, last_q_, last_w_;
  const int m_;
  vector<float> rho_, alpha_, beta_;
  vector<Matrix> s_, y_;
  int start_;

};
#endif
