/**
 * validationlib.h
 * Polycarpo Souza Neto
 * Grouping common functions in reference methods in a single .h
 */

#include <limits>
#include <fstream>
#include <time.h>
#include <math.h>
#include <stdio.h>
#include <iostream>
#include <algorithm>
#include <string>
#include <vector>
#include <cstdlib>
#include <boost/make_shared.hpp>
#include <pcl/point_cloud.h>
#include <pcl/registration/transforms.h>
#include <pcl/io/pcd_io.h>
#include <pcl/point_types.h>
#include <pcl/visualization/common/common.h>
#include <pcl/sample_consensus/ransac.h>
#include <pcl/sample_consensus/sac_model_plane.h>
#include <Eigen/Core>
#include <pcl/kdtree/kdtree_flann.h>
#include <pcl/registration/ia_ransac.h>

typedef pcl::PointXYZ PointT;
typedef pcl::PointCloud<PointT> PointCloud;
typedef pcl::PointNormal PointNormalT;
typedef pcl::PointCloud<PointNormalT> PointCloudWithNormals;

#define PI 3.14159265

using namespace std;
typedef pcl::PointXYZ PointType;
pcl::PointXYZ PCA(PointCloud cloud, Eigen::Vector4f centroid)
{
    Eigen::Vector4f pcaCentroid;
    pcl::compute3DCentroid(cloud, pcaCentroid);
    Eigen::Matrix3f covariance;
    pcl::computeCovarianceMatrixNormalized(cloud, pcaCentroid, covariance);
    Eigen::SelfAdjointEigenSolver<Eigen::Matrix3f> eigen_solver(covariance, Eigen::ComputeEigenvectors);
    Eigen::Matrix3f eigenVectorsPCA = eigen_solver.eigenvectors();
    Eigen::Vector3f eigenValuesPCA = eigen_solver.eigenvalues();
    eigenVectorsPCA.col(2) = eigenVectorsPCA.col(0).cross(eigenVectorsPCA.col(1)); //correct vertical between main directions
    eigenVectorsPCA.col(0) = eigenVectorsPCA.col(1).cross(eigenVectorsPCA.col(2));
    eigenVectorsPCA.col(1) = eigenVectorsPCA.col(2).cross(eigenVectorsPCA.col(0));

    Eigen::Matrix4f tm = Eigen::Matrix4f::Identity();
    Eigen::Matrix4f tm_inv = Eigen::Matrix4f::Identity();
    tm.block<3, 3>(0, 0) = eigenVectorsPCA.transpose();   //R.
    tm.block<3, 1>(0, 3) = -1.0f * (eigenVectorsPCA.transpose()) *(pcaCentroid.head<3>());//  -R*t
    tm_inv = tm.inverse();

    std::cout << "Transformation matrix tm(4x4):\n" << tm << std::endl;
    std::cout << "inverter matrix tm'(4x4):\n" << tm_inv << std::endl;

    pcl::PointCloud<PointType>::Ptr transformedCloud(new pcl::PointCloud<PointType>);
    pcl::transformPointCloud(cloud, *transformedCloud, tm);

    PointType min_p1, max_p1;
    Eigen::Vector3f c1, c;
    pcl::getMinMax3D(*transformedCloud, min_p1, max_p1);
    c1 = 0.5f*(min_p1.getVector3fMap() + max_p1.getVector3fMap());

    std::cout << "Centre c1(3x1):\n" << c1 << std::endl;

    Eigen::Affine3f tm_inv_aff(tm_inv);
    pcl::transformPoint(c1, c, tm_inv_aff);

    Eigen::Vector3f whd, whd1;
    whd1 = max_p1.getVector3fMap() - min_p1.getVector3fMap();
    whd = whd1;
    float sc1 = (whd1(0) + whd1(1) + whd1(2)) / 3; //The average scale of the point cloud, used to set the size of the main direction arrow

    std::cout << "width1=" << whd1(0) << endl;
    std::cout << "heght1=" << whd1(1) << endl;
    std::cout << "depth1=" << whd1(2) << endl;
    std::cout << "scale1=" << sc1 << endl;

    const Eigen::Quaternionf bboxQ1(Eigen::Quaternionf::Identity());
    const Eigen::Vector3f    bboxT1(c1);

    const Eigen::Quaternionf bboxQ(tm_inv.block<3, 3>(0, 0));
    const Eigen::Vector3f    bboxT(c);


    //The main direction of the point cloud transformed to the origin
    PointType op;
    op.x = 0.0;
    op.y = 0.0;
    op.z = 0.0;
    Eigen::Vector3f px, py, pz;
    Eigen::Affine3f tm_aff(tm);
    pcl::transformVector(eigenVectorsPCA.col(0), px, tm_aff);
    pcl::transformVector(eigenVectorsPCA.col(1), py, tm_aff);
    pcl::transformVector(eigenVectorsPCA.col(2), pz, tm_aff);
    PointType pcaX;
    pcaX.x = sc1 * px(0);
    pcaX.y = sc1 * px(1);
    pcaX.z = sc1 * px(2);
    PointType pcaY;
    pcaY.x = sc1 * py(0);
    pcaY.y = sc1 * py(1);
    pcaY.z = sc1 * py(2);
    PointType pcaZ;
    pcaZ.x = sc1 * pz(0);
    pcaZ.y = sc1 * pz(1);
    pcaZ.z = sc1 * pz(2);


    //The main direction of the initial point cloud
    PointType cp;
    cp.x = pcaCentroid(0);
    cp.y = pcaCentroid(1);
    cp.z = pcaCentroid(2);
    PointType pcX;
    pcX.x = sc1 * eigenVectorsPCA(0, 0) + cp.x;
    pcX.y = sc1 * eigenVectorsPCA(1, 0) + cp.y;
    pcX.z = sc1 * eigenVectorsPCA(2, 0) + cp.z;
    PointType pcY;
    pcY.x = sc1 * eigenVectorsPCA(0, 1) + cp.x;
    pcY.y = sc1 * eigenVectorsPCA(1, 1) + cp.y;
    pcY.z = sc1 * eigenVectorsPCA(2, 1) + cp.z;
    PointType pcZ;
    pcZ.x = sc1 * eigenVectorsPCA(0, 2) + cp.x;
    pcZ.y = sc1 * eigenVectorsPCA(1, 2) + cp.y;
    pcZ.z = sc1 * eigenVectorsPCA(2, 2) + cp.z;
}
