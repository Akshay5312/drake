#pragma once

#include <memory>
#include <vector>

#include "drake/common/autodiff.h"
#include "drake/common/drake_assert.h"
#include "drake/common/drake_copyable.h"
#include "drake/multibody/multibody_tree/math/spatial_force.h"
#include "drake/multibody/multibody_tree/multibody_forces.h"
#include "drake/multibody/multibody_tree/multibody_tree_context.h"
#include "drake/multibody/multibody_tree/multibody_tree_element.h"
#include "drake/multibody/multibody_tree/multibody_tree_indexes.h"
#include "drake/multibody/multibody_tree/multibody_tree_topology.h"
#include "drake/multibody/multibody_tree/position_kinematics_cache.h"
#include "drake/multibody/multibody_tree/velocity_kinematics_cache.h"

namespace drake {
namespace multibody {

/// A %ForceElement allows modeling state and time dependent forces in a
/// MultibodyTree model. Examples of such forces are springs, dampers, drag and
/// gravity. Forces that depend on accelerations such as virtual mass cannot be
/// modeled with a %ForceElement.
/// This abstract class provides an API that all force elements subclasses must
/// implement in order to be fully defined. These are:
/// - CalcAndAddForceContribution(): computes the force contribution of a force
///   element in a %MultibodyTree model.
/// - CalcPotentialEnergy(): computes a force element potential energy
///   contribution.
/// - CalcConservativePower(): computes the power generated by conservative
///   forces.
/// - CalcNonConservativePower(): computes the power dissipated by
///   non-conservative forces.
///
/// @tparam T The scalar type. Must be a valid Eigen scalar.
template <typename T>
class ForceElement : public
                     MultibodyTreeElement<ForceElement<T>, ForceElementIndex> {
 public:
  DRAKE_NO_COPY_NO_MOVE_NO_ASSIGN(ForceElement)

  /// Default constructor for a generic force element.
  explicit ForceElement(ModelInstanceIndex model_instance)
      : MultibodyTreeElement<ForceElement<T>, ForceElementIndex>(
            model_instance) {}

  /// Computes the force contribution for `this` force element and **adds** it
  /// to the output arrays of forces. Depending on their model, different force
  /// elements may write into the array of sptial forces `F_B_W` or the array of
  /// generalized forces `tau`.
  ///
  /// @param[in] context
  ///   The context containing the state of the %MultibodyTree model.
  /// @param[in] pc
  ///   A position kinematics cache object already updated to be in sync with
  ///   `context`.
  /// @param[in] vc
  ///   A velocity kinematics cache object already updated to be in sync with
  ///   `context`.
  /// @param[out] forces
  ///   A pointer to a valid, non nullptr, multibody forces object.
  ///   On output `this` force element adds its contribution into `forces`.
  ///   This method will abort if the `forces` pointer is null or if the
  ///   forces object is not compatible with `this` %MultibodyTree, see
  ///   MultibodyForces::CheckInvariants().
  ///
  /// @pre The position kinematics `pc` must have been previously updated with a
  /// call to CalcPositionKinematicsCache().
  /// @pre The velocity kinematics `vc` must have been previously updated with a
  /// call to CalcVelocityKinematicsCache().
  void CalcAndAddForceContribution(const MultibodyTreeContext<T>& context,
                                   const PositionKinematicsCache<T>& pc,
                                   const VelocityKinematicsCache<T>& vc,
                                   MultibodyForces<T>* forces) const {
    DRAKE_DEMAND(forces != nullptr);
    DRAKE_DEMAND(forces->CheckHasRightSizeForModel(this->get_parent_tree()));
    DoCalcAndAddForceContribution(context, pc, vc, forces);
  }

  /// @name Methods to track the energy budget.
  /// The methods in this group are used to track the transfer of energy within
  /// a MultibodyTree model. This methods are pure virtual and subclasses must
  /// provide an implementation. Conservative force elements must implement
  /// CalcPotentialEnergy() in order to track the potential energy stored in
  /// force elements. In addition, conservative force elements must implement
  /// the rate of change of this potential energy in CalcConservativePower().
  /// Non-conservative force elements need to implement the rate of change,
  /// power, at which they either generate or dissipate energy in the system in
  /// CalcNonConservativePower(). Note that a force element may have both
  /// conservative and non-conservative contributions, such as a spring-damper
  /// element for instance.
  /// All of these methods have the same signature. Refer to the documentation
  /// for CalcPotentialEnergy() for a detailed description of the input
  /// arguments of the methods in this group.
  ///
  // TODO(amcastro-tri): make this methods DoCalcXXX() when caching gets in and
  // make the public API's to only take a Context.
  //@{

  /// Calculates the potential energy currently stored given the configuration
  /// provided in `context`. Non-conservative force elements will return zero.
  /// @param[in] context
  ///   The context containing the state of the %MultibodyTree model.
  /// @param[in] pc
  ///   A position kinematics cache object already updated to be in sync with
  ///   `context`.
  ///
  /// @pre The position kinematics `pc` must have been previously updated with a
  /// call to CalcPositionKinematicsCache().
  ///
  /// @returns For conservative force models, the potential energy stored by
  /// `this` force element. For non-conservative force models, zero.
  ///
  /// @see CalcConservativePower()
  virtual T CalcPotentialEnergy(const MultibodyTreeContext<T>& context,
                                const PositionKinematicsCache<T>& pc) const = 0;

  /// Calculates and returns the power generated by conservative force elements
  /// or zero if `this` force element is non-conservative. This quantity is
  /// defined to be positive when the potential energy is decreasing. In other
  /// words, if `PE` is the potential energy as defined by
  /// CalcPotentialEnergy(), then the conservative power, `Pc`, is
  /// `Pc = -d(PE)/dt`.
  ///
  /// @see CalcPotentialEnergy(), CalcNonConservativePower()
  virtual T CalcConservativePower(
      const MultibodyTreeContext<T>& context,
      const PositionKinematicsCache<T>& pc,
      const VelocityKinematicsCache<T>& vc) const = 0;

  /// Calculates the rate at which mechanical energy is being generated
  /// (positive) or dissipated (negative) *other than* by conversion between
  /// potential and kinetic energy. Integrating this quantity yields work W,
  /// and the total energy `E = PE + KE - W` should be
  /// conserved by any physically-correct model, to within integration accuracy
  /// of W.
  /// @see CalcConservativePower()
  virtual T CalcNonConservativePower(
      const MultibodyTreeContext<T>& context,
      const PositionKinematicsCache<T>& pc,
      const VelocityKinematicsCache<T>& vc) const = 0;

  // End of group on energy budget tracking.
  //@}

  /// @cond
  // For internal use only.
  // NVI to DoCloneToScalar() templated on the scalar type of the new clone to
  // be created. This method is intended to be called by
  // MultibodyTree::CloneToScalar().
  template <typename ToScalar>
  std::unique_ptr<ForceElement<ToScalar>> CloneToScalar(
  const MultibodyTree<ToScalar>& cloned_tree) const {
    return DoCloneToScalar(cloned_tree);
  }
  /// @endcond

 protected:
  /// This method is called only from the public non-virtual
  /// CalcAndAddForceContributions() which will already have error-checked
  /// the parameters so you don't have to. Refer to the documentation for
  /// CalcAndAddForceContribution() for details describing the purpose and
  /// parameters of this method.
  /// It assumes `forces` to be a valid pointer to a MultibodyForces object
  /// compatible with the MultibodyTree model owning `this` force element.
  ///
  /// @pre The position kinematics `pc` must have been previously updated with a
  /// call to CalcPositionKinematicsCache().
  /// @pre The velocity kinematics `vc` must have been previously updated with a
  /// call to CalcVelocityKinematicsCache().
  virtual void DoCalcAndAddForceContribution(
      const MultibodyTreeContext<T>& context,
      const PositionKinematicsCache<T>& pc,
      const VelocityKinematicsCache<T>& vc,
      MultibodyForces<T>* forces) const = 0;

  /// @name Methods to make a clone templated on different scalar types.
  ///
  /// Specific force element subclasses must implement these to support scalar
  /// conversion to other types. These methods are only called from
  /// MultibodyTree::CloneToScalar(); users _must_ not call these explicitly
  /// since there is no external mechanism to ensure the argument `tree_clone`
  /// is in a valid stage of cloning. In contrast,
  /// MultibodyTree::CloneToScalar() guarantees that by when
  /// ForceElement::CloneToScalar() is called, all Body, Frame and Mobilizer
  /// objects in the original tree (templated on T) to which `this`
  /// %ForceElement<T> belongs, have a corresponding clone in the cloned tree
  /// (argument `tree_clone` for these methods). Therefore, implementations of
  /// ForceElement::DoCloneToScalar() can retrieve clones from `tree_clone` as
  /// needed.
  /// Consider the following example for a `SpringElement<T>` used to model
  /// an elastic spring between two bodies:
  /// @code
  ///   template <typename T>
  ///   class SpringElement {
  ///    public:
  ///     // Class's constructor.
  ///     SpringElement(
  ///       const Body<T>& body1, const Body<T>& body2, double stiffness);
  ///     // Get the first body to which this spring is connected.
  ///     const Body<T>& get_body1() const;
  ///     // Get the second body to which this spring is connected.
  ///     const Body<T>& get_body2() const;
  ///     // Get the spring stiffness constant.
  ///     double get_stiffness() const;
  ///    protected:
  ///     // Implementation of the scalar conversion from T to double.
  ///     std::unique_ptr<ForceElement<double>> DoCloneToScalar(
  ///       const MultibodyTree<double>& tree_clone) const) {
  ///         const Body<ToScalar>& body1_clone =
  ///           tree_clone.get_variant(get_body1());
  ///         const Body<ToScalar>& body2_clone =
  ///           tree_clone.get_variant(get_body2());
  ///         return std::make_unique<SpringElement<double>>(
  ///           body1_clone, body2_clone, get_stiffness());
  ///     }
  /// @endcode
  ///
  /// MultibodyTree::get_variant() methods are available to retrieve cloned
  /// variants from `tree_clone`, and are overloaded on different element types.
  ///
  /// For examples on how a MultibodyTree model can be converted to other
  /// scalar types, please refer to the documentation for
  /// MultibodyTree::CloneToScalar().
  /// @{

  /// Clones this %ForceElement (templated on T) to a mobilizer templated on
  /// `double`.
  virtual std::unique_ptr<ForceElement<double>> DoCloneToScalar(
      const MultibodyTree<double>& tree_clone) const = 0;

  /// Clones this %ForceElement (templated on T) to a mobilizer templated on
  /// AutoDiffXd.
  virtual std::unique_ptr<ForceElement<AutoDiffXd>> DoCloneToScalar(
      const MultibodyTree<AutoDiffXd>& tree_clone) const = 0;
  /// @}

 private:
  // Implementation for MultibodyTreeElement::DoSetTopology().
  // At MultibodyTree::Finalize() time, each force element retrieves its
  // topology from the parent MultibodyTree.
  void DoSetTopology(const MultibodyTreeTopology&) final {}
};

}  // namespace multibody
}  // namespace drake
