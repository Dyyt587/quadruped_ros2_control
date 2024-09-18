//
// Created by biao on 24-9-14.
//

#ifndef ESTIMATOR_H
#define ESTIMATOR_H
#include <memory>
#include <kdl/frames.hpp>
#include <unitree_guide_controller/common/mathTypes.h>

#include "LowPassFilter.h"

class QuadrupedRobot;
struct CtrlComponent;

class Estimator {
public:
    explicit Estimator(CtrlComponent &ctrl_component);

    ~Estimator() = default;

    /**
     * Get the estimated robot central position
     * @return robot central position
     */
    Vec3 getPosition() {
        return x_hat_.segment(0, 3);
    }

    /**
     * Get the estimated robot central velocity
     * @return robot central velocity
     */
    Vec3 getVelocity() {
        return x_hat_.segment(3, 3);
    }

    /**
     * Get the estimated foot position in world frame
     * @param index leg index
     * @return foot position in world frame
     */
    Vec3 getFootPos(const int index) {
        return getPosition() + Vec3((rotation_ * foot_poses_[index].p).data);
    }

    /**
     * Get the estimated foot position in body frame
     * @return
     */
    Vec34 getFootPos2Body() {
        Vec34 foot_pos;
        const Vec3 body_pos = getPosition();
        for (int i = 0; i < 4; i++) {
            foot_pos.col(i) = getFootPos(i) - body_pos;
        }
        return foot_pos;
    }

    KDL::Rotation getRotation() {
        return rotation_;
    }

    KDL::Vector getGyro() {
        return gyro_;
    }

    [[nodiscard]] KDL::Vector getGlobalGyro() const {
        return rotation_ * gyro_;
    }

    void update();

private:
    CtrlComponent &ctrl_component_;
    QuadrupedRobot &robot_model_;

    Eigen::Matrix<double, 18, 1> x_hat_; // The state of estimator, position(3)+velocity(3)+feet position(3x4)

    Eigen::Matrix<double, 3, 1> _u; // The input of estimator

    Eigen::Matrix<double, 28, 1> _y; // The measurement value of output y
    Eigen::Matrix<double, 28, 1> y_hat_; // The prediction of output y
    Eigen::Matrix<double, 18, 18> A; // The transtion matrix of estimator
    Eigen::Matrix<double, 18, 3> B; // The input matrix
    Eigen::Matrix<double, 28, 18> C; // The output matrix

    // Covariance Matrix
    Eigen::Matrix<double, 18, 18> P; // Prediction covariance
    Eigen::Matrix<double, 18, 18> Ppriori; // Priori prediction covariance
    Eigen::Matrix<double, 18, 18> Q; // Dynamic simulation covariance
    Eigen::Matrix<double, 28, 28> R; // Measurement covariance
    Eigen::Matrix<double, 18, 18> QInit_; // Initial value of Dynamic simulation covariance
    Eigen::Matrix<double, 28, 28> RInit_; // Initial value of Measurement covariance
    Eigen::Matrix<double, 18, 1> Qdig; // adjustable process noise covariance
    Eigen::Matrix<double, 3, 3> Cu; // The covariance of system input u

    // Output Measurement
    Eigen::Matrix<double, 12, 1> _feetPos2Body; // The feet positions to body, in the global coordinate
    Eigen::Matrix<double, 12, 1> _feetVel2Body; // The feet velocity to body, in the global coordinate
    Eigen::Matrix<double, 4, 1> _feetH; // The Height of each foot, in the global coordinate

    Eigen::Matrix<double, 28, 28> S; // _S = C*P*C.T + R
    Eigen::PartialPivLU<Eigen::Matrix<double, 28, 28> > Slu; // _S.lu()
    Eigen::Matrix<double, 28, 1> Sy; // _Sy = _S.inv() * (y - yhat)
    Eigen::Matrix<double, 28, 18> Sc; // _Sc = _S.inv() * C
    Eigen::Matrix<double, 28, 28> SR; // _SR = _S.inv() * R
    Eigen::Matrix<double, 28, 18> STC; // _STC = (_S.transpose()).inv() * C
    Eigen::Matrix<double, 18, 18> IKC; // _IKC = I - KC

    KDL::Vector g_;
    double _dt;

    KDL::Rotation rotation_;
    KDL::Vector acceleration_;
    KDL::Vector gyro_;

    std::vector<KDL::Frame> foot_poses_;
    std::vector<KDL::Vector> foot_vels_;
    std::vector<std::shared_ptr<LowPassFilter> > low_pass_filters_;

    double _largeVariance;
};


#endif //ESTIMATOR_H
