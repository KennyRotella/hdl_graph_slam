// SPDX-License-Identifier: BSD-2-Clause

#include <hdl_graph_slam/information_matrix_calculator.hpp>

namespace hdl_graph_slam {

void InformationMatrixCalculator::print_parameters(){
  std::cout << "INF MATRIX CALCULATOR PARAMS" << std::endl;
  std::cout << "use_const_inf_matrix: " << use_const_inf_matrix << std::endl;
  std::cout << "const_stddev_x: " << const_stddev_x << std::endl;
  std::cout << "const_stddev_q: " << const_stddev_q << std::endl;
  std::cout << "var_gain_a: " << var_gain_a << std::endl;
  std::cout << "min_stddev_x: " << min_stddev_x << std::endl;
  std::cout << "max_stddev_x: " << max_stddev_x << std::endl;
  std::cout << "min_stddev_q: " << min_stddev_q << std::endl;
  std::cout << "max_stddev_q: " << max_stddev_q << std::endl;
  std::cout << "fitness_score_thresh: " << fitness_score_thresh << std::endl;
  std::cout << "b_var_gain_a: " << b_var_gain_a << std::endl;
  std::cout << "b_min_stddev_x: " << b_min_stddev_x << std::endl;
  std::cout << "b_max_stddev_x: " << b_max_stddev_x << std::endl;
  std::cout << "b_min_stddev_q: " << b_min_stddev_q << std::endl;
  std::cout << "b_max_stddev_q: " << b_max_stddev_q << std::endl;
  std::cout << "b_avg_fitness_score: " << b_avg_fitness_score << std::endl;
  std::cout << "b_importance_ratio_global: " << b_importance_ratio_global << std::endl;
  std::cout << "b_importance_ratio_local: " << b_importance_ratio_local << std::endl << std::endl;
}

InformationMatrixCalculator::InformationMatrixCalculator(ros::NodeHandle& nh) {
  use_const_inf_matrix = nh.param<bool>("use_const_inf_matrix", false);
  const_stddev_x = nh.param<double>("const_stddev_x", 0.5);
  const_stddev_q = nh.param<double>("const_stddev_q", 0.1);

  var_gain_a = nh.param<double>("var_gain_a", 20.0);
  min_stddev_x = nh.param<double>("min_stddev_x", 0.1);
  max_stddev_x = nh.param<double>("max_stddev_x", 5.0);
  min_stddev_q = nh.param<double>("min_stddev_q", 0.05);
  max_stddev_q = nh.param<double>("max_stddev_q", 0.2);
  fitness_score_thresh = nh.param<double>("fitness_score_thresh", 0.5);

  b_var_gain_a = nh.param<double>("delta_var_gain_a", 20.0);
  b_min_stddev_x = nh.param<double>("delta_min_stddev_x", 0.1);
  b_max_stddev_x = nh.param<double>("delta_max_stddev_x", 5.0);
  b_min_stddev_q = nh.param<double>("delta_min_stddev_q", 0.05);
  b_max_stddev_q = nh.param<double>("delta_max_stddev_q", 0.2);
  b_avg_fitness_score = nh.param<double>("delta_avg_fitness_score", 0.5);

  b_importance_ratio_global = nh.param<double>("delta_importance_ratio_global", 1.0);
  b_importance_ratio_local = nh.param<double>("delta_importance_ratio_local", 1.0);
}

InformationMatrixCalculator::~InformationMatrixCalculator() {}

Eigen::MatrixXd InformationMatrixCalculator::calc_information_matrix(const pcl::PointCloud<PointT>::ConstPtr& cloud1, const pcl::PointCloud<PointT>::ConstPtr& cloud2, const Eigen::Isometry3d& relpose) const {
  if(use_const_inf_matrix) {
    Eigen::MatrixXd inf = Eigen::MatrixXd::Identity(3, 3);
    inf.topLeftCorner(2, 2).array() /= const_stddev_x;
    inf.bottomRightCorner(1, 1).array() /= const_stddev_q;
    return inf;
  }

  double fitness_score = calc_fitness_score(cloud1, cloud2, relpose);

  double min_var_x = std::pow(min_stddev_x, 2);
  double max_var_x = std::pow(max_stddev_x, 2);
  double min_var_q = std::pow(min_stddev_q, 2);
  double max_var_q = std::pow(max_stddev_q, 2);

  float w_x = weight(var_gain_a, fitness_score_thresh, min_var_x, max_var_x, fitness_score);
  float w_q = weight(var_gain_a, fitness_score_thresh, min_var_q, max_var_q, fitness_score);

  Eigen::MatrixXd inf = Eigen::MatrixXd::Identity(3, 3);
  inf.topLeftCorner(2, 2).array() /= w_x;
  inf.bottomRightCorner(1, 1).array() /= w_q;
  return inf;
}

double InformationMatrixCalculator::calc_fitness_score(const pcl::PointCloud<PointT>::ConstPtr& cloud1, const pcl::PointCloud<PointT>::ConstPtr& cloud2, const Eigen::Isometry3d& relpose, double max_range) {
  pcl::search::KdTree<PointT>::Ptr tree_(new pcl::search::KdTree<PointT>());
  tree_->setInputCloud(cloud1);

  double fitness_score = 0.0;

  // Transform the input dataset using the final transformation
  pcl::PointCloud<PointT> input_transformed;
  pcl::transformPointCloud(*cloud2, input_transformed, relpose.cast<float>());

  std::vector<int> nn_indices(1);
  std::vector<float> nn_dists(1);

  // For each point in the source dataset
  int nr = 0;
  for(size_t i = 0; i < input_transformed.points.size(); ++i) {
    // Find its nearest neighbor in the target
    tree_->nearestKSearch(input_transformed.points[i], 1, nn_indices, nn_dists);

    // Deal with occlusions (incomplete targets)
    if(nn_dists[0] <= max_range) {
      // Add to the fitness score
      fitness_score += nn_dists[0];
      nr++;
    }
  }

  if(nr > 0)
    return (fitness_score / nr);
  else
    return (std::numeric_limits<double>::max());
}

Eigen::MatrixXd InformationMatrixCalculator::calc_information_matrix_buildings_global(double fitness_score) const {

  if(use_const_inf_matrix) {
    Eigen::MatrixXd inf = Eigen::MatrixXd::Identity(3, 3);
    inf.topLeftCorner(2, 2).array() /= const_stddev_x;
    inf.bottomRightCorner(1, 1).array() /= const_stddev_q;
    return inf;
  }

  double min_var_x = std::pow(min_stddev_x, 2);
  double max_var_x = std::pow(max_stddev_x, 2);
  double min_var_q = std::pow(min_stddev_q, 2);
  double max_var_q = std::pow(max_stddev_q, 2);

  float w_x = weight(var_gain_a, fitness_score_thresh, min_var_x, max_var_x, fitness_score);
  float w_q = weight(var_gain_a, fitness_score_thresh, min_var_q, max_var_q, fitness_score);

  Eigen::MatrixXd inf = Eigen::MatrixXd::Identity(3, 3);
  inf.topLeftCorner(2, 2).array() /= w_x;
  inf.bottomRightCorner(1, 1).array() /= w_q;

  return inf / b_importance_ratio_global;
}

Eigen::MatrixXd InformationMatrixCalculator::calc_information_matrix_buildings_local(BestFitAlignment result) const {

  double b_min_var_x = std::pow(b_min_stddev_x, 2);
  double b_max_var_x = std::pow(b_max_stddev_x, 2);
  double b_min_var_q = std::pow(b_min_stddev_q, 2);
  double b_max_var_q = std::pow(b_max_stddev_q, 2);

  float w_x = b_weight(b_var_gain_a, b_avg_fitness_score, b_min_var_x, b_max_var_x, result.fitness_score.avg_distance);
  float w_q = b_weight(b_var_gain_a, b_avg_fitness_score, b_min_var_q, b_max_var_q, result.fitness_score.avg_distance);

  Eigen::MatrixXd inf = Eigen::MatrixXd::Identity(3, 3);
  inf.topLeftCorner(2, 2).array() /= w_x;
  inf.bottomRightCorner(1, 1).array() /= w_q;

  // edges carry more information than single lines
  if(result.isEdgeAligned){
    inf *= b_importance_ratio_local;
  }

  // information based on the overall coverage of the building
  inf *= result.fitness_score.coverage_percentage / 100.;

  return inf;
}

}  // namespace hdl_graph_slam
