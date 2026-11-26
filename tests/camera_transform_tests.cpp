#include "common.h"
#include "Quaternion.h"
#include "FixedStep.h"
#include <cstdlib>
#include <iostream>
#include <stdexcept>

static unsigned checks;
static void Check(bool success, const char *message)
{
	checks++;
	if(!success) throw std::runtime_error(message);
}
static bool Near(float a, float b) { return std::fabs(a - b) < 0.0001f; }

static RwMatrix CameraBasis(float yaw, float pitch)
{
	// Same left/front/up convention as CCamera::Process, written into RW's
	// right/at/up slots. Construct independently of CQuaternion's conversion.
	RwMatrix matrix = {};
	matrix.right = {-Cos(yaw), Sin(yaw), 0.0f};
	matrix.up = {-Sin(yaw) * Sin(pitch), -Cos(yaw) * Sin(pitch), Cos(pitch)};
	matrix.at = {Sin(yaw) * Cos(pitch), Cos(yaw) * Cos(pitch), Sin(pitch)};
	return matrix;
}

static void CheckBasis(const RwMatrix &expected, const RwMatrix &actual)
{
	const RwV3d oldAxes[] = {expected.right, expected.up, expected.at};
	const RwV3d newAxes[] = {actual.right, actual.up, actual.at};
	for(int axis = 0; axis < 3; axis++){
		Check(Near(oldAxes[axis].x, newAxes[axis].x) &&
		      Near(oldAxes[axis].y, newAxes[axis].y) &&
		      Near(oldAxes[axis].z, newAxes[axis].z), "camera matrix round-trip changed its viewing direction or horizon");
	}
}

static void RoundTrip(float yaw, float pitch)
{
	RwMatrix original = CameraBasis(yaw, pitch), restored = {};
	CQuaternion quaternion;
	quaternion.Set(original);
	quaternion.Normalise();
	quaternion.Get(&restored);
	CheckBasis(original, restored);
}

static RwMatrix BlendCamera(float yaw, float nextYaw, float alpha)
{
	RwMatrix previousMatrix = CameraBasis(yaw, 0), currentMatrix = CameraBasis(nextYaw, 0);
	CQuaternion previousRotation, currentRotation;
	previousRotation.Set(previousMatrix);
	currentRotation.Set(currentMatrix);
	previousRotation.Normalise(); currentRotation.Normalise();
	CFixedTransform previous = {{0, 0, 0}, {previousRotation.x, previousRotation.y, previousRotation.z, previousRotation.w}};
	CFixedTransform current = {{0, 0, 0}, {currentRotation.x, currentRotation.y, currentRotation.z, currentRotation.w}};
	CFixedTransform blend = CFixedTransform::Blend(previous, current, alpha);
	CQuaternion rendered(blend.rotation[0], blend.rotation[1], blend.rotation[2], blend.rotation[3]);
	RwMatrix result = {};
	rendered.Get(&result);
	return result;
}

static void Sweep(void)
{
	for(int yaw = -360; yaw <= 360; yaw += 5){
		for(int pitch = -80; pitch <= 80; pitch += 10)
			RoundTrip(yaw * PI / 180, pitch * PI / 180);
		for(int fraction = 0; fraction <= 8; fraction++){
			RwMatrix rendered = BlendCamera(yaw * PI / 180, (yaw + 5) * PI / 180, fraction / 8.0f);
			Check(Near(rendered.right.z, 0) && Near(rendered.up.x, 0) &&
			      Near(rendered.up.y, 0) && Near(rendered.up.z, 1), "horizontal mouse look introduced camera roll");
		}
	}
}

int main(int argc, char **argv)
{
	try {
		if(argc == 3){
			RoundTrip(float(std::atof(argv[1])) * PI / 180, float(std::atof(argv[2])) * PI / 180);
		}else{
			Sweep();
		}
		std::cout << checks << " camera transform checks passed\n";
		return 0;
	}catch(const std::exception &error){
		std::cerr << error.what() << '\n';
		return 1;
	}
}
