/*-------------------------------------------------------------------------------------------------
| This file is distributed under the MIT License.
| See accompanying file /LICENSE for details.
| Author(s): Matthew Amy
*------------------------------------------------------------------------------------------------*/

#pragma once

namespace synthewareQ {
namespace synthesis {

  template<typename T>
  using linear = std::vector<std::vector<T> >;


  std::list<std::pair<size_t, size_t> > steiner_gauss(linear<bool> mat) {

    // Stores the location of the leading 1 for each row
    std::vector<size_t> leading_one(mat.length(), 0);

    for (auto i = 0; i < mat[0].length(); i++) {

      // Phase 1: Compute steiner tree covering the 1's in column i

      // Phase 2: Propagate 1's to column i for each Steiner point

      // Phase 3: Empty all 1's from column i in the Steiner tree

      // Phase 4: For each node that has had a parent with FARTHER LEFT
      //   leading 1 added to it an ODD number of times, add its parent

    }

  }

}
}
