#ifndef Foam_deviceInit_H
#define Foam_deviceInit_H

namespace Foam
{

class deviceInitBase
{
protected:
    static bool initDeviceFlag_;
};

template <typename T>
class deviceInit
: public deviceInitBase
{
public:

    static void Init()
    {
        T::_backendInit();
    }

};


} // end namespace Foam

#endif
