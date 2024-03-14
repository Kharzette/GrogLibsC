#pragma once
#include	<cglm/call.h>


#define	VOL_MISS		0
#define	VOL_INSIDE		1	//whole line segment inside
#define	VOL_HIT			2	//start outside hit
#define	VOL_HIT_INSIDE	4	//start inside hit
#define	VOL_HIT_VISIBLE	8	//hit visible surface


typedef struct	Winding_t
{
	vec3	*mpVerts;
	int		mNumVerts;

	struct Winding_t	*next;
}	Winding;

typedef struct	ConvexVolume_t
{
	vec4	*mpPlanes;
	int		mNumPlanes;
}	ConvexVolume;


//creates
ConvexVolume	*CV_AllocConvexVolumeFromBound(const vec3 min, const vec3 max);
void			CV_MakeConvexVolumeFromBound(const vec3 min, const vec3 max, ConvexVolume *pVol);
ConvexVolume	*CV_MakeFromTri(const vec3 tri[3], float bottomY);
int				CV_GenerateWindings(const ConvexVolume *pVol, Winding **pWL);

//collides
bool	CV_PointInVolume(const ConvexVolume *pVol, const vec3 point);
int		CV_LineIntersectVolume(const ConvexVolume *pVol, const vec3 start, const vec3 end,
								vec3 intersection, vec4 hitPlane);
int		CV_SweptSphereIntersect(const ConvexVolume *pVol, const vec3 start, const vec3 end,
								float radius, vec3 intersection, vec4 hitPlane);
int		CV_SweptBoundIntersect(const ConvexVolume *pVol, const vec3 start, const vec3 end,
								const vec3 min, const vec3 max,
								vec3 intersection, vec4 hitPlane);