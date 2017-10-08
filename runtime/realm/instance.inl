/* Copyright 2017 Stanford University, NVIDIA Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

// instances for Realm

#ifndef REALM_INSTANCE_INL
#define REALM_INSTANCE_INL

// nop, but helps IDEs
#include "instance.h"

#include "indexspace.h"
#include "inst_layout.h"
#include "serialize.h"

TYPE_IS_SERIALIZABLE(Realm::RegionInstance);

namespace Realm {

  extern Logger log_inst;

  ////////////////////////////////////////////////////////////////////////
  //
  // class RegionInstance

  inline bool RegionInstance::operator<(const RegionInstance& rhs) const
  {
    return id < rhs.id;
  }

  inline bool RegionInstance::operator==(const RegionInstance& rhs) const
  {
    return id == rhs.id;
  }

  inline bool RegionInstance::operator!=(const RegionInstance& rhs) const
  {
    return id != rhs.id;
  }

  inline bool RegionInstance::exists(void) const
  {
    return id != 0;
  }

  inline std::ostream& operator<<(std::ostream& os, RegionInstance r)
  {
    return os << std::hex << r.id << std::dec;
  }

  template <int N, typename T>
  inline IndexSpace<N,T> RegionInstance::get_indexspace(void) const
  {
    const InstanceLayout<N,T> *layout = dynamic_cast<const InstanceLayout<N,T> *>(this->get_layout());
    if(!layout) {
      log_inst.fatal() << "dimensionality mismatch between instance and index space!";
      assert(0);
    }
    return layout->space;
  }
		
  template <int N>
  inline IndexSpace<N,int> RegionInstance::get_indexspace(void) const
  {
    return get_indexspace<N,int>();
  }

  template <typename T>
  inline T RegionInstance::read(size_t offset) const
  {
    T val;
    read_untyped(offset, &val, sizeof(T));
    return val;
  }

  template <typename T>
  inline void RegionInstance::write(size_t offset, T val) const
  {
    write_untyped(offset, &val, sizeof(T));
  }

  template <typename T>
  inline void RegionInstance::reduce_apply(size_t offset, ReductionOpID redop_id,
					   T val,
					   bool exclusive /*= false*/) const
  {
    reduce_apply_untyped(offset, redop_id, &val, sizeof(T), exclusive);
  }

  template <typename T>
  inline void RegionInstance::reduce_fold(size_t offset, ReductionOpID redop_id,
					  T val,
					  bool exclusive /*= false*/) const
  {
    reduce_fold_untyped(offset, redop_id, &val, sizeof(T), exclusive);
  }

  template <typename T>
  inline T *RegionInstance::pointer(size_t offset) const
  {
    return static_cast<T *>(pointer_untyped(offset, sizeof(T)));
  }
		
  template <int N, typename T>
  inline /*static*/ Event RegionInstance::create_instance(RegionInstance& inst,
							  Memory memory,
							  const IndexSpace<N,T>& space,
							  const std::vector<size_t> &field_sizes,
							  size_t block_size,
							  const ProfilingRequestSet& reqs,
							  Event wait_on /*= Event::NO_EVENT*/)
  {
    // smoosh hybrid block sizes back to SOA for now
    if(block_size > 1)
      block_size = 0;
    InstanceLayoutConstraints ilc(field_sizes, block_size);
    InstanceLayoutGeneric *layout = InstanceLayoutGeneric::choose_instance_layout(space, ilc);
    return create_instance(inst, memory, layout, reqs, wait_on);
  }

  template <int N, typename T>
  inline /*static*/ Event RegionInstance::create_instance(RegionInstance& inst,
							  Memory memory,
							  const IndexSpace<N,T>& space,
							  const std::map<FieldID, size_t> &field_sizes,
							  size_t block_size,
							  const ProfilingRequestSet& reqs,
							  Event wait_on /*= Event::NO_EVENT*/)
  {
    // smoosh hybrid block sizes back to SOA for now
    if(block_size > 1)
      block_size = 0;
    InstanceLayoutConstraints ilc(field_sizes, block_size);
    InstanceLayoutGeneric *layout = InstanceLayoutGeneric::choose_instance_layout(space, ilc);
    return create_instance(inst, memory, layout, reqs, wait_on);
  }

  // we'd like the methods above to accept a Rect<N,T> in place of the
  //  IndexSpace<N,T>, but that doesn't work unless the method template
  //  parameters are specified explicitly, so provide an overload that
  //  takes a Rect directly
  template <int N, typename T>
  inline /*static*/ Event RegionInstance::create_instance(RegionInstance& inst,
							  Memory memory,
							  const Rect<N,T>& rect,
							  const std::vector<size_t>& field_sizes,
							  size_t block_size, // 0=SOA, 1=AOS, 2+=hybrid
							  const ProfilingRequestSet& prs,
							  Event wait_on /*= Event::NO_EVENT*/)
  {
    return RegionInstance::create_instance<N,T>(inst,
						memory,
						IndexSpace<N,T>(rect),
						field_sizes,
						block_size,
						prs,
						wait_on);
  }

  template <int N, typename T>
  inline /*static*/ Event RegionInstance::create_instance(RegionInstance& inst,
							  Memory memory,
							  const Rect<N,T>& rect,
							  const std::map<FieldID, size_t> &field_sizes,
							  size_t block_size, // 0=SOA, 1=AOS, 2+=hybrid
							  const ProfilingRequestSet& prs,
							  Event wait_on /*= Event::NO_EVENT*/)
  {
    return RegionInstance::create_instance<N,T>(inst,
						memory,
						IndexSpace<N,T>(rect),
						field_sizes,
						block_size,
						prs,
						wait_on);
  }


  ////////////////////////////////////////////////////////////////////////
  //
  // class RegionInstance::DestroyedField


  inline RegionInstance::DestroyedField::DestroyedField(void) 
    : field_id(FieldID(-1)), size(0), serdez_id(0)
  { }

  inline RegionInstance::DestroyedField::DestroyedField(FieldID fid, unsigned s, CustomSerdezID sid)
    : field_id(fid), size(s), serdez_id(sid)
  { }


}; // namespace Realm  

#endif // ifndef REALM_INSTANCE_INL
