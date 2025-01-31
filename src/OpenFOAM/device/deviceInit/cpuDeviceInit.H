#ifndef Foam_cpu_deviceInit_H
#define Foam_cpu_deviceInit_H

#include "deviceInit.H"

namespace Foam
{

class cpuDeviceInit
: public deviceInit<cpuDeviceInit>
{
private:
    /* data */
public:

    static void _backendInit(){};
};

} // namespace Foam

#endif
