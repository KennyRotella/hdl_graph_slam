// SPDX-License-Identifier: BSD-2-Clause

#ifndef KEYFRAME_HPP
#define KEYFRAME_HPP

#include <ros/ros.h>
#include <pcl/point_types.h>
#include <pcl/point_cloud.h>
#include <boost/optional.hpp>
#include <hdl_graph_slam/building.hpp>
#include <hdl_graph_slam/ros_utils.hpp>


namespace g2o {
class VertexSE2;
class HyperGraph;
class SparseOptimizer;
}  // namespace g2o

namespace hdl_graph_slam {

/**
 * @brief KeyFrame (pose node)
 */
struct KeyFrame {
public:
  EIGEN_MAKE_ALIGNED_OPERATOR_NEW
  using PointT = pcl::PointXYZ;
  using Ptr = std::shared_ptr<KeyFrame>;

  KeyFrame(const ros::Time& stamp,
    const Eigen::Isometry3d& odom,
    const Eigen::Isometry2d& odom2D,
    const Eigen::Isometry2d& alignTrans_flatCloud_to_buildCloud,
    double accum_distance,
    const pcl::PointCloud<PointT>::ConstPtr& cloud,
    const pcl::PointCloud<PointT>::ConstPtr& flat_cloud,
    BestFitAlignment aligned_lines,
    std::vector<Building::Ptr> near_buildings);
  virtual ~KeyFrame();

  long id() const;
  Eigen::Isometry2d estimate() const;

public:
  ros::Time stamp;                                        // timestamp
  Eigen::Isometry3d odom;                                 // odometry (estimated by scan_matching_odometry)
  Eigen::Isometry2d odom2D;                               // odometry (used for pose graph 2D)
  Eigen::Isometry2d alignTrans_flatCloud_to_buildCloud;   // scan matching final pose between flat_cloud and buildings cloud
  double accum_distance;                                  // accumulated distance from the first node (by scan_matching_odometry)
  pcl::PointCloud<PointT>::ConstPtr cloud;                // point cloud
  pcl::PointCloud<PointT>::ConstPtr flat_cloud;           // 2D flattened point cloud
  BestFitAlignment aligned_lines;                         // 2D flattened point cloud lines
  std::vector<Building::Ptr> near_buildings;              // buildings found in proximity
  boost::optional<Eigen::Vector2d> utm_coord;             // UTM coord obtained by GPS

  g2o::VertexSE2* node;  // node instance
};


/**
 * @brief KeyFramesnapshot for map cloud generation
 */
struct KeyFrameSnapshot {
public:
  EIGEN_MAKE_ALIGNED_OPERATOR_NEW

  using PointT = KeyFrame::PointT;
  using Ptr = std::shared_ptr<KeyFrameSnapshot>;

  KeyFrameSnapshot(const KeyFrame::Ptr& key);
  KeyFrameSnapshot(const Eigen::Isometry3d& pose, const pcl::PointCloud<PointT>::ConstPtr& cloud);

  ~KeyFrameSnapshot();

public:
  Eigen::Isometry3d pose;                   // pose estimated by graph optimization
  pcl::PointCloud<PointT>::ConstPtr cloud;  // point cloud
};

}  // namespace hdl_graph_slam

#endif  // KEYFRAME_HPP
