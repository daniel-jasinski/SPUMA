#ifndef FOAM_LAMBDAOPTMINLIST_H
#define FOAM_LAMBDAOPTMINLIST_H

#include "scalarList.H"

namespace Foam
{

namespace Chebyshev
{

namespace FirstKind
{
    //- list of optimal values for V-cycle error bound
    const scalarList lambdaOptMinList = {
    0.3333333264963328,
    0.1805358974013031,
    0.1159278474090664,
    0.0820780023852334,
    0.0618496081203676,
    0.0486605907936455,
    0.0395132940691538,
    0.0328701015026835,
    0.0278702889449934,
    0.0239987636410016,
    0.0209304530155615,
    0.0184513099814704,
    0.0164152156249015,
    0.0147195630514484,
    0.0132900883942254,
    0.0120723317797092,
    0.0110250964662371,
    0.0101170167811085,
    0.0093237783639166,
    0.0086261852644572
    };

} // namespace FirstKind


} // namespace Chebyshev

} // namespace Foam

#endif
