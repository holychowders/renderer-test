#pragma once

#include "hc_types.h"

#include <math.h> // TODO: Implement custom math library

//#include "glm/glm.hpp"
//typedef glm::mat4 Mat4F32;

/////////////////////////////////////////////////////////////////////////// SECTION: CONSTANTS

#define PI_F32 3.1415927f

/////////////////////////////////////////////////////////////////////////// SECTION: TRIGONOMETRIC FUNCTIONS

static inline F32 sinf32(F32 v) {
    return sinf(v);
}
static inline F32 cosf32(F32 v) {
    return cosf(v);
}
static inline F32 tanf32(F32 v) {
    return tanf(v);
}

static inline F32 deg_to_rad(F32 d) {
    return d * (PI_F32 / 180.0F);
}

/////////////////////////////////////////////////////////////////////////// SECTION: VECTORS

typedef union Vec3F32 {
    struct {
        F32 x;
        F32 y;
        F32 z;
    };
    struct {
        F32 r;
        F32 g;
        F32 b;
    };
    F32 e[3];
} Vec3F32;

/////////////////////////////////////////////////////////////////////////// SECTION: MATRIX CONVENTIONS

/*
    Row-major (ie: matrix.e[row][column] and contiguous on column index)
      Example: 2x2 Matrix
        [ 1 2 ]
        [ 3 4 ]

        Mat2F32 mat = { 1, 2, 3, 4 };

        mat.e[0][0] = 1
        mat.e[0][1] = 2
        mat.e[1][0] = 3
        mat.e[1][1] = 4
*/

/////////////////////////////////////////////////////////////////////////// SECTION: MATRIX HELPERS

//#define MAT_EQ(a, b) ((sizeof((a).e) == sizeof((b).e)) && (memcmp((a).e, (b).e, sizeof((a).e)) == 0))

/////////////////////////////////////////////////////////////////////////// SECTION: MAT2F32 FUNCTIONS

// TODO: Look at the memory layout of this in the debugger, confirm row-major. Rightmost index is contiguous.
typedef union Mat2F32 {
    F32 e[2][2]; /// e[row][column]
    struct {
        F32 Xx, Yx;
        F32 Xy, Yy;
    };
} Mat2F32;

static inline Mat2F32 mat2f32_identity(void) {
    // clang-format off
    return (Mat2F32){ .e = {
        { 1, 0 },
        { 0, 1 }
    } }; // clang-format on
}

static inline Mat2F32 mat2f32_mul(Mat2F32 a, Mat2F32 b) {
    Mat2F32 result = { 0 };

    //result.e[0][0] = (a.e[0][0] * b.e[0][0]) + (a.e[0][1] * b.e[1][0]);
    //result.e[0][1] = (a.e[0][0] * b.e[0][1]) + (a.e[0][1] * b.e[1][1]);
    //result.e[1][0] = (a.e[1][0] * b.e[0][0]) + (a.e[1][1] * b.e[1][0]);
    //result.e[1][1] = (a.e[1][0] * b.e[0][1]) + (a.e[1][1] * b.e[1][1]);

    // TODO(optimization): Unrolling/SIMD
    for (U32 row = 0; row < 2; ++row) {
        for (U32 column = 0; column < 2; ++column) {
            for (U32 k = 0; k < 2; ++k) {
                result.e[row][column] += a.e[row][k] * b.e[k][column];
            }
        }
    }

    return result;
}

static inline B32 mat2f32_eq(Mat2F32 a, Mat2F32 b) {
    for (U32 row = 0; row < 2; ++row) {
        for (U32 column = 0; column < 2; ++column) {
            if (a.e[row][column] != b.e[row][column]) { return false; }
        }
    }
    return true;
}

/////////////////////////////////////////////////////////////////////////// SECTION: MAT4F32 FUNCTIONS

typedef union Mat4F32 {
    F32 e[4][4];
    struct {
        //              Translation vector
        // Basis vectors|
        //  |   |   |   |
        //  X   Y   Z   |
        //  |   |   |   |
        F32 Xx, Yx, Zx, Tx;
        F32 Xy, Yy, Zy, Ty;
        F32 Xz, Yz, Zz, Tz;
        F32 Xw, Yw, Zw, Tw;
    };
} Mat4F32;

static inline Mat4F32 mat4f32_identity(void) {
    // clang-format off
    return (Mat4F32){ .e = {
        { 1, 0, 0, 0 },
        { 0, 1, 0, 0 },
        { 0, 0, 1, 0 },
        { 0, 0, 0, 1 }
    } }; // clang-format on
}

static inline B32 mat4f32_eq(Mat4F32 a, Mat4F32 b) {
    for (U32 row = 0; row < 4; ++row) {
        for (U32 column = 0; column < 4; ++column) {
            if (a.e[row][column] != b.e[row][column]) { return false; }
        }
    }
    return true;
}

static inline Mat4F32 mat4f32_mul(Mat4F32 a, Mat4F32 b) {
    Mat4F32 result = { 0 };
    // TODO(optimization): Unrolling/SIMD
    for (U32 row = 0; row < 4; ++row) {
        for (U32 column = 0; column < 4; ++column) {
            for (U32 k = 0; k < 4; ++k) {
                result.e[row][column] += a.e[row][k] * b.e[k][column];
            }
        }
    }
    return result;
}

static inline Mat4F32 mat4f32_trans(Mat4F32 m, Vec3F32 t) {
    m.Tx += m.Xx * t.x + m.Yx * t.y + m.Zx * t.z;
    m.Ty += m.Xy * t.x + m.Yy * t.y + m.Zy * t.z;
    m.Tz += m.Xz * t.x + m.Yz * t.y + m.Zz * t.z;
    m.Tw += m.Xw * t.x + m.Yw * t.y + m.Zw * t.z;
    return m;
}

static inline Mat4F32 mat4f32_rot_x(Mat4F32 matrix, F32 rot_rad) {
    F32 c = cosf32(rot_rad);
    F32 s = sinf32(rot_rad);
    Mat4F32 r = mat4f32_identity();
    r.Yy = c;
    r.Zy = -s;
    r.Yz = s;
    r.Zz = c;
    return mat4f32_mul(matrix, r);
}

static inline Mat4F32 mat4f32_rot_y(Mat4F32 matrix, F32 rot_rad) {
    F32 c = cosf32(rot_rad);
    F32 s = sinf32(rot_rad);
    Mat4F32 r = mat4f32_identity();
    r.Xx = c;
    r.Zx = s;
    r.Xz = -s;
    r.Zz = c;
    return mat4f32_mul(matrix, r);
}

static inline Mat4F32 mat4f32_rot_z(Mat4F32 matrix, F32 rot_rad) {
    F32 c = cosf32(rot_rad);
    F32 s = sinf32(rot_rad);
    Mat4F32 r = mat4f32_identity();
    r.Xx = c;
    r.Yx = -s;
    r.Xy = s;
    r.Yy = c;
    return mat4f32_mul(matrix, r);
}

static inline Mat4F32 mat4f32_rot_xyz(Mat4F32 matrix, Vec3F32 rots_rad) {
    matrix = mat4f32_rot_x(matrix, rots_rad.x);
    matrix = mat4f32_rot_y(matrix, rots_rad.y);
    matrix = mat4f32_rot_z(matrix, rots_rad.z);
    return matrix;
}

static inline Mat4F32 mat4f32_scale(Mat4F32 matrix, Vec3F32 scale) {
    Mat4F32 s = mat4f32_identity();
    s.Xx = scale.x;
    s.Yy = scale.y;
    s.Zz = scale.z;
    return mat4f32_mul(matrix, s);
}

static inline Mat4F32 mat4f32_perspective(F32 fov_y_rad, F32 aspect, F32 near_z, F32 far_z) {
    F32 f = 1.0F / tanf32(fov_y_rad * 0.5F);
    Mat4F32 result = { 0 };
    result.Xx = f / aspect;
    result.Yy = f;
    result.Zz = (far_z + near_z) / (near_z - far_z);
    result.Tz = (2.0F * far_z * near_z) / (near_z - far_z);
    result.Zw = -1.0F;
    return result;
}

/*
    Multiplying Matrices (When):
      - To multiply any two matrices, the number of columns in the first matrix must match the number of rows in the second matrix.
      - If two matrices can be multiplied, the dimensions of the resulting matrix will be the number of rows in the first matrix
        by the number of columns in the second matrix.
      - For example, consider two matrices A and B with dimensions `n_A x m_A` and `n_B x m_B`, where `n` is the
        column and `m` is the row. AxB is valid if `m_A = n_B`, and BxA is valid if `n_A = m_b`.

    Multiplying Matrices (How):

    Vectors Operations:
      - For any set of vectors the same length, the dot product may be taken.

    ---------------------------

    Matrix A
      [1 2 3 4]
      [5 6 7 8]
      [9 1 2 3]
      [4 5 6 7]

    times

    I (identity matrix)
      [1 0 0 0]
      [0 1 0 0]
      [0 0 1 0]
      [0 0 0 1]

    equals

    Matrix A
      [1 2 3 4]
      [5 6 7 8]
      [9 1 2 3]
      [4 5 6 7]

    ---------------------------

    Identity Matrix (4x4):
      [1 0 0 0]
      [0 1 0 0]
      [0 0 1 0]
      [0 0 0 1]

*/
