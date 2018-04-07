#ifndef DEFINITIONS_H
#define DEFINITIONS_H



/* Definition of constants */
#define PI 3.14159f



/* Definition of methods*/
#define DEG2RAD(degrees) degrees * PI / 180.0f

#define MAX(a, b) (a > b) ? a : b

#define MIN(a, b) (a < b) ? a : b

#define RAD2DEG(rads) rads * 180.0f / PI 



/* Enumerations */
enum InteractiveAction { NONE, PAN, ROTATE, ZOOM };
enum CameraType { SIMPLE = 0, FPS = 1 };



#endif
