//
// Created by biao on 24-9-9.
//

#include "hardware_unitree_mujoco/HardwareUnitree.h"
#include <rclcpp/logging.hpp>
#include "crc32.h"

#define TOPIC_LOWCMD "rt/lowcmd"
#define TOPIC_LOWSTATE "rt/lowstate"

using namespace unitree::robot;
using hardware_interface::return_type;

rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::CallbackReturn HardwareUnitree::on_init(
    const hardware_interface::HardwareInfo &info) {
    if (SystemInterface::on_init(info) != CallbackReturn::SUCCESS) {
        return CallbackReturn::ERROR;
    }

    joint_torque_command_.assign(12, 0);
    joint_position_command_.assign(12, 0);
    joint_velocities_command_.assign(12, 0);
    joint_kp_command_.assign(12, 0);
    joint_kd_command_.assign(12, 0);

    joint_position_.assign(12, 0);
    joint_velocities_.assign(12, 0);
    joint_effort_.assign(12, 0);

    imu_states_.assign(10, 0);
    foot_force_.assign(4, 0);

    for (const auto &joint: info_.joints) {
        for (const auto &interface: joint.state_interfaces) {
            joint_interfaces[interface.name].push_back(joint.name);
        }
    }

    ChannelFactory::Instance()->Init(1, "lo");
    low_cmd_publisher_ =
            std::make_shared<ChannelPublisher<unitree_go::msg::dds_::LowCmd_> >(
                TOPIC_LOWCMD);
    low_cmd_publisher_->InitChannel();

    /*create subscriber*/
    lows_tate_subscriber_ =
            std::make_shared<ChannelSubscriber<unitree_go::msg::dds_::LowState_> >(
                TOPIC_LOWSTATE);
    lows_tate_subscriber_->InitChannel(
        [this](auto &&PH1) {
            lowStateMessageHandle(std::forward<decltype(PH1)>(PH1));
        },
        1);
    initLowCmd();


    return SystemInterface::on_init(info);
}

std::vector<hardware_interface::StateInterface> HardwareUnitree::export_state_interfaces() {
    std::vector<hardware_interface::StateInterface> state_interfaces;

    int ind = 0;
    for (const auto &joint_name: joint_interfaces["position"]) {
        state_interfaces.emplace_back(joint_name, "position", &joint_position_[ind++]);
    }

    ind = 0;
    for (const auto &joint_name: joint_interfaces["velocity"]) {
        state_interfaces.emplace_back(joint_name, "velocity", &joint_velocities_[ind++]);
    }

    ind = 0;
    for (const auto &joint_name: joint_interfaces["effort"]) {
        state_interfaces.emplace_back(joint_name, "effort", &joint_velocities_[ind++]);
    }

    // export imu sensor state interface
    for (uint i = 0; i < info_.sensors[0].state_interfaces.size(); i++) {
        state_interfaces.emplace_back(
            info_.sensors[0].name, info_.sensors[0].state_interfaces[i].name, &imu_states_[i]);
    }

    // export foot force sensor state interface
    for (uint i = 0; i < info_.sensors[1].state_interfaces.size(); i++) {
        state_interfaces.emplace_back(
            info_.sensors[1].name, info_.sensors[1].state_interfaces[i].name, &foot_force_[i]);
    }


    return state_interfaces;
}

std::vector<hardware_interface::CommandInterface> HardwareUnitree::export_command_interfaces() {
    std::vector<hardware_interface::CommandInterface> command_interfaces;

    int ind = 0;
    for (const auto &joint_name: joint_interfaces["position"]) {
        command_interfaces.emplace_back(joint_name, "position", &joint_position_command_[ind++]);
    }

    ind = 0;
    for (const auto &joint_name: joint_interfaces["velocity"]) {
        command_interfaces.emplace_back(joint_name, "velocity", &joint_velocities_command_[ind++]);
    }

    ind = 0;
    for (const auto &joint_name: joint_interfaces["effort"]) {
        command_interfaces.emplace_back(joint_name, "effort", &joint_torque_command_[ind]);
        command_interfaces.emplace_back(joint_name, "kp", &joint_kp_command_[ind]);
        command_interfaces.emplace_back(joint_name, "kd", &joint_kd_command_[ind]);
        ind++;
    }
    return command_interfaces;
}

return_type HardwareUnitree::read(const rclcpp::Time & /*time*/, const rclcpp::Duration & /*period*/) {
    // joint states
    for (int i(0); i < 12; ++i) {
        joint_position_[i] = low_state_.motor_state()[i].q();
        joint_velocities_[i] = low_state_.motor_state()[i].dq();
        joint_effort_[i] = low_state_.motor_state()[i].tau_est();
    }

    // imu states
    imu_states_[0] = low_state_.imu_state().quaternion()[0]; // w
    imu_states_[1] = low_state_.imu_state().quaternion()[1]; // x
    imu_states_[2] = low_state_.imu_state().quaternion()[2]; // y
    imu_states_[3] = low_state_.imu_state().quaternion()[3]; // z
    imu_states_[4] = low_state_.imu_state().gyroscope()[0];
    imu_states_[5] = low_state_.imu_state().gyroscope()[1];
    imu_states_[6] = low_state_.imu_state().gyroscope()[2];
    imu_states_[7] = low_state_.imu_state().accelerometer()[0];
    imu_states_[8] = low_state_.imu_state().accelerometer()[1];
    imu_states_[9] = low_state_.imu_state().accelerometer()[2];

    // contact states
    foot_force_[0] = low_state_.foot_force()[0];
    foot_force_[1] = low_state_.foot_force()[1];
    foot_force_[2] = low_state_.foot_force()[2];
    foot_force_[3] = low_state_.foot_force()[3];

    return return_type::OK;
}

return_type HardwareUnitree::write(const rclcpp::Time & /*time*/, const rclcpp::Duration & /*period*/) {
    // send command
    for (int i(0); i < 12; ++i) {
        low_cmd_.motor_cmd()[i].mode() = 0x01;
        low_cmd_.motor_cmd()[i].q() = static_cast<float>(joint_position_command_[i]);
        low_cmd_.motor_cmd()[i].dq() = static_cast<float>(joint_velocities_command_[i]);
        low_cmd_.motor_cmd()[i].kp() = static_cast<float>(joint_kp_command_[i]);
        low_cmd_.motor_cmd()[i].kd() = static_cast<float>(joint_kd_command_[i]);
        low_cmd_.motor_cmd()[i].tau() = static_cast<float>(joint_torque_command_[i]);
    }

    low_cmd_.crc() = crc32_core(reinterpret_cast<uint32_t *>(&low_cmd_),
                                (sizeof(unitree_go::msg::dds_::LowCmd_) >> 2) - 1);
    low_cmd_publisher_->Write(low_cmd_);
    return return_type::OK;
}

void HardwareUnitree::initLowCmd() {
    low_cmd_.head()[0] = 0xFE;
    low_cmd_.head()[1] = 0xEF;
    low_cmd_.level_flag() = 0xFF;
    low_cmd_.gpio() = 0;

    for (int i = 0; i < 20; i++) {
        low_cmd_.motor_cmd()[i].mode() =
                0x01; // motor switch to servo (PMSM) mode
        low_cmd_.motor_cmd()[i].q() = 0;
        low_cmd_.motor_cmd()[i].kp() = 0;
        low_cmd_.motor_cmd()[i].dq() = 0;
        low_cmd_.motor_cmd()[i].kd() = 0;
        low_cmd_.motor_cmd()[i].tau() = 0;
    }
}

void HardwareUnitree::lowStateMessageHandle(const void *messages) {
    low_state_ = *static_cast<const unitree_go::msg::dds_::LowState_ *>(messages);
}

#include "pluginlib/class_list_macros.hpp"

PLUGINLIB_EXPORT_CLASS(
    HardwareUnitree, hardware_interface::SystemInterface)
