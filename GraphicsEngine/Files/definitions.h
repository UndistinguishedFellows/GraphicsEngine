#ifndef DEFINITIONS_H
#define DEFINITIONS_H



/* Definition of constants */
#define PI 3.14159f
//#define INFINITY 1e8
#define MAX_RAY_DEPTH 4



/* Definition of methods*/
#define DEG2RAD(degrees) degrees * PI / 180.0f

#define MAX(a, b) (a > b) ? a : b

#define MIN(a, b) (a < b) ? a : b

#define RAD2DEG(rads) rads * 180.0f / PI 



/* Enumerations */
enum InteractiveAction { NONE, PAN, ROTATE, ZOOM };
enum CameraType { STATIC = 0, FPS = 1 };
enum RenderResult {FINAL_RESULT = 0, G_POSITION = 1, NORMAL = 2, AMBIENT_OCLUSSION = 3, COLOR = 4};


#endif
