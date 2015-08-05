#include <iostream>
#include <visp/vpMath.h>
#include <visp/vpRotationMatrix.h>
#include <visp/vpRxyzVector.h>
#include <visp/vpQuaternionVector.h>

int main(int argc, char const *argv[])
{
	vpRxyzVector rxyz;
	rxyz[0] = vpMath::rad( 45.f); // phi   angle in rad around x axis
	rxyz[1] = vpMath::rad(-30.f); // theta angle in rad around y axis
	rxyz[2] = vpMath::rad( 90.f); // psi   angle in rad around z axis

	double c0,c1,c2,s0,s1,s2;
	c0 = cos(rxyz[0]);
    c1 = cos(rxyz[1]);
    c2 = cos(rxyz[2]);
    s0 = sin(rxyz[0]);
    s1 = sin(rxyz[1]);
    s2 = sin(rxyz[2]);
	std::cout << rxyz;

    std::cout<<"1.1="<<c1*c2<<"="<<c1<<"*"<<c2<<std::endl;
	// Construct a rotation matrix from the Euler angles
	vpRotationMatrix R(rxyz);
	// Extract the Euler angles around x,y,z axis from a rotation matrix
	rxyz.buildFrom(R);
	// Print the extracted Euler angles. Values are the same than the
	// one used for initialization
	// Since the rotation vector is 3 values column vector, the
	// transpose operation produce a row vector.
	vpRowVector rxyz_t = rxyz.t();

	std::cout << R << std::endl;

	std::cout << "--Quaternion--" << std::endl;

	vpQuaternionVector quat(1.0, 1.0, 1.0, 1.0);

	std::cout << quat << std::endl;

	vpRotationMatrix Rq(quat);

	quat.buildFrom(Rq);

	std::cout << Rq << std::endl;

	std::cout << "--Matrix->Quaternion--" << std::endl;

	std::cout << quat << std::endl;

	vpQuaternionVector quat2(R);

	std::cout << quat2 << std::endl;

	vpRotationMatrix R2(quat2);

	std::cout << R2 << std::endl;

	std::cout << "--Matrix->Angles--" << std::endl;

	std::cout << rxyz_t << std::endl;

	return 0;
}