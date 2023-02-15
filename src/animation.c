#include "animation.h"


bool update_animation(animation_t *animation, float dt)
{
    bool looped = false;

    animation->time += dt;

    // FIXME: use fmod
    float loop_time = animation->end - animation->start;
    while (animation->time > loop_time) {
        animation->time -= loop_time;
        looped = true;
    }

    return looped;
}


void compute_armature_matrices(
        matrix_t base,
        matrix_t *output,
        const joint_transform_t *transforms,
        const armature_t *armature,
        unsigned index
) {
    joint_transform_t transform = transforms[index];

    matrix_t local = matrix_multiply(matrix_translation(transform.position[0],
                                                        transform.position[1],
                                                        transform.position[2]),
                                     quaternion_to_matrix(transform.rotation));

    matrix_t new_base = matrix_multiply(base, local);

    const unsigned *children = armature->hierarchy + armature->child_offsets[index];
    for (unsigned i = 0; i < armature->child_counts[index]; ++i)
        compute_armature_matrices(new_base, output, transforms, armature, children[i]);

    output[index] = matrix_multiply(new_base, armature->ibms[index]);
}


static float get_bone_transforms(
        const armature_t *armature,
        joint_transform_t *first,
        joint_transform_t *second,
        int index,
        float time
) {
    unsigned frame_count = armature->frame_counts[index];

    if (frame_count == 0)
        assert(NOT_IMPLEMENTED);

    const float *time_stamps = armature->time_stamps;

    unsigned offset = armature->frame_offsets[index];

    unsigned first_off  = offset;
    unsigned second_off = offset;

    for (unsigned i = 1; i < frame_count; ++i) {
        first_off = second_off;
        ++second_off;

        if (time_stamps[offset + i] > time)
            break;
    }

    *first  = armature->transforms[first_off];
    *second = armature->transforms[second_off];

    return (time - time_stamps[first_off]) / (time_stamps[second_off] - time_stamps[first_off]);
}


// TODO: cache per bone positions
void compute_pose_transforms(const armature_t *armature, joint_transform_t *transforms, float time)
{
    for (int i = 0; i < armature->bone_count; ++i) {
        joint_transform_t first, second;
        float blend = get_bone_transforms(armature, &first, &second, i, time);

        transforms[i].rotation = quaternion_n_lerp(first.rotation, second.rotation, blend);
        position_lerp(transforms[i].position, first.position, second.position, blend);
    }
}
