#ifndef Foam_deviceInit_H
#define Foam_deviceInit_H

namespace Foam
{

template <typename T>
class deviceInit
{
public:

    static void Init()
    {
        T::_backendInit();
    }

};


} // end namespace Foam

#endif
