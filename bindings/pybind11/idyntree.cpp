#include <vector>

#include "idyntree_core.h"
#include "idyntree_model.h"

#include <pybind11/pybind11.h>


namespace iDynTree {
namespace {

namespace py = ::pybind11;
PYBIND11_MODULE(pybind, m) {
  iDynTree::bindings::iDynTreeCoreBindings(m);
  iDynTree::bindings::iDynTreeModelBindings(m);
}

}  // namespace

}  // namespace iDynTree