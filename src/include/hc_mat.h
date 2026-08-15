#pragma once

#include "hc_types.h"

//#include "glm/glm.hpp"
//typedef glm::mat4 Mat4x4F32;

/////////////////////////////////////////////////////////////////////////// SECTION: VECTORS

typedef union Vec3F32 Vec3F32;

union Vec3F32 {
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
};

/////////////////////////////////////////////////////////////////////////// SECTION: MATRICES

typedef struct Mat4x4F32 {
    F32 e[4][4];
} Mat4x4F32;

// TODO: Look at the memory layout of this in the debugger, confirm row-major. Rightmost index is contiguous.
typedef struct Mat2x2F32 {
    F32 e[2][2];  /// e[row][column]
} Mat2x2F32;

#if 0
[ 1 2 ]
[ 3 4 ]

// e[0] is the first row
e[0][0] = 1  
e[0][1] = 2

// e[1] is the second row
e[1][0] = 3  
e[1][1] = 4
#endif


static inline Mat4x4F32 mat4x4f32_new(void) {
}

static inline Mat2x2F32 mat2x2f32_mul(Mat2x2F32 a, Mat2x2F32 b) {
    Mat2x2F32 result = {0};

    // Matrix indices
    // [ 0,0  0,1 ]
    // [ 1,0  1,1 ]
    //result.e[0][0] = (a.e[0][0] * b.e[0][0]) + (a.e[0][1] * b.e[1][0]);
    //result.e[0][1] = (a.e[0][0] * b.e[0][1]) + (a.e[0][1] * b.e[1][1]);
    //result.e[1][0] = (a.e[1][0] * b.e[0][0]) + (a.e[1][1] * b.e[1][0]);
    //result.e[1][1] = (a.e[1][0] * b.e[0][1]) + (a.e[1][1] * b.e[1][1]);

    // TODO(optimization): Look into unrolling/SIMD
    for (U32 row = 0; row < 2; ++row) {
        for (U32 col = 0; col < 2; ++col) {
            for (U32 k = 0; k < 2; ++k) {
                result.e[row][col] += a.e[row][k] * b.e[k][col];
            }
        }
    }

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
