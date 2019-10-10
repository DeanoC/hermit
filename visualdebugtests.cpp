#include "al2o3_platform/platform.h"
#include "al2o3_platform/visualdebug.h"
#include "al2o3_cmath/vector.h"
void VisualDebugTests() {

	VISDEBUG_LINE(0, 0, 1.0, -1, -1, 1.0, VISDEBUG_PACKCOLOUR(255, 0, 0, 255));

	float lineStripVerts[] = {
			-0.5, -0.5, 1.0,
			0.5, -0.5, 1.0,
			0.5, 0.5, 1.0,
			-0.5, -0.5, 1.0,
	};
	VISDEBUG_LINESTRIP(3, lineStripVerts, VISDEBUG_PACKCOLOUR(0, 255, 0, 255));
	float lineVerts[] = {
			-0.6, -0.55, 1.0,
			0.6, -0.55, 1.0,
			0.6, -0.55, 1.0,
			0.6, 0.5, 1.0,
			0.6, 0.5, 1.0,
			-0.6, -0.55, 1.0,
	};
	VISDEBUG_LINES(3, lineVerts, VISDEBUG_PACKCOLOUR(200, 0, 255, 255));

	float triVerts[] = {
			-0.0, -0.5, 1.0,
			-0.5, -0.5, 1.0,
			-0.0, 0.0, 1.0,

			0.0, 0.5, 1.0,
			0.5, 0.5, 1.0,
			0.0, 0.0, 1.0,
	};
	VISDEBUG_SOLID_TRIS(2, triVerts, VISDEBUG_PACKCOLOUR(128, 128, 128, 128));
	float quadVerts[] = {
			-0.0, -0.5, 3.0,
			-0.5, -0.5, 3.0,
			-0.5, 0.0, 3.0,
			-0.0, -0.0, 3.0,

			0.0, 0.5, 10.0,
			0.5, 0.5, 10.0,
			0.5, 0.0, 10.0,
			0.0, 0.0, 10.0,
	};
	VISDEBUG_SOLID_QUADS(2, quadVerts, 0);

	static float xpos = 0.0f;
	static float scalef = 1.0f;
	static float rotf = 0.0f;
	Math_Vec3F pos = {0, 0, 0};
	Math_Vec3F rot = {0, 0, 0};
	Math_Vec3F scale = {1, 1, 1};
	VISDEBUG_DODECAHEDRON(pos.v, rot.v, scale.v, 0);
	Math_Vec3F pos2 = {sinf(xpos) * 3, 3, 0};
	Math_Vec3F scale2 = {sinf(scalef) + 1, sinf(scalef) + 1, sinf(scalef) + 1};
	VISDEBUG_OCTAHEDRON(pos2.v, rot.v, scale2.v, 0);
	Math_Vec3F pos3 = {sinf(xpos) * 3, -3, 0};
	Math_Vec3F rot2 = {rotf * Math_PiF(), 0, rotf * Math_PiF()};
	VISDEBUG_ICOSAHEDRON(pos3.v, rot2.v, scale.v, 0);
	Math_Vec3F pos4 = {sinf(xpos) * 3 + 4, -3, 0};
	VISDEBUG_CUBE(pos4.v, rot2.v, scale2.v, 0);
	Math_Vec3F pos5 = {sinf(xpos) * 3 - 4, -3, 0};
	VISDEBUG_TETRAHEDRON(pos5.v, rot2.v, scale.v, 0);
	Math_Vec3F pos6 = {sinf(xpos) * 3 - 4, 3, 0};
	VISDEBUG_DODECAHEDRON(pos6.v, rot2.v, scale.v, 0);

	xpos += 0.001f;
	scalef += 0.005f;
	rotf += 0.001f;

}

