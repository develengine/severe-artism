#ifndef ANIMATION_H
#define ANIMATION_H

#include "linalg.h"
#include "res.h"
#include "utils.h"


typedef struct
{
    float start, end;
    float time;
    int iteration;
} animation_t;


bool update_animation(animation_t *animation, float dt);

void compute_pose_transforms(const armature_t *armature, joint_transform_t *transforms, float time);

void compute_armature_matrices(
        matrix_t base,
        matrix_t *output,
        const joint_transform_t *transforms,
        const armature_t *armature,
        unsigned index
);

#endif
