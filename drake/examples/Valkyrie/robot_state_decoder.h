#pragma once

#include <map>
#include <memory>
#include <string>

#include "drake/manipulation/util/robot_state_msg_translator.h"
#include "drake/multibody/rigid_body_tree.h"
#include "drake/systems/framework/leaf_system.h"

namespace drake {
namespace systems {

// TODO(siyuan): move this to drake/manipulation/util/
// TODO(siyuan): currently doesn't do anything with the efforts or spatial
// forces in the robot_state_t message.

/**
 * Converts a robot_state_t LCM message into a KinematicsCache object.
 *
 * Note that a RobotStateDecoder simply ignores state information for joints
 * that it doesn't know about.
 */
class RobotStateDecoder : public LeafSystem<double> {
 public:
  DRAKE_NO_COPY_NO_MOVE_NO_ASSIGN(RobotStateDecoder)

  explicit RobotStateDecoder(const RigidBodyTree<double>& tree);

  ~RobotStateDecoder() override {}

 protected:
  std::unique_ptr<AbstractValue> AllocateOutputAbstract(
      const OutputPortDescriptor<double>& descriptor) const override;

  void DoCalcOutput(const Context<double>& context,
                    SystemOutput<double>* output) const override;

 private:
  const manipulation::RobotStateLcmMessageTranslator translator_;

  const int robot_state_message_port_index_;
  const int kinematics_cache_port_index_;
};

}  // namespace systems
}  // namespace drake
