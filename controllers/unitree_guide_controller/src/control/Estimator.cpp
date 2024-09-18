//
// Created by biao on 24-9-14.
//

#include "unitree_guide_controller/control/Estimator.h"
#include "unitree_guide_controller/control/CtrlComponent.h"
#include "unitree_guide_controller/common/mathTools.h"

Estimator::Estimator(CtrlComponent &ctrl_component) : ctrl_component_(ctrl_component),
                                                      robot_model_(ctrl_component.robot_model_) {
    g_ = KDL::Vector(0, 0, -9.81);
    _dt = 0.002;
    _largeVariance = 100;
    for (int i(0); i < Qdig.rows(); ++i) {
        Qdig(i) = i < 6 ? 0.0003 : 0.01;
    }

    x_hat_.setZero();
    _u.setZero();

    A.setZero();
    A.block(0, 0, 3, 3) = I3;
    A.block(0, 3, 3, 3) = I3 * _dt;
    A.block(3, 3, 3, 3) = I3;
    A.block(6, 6, 12, 12) = I12;

    B.setZero();
    B.block(3, 0, 3, 3) = I3 * _dt;

    C.setZero();
    C.block(0, 0, 3, 3) = -I3;
    C.block(3, 0, 3, 3) = -I3;
    C.block(6, 0, 3, 3) = -I3;
    C.block(9, 0, 3, 3) = -I3;
    C.block(12, 3, 3, 3) = -I3;
    C.block(15, 3, 3, 3) = -I3;
    C.block(18, 3, 3, 3) = -I3;
    C.block(21, 3, 3, 3) = -I3;
    C.block(0, 6, 12, 12) = I12;
    C(24, 8) = 1;
    C(25, 11) = 1;
    C(26, 14) = 1;
    C(27, 17) = 1;

    P.setIdentity();
    P = _largeVariance * P;

    RInit_ << 0.008, 0.012, -0.000, -0.009, 0.012, 0.000, 0.009, -0.009, -0.000,
            -0.009, -0.009, 0.000, -0.000, 0.000, -0.000, 0.000, -0.000, -0.001,
            -0.002, 0.000, -0.000, -0.003, -0.000, -0.001, 0.000, 0.000, 0.000, 0.000,
            0.012, 0.019, -0.001, -0.014, 0.018, -0.000, 0.014, -0.013, -0.000,
            -0.014, -0.014, 0.001, -0.001, 0.001, -0.001, 0.000, 0.000, -0.001,
            -0.003, 0.000, -0.001, -0.004, -0.000, -0.001, 0.000, 0.000, 0.000, 0.000,
            -0.000, -0.001, 0.001, 0.001, -0.001, 0.000, -0.000, 0.000, -0.000, 0.001,
            0.000, -0.000, 0.000, -0.000, 0.000, 0.000, -0.000, -0.000, 0.000, -0.000,
            -0.000, -0.000, 0.000, 0.000, 0.000, 0.000, 0.000, 0.000, -0.009, -0.014,
            0.001, 0.010, -0.013, 0.000, -0.010, 0.010, 0.000, 0.010, 0.010, -0.000,
            0.001, 0.000, 0.000, 0.001, -0.000, 0.001, 0.002, -0.000, 0.000, 0.003,
            0.000, 0.001, 0.000, 0.000, 0.000, 0.000, 0.012, 0.018, -0.001, -0.013,
            0.018, -0.000, 0.013, -0.013, -0.000, -0.013, -0.013, 0.001, -0.001,
            0.000, -0.001, 0.000, 0.001, -0.001, -0.003, 0.000, -0.001, -0.004,
            -0.000, -0.001, 0.000, 0.000, 0.000, 0.000, 0.000, -0.000, 0.000, 0.000,
            -0.000, 0.001, 0.000, 0.000, -0.000, 0.000, 0.000, -0.000, -0.000, 0.000,
            -0.000, 0.000, 0.000, 0.000, -0.000, -0.000, -0.000, -0.000, 0.000, 0.000,
            0.000, 0.000, 0.000, 0.000, 0.009, 0.014, -0.000, -0.010, 0.013, 0.000,
            0.010, -0.010, -0.000, -0.010, -0.010, 0.000, -0.001, 0.000, -0.001,
            0.000, -0.000, -0.001, -0.001, 0.000, -0.000, -0.003, -0.000, -0.001,
            0.000, 0.000, 0.000, 0.000, -0.009, -0.013, 0.000, 0.010, -0.013, 0.000,
            -0.010, 0.009, 0.000, 0.010, 0.010, -0.000, 0.001, -0.000, 0.000, -0.000,
            0.000, 0.001, 0.002, 0.000, 0.000, 0.003, 0.000, 0.001, 0.000, 0.000,
            0.000, 0.000, -0.000, -0.000, -0.000, 0.000, -0.000, -0.000, -0.000,
            0.000, 0.001, 0.000, 0.000, 0.000, 0.000, -0.000, 0.000, -0.000, 0.000,
            -0.000, 0.000, -0.000, 0.000, 0.000, -0.000, -0.000, 0.000, 0.000, 0.000,
            0.000, -0.009, -0.014, 0.001, 0.010, -0.013, 0.000, -0.010, 0.010, 0.000,
            0.010, 0.010, -0.000, 0.001, 0.000, 0.000, -0.000, -0.000, 0.001, 0.002,
            -0.000, 0.000, 0.003, 0.000, 0.001, 0.000, 0.000, 0.000, 0.000, -0.009,
            -0.014, 0.000, 0.010, -0.013, 0.000, -0.010, 0.010, 0.000, 0.010, 0.010,
            -0.000, 0.001, -0.000, 0.000, -0.000, 0.000, 0.001, 0.002, -0.000, 0.000,
            0.003, 0.001, 0.001, 0.000, 0.000, 0.000, 0.000, 0.000, 0.001, -0.000,
            -0.000, 0.001, -0.000, 0.000, -0.000, 0.000, -0.000, -0.000, 0.001, 0.000,
            -0.000, -0.000, -0.000, 0.000, 0.000, -0.000, 0.000, 0.000, 0.000, 0.000,
            0.000, 0.000, 0.000, 0.000, 0.000, -0.000, -0.001, 0.000, 0.001, -0.001,
            -0.000, -0.001, 0.001, 0.000, 0.001, 0.001, 0.000, 1.708, 0.048, 0.784,
            0.062, 0.042, 0.053, 0.077, 0.001, -0.061, 0.046, -0.019, -0.029, 0.000,
            0.000, 0.000, 0.000, 0.000, 0.001, -0.000, 0.000, 0.000, 0.000, 0.000,
            -0.000, -0.000, 0.000, -0.000, -0.000, 0.048, 5.001, -1.631, -0.036,
            0.144, 0.040, 0.036, 0.016, -0.051, -0.067, -0.024, -0.005, 0.000, 0.000,
            0.000, 0.000, -0.000, -0.001, 0.000, 0.000, -0.001, -0.000, -0.001, 0.000,
            0.000, 0.000, 0.000, -0.000, 0.784, -1.631, 1.242, 0.057, -0.037, 0.018,
            0.034, -0.017, -0.015, 0.058, -0.021, -0.029, 0.000, 0.000, 0.000, 0.000,
            0.000, 0.000, 0.000, 0.001, 0.000, 0.000, 0.000, -0.000, -0.000, -0.000,
            -0.000, -0.000, 0.062, -0.036, 0.057, 6.228, -0.014, 0.932, 0.059, 0.053,
            -0.069, 0.148, 0.015, -0.031, 0.000, 0.000, 0.000, 0.000, -0.000, 0.000,
            -0.000, -0.000, 0.001, 0.000, -0.000, 0.000, 0.000, -0.000, 0.000, 0.000,
            0.042, 0.144, -0.037, -0.014, 3.011, 0.986, 0.076, 0.030, -0.052, -0.027,
            0.057, 0.051, 0.000, 0.000, 0.000, 0.000, -0.001, -0.001, -0.000, 0.001,
            -0.001, 0.000, -0.001, 0.001, -0.000, 0.001, 0.001, 0.000, 0.053, 0.040,
            0.018, 0.932, 0.986, 0.885, 0.090, 0.044, -0.055, 0.057, 0.051, -0.003,
            0.000, 0.000, 0.000, 0.000, -0.002, -0.003, 0.000, 0.002, -0.003, -0.000,
            -0.001, 0.002, 0.000, 0.002, 0.002, -0.000, 0.077, 0.036, 0.034, 0.059,
            0.076, 0.090, 6.230, 0.139, 0.763, 0.013, -0.019, -0.024, 0.000, 0.000,
            0.000, 0.000, 0.000, 0.000, -0.000, -0.000, 0.000, -0.000, 0.000, 0.000,
            -0.000, -0.000, -0.000, 0.000, 0.001, 0.016, -0.017, 0.053, 0.030, 0.044,
            0.139, 3.130, -1.128, -0.010, 0.131, 0.018, 0.000, 0.000, 0.000, 0.000,
            -0.000, -0.001, -0.000, 0.000, -0.001, -0.000, -0.000, 0.000, 0.000,
            0.000, 0.000, 0.000, -0.061, -0.051, -0.015, -0.069, -0.052, -0.055,
            0.763, -1.128, 0.866, -0.022, -0.053, 0.007, 0.000, 0.000, 0.000, 0.000,
            -0.003, -0.004, -0.000, 0.003, -0.004, -0.000, -0.003, 0.003, 0.000,
            0.003, 0.003, 0.000, 0.046, -0.067, 0.058, 0.148, -0.027, 0.057, 0.013,
            -0.010, -0.022, 2.437, -0.102, 0.938, 0.000, 0.000, 0.000, 0.000, -0.000,
            -0.000, 0.000, 0.000, -0.000, 0.000, -0.000, 0.000, -0.000, 0.000, 0.001,
            0.000, -0.019, -0.024, -0.021, 0.015, 0.057, 0.051, -0.019, 0.131, -0.053,
            -0.102, 4.944, 1.724, 0.000, 0.000, 0.000, 0.000, -0.001, -0.001, 0.000,
            0.001, -0.001, 0.000, -0.001, 0.001, -0.000, 0.001, 0.001, 0.000, -0.029,
            -0.005, -0.029, -0.031, 0.051, -0.003, -0.024, 0.018, 0.007, 0.938, 1.724,
            1.569, 0.000, 0.000, 0.000, 0.000, 0.000, 0.000, 0.000, 0.000, 0.000,
            0.000, 0.000, 0.000, 0.000, 0.000, 0.000, 0.000, 0.000, 0.000, 0.000,
            0.000, 0.000, 0.000, 0.000, 0.000, 0.000, 0.000, 0.000, 0.000, 1.0, 0.000,
            0.000, 0.000, 0.000, 0.000, 0.000, 0.000, 0.000, 0.000, 0.000, 0.000,
            0.000, 0.000, 0.000, 0.000, 0.000, 0.000, 0.000, 0.000, 0.000, 0.000,
            0.000, 0.000, 0.000, 0.000, 0.000, 0.000, 0.000, 1.0, 0.000, 0.000, 0.000,
            0.000, 0.000, 0.000, 0.000, 0.000, 0.000, 0.000, 0.000, 0.000, 0.000,
            0.000, 0.000, 0.000, 0.000, 0.000, 0.000, 0.000, 0.000, 0.000, 0.000,
            0.000, 0.000, 0.000, 0.000, 0.000, 1.0, 0.000, 0.000, 0.000, 0.000, 0.000,
            0.000, 0.000, 0.000, 0.000, 0.000, 0.000, 0.000, 0.000, 0.000, 0.000,
            0.000, 0.000, 0.000, 0.000, 0.000, 0.000, 0.000, 0.000, 0.000, 0.000,
            0.000, 0.000, 0.000, 1.0;

    Cu << 268.573, -43.819, -147.211, -43.819, 92.949, 58.082, -147.211, 58.082,
            302.120;

    QInit_ = Qdig.asDiagonal();
    QInit_ += B * Cu * B.transpose();


    low_pass_filters_.resize(3);
    low_pass_filters_[0] = std::make_shared<LowPassFilter>(0.02, 3.0);
    low_pass_filters_[1] = std::make_shared<LowPassFilter>(0.02, 3.0);
    low_pass_filters_[2] = std::make_shared<LowPassFilter>(0.02, 3.0);
}

void Estimator::update() {
    if (robot_model_.mass_ == 0) return;

    Q = QInit_;
    R = RInit_;

    foot_poses_ = robot_model_.getFeet2BPositions();
    foot_vels_ = robot_model_.getFeet2BVelocities();
    _feetH.setZero();

    const std::vector contact(4, 1);
    const std::vector phase(4, 0.5);

    // Adjust the covariance based on foot contact and phase.
    for (int i(0); i < 4; ++i) {
        if (contact[i] == 0) {
            // foot not contact
            Q.block(6 + 3 * i, 6 + 3 * i, 3, 3) = _largeVariance * Eigen::MatrixXd::Identity(3, 3);
            R.block(12 + 3 * i, 12 + 3 * i, 3, 3) = _largeVariance * Eigen::MatrixXd::Identity(3, 3);
            R(24 + i, 24 + i) = _largeVariance;
        } else {
            // foot contact
            const double trust = windowFunc(phase[i], 0.2);
            Q.block(6 + 3 * i, 6 + 3 * i, 3, 3) =
                    (1 + (1 - trust) * _largeVariance) *
                    QInit_.block(6 + 3 * i, 6 + 3 * i, 3, 3);
            R.block(12 + 3 * i, 12 + 3 * i, 3, 3) =
                    (1 + (1 - trust) * _largeVariance) *
                    RInit_.block(12 + 3 * i, 12 + 3 * i, 3, 3);
            R(24 + i, 24 + i) =
                    (1 + (1 - trust) * _largeVariance) * RInit_(24 + i, 24 + i);
        }
        _feetPos2Body.segment(3 * i, 3) = Eigen::Map<Eigen::Vector3d>(foot_poses_[i].p.data);
        _feetVel2Body.segment(3 * i, 3) = Eigen::Map<Eigen::Vector3d>(foot_vels_[i].data);
    }

    // Acceleration from imu as system input
    rotation_ = KDL::Rotation::Quaternion(ctrl_component_.imu_state_interface_[1].get().get_value(),
                                          ctrl_component_.imu_state_interface_[2].get().get_value(),
                                          ctrl_component_.imu_state_interface_[3].get().get_value(),
                                          ctrl_component_.imu_state_interface_[0].get().get_value());

    gyro_ = KDL::Vector(ctrl_component_.imu_state_interface_[4].get().get_value(),
                        ctrl_component_.imu_state_interface_[5].get().get_value(),
                        ctrl_component_.imu_state_interface_[6].get().get_value());

    acceleration_ = KDL::Vector(ctrl_component_.imu_state_interface_[7].get().get_value(),
                                ctrl_component_.imu_state_interface_[8].get().get_value(),
                                ctrl_component_.imu_state_interface_[9].get().get_value());

    _u = Vec3((rotation_ * acceleration_ + g_).data);
    x_hat_ = A * x_hat_ + B * _u;
    y_hat_ = C * x_hat_;

    // Update the measurement value
    _y << _feetPos2Body, _feetVel2Body, _feetH;

    // Update the covariance matrix
    Ppriori = A * P * A.transpose() + Q;
    S = R + C * Ppriori * C.transpose();
    Slu = S.lu();
    Sy = Slu.solve(_y - y_hat_);
    Sc = Slu.solve(C);
    SR = Slu.solve(R);
    STC = S.transpose().lu().solve(C);
    IKC = Eigen::MatrixXd::Identity(18, 18) - Ppriori * C.transpose() * Sc;

    // Update the state and covariance matrix
    x_hat_ += Ppriori * C.transpose() * Sy;
    P = IKC * Ppriori * IKC.transpose() +
        Ppriori * C.transpose() * SR * STC * Ppriori.transpose();

    // Using low pass filter to smooth the velocity
    low_pass_filters_[0]->addValue(x_hat_(3));
    low_pass_filters_[1]->addValue(x_hat_(4));
    low_pass_filters_[2]->addValue(x_hat_(5));
    x_hat_(3) = low_pass_filters_[0]->getValue();
    x_hat_(4) = low_pass_filters_[1]->getValue();
    x_hat_(5) = low_pass_filters_[2]->getValue();
}
