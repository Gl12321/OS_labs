#include <iostream>
#include "Number.h"
#include "Vector.h"

using namespace NumberLib;
using namespace VectorLib;

int main() {
    Number a = CreateNumber(3.0);
    Number b = CreateNumber(4.0);

    Vector v1(a, b);        
    Vector v2 = ONE_VECTOR;  

    Vector result = v1.add(v2);  

    std::cout << "v1 + v2 = ("
        << result.getX().getValue() << ", "
        << result.getY().getValue() << ")\n";

    std::cout << "Radius: " << result.getRadius().getValue() << "\n";
    std::cout << "Angle (radians): " << result.getAngle().getValue() << "\n";

    return 0;
}