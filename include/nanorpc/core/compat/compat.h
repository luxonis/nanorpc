#ifndef __NANO_RPC_CORE_COMPAT_H__
#define __NANO_RPC_CORE_COMPAT_H__


// Check if C++17
#if __cplusplus >= 201703L

#include <any>
#include <optional>
#include <tuple>

// Otherwise use compatible alternatives
#else

#include "nanorpc/core/compat/any.h"
#include "nanorpc/core/compat/optional.h"
#include "nanorpc/core/compat/invoke.h"

#endif


namespace nanorpc
{
namespace core
{
namespace compat
{

// Check if C++17
#if __cplusplus >= 201703L

// Use std namespace
using namespace std;

// Otherwise use compatible alternatives
#else

// Use compatible alternatives for 'optional', 'any' and 'apply'
using namespace tl;
using namespace linb;
using namespace invoke_hpp;

#endif

} // namespace compat
} // namespace core
} // namespace nanorpc


#endif  // !__NANO_RPC_CORE_COMPAT_H__