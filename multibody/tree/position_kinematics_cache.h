#pragma once

#include <limits>
#include <vector>

#include "drake/common/default_scalars.h"
#include "drake/common/drake_assert.h"
#include "drake/common/drake_copyable.h"
#include "drake/common/eigen_types.h"
#include "drake/math/rigid_transform.h"
#include "drake/math/rotation_matrix.h"
#include "drake/multibody/tree/multibody_tree_indexes.h"
#include "drake/multibody/tree/multibody_tree_topology.h"

namespace drake {
namespace multibody {
namespace internal {

// This class is one of the cache entries in the Context. It holds the
// kinematics results of computations that only depend on the generalized
// positions of the system.
// Kinematics results include:
//
// - Body frame B poses X_WB measured and expressed in the world frame W.
// - Pose X_FM of a mobilizer's outboard frame M measured and expressed in the
//   inboard frame F.
// - Mobilizer's matrices H_FM (with F and M defined above) that map the
//   mobilizer's generalized velocities v to cross-joint spatial velocities
//   V_FM = H_FM * v.
//
// @tparam_default_scalar
template <typename T>
class PositionKinematicsCache {
 public:
  DRAKE_DEFAULT_COPY_AND_MOVE_AND_ASSIGN(PositionKinematicsCache);

  template <typename U>
  using RigidTransform = drake::math::RigidTransform<U>;

  // Constructs a position kinematics cache entry for the given
  // MultibodyTreeTopology.
  explicit PositionKinematicsCache(const MultibodyTreeTopology& topology)
      : num_mobods_(topology.num_mobods()) {
    Allocate();
  }

  // Returns a const reference to pose `X_WB` of the body B (associated with
  // mobilized body mobod_index) as measured and expressed in the world frame W.
  // @param[in] mobod_index The unique index for the computational
  //                        BodyNode object associated with body B.

  // @returns `X_WB` the pose of the body frame B measured and expressed in
  //                 the world frame W.
  const RigidTransform<T>& get_X_WB(MobodIndex mobod_index) const {
    DRAKE_ASSERT(0 <= mobod_index && mobod_index < num_mobods_);
    return X_WB_pool_[mobod_index];
  }

  // See documentation on the const version get_X_WB() for details.
  RigidTransform<T>& get_mutable_X_WB(MobodIndex mobod_index) {
    DRAKE_ASSERT(0 <= mobod_index && mobod_index < num_mobods_);
    return X_WB_pool_[mobod_index];
  }

  // Returns a const reference to the rotation matrix `R_WB` that relates the
  // orientation of the world frame W with the body frame B.
  // @param[in] mobod_index The unique index for the computational
  //                        BodyNode object associated with body B.
  const math::RotationMatrix<T>& get_R_WB(MobodIndex mobod_index) const {
    const RigidTransform<T>& X_WB = get_X_WB(mobod_index);
    return X_WB.rotation();
  }

  // Returns a const reference to the pose `X_PB` of the body frame B
  // as measured and expressed in its parent body frame P.
  // @param[in] mobod_index The unique identifier for the computational
  //                        BodyNode object associated with body B.
  // @returns `X_PB` a const reference to the pose of the body frame B
  //                 measured and expressed in the parent body frame P.
  const RigidTransform<T>& get_X_PB(MobodIndex mobod_index) const {
    DRAKE_ASSERT(0 <= mobod_index && mobod_index < num_mobods_);
    return X_PB_pool_[mobod_index];
  }

  // See documentation on the const version get_X_PB() for details.
  RigidTransform<T>& get_mutable_X_PB(MobodIndex mobod_index) {
    DRAKE_ASSERT(0 <= mobod_index && mobod_index < num_mobods_);
    return X_PB_pool_[mobod_index];
  }

  // For the mobilizer associated with the body node indexed by
  // `mobod_index`, this method returns a const reference to the pose
  // `X_FM` of the outboard frame M as measured and expressed in the inboard
  // frame F.
  //
  // @param[in] mobod_index The unique index for the computational BodyNode
  //                        object associated with the mobilizer of interest.
  // @returns A const reference to the pose `X_FM` of the outboard frame M
  //          as measured and expressed in the inboard frame F.
  const RigidTransform<T>& get_X_FM(MobodIndex mobod_index) const {
    DRAKE_ASSERT(0 <= mobod_index && mobod_index < num_mobods_);
    return X_FM_pool_[mobod_index];
  }

  // See documentation on the const version get_X_FM() for details.
  RigidTransform<T>& get_mutable_X_FM(MobodIndex mobod_index) {
    DRAKE_ASSERT(0 <= mobod_index && mobod_index < num_mobods_);
    return X_FM_pool_[mobod_index];
  }

  // Position of node B, with index `mobod_index`, measured in the inboard
  // body frame P, expressed in the world frame W.
  const Vector3<T>& get_p_PoBo_W(MobodIndex mobod_index) const {
    DRAKE_ASSERT(0 <= mobod_index && mobod_index < num_mobods_);
    return p_PoBo_W_pool_[mobod_index];
  }

  // Mutable version of get_p_PoBo_W().
  Vector3<T>& get_mutable_p_PoBo_W(MobodIndex mobod_index) {
    DRAKE_ASSERT(0 <= mobod_index && mobod_index < num_mobods_);
    return p_PoBo_W_pool_[mobod_index];
  }

 private:
  // Pool types:
  // Pools store entries in the same order as the mobilized bodies (BodyNodes)
  // in the multibody forest, i.e. in DFT (Depth-First Traversal) order.
  // Therefore clients of this class will access entries by MobodIndex, see
  // `get_X_WB()` for instance.

  // The type of pools for storing poses.
  typedef std::vector<RigidTransform<T>> X_PoolType;

  // The type of pools for storing 3D vectors.
  typedef std::vector<Vector3<T>> Vector3PoolType;

  // Allocates resources for this position kinematics cache.
  void Allocate() {
    X_WB_pool_.resize(num_mobods_);
    // Even though RigidTransform defaults to identity, we make it explicit.
    // This pose will never change after this initialization.
    X_WB_pool_[world_mobod_index()] = RigidTransform<T>::Identity();

    X_PB_pool_.resize(num_mobods_);
    X_PB_pool_[world_mobod_index()] = NaNPose();  // It should never be used.

    X_FM_pool_.resize(num_mobods_);
    X_FM_pool_[world_mobod_index()] = NaNPose();  // It should never be used.

    p_PoBo_W_pool_.resize(num_mobods_);
    // p_PoBo_W for the world body should never be used.
    p_PoBo_W_pool_[world_mobod_index()].setConstant(
        std::numeric_limits<
            typename Eigen::NumTraits<T>::Literal>::quiet_NaN());
  }

  // Helper method to initialize poses to garbage values including NaNs.
  // This allow us to quickly verify some of the values stored in the pools are
  // never used (however we store them anyway to simplify the indexing).
  static RigidTransform<T> NaNPose() {
    // Note: RotationMatrix will throw in Debug builds if values are NaN. For
    // our purposes, it is enough the translation has NaN values.
    return RigidTransform<T>(math::RotationMatrix<T>::Identity(),
        Vector3<T>::Constant(Eigen::NumTraits<double>::quiet_NaN()));
  }

  // Number of mobilized bodies in the corresponding multibody forest.
  int num_mobods_{0};
  // Pools indexed by MobodIndex.
  X_PoolType X_WB_pool_;
  X_PoolType X_PB_pool_;
  X_PoolType X_FM_pool_;
  Vector3PoolType p_PoBo_W_pool_;
};

}  // namespace internal
}  // namespace multibody
}  // namespace drake

DRAKE_DECLARE_CLASS_TEMPLATE_INSTANTIATIONS_ON_DEFAULT_SCALARS(
    class ::drake::multibody::internal::PositionKinematicsCache);
